#ifndef ORC_WORKFLOW_PARSER_COMPILER_H__
#define ORC_WORKFLOW_PARSER_COMPILER_H__

#include <stdio.h>

#include "orc/workflow/parser/syntax.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*compiler_error_reporter)(const char*, va_list);
void workflow_set_compiler_error_reporter(compiler_error_reporter reporter);
void workflow_compiler_error_report(const char* s, ...);

int workflow_compile(const char* file, struct workflow_syntax* root);

#ifdef __cplusplus
}
#endif

#endif  // ORC_WORKFLOW_PARSER_COMPILER_H__
