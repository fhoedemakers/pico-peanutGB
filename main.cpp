/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/divider.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "ff.h"
#include "tusb.h"
#include "gamepad.h"
#include "menu.h"
#include "nespad.h"
#include "wiipad.h"
#include "FrensHelpers.h"
#include "settings.h"
#include "FrensFonts.h"
#include "menu_settings.h"
#include "mytypes.h"
#include "gb.h"
#include "vumeter.h"
#ifndef CPUKFREQKHZ
// #define CPUKFREQKHZ 266000
#define CPUKFREQKHZ 252000
#endif

// Visibility configuration for options menu (NES specific)
// 1 = show option line, 0 = hide.
// Order must match enum in menu_options.h
const int8_t g_settings_visibility_gb[MOPT_COUNT] = {
    0,                               // Exit Game, or back to menu. Always visible when in-game.
    0,                               // Reset Game
    BOOTLOADER_BUILD,                // Return to emuLoader picker (only when built for the loader)
    // -1, not 0: the in-game menu force-shows Exit Game, Reset Game and Save /
    // Restore State whatever this array says, so 0 only hides a row in the file
    // browser. This port has no save-state implementation at all, so the row has
    // to be suppressed outright, which is what -1 does.
    -1,                              // Save / Restore State
    1,                               // Screen Mode
    0,                               // Scanlines toggle (superseded by Screen Mode)
    HSTX,                            // Scanline Type (HSTX only)
    1,                               // FPS Overlay
    0,                               // Audio Enable
    0,                               // Frame Skip
    HSTX && ENABLEDVI,               // Display Mode (HDMI or DVI, only when HSTX is enabled, because non-HSTX builds always use HDMI)
    (EXT_AUDIO_IS_ENABLED ), // External Audio
    1,                               // Font Color
    1,                               // Font Back Color
    ENABLE_VU_METER,                 // VU Meter
    //(HW_CONFIG == 8),                // Fruit Jam Internal Speaker
    (HW_CONFIG == 8),                // Fruit Jam Volume Control
    1,                               // DMG Palette (NES emulator does not use GameBoy palettes)
    1,                               // Border Mode (Super Gameboy style borders not applicable for NES)
    0,                               // Rapid Fire on A
    0,                               // Rapid Fire on B
    0,                               // Auto Insert Disk A, enabled at runtime on RP2350
    0,                               // Auto Swap FDS, enabled at runtime on RP2350
    0,                               // FDS Disk Swap (toggled on after fdsParse succeeds)
    0,                               // Overclock (CPU high clock toggle)
    0,                               // YM Audio SMS Only
    1,                               // Enter bootsel mode
    1,                               // Controller Test
    // Recent Games is rom-browser only and menu.cpp forces it visible on >= 0,
    // so it already showed via the zero-fill this list left behind. Stated
    // explicitly so the array length matches MOPT_COUNT again: the next option
    // appended to the enum then lands on a slot that is missing here, rather
    // than silently inheriting this one's value. Set to -1 to hide it.
    1,                               // Recent Games
};
const uint8_t g_available_screen_modes_gb[] = {
        0,   // SCANLINE_8_7,
        0,  // NOSCANLINE_8_7,
        1,  // SCANLINE_1_1,
        1   //NOSCANLINE_1_1
};

extern const unsigned char EmuOverlay_444[];
extern const unsigned char EmuOverlay_555[];
char *romName;
bool showSettings = false;
bool isFatalError = false;

static uint32_t start_tick_us = 0;
static uint32_t fps = 0;
static char fpsString[3] = "00";
#define fpsfgcolor 0;     // black
#define fpsbgcolor 0xFFF; // white

#define MARGINTOP (24 * 2)
#define MARGINBOTTOM (24 * 2)

#define LEFTMARGIN 80
#define FPSLEFTMARGIN 40

#define FPSSTART (((MARGINTOP + 7) / 8) * 8)
#define FPSEND ((FPSSTART) + 8)

static bool reset = false;
static bool resetGame = false;
#ifndef NORENDER
#define NORENDER 0 // 0 is render frames in emulation loop
#endif

constexpr uint32_t CPUFreqKHz = CPUKFREQKHZ; // 252000;
#if !HSTX
dvi::DVI::LineBuffer *currentLineBuffer_{};
#endif
WORD *currentLineBuf{nullptr};
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
// Cached Wii pad state updated once per frame in ProcessAfterFrameIsRendered()
static uint16_t wiipad_raw_cached = 0;
#endif
void loadoverlay()
{
    if (!Frens::isFrameBufferUsed())
    {
        return;
    }
    char CRC[9];
    static const char *borderdirs = "ABCDEFGHIJKLMNOPQRSTUVWY";
    static char PATH[FF_MAX_LFN + 1];
    static char CHOSEN[FF_MAX_LFN + 1];
    char *overlay =
#if !HSTX
        (char *)EmuOverlay_444;
#else
        (char *)EmuOverlay_555;
#endif
    ;
    int fldIndex;
    if (settings.flags.borderMode == FrensSettings::DEFAULTBORDER)
    {
        Frens::loadOverLay(nullptr, overlay);
        return;
    }

    if (settings.flags.borderMode == FrensSettings::THEMEDBORDER)
    {
        snprintf(CRC, sizeof(CRC), "%08X", Frens::getCrcOfLoadedRom());
        snprintf(CHOSEN, (FF_MAX_LFN + 1) * sizeof(char), "/metadata/GB/Images/Bezels/%c/%s%s", CRC[0], CRC, FILEXTFORSEARCH);
        printf("Loading bezel: %s\n", CHOSEN);
    }
    else
    {
        fldIndex = (rand() % strlen(borderdirs));
        snprintf(PATH, (FF_MAX_LFN + 1) * sizeof(char), "/metadata/GB/Images/Borders/%c", borderdirs[fldIndex]);
        printf("Scanning random folder: %s\n", PATH);
        FRESULT fr = Frens::pick_random_file_fullpath(PATH, CHOSEN, (FF_MAX_LFN + 1) * sizeof(char));
        if (fr != FR_OK)
        {
            printf("Failed to pick random file from %s: %d\n", PATH, fr);
            Frens::loadOverLay(nullptr, overlay);
            return;
        }
    }
    Frens::loadOverLay(CHOSEN, overlay);
}
// ---------------------------------------------------------------------------
// Audio output
//
// Every sink consumes exactly 44100 stereo frames per second, and the HDMI/DVI
// signal is exactly 60.000 Hz (25.2MHz / (800*525)), so 735 frames per video
// frame is the equilibrium rate. The emulator is not paced at 60 Hz though --
// hstx_paceFrame() targets 59.8261 Hz and waits two vsyncs roughly once every
// 344 frames -- so a fixed count drifts. Instead of tracking the pacer's
// constant, the count is nudged against the sink's own fill level, which
// self-corrects for pacing, clock drift and 59.94 vs 60.00 sinks alike.
//
// Previously the emulator produced a fixed 738 samples per frame:
//   - HSTX and DVI pushed all 738 unresampled (~44152/s), so the ring climbed
//     to HSTX_AUDIO_DI_HIGH_WATERMARK and dropped 4-sample packets from then on;
//   - I2S resampled 738->735 assuming exactly 60.00fps (~43972/s), so its ring
//     starved and the DMA stalled and restarted every few seconds.
// Both are audible as periodic ticks once the buffer reaches its limit.
//
// With the count correct at the source, the linear-interpolation resampler that
// used to sit in the I2S path is gone: the APU is asked for exactly the number
// of samples wanted, which is both cheaper and cleaner than interpolating.
// ---------------------------------------------------------------------------
#define AUDIO_BASE_SAMPLES_PER_FRAME 735
#define AUDIO_RATE_TRIM_MAX 8   // clamped so budget stays <= AUDIO_FRAME_MAX_SAMPLES
#define AUDIO_RATE_TRIM_SCALE 32

// Proportional trim towards `target`, both in stereo frames.
static inline int audioBudgetFor(int level, int target)
{
    int adj = (target - level) / AUDIO_RATE_TRIM_SCALE;
    if (adj > AUDIO_RATE_TRIM_MAX)
        adj = AUDIO_RATE_TRIM_MAX;
    else if (adj < -AUDIO_RATE_TRIM_MAX)
        adj = -AUDIO_RATE_TRIM_MAX;
    return AUDIO_BASE_SAMPLES_PER_FRAME + adj;
}

#if !HSTX
static inline int audioBudgetDVI()
{
    auto &ring = dvi_->getAudioRingBuffer();
    int capacity = (int)ring.getBufferSize();
    int level = capacity - (int)ring.getFullWritableSize() - 1;
    return audioBudgetFor(level, capacity / 2);
}

static inline void audioPushDVI(const uint32_t *samples, int count)
{
    int i = 0;
    while (i < count)
    {
        auto &ring = dvi_->getAudioRingBuffer();
        int writable = (int)ring.getWritableSize();
        if (!writable)
            return; // no space, drop remaining (should not happen once locked)
        int n = std::min(count - i, writable);
        auto p = ring.getWritePointer();
        for (int j = 0; j < n; ++j)
        {
            uint32_t packed = samples[i + j];
            int16_t l = static_cast<int16_t>(packed >> 16);
            int16_t r = static_cast<int16_t>(packed & 0xFFFF);
            *p++ = {(int16_t)(l >> 2), (int16_t)(r >> 2)};
        }
        ring.advanceWritePointer(n);
        i += n;
    }
}
#else
static inline int audioBudgetHSTX()
{
    // The DI ring holds DI_RING_BUFFER_SIZE (256) packets of 4 samples;
    // hstx_push_audio_sample() discards a batch once the level reaches
    // HSTX_AUDIO_DI_HIGH_WATERMARK (200 packets). Hold ~64 packets, ~5.8ms.
    int level = (int)hstx_di_queue_get_level() * 4;
    return audioBudgetFor(level, 64 * 4);
}

static inline void audioPushHSTX(const uint32_t *samples, int count)
{
    for (int i = 0; i < count; ++i)
    {
        uint32_t packed = samples[i];
        int16_t l = static_cast<int16_t>(packed >> 16);
        int16_t r = static_cast<int16_t>(packed & 0xFFFF);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(l);
        }
#endif
        hstx_push_audio_sample(l >> 2, r >> 2);
    }
}
#endif

#if EXT_AUDIO_IS_ENABLED
#ifndef I2S_AUDIO_RING_SIZE
#define I2S_AUDIO_RING_SIZE 1024
#endif
static inline int audioBudgetI2S()
{
    int level = I2S_AUDIO_RING_SIZE - 1 - EXT_AUDIO_GET_FREE();
    return audioBudgetFor(level, I2S_AUDIO_RING_SIZE / 2);
}

// Gain: either a shift (fast) or a Q15 multiplier (define AUDIO_OUTPUT_GAIN_Q15).
#ifndef AUDIO_OUTPUT_GAIN_SHIFT
#define AUDIO_OUTPUT_GAIN_SHIFT 2 // ~ /4, matches the DVI and HSTX paths
#endif

static inline void audioPushI2S(const uint32_t *samples, int count)
{
    // Gentle DC blocker: y[n] = x[n] - x[n-1] + a*y[n-1], a ~ 0.99976 in Q15
    // (corner ~1.7Hz at 44.1kHz). State persists across frames and slices.
    struct DcBlockState
    {
        int32_t yl = 0;
        int32_t yr = 0;
        int16_t xl_prev = 0;
        int16_t xr_prev = 0;
    };
    static DcBlockState dcState{};
    constexpr int32_t kDcCoeffQ15 = 32760;

    auto dc_block = [](int16_t x, int16_t &x_prev, int32_t &y_prev) -> int16_t
    {
        int32_t y = (int32_t)x - (int32_t)x_prev + (int32_t)(((int64_t)kDcCoeffQ15 * y_prev) >> 15);
        x_prev = x;
        if (y > 32767)
            y = 32767;
        else if (y < -32768)
            y = -32768;
        y_prev = y;
        return (int16_t)y;
    };

    for (int i = 0; i < count; ++i)
    {
        uint32_t packed = samples[i];
        int16_t l = static_cast<int16_t>(packed >> 16);
        int16_t r = static_cast<int16_t>(packed & 0xFFFF);

        l = dc_block(l, dcState.xl_prev, dcState.yl);
        r = dc_block(r, dcState.xr_prev, dcState.yr);

#ifdef AUDIO_OUTPUT_GAIN_Q15
        l = (int16_t)(((int32_t)l * AUDIO_OUTPUT_GAIN_Q15) >> 15);
        r = (int16_t)(((int32_t)r * AUDIO_OUTPUT_GAIN_Q15) >> 15);
#else
        l = (int16_t)(l >> AUDIO_OUTPUT_GAIN_SHIFT);
        r = (int16_t)(r >> AUDIO_OUTPUT_GAIN_SHIFT);
#endif

        EXT_AUDIO_ENQUEUE_SAMPLE(l, r);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(l);
        }
#endif
    }
}
#endif // EXT_AUDIO_IS_ENABLED

// Which sink is live can change between frames (headphone jack), so the budget
// query and the pushes select it the same way.
static inline bool audioUseExternal()
{
#if EXT_AUDIO_IS_ENABLED
    return settings.flags.useExtAudio == 1 || Frens::isHeadPhoneJackConnected();
#else
    return false;
#endif
}

// Called by gb.c once per video frame, before the frame is emulated.
int __not_in_flash_func(emu_audio_frame_budget)()
{
#if EXT_AUDIO_IS_ENABLED
    if (audioUseExternal())
        return audioBudgetI2S();
#endif
#if !HSTX
    return audioBudgetDVI();
#else
    return audioBudgetHSTX();
#endif
}

// Called by gb.c several times per video frame, as the frame is emulated.
void __not_in_flash_func(emu_audio_output)(const uint32_t *samples, int count)
{
#if EXT_AUDIO_IS_ENABLED
    if (audioUseExternal())
    {
        audioPushI2S(samples, count);
        return;
    }
#endif
#if !HSTX
    audioPushDVI(samples, count);
#else
    audioPushHSTX(samples, count);
#endif
}

static DWORD prevButtons[2]{};
static DWORD prevButtonssystem[2]{};
static DWORD prevOtherButtons[2]{};

static int rapidFireMask[2]{};
static int rapidFireCounter = 0;
void processinput(bool fromMenu, DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem, bool ignorepushed, char *gamepadType = nullptr)
{
    // pwdPad1 and pwdPad2 are only used in menu and are only set on first push
    *pdwPad1 = *pdwPad2 = *pdwSystem = 0;
    unsigned long pushed;
    for (int i = 0; i < 2; i++)
    {
        int nespadbuttons = 0;
        auto &dst = (i == 0) ? *pdwPad1 : *pdwPad2;
        auto &gp = io::getCurrentGamePadState(i);
        int v = (gp.buttons & io::GamePadState::Button::LEFT ? LEFT : 0) |
                (gp.buttons & io::GamePadState::Button::RIGHT ? RIGHT : 0) |
                (gp.buttons & io::GamePadState::Button::UP ? UP : 0) |
                (gp.buttons & io::GamePadState::Button::DOWN ? DOWN : 0) |
                (gp.buttons & io::GamePadState::Button::A ? A : 0) |
                (gp.buttons & io::GamePadState::Button::B ? B : 0) |
                (gp.buttons & io::GamePadState::Button::SELECT ? SELECT : 0) |
                (gp.buttons & io::GamePadState::Button::START ? START : 0) | 0;
        if (i == 0)
        {
            if (gamepadType)
            {
                strcpy(gamepadType, gp.GamePadName);
            }
#if NES_PIN_CLK != -1
            nespadbuttons = nespad_states[0];
#endif

#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
            nespadbuttons |= wiipad_raw_cached;
#endif
            if (nespadbuttons > 0)
            {

                v |= ((nespadbuttons & NESPAD_UP ? UP : 0) |
                      (nespadbuttons & NESPAD_DOWN ? DOWN : 0) |
                      (nespadbuttons & NESPAD_LEFT ? LEFT : 0) |
                      (nespadbuttons & NESPAD_RIGHT ? RIGHT : 0) |
                      (nespadbuttons & NESPAD_A ? A : 0) |
                      (nespadbuttons & NESPAD_B ? B : 0) |
                      (nespadbuttons & NESPAD_SELECT ? SELECT : 0) |
                      (nespadbuttons & NESPAD_START ? START : 0) | 0);
            }
        }
        auto p1 = v;
        if (ignorepushed == false)
        {
            pushed = v & ~prevButtons[i];
        }
        else
        {
            pushed = v;
        }
        if (p1 & START)
        {
            // Toggle frame rate display
            if (pushed & A)
            {
                settings.flags.displayFrameRate = !settings.flags.displayFrameRate;
                // FrensSettings::savesettings();
                loadoverlay(); // reload overlay to show or hide fps
                // printf("FPS: %s\n", fps_enabled ? "ON" : "OFF");
            } else if (pushed & B)
            {
                // toggle DMG palette
                settings.flags.dmgLCDPalette = (settings.flags.dmgLCDPalette + 1) % 3;
                emu_set_dmg_palette_type((dmg_palette_type_t)settings.flags.dmgLCDPalette);
                printf("DMG LCD Palette: %d:", settings.flags.dmgLCDPalette);
                switch ((dmg_palette_type_t)settings.flags.dmgLCDPalette)
                {
                case DMG_PALETTE_GREENLCD:
                    printf(" Green\n");
                    break;
                case DMG_PALETTE_COLOR:
                    printf(" Color\n");
                    break;
                case DMG_PALETTE_GRAYSCALE:
                    printf(" Grayscale\n");
                    break;
                default:
                    break;
                }
                // FrensSettings::savesettings();
            } else if (pushed & LEFT) {
#if HW_CONFIG == 8
               settings.fruitjamVolumeLevel = std::max(-63, settings.fruitjamVolumeLevel - 1);
               EXT_AUDIO_SETVOLUME(settings.fruitjamVolumeLevel);
#endif
            } else if (pushed & RIGHT) {
#if HW_CONFIG == 8
               settings.fruitjamVolumeLevel = std::min(23, settings.fruitjamVolumeLevel + 1);
               EXT_AUDIO_SETVOLUME(settings.fruitjamVolumeLevel);
#endif
            }
        }
        if (p1 & SELECT)
        {
            if (pushed & B)
            {
                // toggle settings.bordermode between enum values
                settings.flags.borderMode = (settings.flags.borderMode + 1) % 3; 
                printf("Border mode: %d\n", settings.flags.borderMode);
                // FrensSettings::savesettings();
                loadoverlay();
            }
            // else if (pushed & A)
            // {
            //     loadoverlay(); // reload overlay to show or hide fps
            // }
            else if (pushed & START)
            {
                // reset = true;
                // printf("Reset pressed\n");
                showSettings = true;
            }
            else if (pushed & UP)
            {
#if !HSTX
                Frens::screenMode(-1);
#else
                Frens::toggleScanLines();
#endif
            }
            else if (pushed & DOWN)
            {
#if !HSTX
                Frens::screenMode(+1);
#else
                Frens::toggleScanLines();
#endif
            }
            else if (pushed & LEFT)
            {
                // Toggle audio output, ignore if HSTX is enabled, because HSTX must use external audio
#if EXT_AUDIO_IS_ENABLED && !HSTX
                settings.flags.useExtAudio = !settings.flags.useExtAudio;
                if (settings.flags.useExtAudio)
                {
                    printf("Using I2S Audio\n");
                }
                else
                {
                    printf("Using DVIAudio\n");
                }

#else
                settings.flags.useExtAudio = 0;
#endif
                // FrensSettings::savesettings();
            }
#if ENABLE_VU_METER
            else if (pushed & RIGHT)
            {
                settings.flags.enableVUMeter = !settings.flags.enableVUMeter;
                // FrensSettings::savesettings();
                // printf("VU Meter %s\n", settings.flags.enableVUMeter ? "enabled" : "disabled");
                turnOffAllLeds();
            }
#endif
        }

        prevButtons[i] = v;
        // return only on first push
        if (pushed)
        {
            dst = v;
        }
        if (!fromMenu && i == 0)
        {
            emu_set_gamepad(~v);
        }
    }
}
int ProcessAfterFrameIsRendered(bool frommenu)
{

#if NES_PIN_CLK != -1
    nespad_read_start();
#endif
    auto count =
#if !HSTX
        dvi_->getFrameCounter();
#else
        hstx_getframecounter();
#endif
    auto onOff = hw_divider_s32_quotient_inlined(count, 60) & 1;
    Frens::blinkLed(onOff);
#if NES_PIN_CLK != -1
    nespad_read_finish(); // Sets global nespad_state var
#endif
    // nespad_read_finish(); // Sets global nespad_state var
    tuh_task();
    // Frame rate calculation
    if (settings.flags.displayFrameRate)
    {
        // calculate fps and round to nearest value (instead of truncating/floor)
        uint32_t tick_us = Frens::time_us() - start_tick_us;
        fps = (1000000 - 1) / tick_us + 1;
        start_tick_us = Frens::time_us();
        fpsString[0] = '0' + (fps / 10);
        fpsString[1] = '0' + (fps % 10);
    }
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
    // Poll Wii pad once per frame (function called once per rendered frame)
    wiipad_raw_cached = wiipad_read();
#endif
#if ENABLE_VU_METER
        if (isVUMeterToggleButtonPressed())
        {
            settings.flags.enableVUMeter = !settings.flags.enableVUMeter;
            // FrensSettings::savesettings();
            // printf("VU Meter %s\n", settings.flags.enableVUMeter ? "enabled" : "disabled");
            turnOffAllLeds();
        }
#endif
    if (showSettings)
    {
        showSettings = false;
        FrensSettings::savesettings();
        int rval = showSettingsMenu(true);
        if (rval == 3)
        {
            reset = true;
        }
        if (rval == 5) {
           reset = resetGame = true;
        }
        loadoverlay(); // reload overlay to show any changes
        emu_set_dmg_palette_type((dmg_palette_type_t)settings.flags.dmgLCDPalette); // in case palette was changed, GameBoy Specific
    }
    return count;
}

WORD *__not_in_flash_func(dvi_getlinebuffer)(uint_fast8_t line)
{
#if NORENDER
    static WORD tmpbuffer[512];
    currentLineBuf = tmpbuffer;
    return currentLineBuf;
#endif
#if !HSTX
#if FRAMEBUFFERISPOSSIBLE
    if (Frens::isFrameBufferUsed())
    {
        currentLineBuf = &Frens::framebuffer[(line + MARGINTOP) * SCREENWIDTH] + LEFTMARGIN;
    }
    else
    {
#endif
        auto b = dvi_->getLineBuffer();
        currentLineBuf = b->data() + (LEFTMARGIN);
        currentLineBuffer_ = b;
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#else
    currentLineBuf = hstx_getlineFromFramebuffer(line + MARGINTOP) + LEFTMARGIN;
#endif
    return currentLineBuf;
}

/**
 * Draws scanline into framebuffer.
 * GameBoy resolution is 160x144.
 *
 */
void __not_in_flash_func(infogb_plot_line)(uint_fast8_t line)
{
#if !NORENDER
    int origline = line;
    line += MARGINTOP;
    static uint_fast8_t prevline = MARGINTOP - 1;
    if (line == MARGINTOP)
    {
        prevline = MARGINTOP - 1;
    }
    // True when scanout recycles a small pool of line buffers rather than giving
    // every line its own storage. The GB writes only pixels LEFTMARGIN..+159 of
    // a buffer, so whatever else was in one persists into the next line it is
    // reused for -- which made the FPS digits reappear every
    // DVI_N_LINE_BUFFERS lines down the screen. On the framebuffer paths each
    // line has its own storage and the border holds the bezel, so there is
    // nothing to repair there and blanking would punch a hole in the artwork.
#if HSTX
    constexpr bool recycledLineBuffers = false;
#elif FRAMEBUFFERISPOSSIBLE
    const bool recycledLineBuffers = !Frens::isFrameBufferUsed();
#else
    constexpr bool recycledLineBuffers = true;
#endif

    // Display frame rate
    if (settings.flags.displayFrameRate && line >= FPSSTART && line < FPSEND)
    {
        WORD *fpsBuffer = currentLineBuf - LEFTMARGIN + FPSLEFTMARGIN;
        int rowInChar = line % 8;
        for (auto i = 0; i < 2; i++)
        {
            char firstFpsDigit = fpsString[i];
            char fontSlice = getcharslicefrom8x8font(firstFpsDigit, rowInChar);
            for (auto bit = 0; bit < 8; bit++)
            {
                if (fontSlice & 1)
                {
                    *fpsBuffer++ = fpsfgcolor;
                }
                else
                {
                    *fpsBuffer++ = fpsbgcolor;
                }
                fontSlice >>= 1;
            }
        }
    }
    else if (recycledLineBuffers)
    {
        // Keep the same 16 pixels deterministic on every other line, so a
        // recycled buffer cannot carry a stale glyph into it. Also clears the
        // digits when the overlay is switched off.
        WORD *fpsBuffer = currentLineBuf - LEFTMARGIN + FPSLEFTMARGIN;
        for (auto i = 0; i < 16; i++)
        {
            *fpsBuffer++ = 0;
        }
    }
#if !HSTX
#if FRAMEBUFFERISPOSSIBLE
    if (!Frens::isFrameBufferUsed())
    {
#endif
        // whene a scanline is skipped, copy current line buffer to the skipped line.
        if (line - 1 != prevline)
        {
            auto b = dvi_->getLineBuffer();
            WORD *buffer = b->data();
            WORD *currentLineBuffer = currentLineBuffer_->data();
            __builtin_memcpy(buffer, currentLineBuffer, 512 * sizeof(currentLineBuffer[0]));
            dvi_->setLineBuffer(line - 1, b);
        }
        dvi_->setLineBuffer(line, currentLineBuffer_);
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#endif
    prevline = line;
#endif
}

bool load_rom(char *, unsigned char *)
{
    return true;
}
#if EMU_FRAME_STATS
// Frame cost report over UART, once a second.
//
// The on-screen FPS counter cannot answer "how far over budget are we?" on the
// RP2040 PicoDVI path. There is no framebuffer there, so the emulator is paced
// by dvi_getlinebuffer() blocking on a 5-deep line queue, and dvi.cpp only
// accepts a line when `line * 2 == lineCounter_` exactly. Miss one scanline
// deadline and that equality never matches again for the rest of the frame:
// every remaining line renders from listActiveError_ (red) and the loop slips a
// whole display frame. So the counter reads 30 whether the emulator is 1% or
// 90% over budget.
//
// emu_run_frame() includes that blocking, so on a normal build this measures
// the display-bound loop. Build with -DNORENDER=1 to bypass the line queue
// entirely and measure raw emulation cost instead; the difference between the
// two is the whole diagnosis.
static void reportFrameStats(uint32_t emu_us)
{
    static uint32_t acc = 0, peak = 0, n = 0;
    acc += emu_us;
    if (emu_us > peak)
        peak = emu_us;
    if (++n >= 60)
    {
        printf("emu frame: avg %lu us, peak %lu us (budget 16667, norender=%d)\n",
               (unsigned long)(acc / n), (unsigned long)peak, NORENDER);
        acc = peak = n = 0;
    }
}
#endif

void __not_in_flash_func(process)()
{
    DWORD pdwPad1, pdwPad2, pdwSystem; // have only meaning in menu
    emu_init_lcd();
    while (reset == false)
    {
        Frens::PaceFrames60fps(false);
        //Frens::waitForVSync();
        Frens::pollHeadPhoneJack();
        processinput(false, &pdwPad1, &pdwPad2, &pdwSystem, false, nullptr);
        // Audio is rendered and pushed in slices from lcd_draw_line() while the
        // frame runs, so there is no separate output step here any more.
#if EMU_FRAME_STATS
        uint32_t emu_t0 = Frens::time_us();
#endif
        emu_run_frame();
#if EMU_FRAME_STATS
        reportFrameStats((uint32_t)(Frens::time_us() - emu_t0));
#endif
        ProcessAfterFrameIsRendered(false);
    }
}

/// @brief
/// Start emulator. Emulator does not run well in DEBUG mode, lots of red screen flicker. In order to keep it running fast enough, we need to run it in release mode or in
/// RelWithDebugInfo mode.
/// @return

// MARK: main
int main()
{
    char selectedRom[FF_MAX_LFN];
    romName = selectedRom;
    ErrorMessage[0] = selectedRom[0] = 0;

    Frens::setClocksAndStartStdio(CPUFreqKHz, VREG_VOLTAGE_1_30);

    printf("==========================================================================================\n");
    printf("Pico-PeanutGB %s\n", SWVERSION);
    printf("Build date: %s\n", __DATE__);
    printf("Build time: %s\n", __TIME__);
    printf("CPU freq: %d kHz\n", clock_get_hz(clk_sys) / 1000);
#if HSTX
    printf("HSTX freq: %d kHz\n", clock_get_hz(clk_hstx) / 1000);
#endif
    printf("Stack size: %d bytes\n", PICO_STACK_SIZE);
    printf("==========================================================================================\n");
    printf("Starting up...\n");
    FrensSettings::initSettings(FrensSettings::emulators::GAMEBOY);
    isFatalError = !Frens::initAll(selectedRom, CPUFreqKHz, MARGINTOP, MARGINBOTTOM, 512 * 8, false, true);
#if HSTX
    pico_hdmi_set_audio_sample_rate(44100);
#endif
#if !HSTX
    if (settings.screenMode != ScreenMode::NOSCANLINE_1_1 && settings.screenMode != ScreenMode::SCANLINE_1_1)
    {
        // force NOSCANLINE_1_1 mode for GB, as the framebuffer is only 160x144 pixels
        settings.screenMode = ScreenMode::NOSCANLINE_1_1;
        FrensSettings::savesettings();
    }
    scaleMode8_7_ = Frens::applyScreenMode(settings.screenMode);
#else
    hstx_setScanLines(settings.flags.scanlineOn);
#endif

    bool showSplash = true;
    g_settings_visibility = g_settings_visibility_gb;
    g_available_screen_modes = g_available_screen_modes_gb;
    while (true)
    {
        if (strlen(selectedRom) == 0 || reset == true)
        {
            menu("Pico-PeanutGB", ErrorMessage, isFatalError, showSplash, ".gb .gbc", selectedRom); 
        }
      
        printf("Now playing: %s\n", selectedRom);
        printf("Initializing Game Boy Emulator\n");
        do {
            reset = false;
            resetGame = false;
            // EXT_AUDIO_MUTE_INTERNAL_SPEAKER(settings.flags.fruitJamEnableInternalSpeaker == 0);
            EXT_AUDIO_SETVOLUME(settings.fruitjamVolumeLevel);
            loadoverlay(); // load default overlay
            emu_set_dmg_palette_type((dmg_palette_type_t)settings.flags.dmgLCDPalette);
            uint8_t *rom = reinterpret_cast<unsigned char *>(ROM_FILE_ADDR);
            if (startemulation(rom, romName, GAMESAVEDIR, ErrorMessage, HSTX))
            {
                Frens::PaceFrames60fps(true); 
                process();
                stopemulation(romName, GAMESAVEDIR);
            }
        } while (resetGame);
        selectedRom[0] = 0;
        showSplash = false;
    }
    return 0;
}
