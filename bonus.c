/*
typedef struct mpc_ast_t {
  char* tag;
  char* contents;
  mpc_state_t state;
  int children_num;
  struct mpc_ast_t** children;
} mpc_ast_t;
*/
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

// create enums (enumarations) for possible erros
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

// create enum types for lispatron values
enum {LVAL_NUM, LVAL_ERR};

// defining lval structure
typedef struct {
  int type;
  union {
    long num;
    int err;
  } 
} lval;

// function do define lval with number
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

// function do define lval with error 
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

// function to output lisp value
void lval_print(lval v) {
  switch (v.type) {

    case LVAL_NUM: printf("%li", v.num); break;
    
    case LVAL_ERR:
      if (v.err==LERR_DIV_ZERO) printf("Error: Division by zero!");
      if (v.err==LERR_BAD_OP) printf("Error: Invalid operator!");
      if (v.err==LERR_BAD_NUM) printf("Error: Invalid number!");
    break;

  }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

  // check for error types
  if (x.type == LVAL_ERR) return x;
  if (y.type == LVAL_ERR) return y;


  if (strcmp(op, "+")==0) return lval_num(x.num + y.num);
  if (strcmp(op, "-")==0) return lval_num(x.num - y.num);
  if (strcmp(op, "*")==0) return lval_num(x.num * y.num);
  if (strcmp(op, "/")==0) {
    // printf("\n does it even happen \n");
    return y.num == 0 
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "%")==0) return lval_num(x.num % y.num);
  if (strcmp(op, "^")==0) {
    long res = 1;
    for (int i = 0; i<y.num; ++i) {
      res *= x.num; 
    }
    return lval_num(res);
  }
  if (strcmp(op, "max")==0) {
    if (x.num>y.num) return lval_num(x.num);
    else return lval_num(y.num);
  }
  if (strcmp(op, "min")==0) {
    if (x.num<y.num) return lval_num(x.num);
    else return lval_num(y.num);
  }
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

  //If tagged as number return it directly
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    // printf(" asdf %li", x);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  //The operator is always second child
  char* op = t->children[1]->contents;
  // puts(op);

  lval x = eval(t->children[2]);
  // printf(" this is %li", x.num);

  if (t->children_num==4) {
    x.num = -x.num;
    return x;
  };

  int i = 3;
  while(strstr(t->children[i]->tag, "expr")) {
    // printf(" | last test: %li | ", eval(t->children[i]));
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
    "                                                                      \ 
      number    : /-?[0-9]+/  ;                                            \
      operator  : '+' | '-' | '*' | '/' | '%' | '^' | /max/ | /min/ ;      \
      expr      : <number>  | '(' <operator> <expr>+ ')' ;                 \
      lispatron : /^/ <operator> <expr>+ /$/ ;                             \
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
      lval result = eval(r.output);
      lval_println(result);

      // mpc_ast_print(r.output);
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

