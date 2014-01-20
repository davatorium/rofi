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
    MODE_EXIT
} SwitcherMode;



typedef int (*menu_match_cb)(char **tokens, const char *input, int index, void *data);
int menu( char **lines, char **input, char *prompt,
          int selected, Time *time, int *shift, menu_match_cb mmc, void *mmc_data); 


/**
 * Allocator wrappers
 */
void* allocate( unsigned long bytes );
void* allocate_clear( unsigned long bytes );
void* reallocate( void *ptr, unsigned long bytes );


void catch_exit( __attribute__( ( unused ) ) int sig );
#endif
