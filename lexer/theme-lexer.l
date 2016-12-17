%option noyywrap nounput
%option bison-locations

%{
#include <stdio.h>


#include "lexer/theme-parser.h"
int last_state = 0;
GQueue *queue = NULL;

%}
%{

#define YY_USER_ACTION {\
    yylloc->last_column+= yyleng;\
}
#define YY_LLOC_START {\
    yylloc->first_line = yylloc->last_line; yylloc->first_column = yylloc->last_column;\
}
%}
WHITESPACE [ \t]
WORD       [_\-a-zA-Z0-9]+
STRING     [ \t_\-a-zA-Z0-9]+
HEX        [0-9a-fA-F]
NUMBER     [0-9]

%x PROPERTIES
%x NAMESTR
%x ENTRY
%%

%{
YY_LLOC_START
%}
%{
if ( queue == NULL ){
printf("queue create\n");
    queue = g_queue_new ( );
}
%}
<*>"//"            {
    int c;
    while ((c = input()) != EOF){
        if (c == '\n') {
            yylloc->last_column = 1;
            yylloc->last_line ++;
            break;
        }
        yylloc->last_column++;
    }
    YY_LLOC_START
}
<*>"/*"            {
    int c = 0, p;
    int nesting_depth = 1;
    while (nesting_depth) {
        p = c;
        c = input();
        switch (c) {
        case '*': yylloc->last_column++; if (p == '/') { c = 0; nesting_depth++; } break;
        case '/': yylloc->last_column++; if (p == '*') { c = 0; nesting_depth--; } break;
        case '\n':  {
            yylloc->last_column = 1;
            yylloc->last_line ++;
            break;
        }
        case EOF: nesting_depth = 0; break;
        default:
            yylloc->last_column++;
        ;
        }
    }
    YY_LLOC_START
}

  /* Go into parsing an entry */
<INITIAL>"\{"                    {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(ENTRY);
    return BOPEN;
}
  /* Pop out of parsing an entry. */
<ENTRY>"\}"             {
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return BCLOSE;
}

<INITIAL>"@"            { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(NAMESTR);return CLASS_PREFIX;}
<INITIAL>"#"            { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(NAMESTR);return NAME_PREFIX;}
<INITIAL,NAMESTR>"."    { return NSEP; }
<INITIAL,ENTRY>{WORD}   { yylval->sval = g_strdup(yytext); return N_STRING;}
<NAMESTR>{WORD}         { yylval->sval = g_strdup(yytext); return NAME_ELEMENT;}

    /* After Namestr/Classstr we want to go to state str, then to  { */
<NAMESTR>{WHITESPACE}   { BEGIN(GPOINTER_TO_INT (g_queue_pop_head ( queue )));}
<INITIAL,ENTRY>{WHITESPACE}+	; // ignore all whitespace
<PROPERTIES>{WHITESPACE}+	; // ignore all whitespace

<INITIAL,ENTRY>":"               { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(PROPERTIES); return PSEP; }
<PROPERTIES>";"                  { BEGIN(GPOINTER_TO_INT ( g_queue_pop_head ( queue ))); return PCLOSE;}
<PROPERTIES>(true|false)         { yylval->bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
<PROPERTIES>{NUMBER}+            { yylval->ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
<PROPERTIES>{NUMBER}+\.{NUMBER}+ { yylval->fval = g_ascii_strtod(yytext, NULL); return T_DOUBLE;}
<PROPERTIES>\"{STRING}\"         { yytext[yyleng-1] = '\0'; yylval->sval = g_strdup(&yytext[1]); return T_STRING;}
<PROPERTIES>#{HEX}{8}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)strtoull ( &yytext[1], NULL, 16);
    yylval->colorval.alpha = val.a/255.0;
    yylval->colorval.red   = val.r/255.0;
    yylval->colorval.green = val.g/255.0;
    yylval->colorval.blue  = val.b/255.0;
    return T_COLOR;
}
<PROPERTIES>argb:{HEX}{8}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)strtoull ( &yytext[1], NULL, 16);
    yylval->colorval.alpha = val.a/255.0;
    yylval->colorval.red   = val.r/255.0;
    yylval->colorval.green = val.g/255.0;
    yylval->colorval.blue  = val.b/255.0;
    return T_COLOR;
}
<PROPERTIES>#{HEX}{6}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)g_ascii_strtoull ( &yytext[1], NULL, 16);
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = val.r/255.0;
    yylval->colorval.green = val.g/255.0;
    yylval->colorval.blue  = val.b/255.0;
    return T_COLOR;
}
<PROPERTIES>rgba\({NUMBER}{1,3},{NUMBER}{1,3},{NUMBER}{1,3},[01](\.{NUMBER}+)?\) {
    char *endptr = &yytext[5];
    yylval->colorval.red = g_ascii_strtoull ( endptr, &endptr, 10);
    yylval->colorval.green= g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval->colorval.blue= g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval->colorval.alpha= g_ascii_strtod ( endptr+1, NULL);
    return T_COLOR;
}
<PROPERTIES>rgb\({NUMBER}{1,3},{NUMBER}{1,3},{NUMBER}{1,3}\) {
    char *endptr = &yytext[4];
    yylval->colorval.red   = g_ascii_strtoull ( endptr, &endptr, 10);
    yylval->colorval.green = g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval->colorval.blue  = g_ascii_strtoull ( endptr+1, &endptr, 10);
    yylval->colorval.alpha = 1.0;
    return T_COLOR;
}

<*>(\r\n|\n) {
        yylloc->last_column = 1;
        yylloc->last_line ++;
};

<INITIAL><<EOF>>  {
    yyterminate();
    printf("Queue free: %d\n", g_queue_get_length(queue));;
    g_queue_free ( queue );
}
<*>. {
    fprintf(stderr, "Invalid character: '%c'\n", *yytext);
    yyterminate();
}

%%
