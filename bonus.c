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

int main(int argc, char** argv) {

  // Create some parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispatron = mpc_new("lispatron");
  mpc_parser_t* Letter = mpc_new("letter");
  mpc_parser_t* Word = mpc_new("word");

  //define them with the following language
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \ 
      letter    :  /aba*/ | /a*ba*/ ;                               \
      word      : /pit/ | /pot/ | /respite/ ;            \
      number    : /-?[0-9]+/ ;                            \
      operator  : '+' | '-' | '*' | '/' | '%' | /add/ | /sub/ | /mul/ | /div/ ;                 \
      expr      : /<number>'.'<number>/ | <number> | '(' <operator> <expr>+ ')' ; \
      lispatron : /^/ <operator> <expr>+ | <letter>+ | <word> /$/ ;            \
    ",
  Letter, Word, Number, Operator, Expr, Lispatron);


  puts("lispatron version 0.0.1");
  puts("Press Ctrl+c to exit \n"); 

  while (1) {
    char* input = readline("lispatron> "); 
    add_history(input);

    // Attempt to parse input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispatron, &r)) {
      // on success print AST
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispatron, Letter, Word);
  return 0;
}

