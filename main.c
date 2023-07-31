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

  puts("lispatron version 0.0.1");
  puts("Press Ctrl+c to exit \n"); 

  while (1) {
    char* input = readline("lispatron> "); 
    add_history(input);

    input[3]='a';

    printf("check: %s\n", input);
    puts("lispatron version 0.0.1");
    puts("Press Ctrl+c to exit \n"); 

    free(input);
  }

  return 0;
}

