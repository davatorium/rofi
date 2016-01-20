#ifndef ROFI_X11_EVENT_SOURCE_H
#define ROFI_X11_EVENT_SOURCE_H

GSource * x11_event_source_new ( Display  *display );
void x11_event_source_set_callback ( GSource *source, GSourceFunc callback );
#endif // ROFI_X11_EVENT_SOURCE_H
