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
%option bison-bridge

%{
#include "config.h"
#include <stdio.h>
#include <glib.h>
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
    PT_STRING,
    /** Parse environment */
    PT_ENV
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
            case PT_ENV:\
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
ASCNP    [\x00-\t\v-\x21\x23-\x7f]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

 // UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UANYN   {ASCN}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
 // UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

UANYNP   {ASCNP}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

WHITESPACE [[:blank:]]
WSO        [[:blank:]]*
WORD       [[:alnum:]-]+
MEDIA_NAME [[:alpha:]-]+
COLOR_NAME [[:alpha:]]+
STRING     \"{UANYN}*\"
STRING_LIST \"{UANYNP}*\"
CHAR       \'({ASCN}|\\\\|\\\'|\\0)\'
HEX        [[:xdigit:]]
NUMBER     [[:digit:]]
PNNUMBER   [-+]?[[:digit:]]+
PX         (px)
MM         (mm)
EM         (em)
CH         (ch)
PERCENT    (\%)

INHERIT    (inherit)

ASTERIX    \*

ENV        $\{[[:alnum:]]*\}

MODIFIER_ADD      \+
MODIFIER_SUBTRACT -
MODIFIER_MULTIPLY \*
MODIFIER_MIN      (min)
MODIFIER_MAX      (max)

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
CALC    (?i:calc)
COMMA            ,
FORWARD_SLASH    \/

LIST_OPEN        \[
LIST_CLOSE       \]

VAR_START  "var"

CPP_COMMENT  "//"
C_COMMENT_OPEN "/*"


INCLUDE    "@import"
THEME "@theme"

MEDIA "@media"

CONFIGURATION (?i:configuration)

%x INCLUDE
%x PROPERTIES
%x PROPERTIES_ENV
%x PROPERTIES_VAR
%x PROPERTIES_VAR_DEFAULT
%x PROPERTIES_LIST
%x NAMESTR
%x SECTION
%x DEFAULTS
%x MEDIA
%x MEDIA_CONTENT
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
<INITIAL>{THEME} {
    rofi_theme_reset();
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
    char *file2 = helper_get_theme_path ( &yytext[1] );
    char *filename = rofi_theme_parse_prepare_file ( file2, top->filename );
    g_free ( file2 );
    if ( g_list_find_custom ( imported_files, filename, (GCompareFunc)g_strcmp0 ) != NULL ) {
        g_debug ( "Skipping file: '%s' already parsed.", filename );
    } else {
        g_debug ( "Parsing file: '%s'", filename );
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


<INITIAL>{CONFIGURATION} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(DEFAULTS);
    return T_CONFIGURATION;

}

    /**
     * Handle defaults:  * { ... }
     */
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
  /** Everything not yet parsed is an error. */
<DEFAULTS>. {
    return T_ERROR_DEFAULTS;
}

<INITIAL>"#"            {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(NAMESTR);
    return T_NAME_PREFIX;
}
    /* Go into parsing a section. */
<NAMESTR>"\{"                    {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(SECTION);
    return T_BOPEN;
}
  /* Pop out of parsing a section. */
<SECTION>"\}"             {
    g_queue_pop_head ( queue );
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_BCLOSE;
}

<NAMESTR>\.|{WHITESPACE}    { return T_NSEP; }
<NAMESTR>,{WHITESPACE}*      { return T_SSEP; }
 /* Alias color to text-color */
<SECTION>"color"            { yylval->sval = g_strdup("text-color"); return T_PROP_NAME;}
<SECTION>{WORD}   { yylval->sval = g_strdup(yytext); return T_PROP_NAME;}
<NAMESTR>{WORD}         { yylval->sval = g_strdup(yytext); return T_NAME_ELEMENT;}

  /* After Namestr/Classstr we want to go to state str, then to  { */
<INITIAL,SECTION>{WHITESPACE}+	; // ignore all whitespace
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,PROPERTIES_LIST,PROPERTIES_VAR,MEDIA_CONTENT>{WHITESPACE}+	; // ignore all whitespace

<SECTION>":"                      { g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(PROPERTIES); return T_PSEP; }
<PROPERTIES>";"                   { BEGIN(GPOINTER_TO_INT ( g_queue_pop_head ( queue ))); return T_PCLOSE;}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>(true|false)          { yylval->bval= g_strcmp0(yytext, "true") == 0; return T_BOOLEAN;}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,MEDIA_CONTENT>{PNNUMBER}\.{NUMBER}+ { yylval->fval = g_ascii_strtod(yytext, NULL); return T_DOUBLE;}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,MEDIA_CONTENT>{PNNUMBER}            { yylval->ival = (int)g_ascii_strtoll(yytext, NULL, 10); return T_INT;}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{STRING}              { yytext[yyleng-1] = '\0'; yylval->sval = g_strcompress(&yytext[1]); return T_STRING;}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{CHAR}                { yytext[yyleng-1] = '\0'; yylval->cval = g_strcompress(&yytext[1])[0]; return T_CHAR;}

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>@{WORD}               {
    yylval->sval = g_strdup(yytext+1);
    return T_LINK;
}

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{EM}       { return T_UNIT_EM; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{CH}       { return T_UNIT_CH; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,MEDIA_CONTENT>{PX}       { return T_UNIT_PX; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,MEDIA_CONTENT>{MM}       { return T_UNIT_MM; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{PERCENT}  { return T_PERCENT; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{LS_SOLID} { return T_SOLID;   }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{LS_DASH}  { return T_DASH;    }

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{INHERIT}  { return T_INHERIT; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{MODIFIER_ADD}       { return T_MODIFIER_ADD; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{MODIFIER_SUBTRACT}  { return T_MODIFIER_SUBTRACT; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{MODIFIER_MULTIPLY}  { return T_MODIFIER_MULTIPLY; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{MODIFIER_MIN}  { return T_MODIFIER_MIN; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{MODIFIER_MAX}  { return T_MODIFIER_MAX; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{CALC}  { return T_CALC; }

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ENV} {
    yytext[yyleng-1] = '\0';
    const char *val = g_getenv(yytext+2);
    if ( val ) {
        ParseObject *top = g_queue_peek_head ( file_queue );
        top->location = *yylloc;
        ParseObject *po = g_malloc0(sizeof(ParseObject));
        po->type = PT_ENV;
        po->input_str = val;
        po->str_len = strlen(val);
        current = po;
        g_queue_push_head ( file_queue, po );
        imported_files = g_list_append ( imported_files, po->filename );

        yypush_buffer_state (yy_create_buffer ( 0, YY_BUF_SIZE ));
        yylloc->first_line = yylloc->last_line = 1;
        yylloc->first_column = yylloc->last_column = 1;
        yylloc->filename = current->filename;
        g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) ); BEGIN(PROPERTIES_ENV);
    }
}

 /**
  * Color parsing. It is easier to do this at lexer level.
  * Other schemes are done at yacc level.
  */
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>#{HEX}{8}       {
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[2]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[3],yytext[4]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[7],yytext[8]);
    return T_COLOR;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>#{HEX}{6}       {
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[2]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[3],yytext[4]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    return T_COLOR;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>#{HEX}{3}       {
    yylval->colorval.alpha = 1.0;
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[1]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[2],yytext[2]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[3],yytext[3]);
    return T_COLOR;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>#{HEX}{4}       {
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[4],yytext[4]);
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[1],yytext[1]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[2],yytext[2]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[3],yytext[3]);
    return T_COLOR;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>argb:{HEX}{8}       {
    yylval->colorval.alpha = rofi_theme_parse_convert_hex(yytext[5],yytext[6]);
    yylval->colorval.red   = rofi_theme_parse_convert_hex(yytext[7],yytext[8]);
    yylval->colorval.green = rofi_theme_parse_convert_hex(yytext[9],yytext[10]);
    yylval->colorval.blue  = rofi_theme_parse_convert_hex(yytext[11],yytext[12]);
    return T_COLOR;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>argb:{HEX}{7}       {
    return T_ERROR_ARGB_SPEC;
}
 /* Color schemes */
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{RGBA}             { return T_COL_RGBA;       }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{HSL}              { return T_COL_HSL;        }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{HWB}              { return T_COL_HWB;        }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{CMYK}             { return T_COL_CMYK;       }
 /* Fluff */
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{VAR_START}{S_T_PARENT_LEFT}        {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(PROPERTIES_VAR);
    return T_PARENT_LEFT;
}
<PROPERTIES_VAR>{S_T_PARENT_RIGHT} {
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_PARENT_RIGHT;
}
<PROPERTIES_VAR>{COMMA} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(PROPERTIES_VAR_DEFAULT);
    return T_COMMA;
}
<PROPERTIES_VAR_DEFAULT>{S_T_PARENT_RIGHT} {
    // Pop 2.
    g_queue_pop_head ( queue );
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_PARENT_RIGHT;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{S_T_PARENT_LEFT}  { return T_PARENT_LEFT;    }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{S_T_PARENT_RIGHT} { return T_PARENT_RIGHT;   }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,PROPERTIES_LIST>{COMMA}            { return T_COMMA;          }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{LIST_OPEN}        {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(PROPERTIES_LIST);
    return T_LIST_OPEN;
}
<PROPERTIES_LIST>{LIST_CLOSE}       {
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_LIST_CLOSE;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{FORWARD_SLASH}    { return T_FORWARD_SLASH;  }
 /* Position */
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{CENTER}           { return T_POS_CENTER;     }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{EAST}             { return T_POS_EAST;       }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{WEST}             { return T_POS_WEST;       }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{SOUTH}            { return T_POS_SOUTH;      }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{NORTH}            { return T_POS_NORTH;      }
 /* Highlight style */
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{NONE}             { return T_NONE;           }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{BOLD}             { return T_BOLD;           }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ITALIC}           { return T_ITALIC;         }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{UNDERLINE}        { return T_UNDERLINE;      }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{STRIKETHROUGH}    { return T_STRIKETHROUGH;  }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{SMALLCAPS}        { return T_SMALLCAPS;      }

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ANGLE_DEG}        { return T_ANGLE_DEG;      }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ANGLE_RAD}        { return T_ANGLE_RAD;      }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ANGLE_GRAD}       { return T_ANGLE_GRAD;     }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ANGLE_TURN}       { return T_ANGLE_TURN;     }

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ORIENTATION_HORI} { return ORIENTATION_HORI; }
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{ORIENTATION_VERT} { return ORIENTATION_VERT; }

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{COLOR_TRANSPARENT} {
    return T_COLOR_TRANSPARENT;
}
<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT>{COLOR_NAME} {
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

<INITIAL,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT><<EOF>>  {
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


<INITIAL>{MEDIA} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(MEDIA);
    return T_MEDIA;
}

<MEDIA>{S_T_PARENT_LEFT} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(MEDIA_CONTENT);
    return T_PARENT_LEFT;
}
<MEDIA_CONTENT>{MEDIA_NAME} {
    yylval->sval = g_strdup(yytext);
    return T_STRING;
}
<MEDIA_CONTENT>":" {
    return T_PSEP;
}
<MEDIA_CONTENT>{S_T_PARENT_RIGHT} {
    int  id = GPOINTER_TO_INT(g_queue_pop_head ( queue ));
    BEGIN(id);
    return T_PARENT_RIGHT;
}
<MEDIA>"\{" {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(INITIAL);
    return T_BOPEN;
}

<INITIAL>"\}" {
    g_queue_pop_head ( queue );
    BEGIN(GPOINTER_TO_INT(g_queue_pop_head ( queue )));
    return T_BCLOSE;
}
<MEDIA>{WHITESPACE}+	; // ignore all whitespace


<MEDIA,MEDIA_CONTENT>. {
    yytext[yyleng-1] = '\0';
    fprintf(stderr,"found: |%s|\n", yytext);
    return T_ERROR;
}

 /**
  * If we just encounter a word, we assume it is a Widget name.
  * This makes include,theme, configuration a reserved keyword.
  */
<INITIAL>{WORD} {
    g_queue_push_head ( queue, GINT_TO_POINTER (YY_START) );
    BEGIN(NAMESTR);
    yylval->sval = g_strdup(yytext);
    return T_NAME_ELEMENT;
}
<INITIAL>. {
    yytext[yyleng-1] = '\0';
    fprintf(stderr,"initial found: |%s|\n", yytext);
    return T_ERROR;
}
<SECTION>. {
    yytext[yyleng-1] = '\0';
    fprintf(stderr,"section found: |%s|\n", yytext);
    return T_ERROR_SECTION;
}
<PROPERTIES_LIST,PROPERTIES_VAR>{WORD} {
    yylval->sval = g_strdup(yytext);
    return T_ELEMENT;
}
<PROPERTIES_LIST>{STRING_LIST} {
    yytext[yyleng-1] = '\0';
    yylval->sval = g_strdup(yytext+1);
    return T_ELEMENT;
}

<PROPERTIES,PROPERTIES_ENV,PROPERTIES_VAR_DEFAULT,PROPERTIES_LIST>. {
    yytext[yyleng-1] = '\0';
    fprintf(stderr,"prop found: |%s|\n", yytext);
    return T_ERROR_PROPERTY;
}
<NAMESTR>. {
    yytext[yyleng-1] = '\0';
    fprintf(stderr,"namestr found: |%s|\n", yytext);
    return T_ERROR_NAMESTRING;
}
%%


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
    g_debug ( "Parsing top file: '%s'", filename );

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
    g_list_free_full ( imported_files, g_free );
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
    g_debug ( "Parsing string: '%s'", string );

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
    g_list_free_full ( imported_files, g_free );
    imported_files = NULL;
    g_queue_free ( file_queue );
    file_queue = NULL;
    if ( parser_retv != 0 ) {
        return TRUE;
    }
    return FALSE;
}
