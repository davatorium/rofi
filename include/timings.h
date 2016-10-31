#ifndef ROFI_TIMINGS_H
#define ROFI_TIMINGS_H
#include <config.h>

/**
 * @defgroup TIMINGS Timings
 * @ingroup HELPERS
 * @{
 */
#if TIMINGS
/**
 * Init the timestamping mechanism .
 * implementation.
 */
void rofi_timings_init ( void );
/**
 * @param str function name.
 * @param line line number
 * @param msg message
 *
 * Report a tick.
 */
void rofi_timings_tick ( const char *file, char const *str, int line, char const *msg );
/**
 * Stop the timestamping mechanism
 */
void rofi_timings_quit ( void );

/**
 * Start timestamping mechanism.
 * Call to this function is time 0.
 */
#define TIMINGS_START()    rofi_timings_init ()
/**
 * Report current time since TIMINGS_START
 */
#define TICK()             rofi_timings_tick ( __FILE__, __func__, __LINE__, "" )
/**
 * @param a an string
 * Report current time since TIMINGS_START
 */
#define TICK_N( a )        rofi_timings_tick ( __FILE__, __func__, __LINE__, a )
/**
 * Stop timestamping mechanism.
 */
#define TIMINGS_STOP()     rofi_timings_quit ()

#else

/**
 * Start timestamping mechanism.
 * Call to this function is time 0.
 */
#define TIMINGS_START()
/**
 * Stop timestamping mechanism.
 */
#define TIMINGS_STOP()
/**
 * Report current time since TIMINGS_START
 */
#define TICK()
/**
 * @param a an string
 * Report current time since TIMINGS_START
 */
#define TICK_N( a )

#endif // TIMINGS
/*@}*/
#endif // ROFI_TIMINGS_H
