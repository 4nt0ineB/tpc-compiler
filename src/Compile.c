#include "Compile.h"

#include <assert.h>

#include <stdio.h>

#include <string.h>

#define GLABEL "glob_"
#define MAXLABEL 50
/* Shortcuts to struct attributes */
// In symbol
#define S_IS_FUNCTION(symbol)(symbol->type == FUNCTION)
#define S_IDENT(symbol) symbol->ident
#define SV_PTYPE(symbol) symbol->primitive
// In tree
#define F_RETURN(node) node->firstChild->firstChild->type
#define F_IDENT(node) node->firstChild->firstChild->nextSibling->ident
#define F_LABEL(node) node->firstChild->firstChild->label
// Errors
#define ERR_REDEF(location, ident)\
    fprintf(stderr, "%3d | In function ’%s’\n\terror: redefinition of ’%s’\n", location->lineno, S_IDENT(current_function), ident);
#define ERR_G_DECL(location, ident)\
    fprintf(stderr, "%3d | error: ’%s’ previously declared as a %s\n", location->lineno, ident, \
    get_symbol(context.global, ident)->type == FUNCTION ? "function" :"global variable");
#define ERR_MSG_(fmt, ...)\
    fprintf(stderr, "error: " fmt "\n",  ##__VA_ARGS__);
#define ERR_MSG(location, fmt, ...)\
    fprintf(stderr, "%3d | In function ’%s’\n\terror: " fmt "\n", location->lineno,  S_IDENT(current_function), ##__VA_ARGS__);
#define WARN_MSG(location, fmt, ...)\
    fprintf(stderr, "%3d | In function ’%s’\n\twarning: " fmt "\n", location->lineno,  S_IDENT(current_function), ##__VA_ARGS__);

compiler_opt opts = 0;

/* Output asm file */
FILE * file;
/* The current fonction being compiled */
Symbol *current_function = NULL;
char current_f_has_returned = 0;
/* Track stack alignement during compilation */
static int stack_alignment = 0;
Context context;

static void stack_update(int nbits);
static void stack_push();
static void stack_pop();
static int stack_align();
static void new_label(char *dest);

int compile(Node *tree) {
    /* Init compilation context */
    context.global = new_table();
    context.local = new_table();
    /* Open asm file */
    file = fopen("_anonymous.asm", "w");
    if (!file) {
        fprintf(stderr, "Could'nt create asm file\n");
        exit(2);
    }
    /* COMPILE */
    precompiled_asm();
    /* Globals */
    compile_globals(tree);
    /* functions */
    compile_functions(tree);
    /* Print global */
    if(opts & COPTTABLES)
        print_table(context.global, "Global");
    /* Close asm file */
    fclose(file);
    /* free symbol tables */
    free_table(context.global);
    free_table(context.local);
    return 1;
}

void compile_globals(Node *tree) {
    Node * type_ = tree->firstChild->firstChild;
    while (type_) {
        /* int .. */
        Node * global_ident = type_->firstChild;
        while (global_ident) {
            /* int a, b.. */
            Symbol * tmp_sym = new_symbol(global_ident->ident, PRIMITIVE);
            tmp_sym->primitive = ptype_from_string(type_->type);
            /* Ajouter le symbole */
            if (!add_symbol(context.global, tmp_sym)) {
                ERR_G_DECL(global_ident, type_->firstChild->ident);
                exit(2);
            }
            global_ident = global_ident->nextSibling;
        }
        type_ = type_->nextSibling;
    }
    fprintf(file, "\nsection .bss\n");
    fprintf(file, "\tbase: resb %d\n", context.global->size);
}

void compile_functions(Node *tree) {
    fprintf(file, "\nsection .text\n");
    Node * function = SECONDCHILD(tree)->firstChild;
    while (function) {
        /* the stack is considered aligned at the begining of function */
        stack_alignment = 0;
        /* clear the table before each compilation of a function */
        clear_table(context.local);
        /* update the context of compilation */
        compile_function(function);
        function = function->nextSibling;
    }
    // test if main exists
    Symbol *symb = symbol_from_context_("main");
    if(!symb || !S_IS_FUNCTION(symb)){
        ERR_MSG_("no main function provided");
        exit(2);       
    }
}

void compile_function(Node *function) {
    char * ident = F_IDENT(function);
    char * return_type = F_RETURN(function);
    /* a function returning void is not a type but label directly _void */
    if (F_LABEL(function) == _void) strcpy(return_type, "void");
    /* Check if main function then put required asm for start*/
    fprintf(file, "\n%s:\n"
                  "\tpush rbp\n"
                  "\tmov rbp, rsp\n" 
                , ident
    );
    /* Try adding the function into the global table  */
    Symbol * function_symb = new_symbol(ident, FUNCTION);
    function_symb->function.type =
    ptype_from_string(return_type); /* Set return type */
    if (!add_symbol(context.global, function_symb)) {
        ERR_G_DECL(function, ident);
        free(function_symb);
        exit(2);
    }

    // update compilation context
    current_function = function_symb;

    /* PARAMS  */
    /* Add params to local table */
    Node *param = THIRDCHILD(function->firstChild)->firstChild;
    while (param) {
        Symbol * tmp_sym = new_symbol(param->firstChild->ident, PRIMITIVE);
        tmp_sym->primitive = ptype_from_string(param->type);
        if (!add_symbol(context.local, tmp_sym)) {
            /* Symbol already exists */
            ERR_REDEF(param, param->firstChild->ident);
            exit(2);
        }
        /* Valid symbol as parameter then update function signature in global
         * table*/
        function_symb->function.args_type[function_symb->function.n_args++] =
            tmp_sym->primitive;

        param = param->nextSibling;
    }

    /* Check main signature */
    if(!strcmp("main", ident)){
        if(function_symb->function.type != PT_INT){
            ERR_MSG(function, "main return type must be int");
            exit(2);
        }
        if(function_symb->function.n_args != 0){
            ERR_MSG(function, "main function does'nt have any arguments");
            exit(2);
        }
    }
    
    // allocate enough space in stack for params stack
    fprintf(file, "\tsub rsp, %d\n", context.local->size);
    stack_update(context.local->size);

    // save args in tha assigned local variable space in the stack
    int i = 0;
    param = THIRDCHILD(function->firstChild)->firstChild;
    for(; param; param = param->nextSibling){
        Symbol *symb = symbol_from_context_(param->firstChild->ident);
        char *reg = NULL;
        switch(i){
            case 0: reg = "edi"; break;
            case 1: reg = "esi"; break;
            case 2: reg = "edx"; break;
            case 3: reg = "ecx"; break;
            case 4: reg = "e8";  break;
            case 5: reg = "e9";  break;
            default:
                ERR_MSG(function, "Functions with more than 6 parameters are yet not supported\n");
                exit(2);
        }

        char *psize = NULL;
        if (SV_PTYPE(symb) == PT_INT) psize = "dword";
        if (SV_PTYPE(symb) == PT_CHAR) psize = "byte";
        fprintf(file, "\tmov %s [rbp-%d], %s\n", psize, symb->address, reg);
        i++;
    }


    /* Compile body */
    current_f_has_returned = 0;
    compile_body(SECONDCHILD(function));
    if(current_function->function.type != PT_VOID && !current_f_has_returned){
        ERR_MSG(function, "missing return instruction");
        exit(2);
    }
    if(opts & COPTTABLES)
        print_table(context.local, ident);
    fprintf(file, "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n");
    /* Check if main function then put required asm for exit*/
}



void compile_body(Node *body) {
    Node * type_ = body->firstChild->firstChild;
    while (type_) {
        /* int ..; */
        Node * local_ident = type_->firstChild;
        while (local_ident) {
            /* int x, y, ..; */
            Symbol * tmp_sym;
            if (local_ident->label == assign) {
                /* x = (expr) */
                tmp_sym = new_symbol(local_ident->firstChild->ident, PRIMITIVE);
            } else {
                /* int x; */
                tmp_sym = new_symbol(local_ident->ident, PRIMITIVE);
            }
            SV_PTYPE(tmp_sym) = ptype_from_string(type_->type);

            if (!add_symbol(context.local, tmp_sym)) {
                /* Symbol already exists */
                ERR_REDEF(local_ident, local_ident->ident)
                exit(2);
            }
            /* Grow stack */
            fprintf(file, "\tsub rsp, %d\n", primitive_type_size(tmp_sym->type));
            stack_update(primitive_type_size(tmp_sym->type));
            /* compile if assignation instruction */
            if (local_ident->label == assign) {
                compile_instruction(local_ident);
            }
            local_ident = local_ident->nextSibling;
        }
        type_ = type_->nextSibling;
    }
    compile_instructions(SECONDCHILD(body));
}

void compile_instructions(Node * instructions) {
    Node * instruction = instructions->firstChild;
    while (instruction) {
        compile_instruction(instruction);
        instruction = instruction->nextSibling;
    }
}

/**
 * return -1 by default or instr is null
*/
PrimitiveType compile_instruction(Node *instr) {
    PrimitiveType rvalue_type;
    if (!instr) return -1;
    switch (instr->label) {
    case body:       compile_instructions(instr); break;
    case _if:        compile_if(instr);           break;
    case _while:     compile_while(instr);        break;
    case or:         compile_or(instr);           break;
    case and:        compile_and(instr);          break;

    case eq:
    case order:
    case addsub:
    case divstar:    return compile_binary_op(instr);
    case not:
    case unary_sign: return compile_unary_op(instr);
    case ident:      return compile_ident(instr);
    case assign:     return compile_assign(instr);
    case fcall:      return compile_fcall(instr);

    case num:
        fprintf(file, 
        "\tmov rax, %d\n"
        "\tpush rax\n", instr->num);
        stack_push();
        return PT_INT;

    case character:
        fprintf(file, "\tpush %d\n", (int) instr->byte);
        stack_push();
        return PT_CHAR;

    case _return:;
        current_f_has_returned = 1;
   
        rvalue_type = compile_instruction(FIRSTCHILD(instr));
        // CHECK GOOD RETURN TYPE 
        assert(current_function); // function can't not exists
        if(rvalue_type == PT_VOID){
            ERR_MSG(instr, "void value not ignored as it ought to be");
            exit(2);
        }
        if (rvalue_type != -1) { // something returned
            if(rvalue_type != PT_VOID && current_function->function.type == PT_VOID){
                ERR_MSG(instr, "‘return’ a value in a function of type void");
                exit(2);
            }

            if(rvalue_type == PT_INT && current_function->function.type == PT_CHAR){
                WARN_MSG(instr, "returning with overflow in conversion from ‘int’ to ‘char’");
            }    
            
            if(!strcmp(S_IDENT(current_function), "main")){
                fprintf(file, "\tpop rax\n");
                stack_pop();
            }else{
                fprintf(file, "\tpop rax\n");
                fprintf(file, "\tpush rax\n");
            }
        }else if(current_function->function.type != PT_VOID){ // returning void inside a non-void returning function
            char stype[25];
            string_from_ptype(stype, current_function->function.type);
            ERR_MSG(instr, "returning ‘void‘ in a function returning ‘%s‘", stype);
            exit(2);
        }
        fprintf(file, "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n");
        break;

    default:
        fprintf(stderr, "Unexpected instruction label %d\n", instr->label);
        exit(2);
    }
    return -1;
}

// https://stackoverflow.com/questions/47350568/yasm-movsx-movsxd-invalid-size-for-operand-2
PrimitiveType compile_assign(Node *instr){
    Symbol *symb = symbol_from_context(FIRSTCHILD(instr));
    PrimitiveType rvalue_type = compile_instruction(SECONDCHILD(instr));
    if (S_IS_FUNCTION(symb)) {
        ERR_MSG(FIRSTCHILD(instr), "function ’%s’ is not assignable", S_IDENT(symb));
        exit(2);
    }
    if(rvalue_type == PT_VOID){
        ERR_MSG(SECONDCHILD(instr), "void value not ignored as it ought to be");
        exit(2);
    }
    if (SV_PTYPE(symb) == PT_CHAR && rvalue_type == PT_INT) {
        WARN_MSG(instr, "overflow in conversion from ‘int’ to ‘char’");
    }
    fprintf(file,
        "\tpop rax\n"
        "\tmov ");
    if(get_symbol(context.global, FIRSTCHILD(instr)->ident)){
        fprintf(file, "[base+%d], ", symb->address);
    }else{
        fprintf(file, "[rbp-%d], ", symb->address);
    }
    if (SV_PTYPE(symb) == PT_INT) fprintf(file, "eax\n");
    if (SV_PTYPE(symb) == PT_CHAR) fprintf(file, "al\n");
    else
        assert("Compile 'assign': unknown type for nasm register\n");
    stack_pop();
    return SV_PTYPE(symb);
}

void compile_while(Node *expr){
    char label_begin[MAXLABEL];
    char label_end[MAXLABEL];
    new_label(label_begin);
    new_label(label_end);
    fprintf(file, "\t%s:\n", label_begin);
    compile_instruction(FIRSTCHILD(expr));
    fprintf(file, "\tpop rax\n"
                  "\ttest rax, rax\n"
                  "\tjz %s\n"
            , label_end
    );
    compile_instruction(SECONDCHILD(expr));
    fprintf(file, "\tjmp %s\n" 
                  "\t%s:\n" 
            , label_begin
            , label_end
    );
}

void compile_if(Node *expr){
    char label_false[MAXLABEL];
    char label_true[MAXLABEL];
    PrimitiveType pt_cond = compile_instruction(expr->firstChild);
    if(pt_cond == PT_VOID){
        ERR_MSG(expr->firstChild, "void value not ignored as it ought to be");
        exit(2);
    }
    new_label(label_false);
    new_label(label_true);
    /* 
        false ? goto x
        if ...
           goto y
        x else ...
        y
     */

    fprintf(file, 
                "\tpop rax\n"
                "\ttest eax, eax\n"
                "\tjz %s\n"
                , label_false);

    compile_instruction(SECONDCHILD(expr));

    fprintf(file, "\tjmp %s\n", label_true);
    fprintf(file, "\t%s:\n", label_false);
    if(THIRDCHILD(expr) && THIRDCHILD(expr)->label == _else)
        compile_instruction(THIRDCHILD(expr)->firstChild);
    fprintf(file, "\t%s:\n", label_true);
}

PrimitiveType compile_ident(Node *expr) {
    Symbol * symb = symbol_from_context(expr);
    if(S_IS_FUNCTION(symb)){
        ERR_MSG(expr, "trying to use the function identifier ’%s’ as a right value", S_IDENT(symb));
        exit(2);
    }
    if (SV_PTYPE(symb) == PT_INT) fprintf(file, "\tmov eax, dword");
    else if (SV_PTYPE(symb) == PT_CHAR) fprintf(file, "\tmov al, byte");
    else assert("Compile assign: uknown type for nasm register\n");
    if(get_symbol(context.global, expr->ident)) 
         fprintf(file, " [base+%d]\n", symb->address);
    else fprintf(file, " [rbp-%d]\n", symb->address);
    fprintf(file, "\tpush rax\n");
    // fprintf(file, "\tpush rax\n");
    stack_push();
    return SV_PTYPE(symb);
}

/* instructions a = (expr)*/

PrimitiveType compile_or(Node *expr){
    char label_true[MAXLABEL];
    char label_continue[MAXLABEL];
    PrimitiveType op1 = compile_instruction(FIRSTCHILD(expr));
    if (op1 == PT_VOID) {
        ERR_MSG(FIRSTCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    new_label(label_true);
    new_label(label_continue);
    fprintf(file, "\tpop rax\n"
                  "\tcmp rax, 0\n" 
                  "\tjne %s\n" // goto true
            , label_true
    );
    stack_pop();
    PrimitiveType op2 = compile_instruction(SECONDCHILD(expr));
    if (op2 == PT_VOID) {
        ERR_MSG(SECONDCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    fprintf(file, 
                  // here we transform the result from 2ndchild to 0 or 1 
                  "\tpop rax\n"
                  "\ttest rax, rax\n"
                  "\tsetnz al\n"
                  "\tpush rax\n"
                  
                  "\tjmp %s\n"
                  "\t%s:\n" 
                  "\tpush 1\n"
                  "\t%s:\n"
            , label_continue
            , label_true
            , label_continue
    );
    return PT_INT;
}

PrimitiveType compile_and(Node *expr){
    char label_false[MAXLABEL];
    char label_continue[MAXLABEL];
    PrimitiveType op1 = compile_instruction(FIRSTCHILD(expr));
    if(op1 == PT_VOID) {
        ERR_MSG(FIRSTCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    new_label(label_false);
    new_label(label_continue);
    fprintf(file, "\tpop rax\n"
                  "\tcmp rax, 0\n" 
                  "\tje %s\n" // goto true
            , label_false
    );
    stack_pop();
    PrimitiveType op2 = compile_instruction(SECONDCHILD(expr));
    if(op2 == PT_VOID) {
        ERR_MSG(SECONDCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    fprintf(file, 
                  // here we transform the result from 2ndchild to 0 or 1 
                  "\tpop rax\n"
                  "\ttest rax, rax\n"
                  "\tsetnz al\n"
                  "\tpush rax\n"
                  
                  "\tjmp %s\n"
                  "\t%s:\n" 
                  "\tpush 0\n"
                  "\t%s:\n"
            , label_continue
            , label_false
            , label_continue
    );
    return PT_INT;
}

PrimitiveType compile_binary_op(Node *expr) {
    PrimitiveType op1 = compile_instruction(FIRSTCHILD(expr));
    PrimitiveType op2 = compile_instruction(SECONDCHILD(expr));
    if(op1 == PT_VOID){
        ERR_MSG(FIRSTCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    if(op2 == PT_VOID){
        ERR_MSG(SECONDCHILD(expr), "void value not ignored as it ought to be");
        exit(2);
    }
    fprintf(file, "\tpop rcx\n");
    stack_pop();
    fprintf(file, "\tpop rax\n");
    stack_pop();
    switch(expr->byte){
        case '-': fprintf(file, "\tsub rax, rcx\n"); break;
        case '+': fprintf(file, "\tadd rax, rcx\n"); break;
        case '*': fprintf(file, "\timul rax, rcx\n"); break;
        case '/':
            fprintf(file, "\tmov rdx, 0\n"
                          "\tidiv rcx\n"
            );
            break;
        case '%':
            fprintf(file, "\tmov rdx, 0\n"
                          "\tidiv rcx\n"
                          "\tmov rax, rdx\n"
            );
            break;
        default:;
    }
    if(expr->label == eq || expr->label == order){
        static const char* cmp[6][2] = {
            {"==", "sete al"},{"!=", "setne al"},
            {"<=", "setle al"},{">=", "setge al"},
            {"<", "setl al"},{">", "setg al"}
        };
        fprintf(file, "\tcmp rax, rcx\n");
        for(int i = 0; i < 6; i++){
            if(!strcmp(expr->comp, cmp[i][0])){
                fprintf(file, "\t%s\n", cmp[i][1]);
                break;
            }
        }
    }
    fprintf(file, "\tpush rax\n");
    stack_push();
    return PT_INT;
}

PrimitiveType compile_unary_op(Node *expr) {
    assert(expr);
    PrimitiveType op1 = compile_instruction(expr->firstChild);
    if (op1 == PT_VOID) {
        ERR_MSG(expr, "void value not ignored as it ought to be \n");
        exit(2);
    }
    switch(expr->byte){
    case '-':
         fprintf(file, "\tpop rax\n"
            "\tneg rax\n"
            "\tpush rax\n");
        break;
    case '!':
        fprintf(file, "\tpop rax\n"
                      "\ttest rax, rax\n"
                      "\tsete al\n"
                      "\tpush rax\n"
        );
        break;
    default:;
    }
    return op1;
}

PrimitiveType compile_fcall(Node *expr) {
    Symbol * fsymbol = symbol_from_context(expr->firstChild);
    char buff[6], buff2[6];
    // must store rsi, rdi... in stack !
    /* check params type before call */
    int argi = 0;
    for (Node * arg = SECONDCHILD(expr)->firstChild; arg; arg = arg->nextSibling) {
        PrimitiveType type = compile_instruction(SECONDCHILD(expr)->firstChild);
        if (argi >= fsymbol->function.n_args) {
            ERR_MSG(expr, "trying to call function ’%s’, by passing too many arguments", fsymbol->ident);
            exit(2);
        }
        if(type == PT_VOID){
            ERR_MSG(arg, "void value not ignored as it ought to be");
            exit(2);
        }
        if (type != fsymbol->function.args_type[argi]) {
            string_from_ptype(buff, type);
            string_from_ptype(buff2, fsymbol->function.args_type[argi]);
            WARN_MSG(arg, "in call to function ’%s’, argument %d, casting ’%s’ to ’%s’"
                        , fsymbol->ident 
                        , argi + 1
                        , buff, buff2);
        }
        switch (argi) {
        case 0:
            fprintf(file, "\tpop rdi\n");
            break;
        case 1:
            fprintf(file, "\tpop rsi\n");
            break;
        case 2:
            fprintf(file, "\tpop rdx\n");
            break;
        case 3:
            fprintf(file, "\tpop rcx\n");
            break;
        case 4:
            fprintf(file, "\tpop r8\n");
            break;
        case 5:
            fprintf(file, "\tpop r9\n");
            break;
        default:
            assert("Not yet implemented");
            break;
        }
        argi++;
    }
    /* check not enough args */
    if (argi < fsymbol->function.n_args) {
        ERR_MSG(expr, "call function %s require %d arguments, but %d given\n", fsymbol->ident, fsymbol->function.n_args, argi);
        exit(2);
    }
    /* Start fcall with stack aligmenent */
    // we fetch `argi` args from the stack so we decrease stack of `argi` bytes  
    stack_update(-(8 * argi));
    int aligned = stack_align();
    /* fcall */
    fprintf(file, "\tcall %s\n", expr->firstChild->ident);
    if (aligned) 
        fprintf(file, "\tadd rsp, %d\n", aligned);
    if(fsymbol->function.type != PT_VOID){
        fprintf(file, "\tpush rax\n");
        stack_push();
    }
    /* End fcall with stack alignement */
    // must retrieve rsi, rdi... from stack !
    return fsymbol->function.type;
}

void precompiled_asm() {
    char c;
    FILE * futil = fopen("src/utils.asm", "r");
    if (!futil)
        futil = fopen("utils.asm", "r");
    if (!futil)
        futil = fopen("ProjetCompilationL3_BASTOS/bin/utils.asm", "r");
    if(!futil)
    	fprintf(stderr, "Precompile file src/utils.asm not found\n");
    
    while ((c = getc(futil)) != EOF)
        putc(c, file);
    Symbol * function_symb;
    // putchar
    function_symb = new_symbol("putchar", FUNCTION);
    function_symb->function.type = PT_VOID;
    function_symb->function.args_type[function_symb->function.n_args++] = PT_CHAR;
    add_symbol(context.global, function_symb);
    // putint
    function_symb = new_symbol("putint", FUNCTION);
    function_symb->function.type = PT_VOID;
    function_symb->function.args_type[function_symb->function.n_args++] = PT_INT;
    add_symbol(context.global, function_symb);
    // getchar
    function_symb = new_symbol("getchar", FUNCTION);
    function_symb->function.type = PT_CHAR;
    add_symbol(context.global, function_symb);
    // getint
    function_symb = new_symbol("getint", FUNCTION);
    function_symb->function.type = PT_INT;
    add_symbol(context.global, function_symb);
}

Symbol * symbol_from_context_(const char *ident){
    assert(ident);
    Symbol * symb = get_symbol(context.local, ident);
    if (!symb)
        symb = get_symbol(context.global, ident);
    return symb;
}

Symbol * symbol_from_context(Node *node) {
    assert(node);
    Symbol * symb = symbol_from_context_(node->ident);
    if (!symb) {
        ERR_MSG(node, "‘%s’ undeclared ", node->ident);
        exit(2);
    }
    return symb;
}

/**
 * @brief Update the stack by adding a given number of bits
 *
 * @param nbits
 */
static void stack_update(int nbits) {
    stack_alignment += nbits;
    //fprintf(file, "\t; stack: %d\n", stack_alignment);
}

/**
 * @brief Grow the stack of one byte
 *
 */
static void stack_push() {
    stack_update(8);
}

/**
 * @brief Decrease the stack of one byte
 *
 */
static void stack_pop() {
    stack_update(-8);
}

/**
 * @brief Align the stack to a multiple of 16
 * and print in the asm file the code to do it.
 *
 * @return int the added padding if aligned, 0 meaning no alignement.
 */
static int stack_align() {
    int padding = stack_alignment % 16;
    if (padding) {
        fprintf(file, "\tsub rsp, %d\n", 16 - padding);
        return 16 - padding;
    }
    return 0;
}

/**
 * @brief Get a new label 
 * for an activation block
 * 
 * @param dest 
 */
static void new_label(char *dest){
    assert(dest);
    static int id = 0;
    sprintf(dest, "lbl%d", id);
    id++;
}
