%option noyywrap 

%{
#include <stdio.h>


#include "lexer/theme-parser.h"
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
[0-9]+\.[0-9]+      { yylval.fval = g_ascii_strtod(yytext, NULL); return T_DOUBLE;}
[0-9]+              { yylval.ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
(true|false)        { yylval.bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
[_\-a-zA-Z0-9]+        { yylval.sval = g_strdup(yytext); return N_STRING;}
\"[_\-a-zA-Z0-9 \t]+\"    { yytext[yyleng-1] = '\0'; yylval.sval = g_strdup(&yytext[1]); return T_STRING;}
#[0-9A-Fa-f]+       { yylval.colorval = (unsigned int)strtoull ( &yytext[1], NULL, 16); return T_COLOR;}
[\r\n]+		;

<*><<EOF>>  {
    yyterminate();
}
%%
