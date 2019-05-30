#include "orc/workflow/parser/compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static compiler_error_reporter g_reporter = NULL;

static void default_reporter(const char* s, va_list vl) {
  vprintf(s, vl);
  printf("\n");
}

void workflow_set_compiler_error_reporter(compiler_error_reporter reporter) {
  g_reporter = reporter;
}

void workflow_compiler_error_report(const char* s, ...) {
  va_list ap;
  va_start(ap, s);

  if (g_reporter == NULL) {
    g_reporter = &default_reporter;
  }

  g_reporter(s, ap);
  va_end(ap);
}

extern int yydebug;
extern int yyparse(struct workflow_syntax* root);
extern void yyrestart(FILE* input_file);

int workflow_compile(const char* file, struct workflow_syntax* root) {
  FILE* f = fopen(file, "r");
  if (f == NULL) {
    workflow_compiler_error_report("open file: %s fail.", file);
    return -1;
  }

  // yydebug = 1;
  yyrestart(f);
  return yyparse(root);
}
