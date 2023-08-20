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

static char input[2048]; // declare a buffer size of size 2048

/* Fake readline function */
char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char *unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
// #include <editline/history.h>
#endif

#define LASSERT(args, cond, err)                                               \
  if (!(cond)) {                                                               \
    lval_del(args);                                                            \
    return lval_err(err);                                                      \
  }

#define NUMSERT(args)                                                          \
  if (args == ERANGE)                                                          \
    return lval_err("Invalid Number.");

// create enums (enumarations) for possible erros
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// create enum types for lispatron values
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

// defining lval structure
typedef struct lval {
  int type;
  long num;

  char *err;
  char *sym;

  int count;
  struct lval **cell;
} lval;

// function do define lval with number
lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// function do define lval with error
lval *lval_err(char *m) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval *lval_qexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    break;

  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;

  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; ++i) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  }
  free(v);
}

lval *lval_read_num(mpc_ast_t *t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("Invalid Number");
}

lval *lval_add(lval *v, lval *x);
lval *lval_read(mpc_ast_t *t) {

  if (strstr(t->tag, "number"))
    return lval_read_num(t);
  if (strstr(t->tag, "symbol"))
    return lval_sym(t->contents);
  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0)
    x = lval_sexpr();
  if (strstr(t->tag, "sexpr"))
    x = lval_sexpr();
  if (strstr(t->tag, "qexpr"))
    x = lval_qexpr();

  for (int i = 0; i < t->children_num; ++i) {
    if (strcmp(t->children[i]->contents, "(") == 0)
      continue;
    if (strcmp(t->children[i]->contents, ")") == 0)
      continue;
    if (strcmp(t->children[i]->contents, "{") == 0)
      continue;
    if (strcmp(t->children[i]->contents, "}") == 0)
      continue;
    if (strcmp(t->children[i]->tag, "regex") == 0)
      continue;
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval *lval_add(lval *v, lval *x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

lval *lval_pop(lval *v, int i) {

  lval *x = v->cell[i];

  // shift memory after the item at "i" over the top
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  return x;
}

lval *lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval *lval_eval(lval *v);

void lval_print(lval *v);

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; ++i) {

    lval_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->num);
    break;
  case LVAL_ERR:
    printf("Error: %s", v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
  }
}

void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}

lval *builtin_head(lval *a);
lval *builtin_tail(lval *a);
lval *builtin_list(lval *a);
lval *builtin_join(lval *a);
lval *builtin_eval(lval *a);
lval *builtin_cons(lval *a);
lval *builtin_op(lval *a, char *op);

lval *builtin(lval *a, char *func) {
  if (strcmp("list", func) == 0)
    return builtin_list(a);
  if (strcmp("head", func) == 0)
    return builtin_head(a);
  if (strcmp("tail", func) == 0)
    return builtin_tail(a);
  if (strcmp("join", func) == 0)
    return builtin_join(a);
  if (strcmp("eval", func) == 0)
    return builtin_eval(a);
  if (strcmp("cons", func) == 0)
    return builtin_cons(a);
  if (strstr("+-/*%", func))
    return builtin_op(a, func);
  lval_del(a);
  return lval_err("Unknown Function!");
}

lval *builtin_op(lval *a, char *op) {

  // ensure all arguments are numbers
  for (int i = 0; i < a->count; ++i) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non_numbers");
    }
  }

  lval *x = lval_pop(a, 0);

  if (strcmp(op, "-") == 0 && a->count == 0) {
    x->num = -x->num;
  }

  // while there are still elements remaining
  while (a->count > 0) {
    lval *y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0)
      x->num += y->num;
    if (strcmp(op, "-") == 0)
      x->num -= y->num;
    if (strcmp(op, "*") == 0)
      x->num *= y->num;
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero.");
        break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero.");
        break;
      }
      x->num %= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval *builtin_head(lval *a) {

  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'head' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed{}!");

  lval *v = lval_take(a, 0);

  while (v->count > 1)
    lval_del(lval_pop(v, 1));
  return v;
}

lval *builtin_tail(lval *a) {
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types!");

  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed{}!");

  lval *v = lval_take(a, 0);

  lval_del(lval_pop(v, 0));
  return v;
}

lval *builtin_list(lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval *builtin_eval(lval *a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed to many arguments");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'eval' passed incorrect type!");

  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval *lval_join(lval *x, lval *y);
lval *builtin_join(lval *a) {

  for (int i = 0; i < a->count; ++i) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
            "Function 'join' passed incorrect type.");
  }

  lval *x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval *builtin_cons(lval *a) {
  printf("%d\n", a->count);
  LASSERT(a, a->count != 0, "Function 'cons' passed {}");
  LASSERT(a, a->count != 1, "Function 'cons' passed invalid arguments");
  LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
          "Function 'cons' didn't pass qexpr");
  LASSERT(a, a->count == 2, "Function 'cons' passed too many arguments");

  /* has a memory leak and crushes on 'cons 1 {1 2 3 4 5 6}'
  a->cell[1]->count++;
  a->cell[1]->cell =
      realloc(a->cell[1]->cell, sizeof(lval *) * a->cell[1]->count);

  memmove(&a->cell[1]->cell[1], &a->cell[1]->cell[0],
          sizeof(lval *) * a->cell[1]->count - 1);

  a->cell[1]->cell[0] = lval_num(a->cell[0]->num);

  return a->cell[1];
  */

  lval *x = lval_qexpr();
  x->count = a->cell[1]->count + 1;
  x->cell = malloc(sizeof(lval *) * x->count);
  printf("%ld\n", sizeof(lval *));

  memmove(x->cell + 1, a->cell[1]->cell, sizeof(lval *) * a->cell[1]->count);
  printf("%ld\n", sizeof(lval *));
  x->cell[0] = lval_num(a->cell[0]->num);

  return x;
}

lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval *lval_eval_sexpr(lval *v) {

  // evaluate children
  for (int i = 0; i < v->count; ++i) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  for (int i = 0; i < v->count; ++i) {
    if (v->cell[i]->type == LVAL_ERR)
      return lval_take(v, i);
  }

  // empty expression
  if (v->count == 0)
    return v;

  // single expression
  if (v->count == 1)
    return lval_take(v, 0);

  // ensure that the first element is symbol
  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol.");
  }

  lval *result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

lval *lval_eval(lval *v) {

  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(v);
  return v;
}

int main(int argc, char **argv) {

  // Create some parsers
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Qexpr = mpc_new("qexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispatron = mpc_new("lispatron");

  // define them with the following language
  mpca_lang(
      MPCA_LANG_DEFAULT,
      "                                                                      \ 
      number    : /-?[0-9]+/  ;                                            \
      symbol    : \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\"     \
                | \"cons\"                                                 \
                | '+' | '-' | '*' | '/' | '%' | '^' | /max/ | /min/ ;      \
      sexpr     : '(' <expr>* ')' ;                                        \
      qexpr     : '{' <expr>* '}' ;                                        \
      expr      : <number>  | <symbol> | <sexpr> | <qexpr> ;               \
      lispatron : /^/ <expr>* /$/ ;                                        \
    ",
      Number, Symbol, Sexpr, Qexpr, Expr, Lispatron);

  puts("lispatron version 0.0.1");
  puts("Press Ctrl+c to exit \n");

  while (1) {
    char *input = readline("lispatron> ");
    add_history(input);

    // Attempt to parse input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispatron, &r)) {
      // on success print AST
      lval *x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);

      // mpc_ast_print(r.output);
      mpc_ast_delete(r.output);

    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispatron);
  return 0;
}
