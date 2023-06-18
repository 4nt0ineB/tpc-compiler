
#ifndef _TPC_COMPILE_
#define _TPC_COMPILE_

#include "SymbolTable.h"
#include "tree.h"

/**
 * @brief Compiler options
 * 
 */
typedef enum {
    // display warnings by default
    COPTWARN = 1,
    // display errors by default
    COPTERR,    
    // does not display symbols tables by default
    COPTTABLES
} compiler_opt;

/**
 * @brief General compile function to compile the entire
 * abstract syntaxic tree
 *
 * @param tree
 * @return int
 */
int compile(Node *tree);
void precompiled_asm();
void compile_globals(Node *tree);
void compile_functions(Node *tree);
void compile_function(Node *function);
void compile_body(Node *body);
void compile_instructions(Node *instructions);
PrimitiveType compile_instruction(Node *instruction);

/* logical blocks */
void compile_while(Node *expr);
void compile_if(Node *expr);

/* instructions */

/* expressions */
PrimitiveType compile_assign(Node *instr);
PrimitiveType compile_cmp(Node *expr);
PrimitiveType compile_fcall(Node *expr);
PrimitiveType compile_binary_op(Node *expr);
PrimitiveType compile_unary_op(Node *expr);
PrimitiveType compile_ident(Node *expr);
/**
 * @brief 
 * 
 * if 1stchild() then goto true
 * 2ndchild() 
 * transform 2ndchild() stacked value to 0 or 1 
 * goto false
 * true:
 * push 1
 * false:
 * 
 * 
 * @param expr 
 * @return PrimitiveType 
 */
PrimitiveType compile_or(Node *expr);

/**
 * @brief 
 * 
 * if 1stchild() then goto true
 * 2ndchild() 
 * transform 2ndchild() stacked value to 0 or 1 
 * goto false
 * true:
 * push 1
 * false:
 * 
 * 
 * @param expr 
 * @return PrimitiveType 
 */
PrimitiveType compile_and(Node *expr);

/* Utils */
/**
 * @brief Get the symbol from local table, then global
 * (local var hides the global)
 * 
 * @param context 
 * @param ident 
 * @return Symbol* The symbol 
 * @exception exit failure if equired symbol not found
 */
Symbol * symbol_from_context(Node *node);
Symbol * symbol_from_context_(const char *ident);

#endif /* _TPC_COMPILE_ */