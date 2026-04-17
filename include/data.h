#ifndef DATA_H_
#define DATA_H_

/*
 * This file declares font/bitmap data stored in PROGMEM.
 * Definitions live in src/data.c so we only have one copy.
 */

#include <stdint.h>
#include <avr/pgmspace.h>

typedef uint8_t bitmap_t[8][128];
typedef char PROGMEM prog_uchar;

extern const prog_uchar bigNumbers[][96] PROGMEM;
extern const prog_uchar minus[] PROGMEM;
extern const prog_uchar myDregree[8] PROGMEM;
extern const prog_uchar myFont[][8] PROGMEM;

#endif
