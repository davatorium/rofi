/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

#include <glib.h>
#include <math.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "theme.h"

/** Default padding. */
#define WIDGET_DEFAULT_PADDING    0

/* Corner radius - tl, tr, br, bl */
static void draw_rounded_rect ( cairo_t * d,
                                double x1, double y1, double x2, double y2,
                                double r0, double r1, double r2, double r3 )
{
    if ( r0 > 0 ) {
        cairo_move_to ( d, x1, y1+r0 );
        cairo_arc ( d, x1+r0, y1+r0, r0, -G_PI, -G_PI_2 );
    } else {
        cairo_move_to ( d, x1, y1 );
    }
    if ( r1 > 0 ) {
        cairo_line_to ( d, x2-r1, y1 );
        cairo_arc ( d, x2-r1, y1+r1, r1, -G_PI_2, 0.0 );
    } else {
        cairo_line_to ( d, x2, y1 );
    }
    if ( r2 > 0 ) {
        cairo_line_to ( d, x2, y2-r2 );
        cairo_arc ( d, x2-r2, y2-r2, r2, 0.0, G_PI_2 );
    } else {
        cairo_line_to ( d, x2, y2 );
    }
    if ( r3 > 0 ) {
        cairo_line_to ( d, x1+r3, y2 );
        cairo_arc ( d, x1+r3, y2-r3, r3, G_PI_2, G_PI );
    } else {
        cairo_line_to ( d, x1, y2 );
    }
    cairo_close_path ( d );
}

void widget_init ( widget *wid, widget *parent, WidgetType type, const char *name )
{
    wid->type        = type;
    wid->parent      = parent;
    wid->name        = g_strdup ( name );
    wid->def_padding =
        (RofiPadding){ { {WIDGET_DEFAULT_PADDING, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {WIDGET_DEFAULT_PADDING, ROFI_PU_PX,ROFI_DISTANCE_MODIFIER_NONE,  NULL, NULL}, ROFI_HL_SOLID }, { {WIDGET_DEFAULT_PADDING, ROFI_PU_PX,ROFI_DISTANCE_MODIFIER_NONE,  NULL, NULL}, ROFI_HL_SOLID },
                       { {WIDGET_DEFAULT_PADDING, ROFI_PU_PX,ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID } };
    wid->def_border        = (RofiPadding){ { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL} , ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID } };
    wid->def_border_radius = (RofiPadding){ { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL} , ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID } };
    wid->def_margin        = (RofiPadding){ { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL} , ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID }, { {0, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL}, ROFI_HL_SOLID } };

    wid->padding       = rofi_theme_get_padding ( wid, "padding", wid->def_padding );
    wid->border        = rofi_theme_get_padding ( wid, "border", wid->def_border );
    wid->border_radius = rofi_theme_get_padding ( wid, "border-radius", wid->def_border_radius );
    wid->margin        = rofi_theme_get_padding ( wid, "margin", wid->def_margin );

    // bled by default
    wid->enabled = rofi_theme_get_boolean ( wid, "enabled", TRUE );
}

void widget_set_state ( widget *widget, const char *state )
{
    if ( g_strcmp0 ( widget->state, state ) ) {
        widget->state = state;
        // Update border.
        widget->border        = rofi_theme_get_padding ( widget, "border", widget->def_border );
        widget->border_radius = rofi_theme_get_padding ( widget, "border-radius", widget->def_border_radius );
        if ( widget->set_state  ) {
            widget->set_state ( widget, state );
        }
        widget_queue_redraw ( widget );

    }
}

int widget_intersect ( const widget *widget, int x, int y )
{
    if ( widget == NULL ) {
        return FALSE;
    }

    if ( x >= ( widget->x ) && x < ( widget->x + widget->w ) ) {
        if ( y >= ( widget->y ) && y < ( widget->y + widget->h ) ) {
            return TRUE;
        }
    }
    return FALSE;
}

void widget_resize ( widget *widget, short w, short h )
{
    if ( widget != NULL  ) {
        if ( widget->resize != NULL ) {
            if ( widget->w != w || widget->h != h ) {
                widget->resize ( widget, w, h );
            }
        }
        else {
            widget->w = w;
            widget->h = h;
        }
        // On a resize we always want to udpate.
        widget_queue_redraw ( widget );
    }
}
void widget_move ( widget *widget, short x, short y )
{
    if ( widget != NULL ) {
        widget->x = x;
        widget->y = y;
    }
}
void widget_set_type ( widget *widget, WidgetType type )
{
    if ( widget == NULL ) {
        return;
    }
    widget->type = type;
}

WidgetType widget_type ( widget *widget )
{
    if ( widget != NULL ) {
        return widget->type;
    }
    return WIDGET_TYPE_UNKNOWN;
}

gboolean widget_enabled ( widget *widget )
{
    if ( widget != NULL ) {
        return widget->enabled;
    }
    return FALSE;
}

void widget_enable ( widget *widget )
{
    if ( widget && !widget->enabled ) {
        widget->enabled = TRUE;
        widget_update ( widget );
        widget_update ( widget->parent );
        widget_queue_redraw ( widget );
    }
}
void widget_disable ( widget *widget )
{
    if ( widget && widget->enabled ) {
        widget->enabled = FALSE;
        widget_update ( widget );
        widget_update ( widget->parent );
        widget_queue_redraw ( widget );
    }
}
void widget_draw ( widget *widget, cairo_t *d )
{
    // Check if enabled and if draw is implemented.
    if ( widget && widget->enabled && widget->draw ) {
        // Don't draw if there is no space.
        if ( widget->h < 1 || widget->w < 1 ) {
            widget->need_redraw = FALSE;
            return;
        }
        // Store current state.
        cairo_save ( d );
        const int margin_left   = distance_get_pixel ( widget->margin.left, ROFI_ORIENTATION_HORIZONTAL );
        const int margin_top    = distance_get_pixel ( widget->margin.top, ROFI_ORIENTATION_VERTICAL );
        const int margin_right  = distance_get_pixel ( widget->margin.right, ROFI_ORIENTATION_HORIZONTAL );
        const int margin_bottom = distance_get_pixel ( widget->margin.bottom, ROFI_ORIENTATION_VERTICAL );
        const int left          = distance_get_pixel ( widget->border.left, ROFI_ORIENTATION_HORIZONTAL );
        const int right         = distance_get_pixel ( widget->border.right, ROFI_ORIENTATION_HORIZONTAL );
        const int top           = distance_get_pixel ( widget->border.top, ROFI_ORIENTATION_VERTICAL );
        const int bottom        = distance_get_pixel ( widget->border.bottom, ROFI_ORIENTATION_VERTICAL );
        int       radius_bl     = distance_get_pixel ( widget->border_radius.left, ROFI_ORIENTATION_HORIZONTAL );
        int       radius_tr     = distance_get_pixel ( widget->border_radius.right, ROFI_ORIENTATION_HORIZONTAL );
        int       radius_tl     = distance_get_pixel ( widget->border_radius.top, ROFI_ORIENTATION_VERTICAL );
        int       radius_br     = distance_get_pixel ( widget->border_radius.bottom, ROFI_ORIENTATION_VERTICAL );

        double    vspace = widget->h - margin_top - margin_bottom - top / 2.0 - bottom / 2.0;
        double    hspace = widget->w - margin_left - margin_right - left / 2.0 - right / 2.0;
        if ( ( radius_bl + radius_tl ) > ( vspace ) ) {
            int j = ( ( vspace ) / 2.0 );
            radius_bl = MIN ( radius_bl, j );
            radius_tl = MIN ( radius_tl, j );
        }
        if ( ( radius_br + radius_tr ) > ( vspace ) ) {
            int j = ( ( vspace  ) / 2.0 );
            radius_br = MIN ( radius_br, j );
            radius_tr = MIN ( radius_tr, j );
        }
        if ( ( radius_tl + radius_tr ) > ( hspace ) ) {
            int j = ( ( hspace ) / 2.0 );
            radius_tr = MIN ( radius_tr, j );
            radius_tl = MIN ( radius_tl, j );
        }
        if ( ( radius_bl + radius_br ) > ( hspace ) ) {
            int j = ( ( hspace ) / 2.0 );
            radius_br = MIN ( radius_br, j );
            radius_bl = MIN ( radius_bl, j );
        }

        // Calculate the different offsets for the corners.
        double minof_tr = MIN ( right / 2.0, top / 2.0 );
        double minof_tl = MIN ( left / 2.0, top / 2.0 );
        double minof_br = MIN ( right / 2.0, bottom / 2.0 );
        double minof_bl = MIN ( left / 2.0, bottom / 2.0 );

        // Background painting.
        // Set new x/y position.
        cairo_translate ( d, widget->x, widget->y );
        cairo_set_line_width ( d, 0 );

        draw_rounded_rect ( d,
                            margin_left               + ( left   > 2 ? left   - 1 : left   == 1 ? 0.5 : 0 ),
                            margin_top                + ( top    > 2 ? top    - 1 : top    == 1 ? 0.5 : 0 ),
                            widget->w - margin_right  - ( right  > 2 ? right  - 1 : right  == 1 ? 0.5 : 0 ),
                            widget->h - margin_bottom - ( bottom > 2 ? bottom - 1 : bottom == 1 ? 0.5 : 0 ),
                            radius_tl - ( minof_tl > 1 ? minof_tl - 1 : 0 ),
                            radius_tr - ( minof_tr > 1 ? minof_tr - 1 : 0 ),
                            radius_br - ( minof_br > 1 ? minof_br - 1 : 0 ),
                            radius_bl - ( minof_bl > 1 ? minof_bl - 1 : 0 ) );

        cairo_set_source_rgba ( d, 1.0, 1.0, 1.0, 1.0 );
        rofi_theme_get_color ( widget, "background-color", d );
        cairo_fill_preserve ( d );
        cairo_clip ( d );

        widget->draw ( widget, d );
        widget->need_redraw = FALSE;

        cairo_restore ( d );

        if ( left || top || right || bottom ) {
            cairo_save ( d );
            cairo_translate ( d, widget->x, widget->y );
            cairo_new_path ( d );
            rofi_theme_get_color ( widget, "border-color", d );

            double radius_int_tl, radius_int_tr, radius_int_br, radius_int_bl;
            double radius_out_tl, radius_out_tr, radius_out_br, radius_out_bl;

            if (radius_tl > 0) {
                radius_out_tl = radius_tl + minof_tl ,
                radius_int_tl = radius_tl - minof_tl;
            } else {
                radius_out_tl = radius_int_tl = 0;
            }
            if (radius_tr > 0) {
                radius_out_tr = radius_tr + minof_tr ,
                radius_int_tr = radius_tr - minof_tr;
            } else {
                radius_out_tr = radius_int_tr = 0;
            }
            if (radius_br > 0) {
                radius_out_br = radius_br + minof_br ,
                radius_int_br = radius_br - minof_br;
            } else {
                radius_out_br = radius_int_br = 0;
            }
            if (radius_bl > 0) {
                radius_out_bl = radius_bl + minof_bl ,
                radius_int_bl = radius_bl - minof_bl;
            } else {
                radius_out_bl = radius_int_bl = 0;
            }

            draw_rounded_rect ( d,
                                margin_left,
                                margin_top,
                                widget->w - margin_right,
                                widget->h - margin_top,
                                radius_out_tl,
                                radius_out_tr,
                                radius_out_br,
                                radius_out_bl );
            cairo_new_sub_path ( d );
            draw_rounded_rect ( d,
                                margin_left + left,
                                margin_top  + top,
                                widget->w - margin_right - right,
                                widget->h - margin_bottom - bottom,
                                radius_int_tl,
                                radius_int_tr,
                                radius_int_br,
                                radius_int_bl );
            cairo_set_fill_rule ( d, CAIRO_FILL_RULE_EVEN_ODD );
            cairo_fill ( d );
            cairo_restore ( d );
        }
    }
}

void widget_free ( widget *wid )
{
    if ( wid ) {
        if ( wid->name ) {
            g_free ( wid->name );
        }
        if ( wid->free ) {
            wid->free ( wid );
        }
        return;
    }
}

int widget_get_height ( widget *widget )
{
    if ( widget ) {
        if ( widget->get_height ) {
            return widget->get_height ( widget );
        }
        return widget->h;
    }
    return 0;
}
int widget_get_width ( widget *widget )
{
    if ( widget ) {
        if ( widget->get_width ) {
            return widget->get_width ( widget );
        }
        return widget->w;
    }
    return 0;
}
int widget_get_x_pos ( widget *widget )
{
    if ( widget ) {
        return widget->x;
    }
    return 0;
}
int widget_get_y_pos ( widget *widget )
{
    if ( widget ) {
        return widget->y;
    }
    return 0;
}

void widget_xy_to_relative ( widget *widget, gint *x, gint *y )
{
    *x -= widget->x;
    *y -= widget->y;
    if ( widget->parent != NULL ) {
        widget_xy_to_relative ( widget->parent, x, y );
    }
}

void widget_update ( widget *widget )
{
    // When (desired )size of widget changes.
    if ( widget ) {
        if ( widget->update ) {
            widget->update ( widget );
        }
    }
}

void widget_queue_redraw ( widget *wid )
{
    if ( wid ) {
        widget *iter = wid;
        // Find toplevel widget.
        while ( iter->parent != NULL ) {
            iter->need_redraw = TRUE;
            iter              = iter->parent;
        }
        iter->need_redraw = TRUE;
    }
}

gboolean widget_need_redraw ( widget *wid )
{
    if ( wid && wid->enabled ) {
        return wid->need_redraw;
    }
    return FALSE;
}

widget *widget_find_mouse_target ( widget *wid, WidgetType type, gint x, gint y )
{
    if ( !wid ) {
        return NULL;
    }

    if ( wid->find_mouse_target ) {
        widget *target = wid->find_mouse_target ( wid, type, x, y );
        if ( target != NULL ) {
            return target;
        }
    }
    if ( wid->type == type ) {
        return wid;
    }
    return NULL;
}

WidgetTriggerActionResult widget_trigger_action ( widget *wid, guint action, gint x, gint y )
{
    if ( wid && wid->trigger_action ) {
        return wid->trigger_action ( wid, action, x, y, wid->trigger_action_cb_data );
    }
    return FALSE;
}

void widget_set_trigger_action_handler ( widget *wid, widget_trigger_action_cb cb, void * cb_data )
{
    if ( wid == NULL ) {
        return;
    }
    wid->trigger_action         = cb;
    wid->trigger_action_cb_data = cb_data;
}

gboolean widget_motion_notify ( widget *wid, gint x, gint y )
{
    if ( wid && wid->motion_notify ) {
        wid->motion_notify ( wid, x, y );
    }

    return FALSE;
}

int widget_padding_get_left ( const widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    int distance = distance_get_pixel ( wid->padding.left, ROFI_ORIENTATION_HORIZONTAL );
    distance += distance_get_pixel ( wid->border.left, ROFI_ORIENTATION_HORIZONTAL );
    distance += distance_get_pixel ( wid->margin.left, ROFI_ORIENTATION_HORIZONTAL );
    return distance;
}
int widget_padding_get_right ( const widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    int distance = distance_get_pixel ( wid->padding.right, ROFI_ORIENTATION_HORIZONTAL );
    distance += distance_get_pixel ( wid->border.right, ROFI_ORIENTATION_HORIZONTAL );
    distance += distance_get_pixel ( wid->margin.right, ROFI_ORIENTATION_HORIZONTAL );
    return distance;
}
int widget_padding_get_top ( const widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    int distance = distance_get_pixel ( wid->padding.top, ROFI_ORIENTATION_VERTICAL );
    distance += distance_get_pixel ( wid->border.top, ROFI_ORIENTATION_VERTICAL );
    distance += distance_get_pixel ( wid->margin.top, ROFI_ORIENTATION_VERTICAL );
    return distance;
}
int widget_padding_get_bottom ( const widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    int distance = distance_get_pixel ( wid->padding.bottom, ROFI_ORIENTATION_VERTICAL );
    distance += distance_get_pixel ( wid->border.bottom, ROFI_ORIENTATION_VERTICAL );
    distance += distance_get_pixel ( wid->margin.bottom, ROFI_ORIENTATION_VERTICAL );
    return distance;
}

int widget_padding_get_remaining_width ( const widget *wid )
{
    int width = wid->w;
    width -= widget_padding_get_left ( wid );
    width -= widget_padding_get_right ( wid );
    return width;
}
int widget_padding_get_remaining_height ( const widget *wid )
{
    int height = wid->h;
    height -= widget_padding_get_top ( wid );
    height -= widget_padding_get_bottom ( wid );
    return height;
}
int widget_padding_get_padding_height ( const widget *wid )
{
    int height = 0;
    height += widget_padding_get_top ( wid );
    height += widget_padding_get_bottom ( wid );
    return height;
}
int widget_padding_get_padding_width ( const widget *wid )
{
    int width = 0;
    width += widget_padding_get_left ( wid );
    width += widget_padding_get_right ( wid );
    return width;
}

int widget_get_desired_height ( widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    if ( wid->get_desired_height ) {
        return wid->get_desired_height ( wid );
    }
    return wid->h;
}
int widget_get_desired_width ( widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    if ( wid->get_desired_width ) {
        return wid->get_desired_width ( wid );
    }
    return wid->w;
}

int widget_get_absolute_xpos ( widget *wid )
{
    int retv = 0;
    if ( wid ) {
        retv += wid->x;
        if ( wid->parent ) {
            retv += widget_get_absolute_xpos ( wid->parent );
        }
    }
    return retv;
}
int widget_get_absolute_ypos ( widget *wid )
{
    int retv = 0;
    if ( wid ) {
        retv += wid->y;
        if ( wid->parent ) {
            retv += widget_get_absolute_ypos ( wid->parent );
        }
    }
    return retv;
}
