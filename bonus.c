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
#include <stdarg.h>
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

#define LASSERT(args, cond, fmt, ...)                                          \
  if (!(cond)) {                                                               \
    lval *err = lval_err(fmt, ##__VA_ARGS__);                                  \
    lval_del(args);                                                            \
    return err;                                                                \
  }

#define TYPERR(argc, cons, ...)                                                \
  if (!(cond)) {                                                               \
    lval *err = lval_err("Function '%s' got incorrect type for arguments. "    \
                         "Got %s. Expected %s.",                               \
                         ##__VA_ARGS__);                                       \
    lval_del(args);                                                            \
    return err;                                                                \
  }

// create enums (enumarations) for possible erros
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Forward declarations

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

// create enum types for lispatron values
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

const char *defFunc[14] = {"head", "tail", "list", "join", "eval",
                           "cons", "len",  "init", "def",  "+",
                           "-",    "*",    "/",    "%"};

typedef lval *(*lbuiltin)(lenv *, lval *);

// defining lval structure
typedef struct lval {
  int type;
  long num;

  char *err;
  char *sym;
  lbuiltin fun;

  int count;
  struct lval **cell;
} lval;

struct lenv {
  int count;
  char **syms;
  lval **vals;
};

char *ltype_name(int t) {
  switch (t) {
  case LVAL_FUN:
    return "Function";
  case LVAL_NUM:
    return "Number";
  case LVAL_ERR:
    return "Error";
  case LVAL_SYM:
    return "Symbol";
  case LVAL_QEXPR:
    return "Q-expression";
  case LVAL_SEXPR:
    return "S-expression";
  default:
    return "Unknown";
  }
}

lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

// function do define lval with number
lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// function do define lval with error
lval *lval_err(char *fmt, ...) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  va_list va;
  va_start(va, fmt);

  v->err = malloc(512);

  vsnprintf(v->err, 511, fmt, va);

  v->err = realloc(v->err, strlen(v->err) + 1);

  va_end(va);

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

lval *lval_fun(lbuiltin func) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_FUN:
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

void lenv_del(lenv *e) {
  for (int i = 0; i < e->count; ++i) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
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

lval *lval_copy(lval *v) {
  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
  case LVAL_FUN:
    x->fun = v->fun;
    break;
  case LVAL_NUM:
    x->num = v->num;
    break;

  case LVAL_ERR:
    x->err = malloc(strlen(v->err) + 1);
    strcpy(x->err, v->err);
    break;
  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym) + 1);
    strcpy(x->sym, v->sym);
    break;

  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval *) * x->count);
    for (int i = 0; i < x->count; ++i) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
    break;
  }

  return x;
}

lval *lenv_get(lenv *e, lval *k) {
  for (int i = 0; i < e->count; ++i) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  return lval_err("unbound symbol %s!", k->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {
  if (e->count > 15) {
    for (int i = 0; i < 14; i++) {
      if (strcmp(k->sym, defFunc[i]) == 0) {
        printf("Cannot redefine default function\n");
        return;
      }
    }
  }

  for (int i = 0; i < e->count; ++i) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  e->count++;
  e->vals = realloc(e->vals, sizeof(lval *) * e->count);
  e->syms = realloc(e->syms, sizeof(char *) * e->count);

  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
}

lval *lval_eval(lenv *e, lval *v);

void lval_print(lenv *e, lval *v);

void lval_expr_print(lenv *e, lval *v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; ++i) {

    lval_print(e, v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lenv *e, lval *v) {
  switch (v->type) {
  case LVAL_FUN:
    printf("<function>: ");
    for (int i = 0; i < e->count; i++) {
      if (v->fun == e->vals[i]->fun) {
        printf("%s", e->syms[i]);
        break;
      }
    }
    break;
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
    lval_expr_print(e, v, '(', ')');
    break;
  case LVAL_QEXPR:
    lval_expr_print(e, v, '{', '}');
    break;
  }
}

void lval_println(lenv *e, lval *v) {
  lval_print(e, v);
  putchar('\n');
}

lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_cons(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval *a);
lval *builtin_init(lenv *e, lval *a);
lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_enviroment(lenv *e, lval *a);
lval *builtin_exit(lenv *e, lval *a);

/*
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
    return builtin_cons(e, a);
  if (strcmp("len", func) == 0)
    return builtin_len(a);
  if (strcmp("init", func) == 0)
    return builtin_init(a);
  if (strstr("+-/%*", func))
    return builtin_op(a, func);
  lval_del(a);
  return lval_err("Unknown Function!");
}
*/

lval *builtin_add(lenv *e, lval *a) { return builtin_op(e, a, "+"); }
lval *builtin_min(lenv *e, lval *a) { return builtin_op(e, a, "-"); }
lval *builtin_mul(lenv *e, lval *a) { return builtin_op(e, a, "*"); }
lval *builtin_div(lenv *e, lval *a) { return builtin_op(e, a, "/"); }
lval *builtin_mod(lenv *e, lval *a) { return builtin_op(e, a, "%"); }
// lval *lbuiltin_cons(lenv *e, lval *a) { return builtin_cons(e, a); }

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv *e) {
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "init", builtin_init);

  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "enviroment", builtin_enviroment);
  lenv_add_builtin(e, "exit", builtin_exit);

  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_min);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);
}

lval *builtin_op(lenv *e, lval *a, char *op) {

  // ensure all arguments are numbers
  for (int i = 0; i < a->count; ++i) {
    LASSERT(
        a, a->cell[i]->type == LVAL_NUM,
        "Function '%s' got incorrect type for arguments. Got %s. Expected %s.",
        op, ltype_name(a->cell[i]->type), ltype_name(LVAL_NUM))
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

lval *builtin_head(lenv *e, lval *a) {

  LASSERT(a, a->count == 1,
          "Function 'head' passed too many arguments. Got %d. Expected %d.",
          a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'head' passed incorrect type for argument 0. Got %s. "
          "Expected %s.",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed{}!");

  lval *v = lval_take(a, 0);

  while (v->count > 1)
    lval_del(lval_pop(v, 1));
  return v;
}

lval *builtin_tail(lenv *e, lval *a) {
  LASSERT(a, a->count == 1,
          "Function 'tail' passed too many arguments. Got %d. Expected %d",
          a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect types. Got %s. Expected %s.",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed{}!");

  lval *v = lval_take(a, 0);

  lval_del(lval_pop(v, 0));
  return v;
}

lval *builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval *builtin_eval(lenv *e, lval *a) {
  LASSERT(a, a->count == 1,
          "Function 'eval' passed to many arguments. Got %d. Expected %d.",
          a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'eval' passed incorrect type. Got %s. Expected %s",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval *lval_join(lval *x, lval *y);
lval *builtin_join(lenv *e, lval *a) {

  for (int i = 0; i < a->count; ++i) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
            "Function 'join' passed incorrect type. Got %s. Expected %s.",
            ltype_name(a->cell[i]->type), ltype_name(LVAL_QEXPR));
  }

  lval *x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval *builtin_cons(lenv *e, lval *a) {
  LASSERT(a, a->cell[1]->count != 0, "Function 'cons' passed {}");
  LASSERT(a, a->count != 1,
          "Function 'cons' passed invalid arguments. Got %d argument of %s "
          "type. Expected %d arguments of %s type",
          a->cell[0]->count, ltype_name(a->cell[0]->type), 1,
          ltype_name(LVAL_QEXPR));
  LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
          "Function 'cons' didn't pass qexpr");
  LASSERT(a, a->count == 2,
          "Function 'cons' passed too many arguments. Got %d. Expected %d.",
          a->count, 1);

  /* has a memory leak and crushes on 'cons 1 {1 2 3 4 5 6}'
  a->cell[1]->count++;
  a->cell[1]->cell =
      realloc(a->cell[1]->cell, sizeof(lval *) * a->cell[1]->count);

  memmove(&a->cell[1]->cell[1], &a->cell[1]->cell[0],
          sizeof(lval *) * a->cell[1]->count - 1);

  a->cell[1]->cell[0] = lval_num(a->cell[0]->num);

  return a->cell[1];
  */

  // also has a memoy leak but doesn't chrushes on 'cons 1 {1 2 3 4 5 6}'
  lval *x = lval_qexpr();
  x->count = a->cell[1]->count + 1;
  // x->cell = malloc(sizeof(lval) * x->count);
  x->cell = realloc(x->cell, sizeof(lval) * x->count);

  memcpy(x->cell + 1, a->cell[1]->cell, sizeof(lval *) * a->cell[1]->count);
  x->cell[0] = lval_num(a->cell[0]->num);

  return x;

  /* it also has memory leaks
  lval *x = lval_qexpr();
  lval_add(x, lval_num(a->cell[0]->num));
  lval_join(x, a->cell[1]);

  return x;
  */
}

lval *builtin_len(lenv *e, lval *a) {
  LASSERT(a, a->count != 0, "Function 'len' passed {}.");
  LASSERT(a, a->count == 1,
          "Function 'len' passed too many arguments. Got %d. Expected %d.",
          a->count, 1);
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed invalid arguments. Got %s. Expected %s",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  return lval_num(a->cell[0]->count);
}

lval *builtin_init(lenv *e, lval *a) {
  LASSERT(a, a->cell[0]->count != 0, "Function 'len' passed {}.");
  LASSERT(a, a->count == 1,
          "Function 'len' passed too many arguments. Got %d. Expected %d.",
          a->count, 1);
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed invalid arguments. Got %d. Expected %s",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  a->cell[0]->count--;
  lval_del(a->cell[0]->cell[a->cell[0]->count]);

  return a;
}

lval *builtin_def(lenv *e, lval *a) {
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'def' passed incorrect type. Got %s. Expected %s.",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  lval *syms = a->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM,
            "Function 'def' cannot define non-symbol");
  }

  LASSERT(a, syms->count == a->count - 1,
          "Function 'def' cannot define incorrect number of values to symbols");

  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i + 1]);
  }

  lval_del(a);
  return lval_sexpr();
}

lval *builtin_enviroment(lenv *e, lval *a) {

  LASSERT(
      a, a->count == 1,
      "Function 'enviroment' passed to many arguments. Got %d. Expected %d.",
      a->count, 0);

  for (int i = 0; i < e->count; i++) {
    printf("%d) %s\n", i + 1, e->syms[i]);
  }

  return a;
}

lval *builtin_exit(lenv *e, lval *a) {
  printf("Exiting with code %li.\n", a->cell[0]->num);
  exit(a->cell[0]->num);
  return a;
}

lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval *lval_eval_sexpr(lenv *e, lval *v) {

  // evaluate children
  for (int i = 0; i < v->count; ++i) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v);
    return lval_err("first element is not a function");
  }

  lval *result = f->fun(e, v);
  lval_del(f);
  return result;
}

lval *lval_eval(lenv *e, lval *v) {

  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR)
    return lval_eval_sexpr(e, v);
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
      "                                                                    \ 
      number    : /-?[0-9]+/  ;                                            \
      symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;                       \
      sexpr     : '(' <expr>* ')' ;                                        \
      qexpr     : '{' <expr>* '}' ;                                        \
      expr      : <number>  | <symbol> | <sexpr> | <qexpr> ;               \
      lispatron : /^/ <expr>* /$/ ;                                        \
    ",
      Number, Symbol, Sexpr, Qexpr, Expr, Lispatron);

  puts("lispatron version 0.0.1");
  puts("Press Ctrl+c to exit \n");

  lenv *e = lenv_new();
  lenv_add_builtins(e);

  while (1) {
    char *input = readline("lispatron> ");
    add_history(input);

    // Attempt to parse input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispatron, &r)) {
      // on success print AST
      lval *x = lval_eval(e, lval_read(r.output));
      lval_println(e, x);
      lval_del(x);

      // mpc_ast_print(r.output);
      mpc_ast_delete(r.output);

    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }
  lenv_del(e);

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispatron);
  return 0;
}
