%option noyywrap nodefault yylineno

%{
#include "orc/workflow/parser/syntax.h"  
#include "orc/workflow/parser/parser.h"  
%}

%%

"if"        { return IF; }
"else"      { return ELSE; }
"do"        { return DO; }
"while"     { return WHILE; }
"workflow"  { return WORKFLOW; }
"return"    { return RETURN; }

[A-Za-z][A-Za-z0-9_]* { yylval.node = workflow_syntax_new_name_stmt(yytext); return NAME; }

[ \t\n]+ /* ignore space */

. { return *yytext; }

%%
