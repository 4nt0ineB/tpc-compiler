%{

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "tree.h"
#include "SymbolTable.h"
#include "Compile.h"


extern FILE *yyin;
extern int lineno;
extern int colno;
extern const char *StringFromLabel[];
extern compiler_opt opts;
Node *mytree;


int yylex();
int yyerror(const char *msg);
void tree_to_dot_(Node *tree, FILE *stream);
void tree_to_dot(Node *tree, const char *path);
void help();


%}

%union{
    Node *node;
    char byte;
    int num;
    char ident[64];
    char comp[3];
    char type[30];
}

%type <node> Prog DeclVarsG DeclFoncts DeclFonct DeclarateursG Declarateurs EnTeteFonct Parametres ListTypVar Corps SuiteInstr Instr
%type <node> Exp TB FB M E T F LValue Arguments ListExp Declarateur DeclVars Assign 

%token <byte> CHARACTER ADDSUB DIVSTAR
%token <num> NUM
%token <ident> IDENT
%token <comp> ORDER EQ
%token <type> TYPE
%token OR
%token AND
%token WHILE RETURN IF ELSE VOID

%expect 1
%%

Prog:  DeclVarsG DeclFoncts                  { 
                                                $$ = makeNode(program);
                                                Node *n1 = makeNode(variables);
                                                Node *n2 = makeNode(functions);
                                                addChild($$, n1); 
                                                addChild($$, n2); 
                                                addChild(n1, $1);
                                                addChild(n2, $2);
                                                mytree = $$;
                                                
                                            }
    ;
DeclVarsG:
       DeclVarsG TYPE DeclarateursG ';'     { 
                                                Node *n2 = makeNode(type);
                                                strcpy(n2->type, $2); 
                                                if($1 == NULL){             /* La liste de variables est vide */
                                                    $$ = n2;                /* On l'initialise */
                                                }else{
                                                    addSibling($$, n2);     /* Sinon on ajoute la déclaration à la liste */
                                                }
                                                addChild(n2, $3);
                                            }
    |                                       { $$ = NULL; }
    ;
DeclarateursG:
       DeclarateursG ',' IDENT              { 
                                                $$ = $1;
                                                Node *n3 = makeNode(ident);
                                                strcpy(n3->ident, $3);
                                                addSibling($$, n3);
                                            }
    |  IDENT                                { 
                                                $$ = makeNode(ident);
                                                strcpy($$->ident, $1);
                                            }
    ;
DeclFoncts:
       DeclFoncts DeclFonct                 { 
                                                $$ = $1; 
                                                Node* n2 = makeNode(function); 
                                                addSibling($$, n2); 
                                                addChild(n2, $2); 
                                            }
    |  DeclFonct                            { 
                                                $$ = makeNode(function); 
                                                addChild($$, $1);
                                            }
    ;
DeclFonct:
       EnTeteFonct Corps                    { 
                                                $$ = makeNode(signature); 
                                                addChild($$, $1); 
                                                addSibling($$, $2); 
                                                
                                            }
    ;
EnTeteFonct:
       TYPE IDENT '(' Parametres ')'        { 
                                                /*      
                                                    signature
                                                        |---- type
                                                        |---- ident
                                                        |---- parameters
                                                            |
                                                */   
                                                
                                                $$ = makeNode(type); 
                                                strcpy($$->type, $1);
                                                Node *n2 = makeNode(ident);
                                                addSibling($$, n2); 
                                                strcpy(n2->ident, $2);
                                                Node *n4 = makeNode(parameters);
                                                addChild(n4, $4);
                                                addSibling($$, n4); 
                                            }
    |  VOID IDENT '(' Parametres ')'        { 
                                                $$ = makeNode(_void); 
                                                Node *n2 = makeNode(ident);
                                                strcpy(n2->ident, $2);
                                                addSibling($$, n2); 
                                                Node *n4 = makeNode(parameters);
                                                addChild(n4, $4);
                                                addSibling($$, n4); 
                                            }
    ;
Parametres:
       VOID                                 { $$ = NULL; }
    |  ListTypVar                           { $$ = $1; }
    ;
ListTypVar:
       ListTypVar ',' TYPE IDENT            { 
                                                Node *n3 = makeNode(type);
                                                Node *n4 = makeNode(ident);
                                                strcpy(n3->type, $3);
                                                strcpy(n4->ident, $4);
                                                addChild(n3, n4);

                                                if($$ == NULL){
                                                    $$ = n3;
                                                }else{
                                                    addSibling($$, n3);
                                                }
                                                

                                                
                                                
                                            }
    |  TYPE IDENT                           { 
                                                Node *n2 = makeNode(ident);
                                                strcpy(n2->ident, $2);
                                                $$ = makeNode(type);
                                                strcpy($$->type, $1);
                                                addChild($$, n2); 
                                            }
    ;
Corps: '{' DeclVars SuiteInstr '}'          { 
                                                $$ = makeNode(body);
                                                Node *n2 = makeNode(variables);
                                                Node *n3 = makeNode(instructions);
                                                addChild(n2, $2);
                                                addChild(n3, $3);
                                                addChild($$, n2);
                                                addChild($$, n3);
                                            }
    ;
DeclVars:
       DeclVars TYPE Declarateurs ';'       { 
                                                Node *n2 = makeNode(type);
                                                strcpy(n2->type, $2); 
                                                if($1 == NULL){             /* La liste de variables est vide */
                                                    $$ = n2;                /* On l'initialise */
                                                }else{
                                                    addSibling($$, n2);     /* Sinon on ajoute la déclaration à la liste */
                                                }
                                                addChild(n2, $3);
                                                

                                            }
    |                                       { $$ = NULL; }
    ;
Declarateurs:
       Declarateurs ',' Declarateur         { 
                                                $$ = $1;
                                                addSibling($$, $3);
                                            }
    |  Declarateur                          { $$ = $1; }
    ;
Declarateur: IDENT                          { 
                                                $$ = makeNode(ident);
                                                strcpy($$->ident, $1);
                                            }
    | Assign                                { $$ = $1; }
    ;
SuiteInstr:
       SuiteInstr Instr                     { 
                                                if($1 == NULL){         /* La liste est vide, on l'initialise */
                                                    $$ = $2;
                                                }else{                  /* Sinon on ajoute l'instruction */
                                                    addSibling($$, $2);
                                                }
                                            }
    |                                       { $$ = NULL; }
    ;
Instr:
       Assign ';'                           { $$ = $1; }
    |  IF '(' Exp ')' Instr                 {
                                                $$ = makeNode(_if);
                                                addChild($$, $3);
                                                addChild($$, $5);
                                            }
    |  IF '(' Exp ')' Instr ELSE Instr      {
                                                $$ = makeNode(_if);
                                                addChild($$, $3);
                                                addChild($$, $5);
                                                Node *n6 = makeNode(_else);
                                                addChild(n6, $7);
                                                addChild($$, n6);
                                            }
    |  WHILE '(' Exp ')' Instr              {
                                                $$ = makeNode(_while);
                                                addChild($$, $3);
                                                addChild($$, $5);
                                            }
    |  IDENT '(' Arguments  ')' ';'         {
                                                Node *n1 = makeNode(ident);
                                                strcpy(n1->ident, $1);
                                                $$ = makeNode(fcall);
                                                addChild($$, n1);
                                                Node *n3 = makeNode(args);
                                                addChild(n3, $3);
                                                addChild($$, n3);
                                            }
    |  RETURN Exp ';'                       { 
                                                $$ = makeNode(_return);
                                                addChild($$, $2);
                                            }
    |  RETURN ';'                           { $$ = makeNode(_return); }
    |  '{' SuiteInstr '}'                   { 
                                                $$ = makeNode(body);
                                                addChild($$, $2); 
                                            }
    |  ';'                                  { $$ = NULL; }
    ;
Assign : LValue '=' Exp                     {
                                                $$ = makeNode(assign);
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    ;
Exp :  Exp OR TB                            { 
                                                $$ = makeNode(or);
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  TB                                   { $$ = $1; }
    ;
TB  :  TB AND FB                            { 
                                                $$ = makeNode(and);
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  FB                                   { $$ = $1; }
    ;
FB  :  FB EQ M                              { 
                                                $$ = makeNode(eq);
                                                strcpy($$->comp, $2);
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  M                                    { $$ = $1; }
    ;
M   :  M ORDER E                            { 
                                                $$ = makeNode(order);
                                                strcpy($$->comp, $2);
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  E                                    { $$ = $1; }
    ;
E   :  E ADDSUB T                           { 
                                                $$ = makeNode(addsub);
                                                $$->byte = $2;
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  T                                    { $$ = $1; }
    ;    
T   :  T DIVSTAR F                          { 
                                                $$ = makeNode(divstar);
                                                $$->byte = $2;
                                                addChild($$, $1);
                                                addChild($$, $3);
                                            }
    |  F                                    { $$ = $1; }
    ;
F   :  ADDSUB F                             { 
                                                $$ = makeNode(unary_sign);
                                                $$->byte = $1;
                                                addChild($$, $2);
                                            }
    |  '!' F                                { 
                                                $$ = makeNode(not);
                                                $$->byte = '!';
                                                addChild($$, $2);
                                            }
    |  '(' Exp ')'                          { $$ = $2; }
    |  NUM                                  { 
                                                $$ = makeNode(num);
                                                $$->num = $1;
                                            }
    |  CHARACTER                            { 
                                                $$ = makeNode(character); 
                                                $$->byte = $1;
                                            }
    |  LValue                               { $$ = $1; }
    |  IDENT '(' Arguments  ')'             { 
                                                Node *n1 = makeNode(ident);
                                                strcpy(n1->ident, $1);
                                                $$ = makeNode(fcall);
                                                addChild($$, n1);
                                                Node *n3 = makeNode(args);
                                                addChild(n3, $3);
                                                addChild($$, n3);
                                            }
    ;
LValue:
       IDENT                                { 
                                                $$ = makeNode(ident);
                                                strcpy($$->ident, $1);
                                            }
    ;
Arguments:
       ListExp                              { $$ = $1; }
    |                                       { $$ = NULL; }
    ;
ListExp:
       ListExp ',' Exp                      {
                                                $$ = $1;
                                                addSibling($$, $3);
                                            }
    |  Exp                                  { $$ = $1; }
    ;
%%



int main(int argc, char *argv[]){

    /* http://langevin.univ-tln.fr/CDE/LEXYACC/Lex-Yacc2.html */
    /* https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html */

    int res;
    int opt;
    static int f_tree;
    static int f_help;
    static int f_tables;
    int f_dot = 0;
    char *dot_out_path = NULL;
    char *file_path = NULL;
    static const struct option long_options[] = {
        {"tree", no_argument, &f_tree, 1},
        {"help", no_argument, &f_help, 1},
        {"symtabs", no_argument, &f_tables, COPTTABLES},
        {NULL,      0,                 NULL,  0 }
    };
    
    while ((opt = getopt_long(argc, argv, "sthd:", long_options, NULL)) != -1) {
        switch (opt) {
            case 0: break;
            case 's':
                f_tables = COPTTABLES;
                break;
            case 't': 
                f_tree = 1;
                break;
            case 'd':
                f_dot = 1;
                dot_out_path = optarg;
                break;
            case 'h': 
                f_help = 1;
                break;
            default:
                help();
                return 3;
        }
    }
    // just display the help
    if(f_help){
        help();
        return 0;
    }
    // update options for the compiler
    if(f_tables) opts |= f_tables; 
    /* if an arg remain then its the path of the file to parse */
    if(optind == argc - 1) {
        file_path = argv[optind];
        
        yyin = fopen(file_path, "r" );
        if(!yyin){
            fprintf(stderr
            , "cannot parse '%s' : No such file or directory\n"
            , file_path);
            return 3;
        }
    }
    // let's parse
    res = yyparse();
    if(res == 0){ /* everything went fine */
        if(f_tree)  printTree(mytree);
        // translate to dot if option set
        if(f_dot)   tree_to_dot(mytree, dot_out_path);
        /* Compile */
        if(mytree){
            compile(mytree);
            deleteTree(mytree);
        }
    }
    
    // return with the parsing error code
    return res;
}

/**
 * @brief Print the help 
 */
void help(){
    printf("./tpcas\n"
    "\nNAME\n\t./tpcas - a parser for the TPC language, a subset of C\n"
    "\nSYNOPSIS\n"
    "\t./tpcas [OPTION]\n"
    "\t./tpcas [OPTION] [FILE]\n"
    "\nDESCRIPTION\n"
    "\t Parses inputs (from stdin or FILE) as TPC formated text\n\n"
    "\t-h, --help\n\t\tdisplay this help and exit\n\n"
    "\t-t, --tree\n\t\tparse the input and display the abstract syntax tree\n\n"
    "\t"
    "\t-d [file]\n\t\tproduce a dot formated file from the abstract syntax tree\n\n"
    "\t-s display symbol tables\n"
    "\nAUTHOR\n"
    "\tWritten by Antoine Bastos\n\n"
    );
}

void tree_to_dot_(Node *tree, FILE *stream){
    Node *tmp;
    static char buffer[64];
    if(!tree) return;
    node_sprintf(tree, buffer);
    fprintf(stream, "  n%p [label=\"%s\"", (void *) tree, buffer);
    if(tree->label <= args){
        fprintf(stream, " shape=rect style=\"filled,setlinewidth(0)\" fillcolor=gray79");
    }
    
    fprintf(stream, "]\n");
    if(tree->firstChild){
        fprintf(stream,
                "  n%p -> n%p [color=\"#2257ab\", label=\"\"]\n",
                (void *) tree, (void *) tree->firstChild);
        tree_to_dot_(tree->firstChild, stream);
        if(!(tmp = tree->firstChild->nextSibling))
            return;
        while(tmp){
            fprintf(stream, "  n%p [label=\"%s\"]", (void *) tmp, StringFromLabel[tmp->label]);
            fprintf(stream, "  n%p -> n%p [color=\"#2257ab\", label=\"\"]\n", (void *) tree, (void *) tmp);
            tree_to_dot_(tmp, stream);
            tmp = tmp->nextSibling;
        }
    }
}

/**
 * @brief Translate AST to dot
 * in a file
 * 
 * @param tree an AST
 * @param path file name
 */
void tree_to_dot(Node *tree, const char *path){
    FILE *file = fopen(path, "w");
    fprintf(file, "digraph AST {\n\n\n");
    tree_to_dot_(tree, file);
    fprintf(file, "\n}");
}

int yyerror(const char *msg){
    fprintf(stderr, "%s on line %d near char %d\n", msg, lineno, colno);
    return 1;
}