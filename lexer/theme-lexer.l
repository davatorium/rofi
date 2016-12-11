%option noyywrap nounput batch debug

%{
#include <stdio.h>


#include "lexer/theme-parser.h"
int yylex(void);
#define YY_DECL int yylex()

%}

%%
"//"            {
    int c;
    while ((c = input()) != EOF)
        if (c == '\n') {
            break;
        }
}
"/*"            {
    int c = 0, p;
    int nesting_depth = 1;
    while (nesting_depth) {
        p = c;
        c = input();
        switch (c) {
        case '*':  if (p == '/') { c = 0; nesting_depth++; } break;
        case '/':  if (p == '*') { c = 0; nesting_depth--; } break;
        case '\n':  break;
        case EOF: nesting_depth = 0; break;
        default: ;
        }
    }
}
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

#[0-9A-Fa-f]{8}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)strtoull ( &yytext[1], NULL, 16);
    yylval.colorval.alpha = val.a/255.0;
    yylval.colorval.red   = val.r/255.0;
    yylval.colorval.green = val.g/255.0;
    yylval.colorval.blue  = val.b/255.0;
    return T_COLOR;
}
#[0-9A-Fa-f]{6}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)g_ascii_strtoull ( &yytext[1], NULL, 16);
    yylval.colorval.alpha = 1.0;
    yylval.colorval.red   = val.r/255.0;
    yylval.colorval.green = val.g/255.0;
    yylval.colorval.blue  = val.b/255.0;
    return T_COLOR;
}
rgba\([0-9]{1,3},[0-9]{1,3},[0-9]{1,3},[01](\.[0-9]+)?\) {
    char *endptr = &yytext[5];
    yylval.colorval.red = g_ascii_strtoull ( endptr, &endptr, 10);
    yylval.colorval.green= g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval.colorval.blue= g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval.colorval.alpha= g_ascii_strtod ( endptr+1, NULL);
    return T_COLOR;
}
rgb\([0-9]{1,3},[0-9]{1,3},[0-9]{1,3}\) {
    char *endptr = &yytext[4];
    yylval.colorval.red   = g_ascii_strtoull ( endptr, &endptr, 10);
    yylval.colorval.green = g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval.colorval.blue  = g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval.colorval.alpha = 1.0;
    return T_COLOR;
}
    [\r\n]+		;

<*><<EOF>>  {
    yyterminate();
}
%%
