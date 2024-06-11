
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

static constexpr int LEFT = 0x02;
static constexpr int RIGHT = 0x01;
static constexpr int UP = 0x04;
static constexpr int DOWN = 0x08;
static constexpr int SELECT = 0x40;
static constexpr int START = 0x80;
static constexpr int A = 0x10;
static constexpr int B = 0x20;

#define NESPAD_SELECT (0x04)
#define NESPAD_START (0x08)
#define NESPAD_UP (0x10)
#define NESPAD_DOWN (0x20)
#define NESPAD_LEFT (0x40)
#define NESPAD_RIGHT (0x80)
#define NESPAD_A (0x01)
#define NESPAD_B (0x02)
#endif /* !nytypes_TYPES_H_INCLUDED */
