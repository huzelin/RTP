%{
#include "orc/workflow/parser/syntax.h"
#include "orc/workflow/parser/compiler.h"

#define YYDEBUG 1

int yylex();
void yyerror(struct workflow_syntax* root_node, const char* s);

%}

%parse-param {struct workflow_syntax* root_node}

%union {
  struct workflow_syntax* node;  
}

%token <node> NAME
%token IF ELSE DO WHILE WORKFLOW RETURN

%type <node> workflowlist workflow block stmtlist stmt exp
%type <node> ifstmt elsestmt whilestmt dowhilestmt returnstmt

%start workflowlist

%%

workflowlist: %empty {}
            | workflowlist workflow { workflow_syntax_add_stmt(root_node, $2); }
            ;

workflow: WORKFLOW NAME block {
          $$ = workflow_syntax_new_workflow($2, $3); 
        }
        ;

block: '{' stmtlist '}' { $$ = $2; }
     ;

stmtlist: %empty { $$ = workflow_syntax_new_stmt_list(); }
        | stmtlist stmt { workflow_syntax_add_stmt($1, $2); $$ = $1; }
        ;

stmt: ifstmt
    | whilestmt
    | dowhilestmt
    | returnstmt
    | exp
    ;

ifstmt: IF exp block elsestmt {
        $$ = workflow_syntax_new_if_stmt($2, $3, $4); 
      }
      ;

elsestmt: %empty { $$ = workflow_syntax_new_empty_stmt(); }
        | ELSE block { $$ = $2; }
        | ELSE ifstmt { $$ = $2; }

whilestmt: WHILE exp block {
            $$ = workflow_syntax_new_while_stmt($2, $3);
         }
         ;

dowhilestmt: DO block WHILE exp {
              $$ = workflow_syntax_new_dowhile_stmt($2, $4);
           }
           ;

returnstmt: RETURN { $$ = workflow_syntax_new_return_stmt(); }

exp: '!' exp { $$ = workflow_syntax_new_unop_exp(WORKFLOW_SYNTAX_OPKIND_NOT, $2); }
   | '(' exp ')' { $$ = $2; }
   | NAME
   ;

%%

void yyerror(struct workflow_syntax* root_node, const char* s) {
  workflow_compiler_error_report(s);
}
