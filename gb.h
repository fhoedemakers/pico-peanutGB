#ifndef GB_H
#define GB_H
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
extern uint16_t *audio_stream;
extern int sample_size;
extern uint8_t *GBaddress; // pointer to the GB ROM file
#ifdef __cplusplus
extern "C" {
#endif
#if ENABLE_SOUND
#include "minigb_apu.h"
#endif
int  startemulation(uint8_t *rom, char *romname, const char *savedir, char *errormessage, int USEHSTX);
void stopemulation(char *romname, const char *savedir);
void emu_init_lcd(void (*lcd_draw_line)(const uint_fast8_t line));
void emu_run_frame();
void emu_set_gamepad(uint8_t joypad);
WORD *dvi_getlinebuffer(uint8_t line);
#ifdef __cplusplus
}
#endif
#endif // GB_H