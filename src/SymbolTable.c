
#include "SymbolTable.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define COLOR_ITALIC "\e[1m"
#define COLOR_ITALIC_OFF   "\e[m"

Symbol * new_symbol(const char *ident, SymbolType symbol_type) {
    Symbol *symbol = (Symbol *) calloc(1, sizeof(Symbol));
    if(!symbol){
        fprintf(stderr, "Error alloc");
        exit(EXIT_FAILURE);
    }
    int ident_len = strlen(ident);
    if(ident_len > IDENT_LENGTH){
        fprintf(stderr, "Max identifier length is 31");
        exit(EXIT_FAILURE);
    }
    strcpy(symbol->ident, ident);
    symbol->type = symbol_type;
    /* 0 would be a valid relative address, so we init with -1 */
    symbol->address = -1; 
    return symbol;
}


SymbolTable *new_table(){
    SymbolTable *table = (SymbolTable *) malloc(sizeof(SymbolTable));
    if(!table){
        perror("Failed to allocate a symbol table");
        exit(EXIT_FAILURE);
    }
    memset(table->table, 0, __BUCKETS * sizeof(void *));
    table->size = 0;
    return table;
}

int hash(const char *ident){
    int h = 0;
    char *c = (char *) ident;
    while(*c){
        h = __HASH_K * h + *c;
        c++;
    }
    return abs(h % (2 << 29)) % __BUCKETS;
}


Symbol * get_symbol(SymbolTable *table, const char *ident){
    assert(table);
    int key = hash(ident);
    /* Pick the bucket */
    Symbol *bucket_cursor = table->table[key];
    /* Find identifier in the bucket */
    while(bucket_cursor){
        if(!strcmp(bucket_cursor->ident, ident))
            return bucket_cursor;
        bucket_cursor = bucket_cursor->next;
    }
    return NULL;
}


/**
 * @brief Adding a symbol to a symbol table if 
 * not already present
 * 
 * @param symbol_table 
 * @param symbol 
 * @return int 1 if added, 0 otherwise
 */
int add_symbol(SymbolTable *table, Symbol *symbol){
    assert(symbol);
    assert(symbol);
    int key = hash(symbol->ident);
    /* Check if identifier already exists */
    if(get_symbol(table, symbol->ident))
        return 0;
    /* Add the symbol to the bucket, at the head : O(1) */
    symbol->next = table->table[key];
    table->table[key] = symbol;
    /* Update table size */
    if(symbol->type == PRIMITIVE){
        int t_size = primitive_type_size(symbol->type);
        if(t_size == -1){
             fprintf(stderr, "Unwanted behavior :"
            "asking for undefined primitive type, "
            "or the size of void for an expr or a var\n");
            exit(1);
        }
        table->size += t_size;
        /* Update symbol address */
        symbol->address = table->size;
    }
    return 1;
}

void free_symbol_chain(Symbol *head){
    if(!head) return;
    free_symbol_chain(head->next);
    free(head);
}

void clear_table(SymbolTable *table){
    int i;
    for(i = 0; i < __BUCKETS; i++){
        free_symbol_chain(table->table[i]);
        table->table[i] = NULL;
    }
}

void free_table(SymbolTable *table){
    int i;
    for(i = 0; i < __BUCKETS; i++){
        free_symbol_chain(table->table[i]);
    }
    free(table);
}

int primitive_type_size(PrimitiveType primitive_type){
    switch(primitive_type){
        case PT_INT:
        case PT_CHAR:
            return 4;
        default:
           return -1;
    }
}

void print_table(const SymbolTable *table, const char *title){
    assert(table);
    char type_label[6];
    int i;
    printf("\n┌[%s table]\n", title);
    for(i = 0; i < __BUCKETS; i++){
        Symbol *tmp = table->table[i];
        while(tmp){
            string_from_ptype(type_label, tmp->primitive);
            printf("│");
            if(tmp->type == 0){
                /* function */
                printf(COLOR_ITALIC "%-4s %s(" COLOR_ITALIC_OFF, type_label, tmp->ident);
                for(int i = 0; i < tmp->function.n_args; i++){
                    string_from_ptype(type_label, tmp->function.args_type[i]);
                    printf("%s,", type_label);
                }
                printf(")\n");
            }else{
                /* var */
                static const char padder[] = "......................"; // Many chars
                printf("%-4s %s%s[%d]\n",  type_label, tmp->ident, padder+strlen(tmp->ident), tmp->address);
            }
            tmp = tmp->next;
        }
    }
    printf("└─\n");
}

PrimitiveType ptype_from_string(const char *label){

    if(!strcmp("int", label)){
        return PT_INT;
    }
    if(!strcmp("char", label)){
        return PT_CHAR;
    }
    if(!strcmp("void", label)){
        return PT_VOID;
    }
    fprintf(stderr, "undefined type %s\n", label);
    return -1;
}

void string_from_ptype(char *dest, PrimitiveType ptype){
    switch (ptype){
    case PT_CHAR:
        sprintf(dest, "char");
        break;
    case PT_INT:
        sprintf(dest, "int");
        break;
    case PT_VOID:
        sprintf(dest, "void");
        break;
    default:
        fprintf(stderr, "Type error %d\n", ptype);
        exit(EXIT_FAILURE);
    }
}