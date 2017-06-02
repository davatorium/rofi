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
#include <math.h>
#include <strings.h>
#include "rofi.h"
#include "theme.h"

#include "theme-parser.h"
#include "css-colors.h"

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


static double rofi_theme_parse_convert_hex ( char high, char low)
{
    uint8_t retv = 0;

    int t = g_ascii_toupper ( high );
    t = ( t > '9')? (t-'A'+10):(t-'0');
    retv = t<<4;
    t = g_ascii_toupper ( low );
    t = ( t > '9')? (t-'A'+10):(t-'0');
    retv +=t;
    return retv/255.0;
}

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
WSO        [[:blank:]]*
WORD       [[:alnum:]-]+
COLOR_NAME [[:alpha:]]+
STRING     \"{UANYN}+\"
HEX        [[:xdigit:]]
NUMBER     [[:digit:]]
PNNUMBER   [-+]?[[:digit:]]+
REAL       [-+]?[[:digit:]]+(\.[[:digit:]]+)?
PX         (px)
EM         (em)
PERCENT    (\%)

ASTERIX    \*

 /* Position */
CENTER     (?i:center)
NORTH      (?i:north)
SOUTH      (?i:south)
EAST       (?i:east)
WEST       (?i:west)

 /* Line Style */
NONE          (?i:none)
BOLD          (?i:bold)
UNDERLINE     (?i:underline)
ITALIC        (?i:italic)
STRIKETHROUGH (?i:strikethrough)
SMALLCAPS     (?i:small\ caps)

/* ANGLES */

ANGLE_DEG  (?i:deg)
ANGLE_GRAD (?i:grad)
ANGLE_RAD  (?i:rad)
ANGLE_TURN (?i:turn)

/* LINE STYLE */
LS_DASH    (?i:dash)
LS_SOLID   (?i:solid)

/* Orientation */

ORIENTATION_HORI (?i:horizontal)
ORIENTATION_VERT (?i:vertical)

 /* Color schema */
RGBA (?i:rgb[a]?)
HWB  (?i:hwb)
CMYK (?i:cmyk)
HSL  (?i:hsl[a]?)

COLOR_TRANSPARENT (?i:transparent)

S_T_PARENT_LEFT  \(
S_T_PARENT_RIGHT \)
COMMA            ,
FORWARD_SLASH    \/

LIST_OPEN        \[
LIST_CLOSE       \]

CPP_COMMENT  "//"
C_COMMENT_OPEN "/*"


INCLUDE    "@import"

CONFIGURATION (?i:configuration)

%x INCLUDE
%x PROPERTIES
%x PROPERTIES_LIST
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

<*>{CPP_COMMENT}            {
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
<*>{C_COMMENT_OPEN}            {
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
<INCLUDE>{STRING} {
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
    return T_CONFIGURATION;

}
<INITIAL>{ASTERIX} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(DEFAULTS);
    return T_PDEFAULTS;
}
    /** Skip all whitespace */
<DEFAULTS>{WHITESPACE} {}
<DEFAULTS>"\{"   {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(SECTION);
    return T_BOPEN;
}
  /** Everythin not yet parsed is an error. */
<DEFAULTS>. {
    return T_ERROR_DEFAULTS;
}

<INITIAL>"#"            { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(NAMESTR);return T_NAME_PREFIX;}
  /* Go into parsing an section*/
<NAMESTR>"\{"                    {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(SECTION);
    return T_BOPEN;
}
  /* Pop out of parsing an section. */
<SECTION>"\}"             {
    g_queue_pop_head ( queue );
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_BCLOSE;
}

<NAMESTR>\.|{WHITESPACE}    { return T_NSEP; }
<SECTION>{WORD}   { yylval->sval = g_strdup(yytext); return T_PROP_NAME;}
<NAMESTR>{WORD}         { yylval->sval = g_strdup(yytext); return T_NAME_ELEMENT;}

  /* After Namestr/Classstr we want to go to state str, then to  { */
<INITIAL,SECTION>{WHITESPACE}+	; // ignore all whitespace
<PROPERTIES,PROPERTIES_LIST>{WHITESPACE}+	; // ignore all whitespace

<SECTION>":"                      { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(PROPERTIES); return T_PSEP; }
<PROPERTIES>";"                   { BEGIN(GPOINTER_TO_INT ( g_queue_pop_head ( queue ))); return T_PCLOSE;}
<PROPERTIES>(true|false)          { yylval->bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
<PROPERTIES>{PNNUMBER}\.{NUMBER}+ { yylval->fval = g_ascii_strtod(yytext, NULL); return T_DOUBLE;}
<PROPERTIES>{PNNUMBER}            { yylval->ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
<PROPERTIES>{STRING}              { yytext[yyleng-1] = '\0'; yylval->sval = g_strcompress(&yytext[1]); return T_STRING;}

<PROPERTIES>@{WORD}               {
    yylval->sval = g_strdup(yytext);
    return T_LINK;
}

<PROPERTIES>{EM}       { return T_UNIT_EM; }
<PROPERTIES>{PX}       { return T_UNIT_PX; }
<PROPERTIES>{PERCENT}  { return T_PERCENT; }
<PROPERTIES>{LS_SOLID} { return T_SOLID; }
<PROPERTIES>{LS_DASH}  { return T_DASH; }

 /**
  * Color parsing. It is easier to do this at lexer level.
  * Other schemes are done at yacc level.
  */
<PROPERTIES>#{HEX}{8}       {
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[2]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[3],yytext[4]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[7],yytext[8]);
    return T_COLOR;
}
<PROPERTIES>#{HEX}{6}       {
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[2]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[3],yytext[4]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    return T_COLOR;
}
<PROPERTIES>#{HEX}{3}       {
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[1]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[2],yytext[2]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[3],yytext[3]);
    return T_COLOR;
}
<PROPERTIES>#{HEX}{4}       {
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[4],yytext[4]);
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[1]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[2],yytext[2]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[3],yytext[3]);
    return T_COLOR;
}
<PROPERTIES>argb:{HEX}{8}       {
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[7],yytext[8]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[9],yytext[10]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[11],yytext[12]);
    return T_COLOR;
}
 /* Color schemes */
<PROPERTIES>{RGBA}             { return T_COL_RGBA;       }
<PROPERTIES>{HSL}              { return T_COL_HSL;        }
<PROPERTIES>{HWB}              { return T_COL_HWB;        }
<PROPERTIES>{CMYK}             { return T_COL_CMYK;       }
 /* Fluff */
<PROPERTIES>{S_T_PARENT_LEFT}  { return T_PARENT_LEFT;    }
<PROPERTIES>{S_T_PARENT_RIGHT} { return T_PARENT_RIGHT;   }
<PROPERTIES,PROPERTIES_LIST>{COMMA}            { return T_COMMA;          }
<PROPERTIES>{LIST_OPEN}        {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(PROPERTIES_LIST);
    return T_LIST_OPEN;
}
<PROPERTIES_LIST>{LIST_CLOSE}       {
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_LIST_CLOSE;
}
<PROPERTIES>{FORWARD_SLASH}    { return T_FORWARD_SLASH;  }
 /* Position */
<PROPERTIES>{CENTER}           { return T_POS_CENTER;     }
<PROPERTIES>{EAST}             { return T_POS_EAST;       }
<PROPERTIES>{WEST}             { return T_POS_WEST;       }
<PROPERTIES>{SOUTH}            { return T_POS_SOUTH;      }
<PROPERTIES>{NORTH}            { return T_POS_NORTH;      }
 /* Highlight style */
<PROPERTIES>{NONE}             { return T_NONE;           }
<PROPERTIES>{BOLD}             { return T_BOLD;           }
<PROPERTIES>{ITALIC}           { return T_ITALIC;         }
<PROPERTIES>{UNDERLINE}        { return T_UNDERLINE;      }
<PROPERTIES>{STRIKETHROUGH}    { return T_STRIKETHROUGH;  }
<PROPERTIES>{SMALLCAPS}        { return T_SMALLCAPS;      }

<PROPERTIES>{ANGLE_DEG}        { return T_ANGLE_DEG;      }
<PROPERTIES>{ANGLE_RAD}        { return T_ANGLE_RAD;      }
<PROPERTIES>{ANGLE_GRAD}       { return T_ANGLE_GRAD;     }
<PROPERTIES>{ANGLE_TURN}       { return T_ANGLE_TURN;     }

<PROPERTIES>{ORIENTATION_HORI} { return ORIENTATION_HORI; }
<PROPERTIES>{ORIENTATION_VERT} { return ORIENTATION_VERT; }

<PROPERTIES>{COLOR_TRANSPARENT} {
    return T_COLOR_TRANSPARENT;
}
<PROPERTIES>{COLOR_NAME} {
    for ( unsigned int iter = 0; iter < num_CSSColors; iter++){
        if ( strcasecmp(yytext, CSSColors[iter].name )== 0 ) {
            yylval->colorval.alpha = 1.0;
            yylval->colorval.red   = CSSColors[iter].r/255.0;
            yylval->colorval.green = CSSColors[iter].g/255.0;
            yylval->colorval.blue  = CSSColors[iter].b/255.0;
            return T_COLOR_NAME;
        }
    }
    REJECT;
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
<PROPERTIES_LIST>{WORD} {
    yylval->sval = g_strdup(yytext);
    return T_ELEMENT;
}

<PROPERTIES,PROPERTIES_LIST>. {
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
