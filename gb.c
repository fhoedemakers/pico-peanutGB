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
static uint16_t (*dmgPal)[4] = NULL; // pointer to current DMG palette
static dmg_palette_type_t currentDmgPaletteType = DMG_PALETTE_GREENLCD;

#if ENABLE_SOUND
#define AUDIO_BUFFER_SIZE (AUDIO_SAMPLES * sizeof(u_int32_t))
uint16_t *audio_stream;
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
}
int startemulation(uint8_t *rom, char *romname, const char *savedir, char *ErrorMessage, int USEHSTX)
{
    useHSTX = USEHSTX;
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
    printf("Number of %d-byte samples per frame: %d\n", sizeof(u_int32_t), AUDIO_SAMPLES);
    printf("Allocating %d bytes for sample buffer (%d * %d).\n", AUDIO_BUFFER_SIZE, AUDIO_SAMPLES, sizeof(u_int32_t));
    printf("Audio Samples per frame: %d\n", AUDIO_SAMPLES);
    // Audiobuffer is a 32 bit array of AUDIO_SAMPLES
    audio_stream = (uint16_t *)frens_f_malloc(AUDIO_BUFFER_SIZE);
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
            // uint8_t color_index = pixels[x] & 0x03; // Get the 2-bit color index
            // buff[x] = currentpalette[color_index]
            buff[x] = dmgPal[(pixels[x] & LCD_PALETTE_ALL) >> 4][pixels[x] & 3];
        }
#if PEANUT_FULL_GBC_SUPPORT
    }
#endif

    infogb_plot_line(line);
    //   /* If external callback provided, invoke it with current line. */
    //   if (dvi_drawline_cb) {
    //       dvi_drawline_cb(line);
    //   }
    return;
}

// void emu_init_lcd(void (*lcd_draw_line)(const uint_fast8_t line)) {
void emu_init_lcd()
{
    gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = false;
    gb.direct.frame_skip = false;
}

void emu_run_frame()
{
    switch (currentDmgPaletteType)
    {
    case DMG_PALETTE_GREENLCD:
        dmgPal = useHSTX ? dmgGreenPalette555 : dmgGreenPalette444;
        break;
    case DMG_PALETTE_COLOR:
        dmgPal = useHSTX ? dmgColorPalette555 : dmgColorPalette444;
        break;
    case DMG_PALETTE_GRAYSCALE:
    default:
        dmgPal = useHSTX ? dmgGreyscalePalette555 : dmgGreyscalePalette444;
        break;
    }
    gb_run_frame(&gb);
#if ENABLE_SOUND
    // send audio buffer to playback device
    audio_callback(NULL, (uint8_t *)audio_stream, AUDIO_BUFFER_SIZE);
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
    }
    if (audio_stream != NULL)
    {
        frens_f_free(audio_stream);
    }
#if false
    frens_f_free(gb.wram);
    frens_f_free(gb.vram);
    frens_f_free(gb.oam);
    frens_f_free(gb.hram_io);
#endif
    printf("Stopped GB emulation\n");
}