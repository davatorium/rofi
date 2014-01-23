#ifndef __SIMPLESWITCHER_H__
#define __SIMPLESWITCHER_H__

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NEAR(a,o,b) ((b) > (a)-(o) && (b) < (a)+(o))
#define OVERLAP(a,b,c,d) (((a)==(c) && (b)==(d)) || MIN((a)+(b), (c)+(d)) - MAX((a), (c)) > 0)
#define INTERSECT(x,y,w,h,x1,y1,w1,h1) (OVERLAP((x),(w),(x1),(w1)) && OVERLAP((y),(h),(y1),(h1)))


typedef enum {
    WINDOW_SWITCHER,
    RUN_DIALOG,
    SSH_DIALOG,
    NUM_DIALOGS,
    MODE_EXIT,
    NEXT_DIALOG
} SwitcherMode;


typedef int ( *menu_match_cb )( char **tokens, const char *input, int index, void *data );
int menu( char **lines, char **input, char *prompt,
          unsigned int selected, Time *time, int *shift, menu_match_cb mmc, void *mmc_data );


/**
 * Allocator wrappers
 */
void* allocate( unsigned long bytes );
void* allocate_clear( unsigned long bytes );
void* reallocate( void *ptr, unsigned long bytes );


void catch_exit( __attribute__( ( unused ) ) int sig );

/**
 * Settings
 */

typedef struct _Settings {
    // Window settings
    unsigned int window_opacity;
    // Menu settings
    unsigned int menu_bw;
    unsigned int menu_width;
    unsigned int menu_lines;
    char *       menu_font;
    char *       menu_fg;
    char *       menu_bg;
    char *       menu_bgalt;
    char *       menu_hlfg;
    char *       menu_hlbg;
    char *       menu_bc;
    // Behavior
    unsigned int zeltak_mode;
    char *       terminal_emulator;
    unsigned int i3_mode;
    // Key bindings
    char *       window_key;
    char *       run_key;
    char *       ssh_key;
} Settings;

extern Settings config;
#endif
