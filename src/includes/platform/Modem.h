/*
 * Modem.h -- platform definitions.
 */


#ifndef BOTLETICS_MODEM_PLATFORM_H
#define BOTLETICS_MODEM_PLATFORM_H

// only "standard" config supported in this release -- namely AVR-based arduino type affairs
#include "ModemStd.h"



#ifndef DEBUG_PRINT
// debug is disabled

#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)

#endif


#ifndef prog_char_strcmp
#define prog_char_strcmp(a, b)					strcmp((a), (b))
#endif

#ifndef prog_char_strstr
#define prog_char_strstr(a, b)					strstr((a), (b))
#endif

#ifndef prog_char_strlen
#define prog_char_strlen(a)						strlen((a))
#endif


#ifndef prog_char_strcpy
#define prog_char_strcpy(to, fromprogmem)		strcpy((to), (fromprogmem))
#endif


#endif /* BOTLETICS_MODEM_PLATFORM_H */
