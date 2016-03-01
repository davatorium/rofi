#ifndef ROFI_XCB_H
#define ROFI_XCB_H

typedef struct _xcb_stuff   xcb_stuff;

extern xcb_stuff *xcb;

xcb_window_t xcb_stuff_get_root_window ( xcb_stuff *xcb );
void xcb_stuff_wipe ( xcb_stuff *xcb );
#endif
