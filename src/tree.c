/* tree.c */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
extern int lineno;       /* from lexer */

const char *StringFromLabel[] = {
  FOREACH_LABEL(GENERATE_STRING)
  /* list all other node labels, if any */
  /* The list must coincide with the label_t enum in tree.h */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
};

Node *makeNode(label_t label) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    printf("Run out of memory\n");
    exit(1);
  }
  node->label = label;
  node-> firstChild = node->nextSibling = NULL;
  node->lineno=lineno;
  return node;
}

void addSibling(Node *node, Node *sibling) {
  Node *curr = node;
  while (curr->nextSibling != NULL) {
    curr = curr->nextSibling;
  }
  curr->nextSibling = sibling;
}

void addChild(Node *parent, Node *child) {
  if (parent->firstChild == NULL) {
    parent->firstChild = child;
  }
  else {
    addSibling(parent->firstChild, child);
  }
}

void deleteTree(Node *node) {
  if (node->firstChild) {
    deleteTree(node->firstChild);
  }
  if (node->nextSibling) {
    deleteTree(node->nextSibling);
  }
  free(node);
}

void node_sprintf(Node *node, char *dest){
  switch(node->label){
    case type:
      sprintf(dest, "%s", node->type); break;
    case assign:
      sprintf(dest, "="); break;
    case num:
      sprintf(dest, "%d", node->num); break;
    case unary_sign:
      sprintf(dest, "%c", node->byte); break;
    case divstar:
    case addsub:
      sprintf(dest, "%c", node->byte); break;
    case character:
      sprintf(dest, "'%c'", node->byte); break;
    case eq:
    case order:
      sprintf(dest, "%s", node->comp); break;
    case ident:
      sprintf(dest, "%s", node->ident); break;
    default: 
      strcpy(dest, StringFromLabel[node->label]);
  }
}

void printTree(Node *node) {
  if(!node) return;
  static char buffer[64] = {0};
  static bool rightmost[128]; // tells if node is rightmost sibling
  static int depth = 0;       // depth of current node
  for (int i = 1; i < depth; i++) { // 2502 = vertical line
    printf(rightmost[i] ? "    " : "\u2502   ");
  }
  if (depth > 0) { // 2514 = L form; 2500 = horizontal line; 251c = vertical line and right horiz 
    printf(rightmost[depth] ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");
  }
  node_sprintf(node, buffer);
  printf("%s", buffer);
  printf("\n");
  depth++;
  for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
    rightmost[depth] = (child->nextSibling) ? false : true;
    printTree(child);
  }
  depth--;
}
