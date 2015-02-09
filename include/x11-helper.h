#ifndef __X11_HELPER_H__
#define __X11_HELPER_H__

// window lists
typedef struct
{
    Window *array;
    void   **data;
    int    len;
} winlist;


/**
 * Create a window list, pre-seeded with WINLIST entries.
 *
 * @returns A new window list.
 */
winlist* winlist_new ();


/**
 * @param l The winlist.
 * @param w The window to find.
 *
 * Find the window in the list, and return the array entry.
 *
 * @returns -1 if failed, index is successful.
 */
int winlist_find ( winlist *l, Window w );

/**
 * @param l The winlist entry
 *
 * Free the winlist.
 */
void winlist_free ( winlist *l );

/**
 * @param l The winlist entry
 *
 * Empty winlist without free-ing
 */
void winlist_empty ( winlist *l );

/**
 * @param l The winlist.
 * @param w The window to add.
 * @param d Data pointer.
 *
 * Add one entry. If Full, extend with WINLIST entries.
 *
 * @returns 0 if failed, 1 is successful.
 */
int winlist_append ( winlist *l, Window w, void *d );

/**
 * @param d Display connection to X server
 * @param w window
 *
 * Get window attributes.
 * This functions uses caching.
 *
 * @returns a XWindowAttributes
 */
XWindowAttributes* window_get_attributes ( Display *display, Window w );


int window_get_prop ( Display *display, Window w, Atom prop,
                      Atom *type, int *items,
                      void *buffer, unsigned int bytes ) __attribute__ ( ( nonnull ( 4, 5 ) ) );

/**
 * @param display Connection to the X server.
 * @param w The Window to read property from.
 * @param atom The property identifier
 *
 * Get text property defined by atom from window.
 * Support utf8.
 *
 * @returns a newly allocated string with the result or NULL
 */
char* window_get_text_prop ( Display *display, Window w, Atom atom );

int window_get_atom_prop ( Display *display, Window w, Atom atom, Atom *list, int count );
void window_set_atom_prop ( Display *display, Window w, Atom prop, Atom *atoms, int count );
int window_get_cardinal_prop ( Display *display, Window w, Atom atom, unsigned long *list, int count );

/**
 * Create empty X11 cache for windows and windows attributes.
 */
void x11_cache_create ( void );

/**
 * Empty the X11 cache.
 * (does not free it.)
 */
void x11_cache_empty ( void );

/**
 * Free the cache.
 */
void x11_cache_free ( void );


/**
 * Window info.
 */
#define ATOM_ENUM( x )    x
#define ATOM_CHAR( x )    # x

// usable space on a monitor
#define EWMH_ATOMS( X )               \
    X ( _NET_CLIENT_LIST_STACKING ),  \
    X ( _NET_NUMBER_OF_DESKTOPS ),    \
    X ( _NET_CURRENT_DESKTOP ),       \
    X ( _NET_ACTIVE_WINDOW ),         \
    X ( _NET_WM_NAME ),               \
    X ( _NET_WM_STATE ),              \
    X ( _NET_WM_STATE_SKIP_TASKBAR ), \
    X ( _NET_WM_STATE_SKIP_PAGER ),   \
    X ( _NET_WM_STATE_ABOVE ),        \
    X ( _NET_WM_DESKTOP ),            \
    X ( CLIPBOARD ),                  \
    X ( UTF8_STRING ),                \
    X ( _NET_WM_WINDOW_OPACITY )

enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };

extern const char *netatom_names[];
extern Atom       netatoms[NUM_NETATOMS];
typedef struct
{
    int x, y, w, h;
    int l, r, t, b;
} workarea;

#define CLIENTTITLE    100
#define CLIENTCLASS    50
#define CLIENTNAME     50
#define CLIENTSTATE    10
#define CLIENTROLE     50

// a managable window
typedef struct
{
    Window            window, trans;
    XWindowAttributes xattr;
    char              title[CLIENTTITLE];
    char class[CLIENTCLASS];
    char              name[CLIENTNAME];
    char              role[CLIENTROLE];
    int               states;
    Atom              state[CLIENTSTATE];
    workarea          monitor;
    int               active;
} client;
// collect info on any window
// doesn't have to be a window we'll end up managing
client* window_client ( Display *display, Window win );
int client_has_state ( client *c, Atom state );

void monitor_active ( Display *display, workarea *mon );

int window_send_message ( Display *display, Window target, Window subject,
                          Atom atom, unsigned long protocol,
                          unsigned long mask, Time time );


/**
 * @param display The display.
 *
 * Release keyboard.
 */
void release_keyboard ( Display *display );

/**
 * @param display The display.
 * @param w       Window we want to grab keyboard on.
 *
 * Grab keyboard and mouse.
 *
 * @return 1 when keyboard is grabbed, 0 not.
 */
int take_keyboard ( Display *display, Window w );


void grab_key ( Display *display, unsigned int modmask, KeySym key );
void x11_parse_key ( char *combo, unsigned int *mod, KeySym *key );

/**
 * @param display The connection to the X server.
 * @param box     The window to set the opacity on.
 * @param opacity The opacity value. (0-100)
 *
 * Set the opacity of the window and sub-windows.
 */
void x11_set_window_opacity ( Display *display, Window box, unsigned int opacity );

/**
 * Setup several items required.
 * * Error handling,
 * * Numlock detection
 * * Cache
 */
void x11_setup ( Display *display );
#endif
