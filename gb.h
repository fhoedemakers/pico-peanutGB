
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

typedef enum {
	DMG_PALETTE_GREENLCD,
	DMG_PALETTE_COLOR,
	DMG_PALETTE_GRAYSCALE
} dmg_palette_type_t;
extern uint16_t *audio_stream;
extern int sample_size;
extern uint8_t *GBaddress; // pointer to the GB ROM file
extern uint16_t *currentpalette;
#ifdef __cplusplus
extern "C" {
#endif
/* Convert a 15-bit RGB555 value (0RRRRRGGGGGBBBBB) to packed 12-bit RGB444 (RRRR GGGG BBBB).
 * Strategy: simple floor truncation: 5-bit channel (0..31) -> 4-bit (0..15) via >>1.
 * Returned format places R in bits 11..8, G in 7..4, B in 3..0. Upper 4 bits are zero.
 */
#define RGB555_TO_RGB444(rgb555) ( \
	((((rgb555) >> 10) & 0x1F) >> 1) << 8 | \
	((((rgb555) >> 5) & 0x1F) >> 1) << 4 | \
	(((rgb555) & 0x1F) >> 1) \
)
#if ENABLE_SOUND
#include "minigb_apu.h"
#endif
int  startemulation(uint8_t *rom, char *romname, const char *savedir, char *errormessage, int USEHSTX);
void stopemulation(char *romname, const char *savedir);
void emu_init_lcd();
void emu_run_frame();
void emu_set_gamepad(uint8_t joypad);
void emu_set_dmg_palette_type(dmg_palette_type_t palette_type); 
WORD *dvi_getlinebuffer(uint_fast8_t line);
void infogb_plot_line(uint_fast8_t line);
void *frens_f_malloc(size_t size);
void frens_f_free(void *ptr);
#ifdef __cplusplus
}
#endif
#endif // GB_H