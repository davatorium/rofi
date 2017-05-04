/*
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

%option nodefault noyywrap
%option nostdinit
%option nounput
%option never-interactive
%option bison-locations

%{
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <helper.h>
#include "rofi.h"
#include "theme.h"

#include "theme-parser.h"

#define LOG_DOMAIN "Parser"
int last_state = 0;

/**
 * Type of Object to parse.
 */
typedef enum {
    /** Parse a file */
    PT_FILE,
    /** Parse a string */
    PT_STRING
} ParseType;

/**
 * Parse object
 */
typedef struct _ParseObject {
    /** Type */
    ParseType type;

    /** File pointer */
    FILE *filein;
    char *filename;

    /** Length of string  */
    int str_len;
    /** String */
    const char *input_str;
    /** Position in file */
    YYLTYPE  location;
} ParseObject;


GList  *imported_files = NULL;
GList  *prev_imported_files = NULL;
GQueue *file_queue = NULL;
GQueue *queue = NULL;

ParseObject *current = NULL;

static char * rofi_theme_parse_prepare_file ( const char *file, const char *parent_file );
%}
%{

#define YY_INPUT(buf,result,max_size) \
{\
    if ( current == NULL ) {\
        result = 0;\
    } else {\
        switch ( current->type ) { \
            case PT_FILE:\
            {\
                errno =0; \
                while ( (result = (int) fread(buf, 1, max_size, current->filein))==0 && ferror(current->filein)) \
                { \
                    if( errno != EINTR) \
                    { \
                        YY_FATAL_ERROR( "input in flex scanner failed" ); \
                        break; \
                    } \
                    errno=0; \
                    clearerr(current->filein); \
                } \
                break;\
            }\
            case PT_STRING:\
            {\
                yy_size_t len = MIN (max_size, current->str_len);\
                if ( len > 0 ){\
                    memcpy (buf, current->input_str, len);\
                    current->input_str+=len;\
                    current->str_len-=len;\
                    result = len;\
                } else {\
                    result = 0;\
                }\
            }\
        }\
    }\
}


#define YY_USER_ACTION {\
    yylloc->last_column+= yyleng;\
}
#define YY_LLOC_START {\
    yylloc->first_line = yylloc->last_line;\
    yylloc->first_column = yylloc->last_column;\
}
%}

ASC     [\x00-\x7f]
ASCN    [\x00-\t\v-\x7f]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

 // UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UANYN   {ASCN}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
 // UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

WHITESPACE [[:blank:]]
WORD       [[:alnum:]-]+
STRING     {UANYN}+
HEX        [[:xdigit:]]
NUMBER     [[:digit:]]
PNNUMBER   [-+]?[[:digit:]]+
REAL       [-+]?[[:digit:]]+(\.[[:digit:]]+)?
PX         (px)
EM         (em)
PERCENT    (\%)

ASTERIX    \*

CENTER     "center"
NORTH      "north"
SOUTH      "south"
EAST       "east"
WEST       "west"

NONE        "none"
BOLD        "bold"
UNDERLINE   "underline"
ITALIC      "italic"

LS_DASH    "dash"
LS_SOLID   "solid"

INCLUDE    "@import"

CONFIGURATION "configuration"

%x INCLUDE
%x PROPERTIES
%x NAMESTR
%x SECTION
%x DEFAULTS
%%

%{
YY_LLOC_START
%}
%{
if ( queue == NULL ){
    queue = g_queue_new ( );
    yylloc->filename = current->filename;
    // unsure why todo this.
    yylloc->first_line = yylloc->last_line = 1;
    yylloc->first_column = yylloc->last_column = 1;
}
%}

  /**
   * General code for handling comments.
   * Both C and C++ style comments, including nexting.
   */

<*>"//"            {
    int c = input();
    while ( c != 0 && c != EOF){
        if (c == '\n') {
            yylloc->last_column = 1;
            yylloc->last_line ++;
            break;
        }
        yylloc->last_column++;
        c = input();
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
        case 0: nesting_depth = 0; break;
        case EOF: nesting_depth = 0; break;
        default:
            yylloc->last_column++;
        ;
        }
    }
    YY_LLOC_START
}

    /**
     * HANDLE INCLUDES
     */
<INITIAL>{INCLUDE} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(INCLUDE);
}
    /** Skip all whitespace */
<INCLUDE>{WHITESPACE} {}

    /** Parse path. Last element in this INCLUDE */
<INCLUDE>\"{STRING}\" {
    yytext[yyleng-1] = '\0';
    ParseObject *top = g_queue_peek_head ( file_queue );
    g_assert ( top != NULL );
    char *filename = rofi_theme_parse_prepare_file ( &yytext[1], top->filename );
    if ( g_list_find_custom ( imported_files, filename, (GCompareFunc)g_strcmp0 ) != NULL ) {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Skipping file: '%s' already parsed.", filename );
    } else {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Parsing file: '%s'", filename );
        FILE *f = fopen ( filename, "rb" );
        if ( f ) {
            top->location = *yylloc;
            ParseObject *po = g_malloc0(sizeof(ParseObject));
            po->type = PT_FILE;
            po->filename = filename;
            po->filein = f;
            current = po;
            g_queue_push_head ( file_queue, po );
            imported_files = g_list_append ( imported_files, po->filename );

            yypush_buffer_state (yy_create_buffer ( 0, YY_BUF_SIZE ));
            yylloc->first_line = yylloc->last_line = 1;
            yylloc->first_column = yylloc->last_column = 1;
            yylloc->filename = current->filename;
        } else {
            char *str = g_markup_printf_escaped ( "Failed to open theme: <i>%s</i>\nError: <b>%s</b>",
                    filename, strerror ( errno ) );
            rofi_add_error_message ( g_string_new ( str ) );
            g_free ( str );
            g_free(filename);
        }
    }
    // Pop out of include. */
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
}
  /** Everythin not yet parsed is an error. */
<INCLUDE>. {
    return T_ERROR_INCLUDE;
}

    /**
     * END INCLUDES
     */


    /**
     * Handle defaults:  * { ... }
     */
<INITIAL>{CONFIGURATION} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(DEFAULTS);
    return CONFIGURATION;

}
<INITIAL>{ASTERIX} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(DEFAULTS);
    return PDEFAULTS;
}
    /** Skip all whitespace */
<DEFAULTS>{WHITESPACE} {}
<DEFAULTS>"\{"   {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(SECTION);
    return BOPEN;
}
  /** Everythin not yet parsed is an error. */
<DEFAULTS>. {
    return T_ERROR_DEFAULTS;
}

<INITIAL>"#"            { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(NAMESTR);return NAME_PREFIX;}
  /* Go into parsing an section*/
<NAMESTR>"\{"                    {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(SECTION);
    return BOPEN;
}
  /* Pop out of parsing an section. */
<SECTION>"\}"             {
    g_queue_pop_head ( queue );
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return BCLOSE;
}

<NAMESTR>\.|{WHITESPACE}    { return NSEP; }
<SECTION>{WORD}   { yylval->sval = g_strdup(yytext); return N_STRING;}
<NAMESTR>{WORD}         { yylval->sval = g_strdup(yytext); return NAME_ELEMENT;}

  /* After Namestr/Classstr we want to go to state str, then to  { */
<INITIAL,SECTION>{WHITESPACE}+	; // ignore all whitespace
<PROPERTIES>{WHITESPACE}+	; // ignore all whitespace

<SECTION>":"               { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(PROPERTIES); return PSEP; }
<PROPERTIES>";"                  { BEGIN(GPOINTER_TO_INT ( g_queue_pop_head ( queue ))); return PCLOSE;}
<PROPERTIES>(true|false)         { yylval->bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
<PROPERTIES>{PNNUMBER}            { yylval->ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
<PROPERTIES>{PNNUMBER}\.{NUMBER}+ { yylval->fval = g_ascii_strtod(yytext, NULL); return T_DOUBLE;}
<PROPERTIES>\"{STRING}\"         { yytext[yyleng-1] = '\0'; yylval->sval = g_strdup(&yytext[1]); return T_STRING;}
<PROPERTIES>@{WORD}              {
    yylval->sval = g_strdup(yytext);
    return T_LINK;
}

<PROPERTIES>{REAL}{EM}        {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_EM;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{PNNUMBER}{PX}        {
    yylval->distance.distance = (double)g_ascii_strtoll(yytext, NULL, 10);
    yylval->distance.type     = PW_PX;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{PNNUMBER}{PX}{WHITESPACE}{LS_DASH} {
    yylval->distance.distance = (double)g_ascii_strtoll(yytext, NULL, 10);
    yylval->distance.type     = PW_PX;
    yylval->distance.style    = DASH;
    return T_PIXEL;
}
<PROPERTIES>{REAL}{EM}{WHITESPACE}{LS_DASH} {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_EM;
    yylval->distance.style    = DASH;
    return T_PIXEL;
}
<PROPERTIES>{PNNUMBER}{PX}{WHITESPACE}{LS_SOLID} {
    yylval->distance.distance = (double)g_ascii_strtoll(yytext, NULL, 10);
    yylval->distance.type     = PW_PX;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{REAL}{EM}{WHITESPACE}{LS_SOLID} {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_EM;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{REAL}{PERCENT}        {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_PERCENT;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{REAL}{PERCENT}{WHITESPACE}{LS_SOLID}        {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_PERCENT;
    yylval->distance.style    = SOLID;
    return T_PIXEL;
}
<PROPERTIES>{REAL}{PERCENT}{WHITESPACE}{LS_DASH}        {
    yylval->distance.distance = (double)g_ascii_strtod(yytext, NULL);
    yylval->distance.type     = PW_PERCENT;
    yylval->distance.style    = DASH;
    return T_PIXEL;
}
<PROPERTIES>#{HEX}{8}       {
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
<PROPERTIES>#{HEX}{3}       {
    union {  uint16_t val; struct { unsigned char b:4,g:4,r:4,a :4;};} val;
    val.val = (uint16_t )g_ascii_strtoull ( &yytext[1], NULL, 16);
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = val.r/15.0;
    yylval->colorval.green = val.g/15.0;
    yylval->colorval.blue  = val.b/15.0;
    return T_COLOR;
}
<PROPERTIES>rgba\({NUMBER}{1,3},{NUMBER}{1,3},{NUMBER}{1,3},[01](\.{NUMBER}+)?\) {
    char *endptr = &yytext[5];
    yylval->colorval.red = g_ascii_strtoull ( endptr, &endptr, 10)/255.0;
    yylval->colorval.green= g_ascii_strtoull ( endptr+1, &endptr, 10)/255.0;
    yylval->colorval.blue= g_ascii_strtoull ( endptr+1, &endptr, 10)/255.0;
    yylval->colorval.alpha= g_ascii_strtod ( endptr+1, NULL);
    return T_COLOR;
}
<PROPERTIES>rgb\({NUMBER}{1,3},{NUMBER}{1,3},{NUMBER}{1,3}\) {
    char *endptr = &yytext[4];
    yylval->colorval.red   = g_ascii_strtoull ( endptr, &endptr, 10)/255.0;
    yylval->colorval.green = g_ascii_strtoull ( endptr+1, &endptr, 10)/255.0;
    yylval->colorval.blue  = g_ascii_strtoull ( endptr+1, &endptr, 10)/255.0;
    yylval->colorval.alpha = 1.0;
    return T_COLOR;
}
<PROPERTIES>argb:{HEX}{1,8}       {
    union {  unsigned int val; struct { unsigned char b,g,r,a;};} val;
    val.val = (unsigned int)strtoull ( &yytext[5], NULL, 16);
    yylval->colorval.alpha = val.a/255.0;
    yylval->colorval.red   = val.r/255.0;
    yylval->colorval.green = val.g/255.0;
    yylval->colorval.blue  = val.b/255.0;
    return T_COLOR;
}

<PROPERTIES>{CENTER} {
    yylval->ival = WL_CENTER;
    return T_POSITION;
}
<PROPERTIES>{EAST} {
    yylval->ival = WL_EAST;
    return T_POSITION;
}
<PROPERTIES>{WEST} {
    yylval->ival = WL_WEST;
    return T_POSITION;
}
<PROPERTIES>{SOUTH}{EAST} {
    yylval->ival = WL_SOUTH_EAST;
    return T_POSITION;
}
<PROPERTIES>{SOUTH}{WEST} {
    yylval->ival = WL_SOUTH_WEST;
    return T_POSITION;
}
<PROPERTIES>{SOUTH} {
    yylval->ival = WL_SOUTH;
    return T_POSITION;
}
<PROPERTIES>{NORTH}{EAST} {
    yylval->ival = WL_NORTH_EAST;
    return T_POSITION;
}
<PROPERTIES>{NORTH}{WEST} {
    yylval->ival = WL_NORTH_WEST;
    return T_POSITION;
}
<PROPERTIES>{NORTH} {
    yylval->ival = WL_NORTH;
    return T_POSITION;
}
<PROPERTIES>{NONE} {
    yylval->ival = HL_NONE;
    return T_HIGHLIGHT_STYLE;
}
<PROPERTIES>{BOLD} {
    yylval->ival = HL_BOLD;
    return T_HIGHLIGHT_STYLE;
}
<PROPERTIES>{ITALIC} {
    yylval->ival = HL_ITALIC;
    return T_HIGHLIGHT_STYLE;
}
<PROPERTIES>{UNDERLINE} {
    yylval->ival = HL_UNDERLINE;
    return T_HIGHLIGHT_STYLE;
}
<INITIAL><<EOF>>  {
    ParseObject *po = g_queue_pop_head ( file_queue );
    if ( po ) {
        if ( po->type == PT_FILE ){
            fclose ( po->filein );
        }
        g_free (  po );
    }
    po = g_queue_peek_head ( file_queue );
    if ( po == NULL ) {
        g_queue_free ( queue );
        // Reset pointer to NULL
        queue = NULL;
        yyterminate();
    } else {
        yypop_buffer_state();
        current = po;
        *yylloc = current->location;
        BEGIN(GPOINTER_TO_INT ( g_queue_pop_head ( queue )));
    }
}

<*>\n {
        yylloc->last_column = 1;
        yylloc->last_line ++;
};
<*>(\r\n) {
        yylloc->last_column = 1;
        yylloc->last_line ++;
};
<INITIAL>. {
    return T_ERROR;
}
<SECTION>. {
    return T_ERROR_SECTION;
}
<PROPERTIES>. {
    return T_ERROR_PROPERTY;
}
<NAMESTR>. {
    return T_ERROR_NAMESTRING;
}
%%


static char * rofi_theme_parse_prepare_file ( const char *file, const char *parent_file )
{
    char *filename = rofi_expand_path ( file );
    // If no absolute path specified, expand it.
    if ( parent_file != NULL && ! g_path_is_absolute ( filename )   ) {
        char *basedir = g_path_get_dirname ( parent_file );
        char *path = g_build_filename ( basedir, filename, NULL );
        g_free ( filename);
        filename = path;
        g_free ( basedir );
    }
    GFile *gf = g_file_new_for_path ( filename );
    g_free(filename);
    filename = g_file_get_path ( gf );
    g_object_unref ( gf );

    return filename;
}
gboolean rofi_theme_parse_file ( const char *file )
{
    char *file2 = helper_get_theme_path ( file );
    char *filename = rofi_theme_parse_prepare_file ( file2, NULL );
    g_free ( file2 );

    yyin = fopen ( filename, "rb" );
    if ( yyin == NULL ) {
        char *str = g_markup_printf_escaped ( "Failed to open theme: <i>%s</i>\nError: <b>%s</b>",
                filename, strerror ( errno ) );
        rofi_add_error_message ( g_string_new ( str ) );
        g_free ( str );
        g_free ( filename );
        return TRUE;
    }

    /** Add Parse object */
    file_queue = g_queue_new ();
    ParseObject *po = g_malloc0(sizeof(ParseObject));
    po->type = PT_FILE;
    po->filename = filename;
    po->filein = yyin;
    current = po;
    imported_files = g_list_append ( imported_files, po->filename );
    g_queue_push_head ( file_queue, po );
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Parsing top file: '%s'", filename );

    int parser_retv = yyparse ( file );
    yylex_destroy ();
    yyin = NULL;

    while ( (po = g_queue_pop_head ( file_queue ) )) {
        if ( po ) {
            if ( po->type == PT_FILE ){
                fclose ( po->filein );
            }
            g_free ( po );
        }
    }
    // Free up.
    g_list_foreach ( imported_files, (GFunc)g_free, NULL);
    g_list_free ( imported_files );
    g_queue_free ( file_queue );
    imported_files = NULL;
    file_queue = NULL;
    if ( parser_retv != 0 ) {
        return TRUE;
    }
    return FALSE;
}
gboolean rofi_theme_parse_string ( const char *string )
{
    yyin      = NULL;

    /** Add Parse object */
    file_queue = g_queue_new ();
    ParseObject *po = g_malloc0(sizeof(ParseObject));
    po->type = PT_STRING;
    po->input_str = string;
    po->str_len   = strlen(string);
    current = po;
    g_queue_push_head ( file_queue, po );
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Parsing string: '%s'", string );

    int parser_retv = yyparse ( string );
    yylex_destroy ();

    while ( (po = g_queue_pop_head ( file_queue ) )) {
        if ( po ) {
            if ( po->type == PT_FILE ){
                fclose ( po->filein );
            }
            g_free ( po );
        }
    }
    // Free up.
    g_list_foreach ( imported_files, (GFunc)g_free, NULL);
    g_list_free ( imported_files );
    imported_files = NULL;
    g_queue_free ( file_queue );
    file_queue = NULL;
    if ( parser_retv != 0 ) {
        return TRUE;
    }
    return FALSE;
}
