%option noyywrap 

%{
#include <stdio.h>


#include "lexer/theme-parser.tab.h"
int yylex(void);
#define YY_DECL int yylex()

%}

%%

"@"                 { return CLASS;}
"\{"                { return BOPEN;}
"\}"                { return BCLOSE;}
":"                 { return PSEP; }
";"                 { return PCLOSE;}
"."                 { return NSEP; }
[ \t]	; // ignore all whitespace
[0-9]+\.[0-9]+      { yylval.fval = g_ascii_strtod(yytext, NULL); return T_FLOAT;}
[0-9]+              { yylval.ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
(true|false)        { yylval.bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
[a-zA-Z0-9]+        { yylval.sval = g_strdup(yytext); return N_STRING;}
\"[a-zA-Z0-9]+\"    { yylval.sval = g_strdup(yytext); return T_STRING;}
#[0-9A-Fa-f]+       { yylval.colorval = (unsigned int)strtoull ( &yytext[1], NULL, 16); return T_COLOR;}
[\r\n]+		;

<*><<EOF>>  {
    yyterminate();
}
%%
