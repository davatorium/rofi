#ifndef ROFI_TIMINGS_H
#define ROFI_TIMINGS_H
#include <config.h>
#if TIMINGS

void rofi_timings_init ( void );
void rofi_timings_tick ( char const *str, int line, char const *msg );
void rofi_timings_quit ( void );

#define TIMINGS_START()    rofi_timings_init ()
#define TICK()             rofi_timings_tick ( __func__, __LINE__, "" )
#define TICK_N( a )        rofi_timings_tick ( __func__, __LINE__, a )
#define TIMINGS_STOP()     rofi_timings_quit ()

#else

#define TIMINGS_START()
#define TIMINGS_STOP()
#define TICK()
#define TICK_N( a )

#endif // TIMINGS
#endif // ROFI_TIMINGS_H
