

#ifndef _TPC_SYMBOL_TABLE_
#define _TPC_SYMBOL_TABLE_

#include <stdlib.h>

#define IDENT_LENGTH 31
#define FUNCTION_MAX_ARGS 127

#define __BUCKETS 1008
#define __HASH_K 613

/**
 * @brief Available objects that can be stored as
 * symbol in a symbol table
 * 
 */
typedef enum {
      FUNCTION
    , PRIMITIVE
} SymbolType;

/**
 * @brief Available primitive types
 * 
 */
typedef enum {
      PT_VOID /* Not a type, but it is used 
      to represent the lack of data type */
    , PT_INT
    , PT_CHAR
} PrimitiveType;

/**
 * @brief Representation of a function to
 * be stored in a symbol table
 * 
 */
typedef struct function {
    int n_args;
    PrimitiveType type; /* The type of the function */
    PrimitiveType args_type[FUNCTION_MAX_ARGS];
} Function;

/**
 * @brief Implementation of a symbol for a symbol table
 * 
 */
typedef struct symbol {
    SymbolType type; /* The kind of symbol (var/function) */
    int address; /* The relative address of the next symbol */
    struct symbol *next; /* Next symbol. The bucket being a linked list of symbols*/
    union {
        PrimitiveType primitive;
        Function function;
    };
    char ident[IDENT_LENGTH + 1]; /* The associated identifier */
} Symbol;

typedef struct symbol_table {
    int size; /* The sum of the symbols size in the table */
    Symbol *table[__BUCKETS];
} SymbolTable;

/**
 * @brief Hold the symbol declaration 
 * during the compilation.
 * Its content change over time, during compilation
 * 
 */
typedef struct table_context{
    SymbolTable *global;
    SymbolTable *local;
} Context;


/**
 * @brief Hash a identifier to get a key 
 * for a open-bucket hash table of size __BUCKETS
 * 
 * @param ident 
 * @return int 
 */
int hash(const char *ident);

/**
 * @brief Alloc a new symbol table
 * 
 * @return SymbolTable* 
 */
SymbolTable * new_table();

/**
 * @brief Alloc a new symbol of type symbol_type
 * 
 * @param ident 
 * @param symbol_type 
 * @return Symbol* 
 */
Symbol * new_symbol(const char *ident, SymbolType symbol_type);

/**
 * @brief Add a symbol to the symbol table
 * if not already exFUNCTIONists 
 * @param table 
 * @param symbol 
 * @return int 1 if added, otherwise 0
 */
int add_symbol(SymbolTable *table, Symbol *symbol);

/**
 * @brief Get the symbol object
 * for a given identifier
 * 
 * @param table 
 * @param ident 
 * @return Symbol* 
 */
Symbol * get_symbol(SymbolTable *table, const char *ident);

/**
 * @brief Free a linked list of symbols
 * 
 * @param head 
 */
void free_symbol_chain(Symbol *head);

/**
 * @brief Free a table of symbol
 * 
 * @param table 
 */
void free_table(SymbolTable *table);

void clear_table(SymbolTable *table);

/**
 * @brief Return the size of a primitive type
 * 
 * @param primitive_type 
 * @return int 
 */
int primitive_type_size(PrimitiveType primitive_type);

void print_table(const SymbolTable *table, const char *title);


PrimitiveType ptype_from_string(const char *label);
void string_from_ptype(char *dest, PrimitiveType ptype);


#endif /* ifndef _TPC_SYMBOL_TABLE_ */