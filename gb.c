#include <stdint.h>
#include <stdio.h>
#include "gb.h"
#include "ff.h"
#include "stdbool.h"
#include "pico.h"

#include "peanut_gb.h"

#define MAX_SRAM_SIZE (0x2000 * 4) // Max 32KB SRAM

struct priv_t
{
    /* Pointer to allocated memory holding GB file. */
    uint8_t *rom;
    /* Pointer to allocated memory holding save file. */
    uint8_t *cart_ram;
    /* Frame buffer */
};
static struct priv_t priv;
enum gb_init_error_e ret;
static struct gb_s gb;

uint8_t *GBaddress; // pointer to the GB ROM file
static bool useHSTX = false;

// Palette definition
static uint16_t __not_in_flash_func(dmgGreyscalePalette555)[3][4] = {
    {0x7FFF, 0x56B5, 0x294A, 0x0000},
    {0x7FFF, 0x56B5, 0x294A, 0x0000},
    {0x7FFF, 0x56B5, 0x294A, 0x0000},
};

static uint16_t __not_in_flash_func(dmgColorPalette555)[3][4] = {
    {
        0xFFFF,
        0xED13,
        0xA207,
        0x0000,
    },
    {
        0xFFFF,
        0xED13,
        0xA207,
        0x0000,
    },
    {
        0xFFFF,
        0x9E89,
        0x3C9B,
        0x0000,
    },
};

static uint16_t __not_in_flash_func(dmgGreyscalePalette444)[3][4];

static uint16_t __not_in_flash_func(dmgColorPalette444)[3][4];

static uint16_t dmgGreenPalette444[3][4] = {
    /* DMG (original Game Boy) canonical green palette (light -> dark)
     * Source 24-bit colors: #9BBC0F, #8BAC0F, #306230, #0F380F
     * RGB444 layout actually used by encodeTMDS_RGB444():
     *   Bits 11..8 = Red (Rrrr)
     *   Bits  7..4 = Green (Gggg)
     *   Bits  3..0 = Blue (Bbbb)
     * (Lower 12 bits packed as RRRR GGGG BBBB; upper 4 bits ignored.)
     * Previous values were shifted left by 4 (Rrrrr Gggg Bbbb ----) causing color channels
     * to be misaligned (producing a red-tinted background). Correct packing below.
     * Converted with floor reduction v4 = v8 >> 4:
     *   #9BBC0F -> R=9,G=11,B=0 => 0x09B0
     *   #8BAC0F -> R=8,G=10,B=0 => 0x08A0
     *   #306230 -> R=3,G=6 ,B=3 => 0x0363
     *   #0F380F -> R=0,G=3 ,B=0 => 0x0030
     */
    {0x09B0, 0x08A0, 0x0363, 0x0030},
    {0x09B0, 0x08A0, 0x0363, 0x0030},
    {0x09B0, 0x08A0, 0x0363, 0x0030},
};
static uint16_t dmgGreenPalette555[3][4] = {
    /* DMG (original Game Boy) canonical green palette (light -> dark)
     * Source colors (24-bit): #9BBC0F, #8BAC0F, #306230, #0F380F
     * Converted to RGB555:    0x4EE2 , 0x46A2 , 0x1986 , 0x08E2
     */
    {0x4EE2, 0x46A2, 0x1986, 0x08E2},
    {0x4EE2, 0x46A2, 0x1986, 0x08E2},
    {0x4EE2, 0x46A2, 0x1986, 0x08E2},
};
#if PEANUT_FULL_GBC_SUPPORT
static uint16_t *gbcPal = NULL;      // pointer to current CGB palette
#endif
static dmg_palette_type_t currentDmgPaletteType = DMG_PALETTE_GREENLCD;

/* Flattened DMG palette. Peanut packs the palette selector in bits 4-5
 * (LCD_PALETTE_ALL) and the colour index in bits 0-1 (LCD_COLOUR), so masking
 * the pixel byte with 0x33 covers every reachable value in 64 entries -- one
 * load per pixel instead of the two dependent loads plus shift that
 * dmgPal[(p & 0x30) >> 4][p & 3] compiled to, 23040 times a frame. */
static uint16_t __not_in_flash_func(dmgPaletteLut)[64];
static bool dmgPaletteLutValid = false;

static void buildDmgPaletteLut(void)
{
    uint16_t(*pal)[4];
    switch (currentDmgPaletteType)
    {
    case DMG_PALETTE_COLOR:
        pal = useHSTX ? dmgColorPalette555 : dmgColorPalette444;
        break;
    case DMG_PALETTE_GRAYSCALE:
        pal = useHSTX ? dmgGreyscalePalette555 : dmgGreyscalePalette444;
        break;
    case DMG_PALETTE_GREENLCD:
    default:
        pal = useHSTX ? dmgGreenPalette555 : dmgGreenPalette444;
        break;
    }
    for (int v = 0; v < 64; v++)
    {
        /* Selector 3 (both palette bits set) is not produced by the renderer;
         * fold it onto 0 so this loop stays inside the [3][4] source table. */
        int sel = (v & LCD_PALETTE_ALL) >> 4;
        if (sel > 2)
            sel = 0;
        dmgPaletteLut[v] = pal[sel][v & LCD_COLOUR];
    }
    dmgPaletteLutValid = true;
}

#if ENABLE_SOUND
/* Headroom over the nominal 735 samples/frame so the sink-level trim in
 * emu_audio_frame_budget() (up to +AUDIO_RATE_TRIM_MAX) always fits. */
#define AUDIO_FRAME_MAX_SAMPLES 768
/* Samples rendered per audio_callback() call. Because each slice is pushed to
 * the sink before the next is rendered, the staging buffer only has to hold one
 * slice, not a whole frame: 512 bytes of .bss instead of 3072. A normal slice is
 * ~92 samples (735/8), so this bound only splits the fallback path where the LCD
 * is off and the whole frame is flushed at once. */
#define AUDIO_SLICE_MAX_SAMPLES 128
#define AUDIO_BUFFER_SIZE (AUDIO_SLICE_MAX_SAMPLES * sizeof(u_int32_t))
/* Static .bss rather than frens_f_malloc(): that allocator hands out PSRAM on
 * boards that have it (Fruit Jam, Metro, Pico Plus 2), which put this buffer --
 * memset and read-modify-written four times every frame, then read again by the
 * output stage -- on the QSPI bus. On boards without PSRAM it came from the SRAM
 * heap anyway, so this costs those builds nothing. */
static uint32_t audio_stream_buf[AUDIO_SLICE_MAX_SAMPLES];

/* Audio is rendered in slices spread through the frame instead of one block at
 * the end. minigb_apu is not cycle-stepped: a single end-of-frame call renders
 * all ~735 samples from the register state as it stands *after* the frame has
 * run, so every APU write made during those 16.7ms is applied retroactively to
 * the whole block, smearing short notes and envelopes. Slicing on the scanline
 * callback costs nothing in the CPU loop and cuts that quantisation to ~2ms. */
/* Slicing is free where a framebuffer decouples the emulator from scanout, and
 * costs video margin where one does not. On the RP2040 PicoDVI path
 * dvi_getlinebuffer() blocks on freeLineQueue_, so the active window is the
 * display's critical path: audio rendered at end of frame lands in the
 * bottom-border/VBlank window and costs nothing, whereas slicing moves the same
 * work into the active window and spends the queue lead directly. CMake sets
 * AUDIO_CHUNKS_PER_FRAME to 1 there and renders it all at end of frame. */
#ifndef AUDIO_CHUNKS_PER_FRAME
#define AUDIO_CHUNKS_PER_FRAME 8
#endif
#if AUDIO_CHUNKS_PER_FRAME > 1
#define AUDIO_CHUNK_LINE_STEP (LCD_HEIGHT / AUDIO_CHUNKS_PER_FRAME)
#else
#define AUDIO_CHUNK_LINE_STEP LCD_HEIGHT
#endif

static int audio_frame_budget = 0;    /* samples the sink wants this frame */
static int audio_emitted = 0;         /* samples already rendered+pushed */
static uint_fast8_t audio_chunk_countdown = AUDIO_CHUNK_LINE_STEP;

/* Render and push audio so that `upto` samples have been emitted this frame.
 * Each slice is pushed before the next is rendered, so the staging buffer is
 * reused rather than accumulated. */
static void __not_in_flash_func(emu_audio_emit_upto)(int upto)
{
    while (audio_emitted < upto)
    {
        int n = upto - audio_emitted;
        if (n > AUDIO_SLICE_MAX_SAMPLES)
            n = AUDIO_SLICE_MAX_SAMPLES;
        audio_callback(NULL, (uint8_t *)audio_stream_buf, n * (int)sizeof(uint32_t));
        emu_audio_output(audio_stream_buf, n);
        audio_emitted += n;
    }
}
#endif

char *GetfileNameFromFullPath(char *fullPath)
{
    char *fileName = fullPath;
    char *ptr = fullPath;
    while (*ptr)
    {
        if (*ptr == '/')
        {
            fileName = ptr + 1;
        }
        ptr++;
    }
    return fileName;
}

void stripextensionfromfilename(char *filename)
{
    char *ptr = filename;
    char *lastdot = filename;
    while (*ptr)
    {
        if (*ptr == '.')
        {
            lastdot = ptr;
        }
        ptr++;
    }
    *lastdot = 0;
}
/**
 * Returns a byte from the ROM file at the given address. Not used. Emulator reads directly from GBaddress.
 */

// uint8_t *address = (uint8_t *)GB_FILE_ADDR;

uint8_t __not_in_flash_func(gb_rom_read)(struct gb_s *gb, const uint_fast32_t addr)
{
    // const struct priv_t * const p = gb->direct.priv;
    // const struct priv_t *const p = static_cast<const struct priv_t *>(gb->direct.priv);
    return GBaddress[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t __not_in_flash_func(gb_cart_ram_read)(struct gb_s *gb, const uint_fast32_t addr)
{
    // const struct priv_t * const p = gb->direct.priv;
    const struct priv_t *const p = (const struct priv_t *)(gb->direct.priv);
    return p->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */

void __not_in_flash_func(gb_cart_ram_write)(struct gb_s *gb, const uint_fast32_t addr,
                                            const uint8_t val)
{
    // const struct priv_t * const p = gb->direct.priv;
    const struct priv_t *const p = (const struct priv_t *)(gb->direct.priv);
    p->cart_ram[addr] = val;
}

/**
 * Returns a pointer to the allocated space containing the ROM. Must be freed.
 */
uint8_t *read_rom_to_ram(const char *file_name)
{
    uint8_t *rom = NULL;
#if 0
	FILE *rom_file = fopen(file_name, "rb");
	size_t rom_size;
	

	if(rom_file == NULL)
		return NULL;

	fseek(rom_file, 0, SEEK_END);
	rom_size = ftell(rom_file);
	rewind(rom_file);
	rom = malloc(rom_size);

	if(fread(rom, sizeof(uint8_t), rom_size, rom_file) != rom_size)
	{
		free(rom);
		fclose(rom_file);
		return NULL;
	}

	fclose(rom_file);
	return rom;
#endif
    return rom;
}

/**
 * Ignore all errors.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val)
{
    const char *gb_err_str[GB_INVALID_MAX] = {
        "UNKNOWN",
        "INVALID OPCODE",
        "INVALID READ",
        "INVALID WRITE",
        "HALT FOREVER"};
    // struct priv_t *priv = gb->direct.priv;
    const struct priv_t *const priv = (const struct priv_t *)(gb->direct.priv);
    printf("Error %d occurred: %s at %04X\n. Exiting.\n",
           gb_err, gb_err_str[gb_err], val);

    /* Free memory and then exit. */
    free(priv->cart_ram);
    free(priv->rom);
    exit(EXIT_FAILURE);
}

// read cart ram from file
void loadsram(char *romname, const char *savedir)
{
    char pad[FF_MAX_LFN];
    int bytesread;
    char fileName[FF_MAX_LFN];
    FILINFO fno;
    strcpy(fileName, GetfileNameFromFullPath(romname));
    stripextensionfromfilename(fileName);
    snprintf(pad, FF_MAX_LFN, "%s/%s.SAV", savedir, fileName);
    printf("Loading SRAM from %s\n", pad);
    if (f_stat(pad, &fno) == FR_OK)
    {
        FIL file;
        if (f_open(&file, pad, FA_READ) == FR_OK)
        {
            if (f_read(&file, priv.cart_ram, fno.fsize, &bytesread) != FR_OK)
            {
                printf("Error reading SRAM\n");
            }
            f_close(&file);
        }
    }
    else
    {
        printf("No SRAM file found\n");
    }
}

// save SRAM to file
void savesram(char *romname, const char *savedir)
{
    char pad[FF_MAX_LFN];
    char fileName[FF_MAX_LFN];
    strcpy(fileName, GetfileNameFromFullPath(romname));
    stripextensionfromfilename(fileName);
    snprintf(pad, FF_MAX_LFN, "%s/%s.SAV", savedir, fileName);
    printf("Saving SRAM to %s\n", pad);
    FIL file;
    if (f_open(&file, pad, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
    {
        UINT byteswritten;
        if (f_write(&file, priv.cart_ram, gb_get_save_size(&gb), &byteswritten) != FR_OK)
        {
            printf("Error writing SRAM\n");
        }
        f_close(&file);
    }
}
void emu_set_dmg_palette_type(dmg_palette_type_t dmg_palette_type)
{
    currentDmgPaletteType = dmg_palette_type;
    dmgPaletteLutValid = false; // rebuilt on the next frame
}
int startemulation(uint8_t *rom, char *romname, const char *savedir, char *ErrorMessage, int USEHSTX)
{
    useHSTX = USEHSTX;
    dmgPaletteLutValid = false; // RGB555 vs RGB444 depends on useHSTX
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            dmgGreyscalePalette444[i][j] = RGB555_TO_RGB444(dmgGreyscalePalette555[i][j]);
            dmgColorPalette444[i][j] = RGB555_TO_RGB444(dmgColorPalette555[i][j]);
        }
    }
#if PEANUT_FULL_GBC_SUPPORT
    gbcPal = useHSTX ? gb.cgb.fixPalette : gb.cgb.fixPalette444;
#endif
#if 0
    gb.wram = (uint8_t *)frens_f_malloc(WRAM_SIZE);
	gb.vram = (uint8_t *)frens_f_malloc(VRAM_SIZE);
	gb.oam = (uint8_t *)frens_f_malloc(OAM_SIZE);
	gb.hram_io = (uint8_t *)frens_f_malloc(HRAM_IO_SIZE);
#endif
    printf("Starting GB emulation\n");
    ErrorMessage[0] = 0;
#if !PEANUT_FULL_GBC_SUPPORT
    /* GBC support is compiled out of this build. gb_init() ignores the CGB flag
     * entirely in that case, so a cartridge with no DMG code path would just
     * render garbage. Header byte 0x0143: 0x80 = CGB-enhanced but backwards
     * compatible (runs correctly as DMG, allow it), 0xC0 = CGB-only (reject). */
    if (rom[0x0143] == 0xC0)
    {
        snprintf(ErrorMessage, 40, "Game Boy Color game, DMG-only build");
        printf("%s\n", ErrorMessage);
        return 0;
    }
#endif
    priv.rom = GBaddress = rom;
    ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read,
                  &gb_cart_ram_write, &gb_error, &priv);

    if (ret != GB_INIT_NO_ERROR)
    {
        snprintf(ErrorMessage, 40, "Cannot init emulator %d", ret);
        printf("%s\n", ErrorMessage);
        return 0;
    }
    printf("Emulator initialized, rom name.\n");
#if ENABLE_SOUND
    printf("Starting audio\n");
    printf("Audio: %d slices/frame, %d-sample staging buffer (%d bytes).\n",
           AUDIO_CHUNKS_PER_FRAME, AUDIO_SLICE_MAX_SAMPLES, (int)AUDIO_BUFFER_SIZE);
    audio_frame_budget = 0;
    audio_emitted = 0;
    audio_chunk_countdown = AUDIO_CHUNK_LINE_STEP;
    audio_init();
#endif
    uint32_t save_size = gb_get_save_size(&gb);
    printf("Allocating %d bytes for cart ram.\n", save_size);
    priv.cart_ram = NULL;
    if (save_size > 0 && save_size <= MAX_SRAM_SIZE)
    {
        priv.cart_ram = (uint8_t *)frens_f_malloc(save_size);
        memset(priv.cart_ram, 0, save_size);
        if (priv.cart_ram == NULL)
        {
            strcpy(ErrorMessage, "Cannot allocate memory for save file");
            printf("%s\n", ErrorMessage);
            return 0;
        }
        else
        {
            loadsram(romname, savedir);
        }
    }
    if (save_size > MAX_SRAM_SIZE)
    {
        strcpy(ErrorMessage, "Save size too large, max 32KB");
        printf("%s\n", ErrorMessage);
        return 0;
    }

    return 1;
}

void __not_in_flash_func(lcd_draw_line)(struct gb_s *gb,
                                        const uint8_t *pixels,
                                        const uint_fast8_t line)
{
    WORD *buff = dvi_getlinebuffer(line);
#if PEANUT_FULL_GBC_SUPPORT
    if (gb->cgb.cgbMode)
    { // CGB
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            buff[x] = gbcPal[pixels[x]];
        }
    }
    else
    { // DMG
#endif
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            buff[x] = dmgPaletteLut[pixels[x] & (LCD_PALETTE_ALL | LCD_COLOUR)];
        }
#if PEANUT_FULL_GBC_SUPPORT
    }
#endif

    infogb_plot_line(line);

#if ENABLE_SOUND && AUDIO_CHUNKS_PER_FRAME > 1
    /* Sub-frame audio slice. This callback already fires once per visible
     * scanline, so hooking it costs nothing in the CPU dispatch loop. Sample
     * position is tracked against LCD_VERT_LINES (154), not LCD_HEIGHT, so the
     * slices follow real time; the ~6.5% of the frame spent in VBlank is
     * flushed by emu_run_frame() once the frame completes. When the LCD is off
     * this never runs and the end-of-frame flush renders the lot, which is the
     * original behaviour. Compiled out entirely where scanout has no
     * framebuffer to hide the stall behind -- see AUDIO_CHUNKS_PER_FRAME. */
    if (--audio_chunk_countdown == 0)
    {
        audio_chunk_countdown = AUDIO_CHUNK_LINE_STEP;
        emu_audio_emit_upto((int)(((uint32_t)audio_frame_budget * (line + 1)) / LCD_VERT_LINES));
    }
#endif
    return;
}

// void emu_init_lcd(void (*lcd_draw_line)(const uint_fast8_t line)) {
void emu_init_lcd()
{
    gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = false;
    gb.direct.frame_skip = false;
}

/* __not_in_flash_func matters here even though gb_run_frame() already carries
 * it: GCC inlines that one-line wrapper into this function, so without the
 * attribute the whole `while (!gb->gb_frame) __gb_step_cpu(gb);` loop stayed in
 * flash and every emulated instruction paid for a flash-resident loop body plus
 * an indirect ldr-pc through ____gb_step_cpu_veneer -- while sharing the XIP
 * cache with the ROM reads gb_rom_read() issues on each fetch. */
void __not_in_flash_func(emu_run_frame)()
{
    if (!dmgPaletteLutValid)
    {
        buildDmgPaletteLut();
    }
#if ENABLE_SOUND
    /* Ask the sink how many samples it wants before running the frame, so the
     * slices emitted from lcd_draw_line() add up to exactly that. */
    audio_frame_budget = emu_audio_frame_budget();
    if (audio_frame_budget > AUDIO_FRAME_MAX_SAMPLES)
        audio_frame_budget = AUDIO_FRAME_MAX_SAMPLES;
    else if (audio_frame_budget < 0)
        audio_frame_budget = 0;
    audio_emitted = 0;
    audio_chunk_countdown = AUDIO_CHUNK_LINE_STEP;
#endif
    gb_run_frame(&gb);
#if ENABLE_SOUND
    /* VBlank's share, plus everything else if the LCD was off this frame. */
    emu_audio_emit_upto(audio_frame_budget);
#endif
}
void emu_set_gamepad(uint8_t joypad)
{
    gb.direct.joypad = joypad;
}

void stopemulation(char *romname, const char *savedir)
{
    if (priv.cart_ram != NULL)
    {
        savesram(romname, savedir);
        frens_f_free(priv.cart_ram);
        priv.cart_ram = NULL;
    }
#if false
    frens_f_free(gb.wram);
    frens_f_free(gb.vram);
    frens_f_free(gb.oam);
    frens_f_free(gb.hram_io);
#endif
    printf("Stopped GB emulation\n");
}