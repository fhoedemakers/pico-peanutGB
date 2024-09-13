
#ifndef mytypes_TYPES_H_INCLUDED
#define mytypes_TYPES_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  Type definition                                                  */
/*-------------------------------------------------------------------*/
#ifndef DWORD
typedef unsigned long  DWORD;
#endif /* !DWORD */

#ifndef WORD
typedef unsigned short WORD;
#endif /* !WORD */

#ifndef BYTE
typedef unsigned char  BYTE;
#endif /* !BYTE */

/*-------------------------------------------------------------------*/
/*  NULL definition                                                  */
/*-------------------------------------------------------------------*/
#ifndef NULL
#define NULL  0
#endif /* !NULL */

/* Joypad buttons */
#define JOYPAD_A            0x01
#define JOYPAD_B            0x02
#define JOYPAD_SELECT       0x04
#define JOYPAD_START        0x08
#define JOYPAD_RIGHT        0x10
#define JOYPAD_LEFT         0x20
#define JOYPAD_UP           0x40
#define JOYPAD_DOWN         0x80

static constexpr int LEFT = JOYPAD_LEFT;
static constexpr int RIGHT = JOYPAD_RIGHT;
static constexpr int UP = JOYPAD_UP;
static constexpr int DOWN = JOYPAD_DOWN;
static constexpr int SELECT = JOYPAD_SELECT;
static constexpr int START = JOYPAD_START;
static constexpr int A = JOYPAD_A;
static constexpr int B = JOYPAD_B;

#define NESPAD_SELECT (0x04)
#define NESPAD_START (0x08)
#define NESPAD_UP (0x10)
#define NESPAD_DOWN (0x20)
#define NESPAD_LEFT (0x40)
#define NESPAD_RIGHT (0x80)
#define NESPAD_A (0x01)
#define NESPAD_B (0x02)
#endif /* !nytypes_TYPES_H_INCLUDED */
