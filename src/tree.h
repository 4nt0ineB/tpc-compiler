/* tree.h */


#ifndef _TPC_TREE_
#define _TPC_TREE_

/*  We use the `_` as prefix for reserved identifiers */
/* 
  assign        for the non-terminating   `=`
  not           for non-terminating `!`
  unary_sign    e.g.: `-`(6+6) or `+`6 
*/

/*
  0 <-> 10 bloc logique
*/
#define FOREACH_LABEL(DO_THING) \
        DO_THING(program)       \
        DO_THING(variables)     \
        DO_THING(functions)     \
        DO_THING(function)      \
        DO_THING(signature)     \
        DO_THING(parameters)    \
        DO_THING(body)          \
        DO_THING(instructions)  \
        DO_THING(assign)        \
        DO_THING(fcall)         \
        DO_THING(args)          \
        DO_THING(_while)        \
        DO_THING(_return)       \
        DO_THING(_if)           \
        DO_THING(_else)         \
        DO_THING(_void)         \
        DO_THING(character)     \
        DO_THING(num)           \
        DO_THING(ident)         \
        DO_THING(type)          \
        DO_THING(eq)            \
        DO_THING(order)         \
        DO_THING(addsub)        \
        DO_THING(divstar)       \
        DO_THING(or)            \
        DO_THING(and)           \
        DO_THING(not)           \
        DO_THING(unary_sign)    \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
  FOREACH_LABEL(GENERATE_ENUM)
  /* list all other node labels, if any */
  /* The list must coincide with the string array in tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} label_t;

typedef struct Node {
  label_t label;
  struct Node *firstChild, *nextSibling;
  int lineno;
  /* associated data */ 
  struct Node *node;
  char byte;
  int num;
  char ident[64];
  char comp[3];
  char type[30];
} Node;

Node *makeNode(label_t label);
void addSibling(Node *node, Node *sibling);
void addChild(Node *parent, Node *child);
void deleteTree(Node*node);
void printTree(Node *node);
void node_sprintf(Node *node, char *dest);

#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling

#endif /* _TPC_TREE_ */