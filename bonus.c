#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef _WIN32
#include <string.h>

static char input[2048]; //declare a buffer size of size 2048

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
// #include <editline/history.h>
#endif

/*
typedef struct mpc_ast_t {
  char* tag;
  char* contents;
  mpc_state_t state;
  int children_num;
  struct mpc_ast_t** children;
} mpc_ast_t;
*/
long leaves(mpc_ast_t* t) {
  if (t->children_num==0) return 1;
  if (t->children_num>=1) {
    long total = 1; 
    for (int i = 0; i<t->children_num; ++i) {
      total = total + leaves(t->children[i]);
    }
    return total;
  }
  return 0;
}

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+")==0) return x+y;
  if (strcmp(op, "-")==0) return x-y;
  if (strcmp(op, "*")==0) return x*y;
  if (strcmp(op, "/")==0) return x/y;
  return 0;
}

long eval(mpc_ast_t* t) {

  //If tagged as number return it directly
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  //The operator is always second child
  char* op = t->children[1]->contents;

  long x = eval(t->children[2]);


  int i = 3;
  while(strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {

  // Create some parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispatron = mpc_new("lispatron");

  //define them with the following language
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                      \ 
      number    : /-?[0-9]+/  ;                            \
      operator  : '+' | '-' | '*' | '/' ;                  \
      expr      : <number>  | '(' <operator> <expr>+ ')' ; \
      lispatron : /^/ <operator> <expr>+ /$/ ;             \
    ",
   Number ,Operator, Expr, Lispatron);


  puts("lispatron version 0.0.1");
  puts("Press Ctrl+c to exit \n"); 

  while (1) {
    char* input = readline("lispatron> "); 
    add_history(input);

    // Attempt to parse input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispatron, &r)) {
      // on success print AST
      long result = eval(r.output);
      long leaf = leaves(r.output);
      printf("%li\n", result);
      printf("Total amount of leaves is: %li\n", leaf);
      
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
      
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispatron);
  return 0;
}

