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
const uint8_t g_settings_visibility[MOPT_COUNT] = {
    0,                               // Exit Game, or back to menu. Always visible when in-game.
    !HSTX,                           // Screen Mode (only when not HSTX)
    HSTX,                            // Scanlines toggle (only when HSTX)
    1,                               // FPS Overlay
    0,                               // Audio Enable
    0,                               // Frame Skip
    (EXT_AUDIO_IS_ENABLED && !HSTX), // External Audio
    1,                               // Font Color
    1,                               // Font Back Color
    ENABLE_VU_METER,                 // VU Meter
    (HW_CONFIG == 8),                // Fruit Jam Internal Speaker
    1,                               // DMG Palette (NES emulator does not use GameBoy palettes)
    1,                               // Border Mode (Super Gameboy style borders not applicable for NES)
    0,                               // Rapid Fire on A
    0                                // Rapid Fire on B

};
const uint8_t g_available_screen_modes[] = {
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

bool reset = false;

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
#if 0
void __not_in_flash_func(processaudio)()
{
    // the audio_buffer is in fact a 32 bit array.
    // the first 16 bits are the left channel, the next 16 bits are the right channel
    uint32_t *sample_buffer = (uint32_t *)audio_stream;
    int samples = 6; //  (739/144)

#if !HSTX

    while (samples)
    {

        auto &ring = dvi_->getAudioRingBuffer();
        auto n = std::min<int>(samples, ring.getWritableSize());
        if (!n)
        {
            return;
        }
        auto p = ring.getWritePointer();
        int ct = n;
        while (ct-- && sample_index < 738)
        {

            // Extract the left and right channel from the packed 32-bit sample.
            // Perform proper sign extension by casting to int16_t before widening.
            uint32_t packed = sample_buffer[sample_index];
            int16_t l = static_cast<int16_t>(packed >> 16);
            int16_t r = static_cast<int16_t>(packed & 0xFFFF);
            // Write unscaled to internal DVI ring buffer (it expects int16_t PCM).
            *p++ = {l, r};
            sample_index++;
        }
        ring.advanceWritePointer(n);
        samples -= n;
    }
#endif
}
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
#if !HSTX
static void inline processaudioPerFrameDVI()
{
    uint32_t *sample_buffer = (uint32_t *)audio_stream;
    constexpr int kSamplesPerFrame = 738; // stereo frames (left/right packed)
    int i = 0;
    while (i < kSamplesPerFrame)
    {
        auto &ring = dvi_->getAudioRingBuffer();
        int writable = ring.getWritableSize();
        if (!writable)
            return; // no space, drop remaining (rare)
        int n = std::min(kSamplesPerFrame - i, writable);
        auto p = ring.getWritePointer();
        for (int j = 0; j < n; ++j)
        {
            uint32_t packed = sample_buffer[i + j];
            int16_t l = static_cast<int16_t>(packed >> 16);
            int16_t r = static_cast<int16_t>(packed & 0xFFFF);
            // Optionally apply attenuation (reuse same macro as I2S for consistency)
            l = l >> 2;
            r = r >> 2;
            *p++ = {l, r};
        }
        ring.advanceWritePointer(n);
        i += n;
    }
}
#endif

// #define IMPROVED_I2S_DISABLE 0 // set to 1 to disable improved I2S path and use legacy simple path
static void inline processaudioPerFrameI2S()
{
    // Improved I2S path:
    // 1. Convert emulator frame audio (738 input samples @ ~44.28 kHz frame-based) to 44.1 kHz (735 samples) to
    //    match I2S configuration (PICO_AUDIO_I2S_FREQ).
    // 2. Apply gentle DC blocking filter to remove bias and increase headroom.
    // 3. Apply adjustable gain (bit shift or Q15 multiplier) with optional soft clip.
    // 4. Feed continuous ring buffer via EXT_AUDIO_ENQUEUE_SAMPLE (already DMA driven in driver).

#ifndef IMPROVED_I2S_DISABLE
    constexpr int kInSamplesPerFrame = 738;                             // emulator delivers this per video frame
    constexpr int kFramesPerSecond = 60;                                // video refresh
    constexpr int kTargetRate = PICO_AUDIO_I2S_FREQ;                    // normally 44100
    constexpr int kOutSamplesPerFrame = kTargetRate / kFramesPerSecond; // 44100 / 60 = 735
    static_assert(kTargetRate % kFramesPerSecond == 0, "I2S target rate must divide by FPS for fixed per-frame output count");

    uint32_t *sample_buffer = (uint32_t *)audio_stream; // packed emulator samples

    // Resample using simple linear interpolation: output index -> fractional input position
    // pos = j * kInSamplesPerFrame / kOutSamplesPerFrame
    // Keep DC-block filter state across frames.
    struct DcBlockState
    {
        int32_t yl = 0; // previous filtered sample (Q15 domain scaled back to int16 range)
        int32_t yr = 0;
        int16_t xl_prev = 0; // previous raw input sample
        int16_t xr_prev = 0;
    };
    static DcBlockState dcState{};

    // Q15 coefficient for 0.995 high-pass: y[n] = x[n]-x[n-1] + a*y[n-1]
    constexpr int32_t kDcCoeffQ15 = 32760; // ~0.9997? Actually 0.999 -> could tune; using slightly lower (0.995 = 32540). Adjust for taste.
    // Use 0.995 (32540). Replace constant above if wanting exactly 0.995.

    auto dc_block = [&](int16_t x, int16_t &x_prev, int32_t &y_prev) -> int16_t
    {
        int32_t y = (int32_t)x - (int32_t)x_prev + ((int64_t)kDcCoeffQ15 * y_prev >> 15);
        x_prev = x;
        // Soft limit in case of slight overshoot
        if (y > 32767)
            y = 32767;
        else if (y < -32768)
            y = -32768;
        y_prev = y;
        return (int16_t)y;
    };

    // Optional simple 5-tap binomial low-pass (commented out by default); enable if high-frequency alias present.
    // Keeping minimal overhead by disabled default.
    struct LP5State
    {
        int16_t d[5]{0, 0, 0, 0, 0};
    };
    static LP5State lpL, lpR;
    auto lowpass5 = [](int16_t x, LP5State &st) -> int16_t
    {
        // shift
        st.d[4] = st.d[3];
        st.d[3] = st.d[2];
        st.d[2] = st.d[1];
        st.d[1] = st.d[0];
        st.d[0] = x;
        // 1 4 6 4 1 kernel /16
        int32_t acc = st.d[0] + 4 * st.d[1] + 6 * st.d[2] + 4 * st.d[3] + st.d[4];
        return (int16_t)(acc >> 4);
    };

    // Gain control: either shift (fast) or Q15 multiplier (define AUDIO_OUTPUT_GAIN_Q15).
#ifndef AUDIO_OUTPUT_GAIN_SHIFT
#define AUDIO_OUTPUT_GAIN_SHIFT 2 // default attenuation ~ /4 to match DVI path (was 5 previously => too quiet)
#endif

#ifdef AUDIO_OUTPUT_GAIN_Q15
    static uint16_t gain_q15 = AUDIO_OUTPUT_GAIN_Q15; // allow external modification
#endif

    for (int j = 0; j < kOutSamplesPerFrame; ++j)
    {
        // Fractional mapping
        // Multiply first to preserve precision (both small ints); using 64-bit to avoid overflow
        uint64_t num = (uint64_t)j * kInSamplesPerFrame;
        uint32_t pos_int = num / kOutSamplesPerFrame;                                     // integer part
        uint32_t pos_next = (pos_int + 1 < kInSamplesPerFrame) ? (pos_int + 1) : pos_int; // clamp at end
        uint32_t frac_num = num - (uint64_t)pos_int * kOutSamplesPerFrame;                // remainder relative to denominator
        // Retrieve packed 32-bit samples
        uint32_t s0 = sample_buffer[pos_int];
        uint32_t s1 = sample_buffer[pos_next];
        int16_t l0 = (int16_t)(s0 >> 16);
        int16_t r0 = (int16_t)(s0 & 0xFFFF);
        int16_t l1 = (int16_t)(s1 >> 16);
        int16_t r1 = (int16_t)(s1 & 0xFFFF);
        // Linear interpolation: value = v0 + (v1 - v0) * frac
        // frac = frac_num / kOutSamplesPerFrame
        int32_t dl = (int32_t)l1 - (int32_t)l0;
        int32_t dr = (int32_t)r1 - (int32_t)r0;
        int16_t l = (int16_t)(l0 + (dl * (int32_t)frac_num) / kOutSamplesPerFrame);
        int16_t r = (int16_t)(r0 + (dr * (int32_t)frac_num) / kOutSamplesPerFrame);

        // DC block per channel
        l = dc_block(l, dcState.xl_prev, dcState.yl);
        r = dc_block(r, dcState.xr_prev, dcState.yr);

        // Optional smoothing low-pass (disabled by default for brightness)
#ifdef ENABLE_I2S_LP5_FILTER
        l = lowpass5(l, lpL);
        r = lowpass5(r, lpR);
#endif

        // Apply gain
#ifdef AUDIO_OUTPUT_GAIN_Q15
        int32_t l32 = (int32_t)l * gain_q15;
        l = (int16_t)(l32 >> 15);
        int32_t r32 = (int32_t)r * gain_q15;
        r = (int16_t)(r32 >> 15);
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
#else // IMPROVED_I2S_DISABLE
    // Legacy simple path (still here for fallback/testing)
    uint32_t *sample_buffer = (uint32_t *)audio_stream;
    constexpr int kSamplesPerFrame = 738;
    for (int i = 0; i < kSamplesPerFrame; ++i)
    {
        uint32_t packed = sample_buffer[i];
        int16_t l = static_cast<int16_t>(packed >> 16);
        int16_t r = static_cast<int16_t>(packed & 0xFFFF);
#ifdef AUDIO_OUTPUT_GAIN_SHIFT
        l = l >> AUDIO_OUTPUT_GAIN_SHIFT;
        r = r >> AUDIO_OUTPUT_GAIN_SHIFT;
#endif
        EXT_AUDIO_ENQUEUE_SAMPLE(l, r);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(l);
        }
#endif
    }
#endif // IMPROVED_I2S_DISABLE
}
void inline output_audio_per_frame()
{

#if !HSTX
#if EXT_AUDIO_IS_ENABLED
    if (settings.flags.useExtAudio == 1)
    {
        processaudioPerFrameI2S();
    }
    else
    {
        processaudioPerFrameDVI();
    }
#else
    processaudioPerFrameDVI();
#endif
#else
    processaudioPerFrameI2S();
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
                FrensSettings::savesettings();
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
                FrensSettings::savesettings();
            }
        }
        if (p1 & SELECT)
        {
            if (pushed & B)
            {
                // toggle settings.bordermode between enum values
                settings.flags.borderMode = (settings.flags.borderMode + 1) % 3; 
                printf("Border mode: %d\n", settings.flags.borderMode);
                FrensSettings::savesettings();
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
                FrensSettings::savesettings();
            }
#if ENABLE_VU_METER
            else if (pushed & RIGHT)
            {
                settings.flags.enableVUMeter = !settings.flags.enableVUMeter;
                FrensSettings::savesettings();
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
            FrensSettings::savesettings();
            // printf("VU Meter %s\n", settings.flags.enableVUMeter ? "enabled" : "disabled");
            turnOffAllLeds();
        }
#endif
    if (showSettings)
    {
        int rval = showSettingsMenu(true);
        if (rval == 3)
        {
            reset = true;
        }
        showSettings = false;
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
    // Display frame rate
    if (settings.flags.displayFrameRate)
    {
        if (line >= FPSSTART && line < FPSEND)
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
        // #if FPSLEFTMARGIN < LEFTMARGIN
        //         if (line >= FPSEND)
        //         {
        //             WORD *fpsBuffer = currentLineBuf - LEFTMARGIN + FPSLEFTMARGIN;
        //             for (auto i = 0; i < 16; i++)
        //             {
        //                 *fpsBuffer++ = 0;
        //             }
        //         }
        // #endif
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
            // processaudio();
        }
        dvi_->setLineBuffer(line, currentLineBuffer_);
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#endif
    // processaudio();
    prevline = line;
#endif
}

bool load_rom(char *, unsigned char *)
{
    return true;
}
void __not_in_flash_func(process)()
{
    DWORD pdwPad1, pdwPad2, pdwSystem; // have only meaning in menu
    int fcount = 0;
    emu_init_lcd();
    uint32_t ti1, ti2;
    int frametime = 0;
    while (reset == false)
    {
        Frens::PaceFrames60fps(false);
        processinput(false, &pdwPad1, &pdwPad2, &pdwSystem, false, nullptr);
        ti1 = Frens::time_us();
        emu_run_frame();
        ti2 = Frens::time_us();
        frametime = ti2 - ti1;
        fcount++;
        ProcessAfterFrameIsRendered(false);
        output_audio_per_frame();
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
#if !HSTX
    if (settings.screenMode != ScreenMode::NOSCANLINE_1_1 && settings.screenMode != ScreenMode::SCANLINE_1_1)
    {
        // force NOSCANLINE_1_1 mode for GB, as the framebuffer is only 160x144 pixels
        settings.screenMode = ScreenMode::NOSCANLINE_1_1;
        FrensSettings::savesettings();
    }
    scaleMode8_7_ = Frens::applyScreenMode(settings.screenMode);
#endif

    bool showSplash = true;
    while (true)
    {
        if (strlen(selectedRom) == 0 || reset == true)
        {
            menu("Pico-PeanutGB", ErrorMessage, isFatalError, showSplash, ".gb .gbc", selectedRom); 
        }
        reset = false;
        printf("Now playing: %s\n", selectedRom);

        printf("Initializing Game Boy Emulator\n");
        EXT_AUDIO_MUTE_INTERNAL_SPEAKER(settings.flags.fruitJamEnableInternalSpeaker == 0);
        loadoverlay(); // load default overlay
        emu_set_dmg_palette_type((dmg_palette_type_t)settings.flags.dmgLCDPalette);
        uint8_t *rom = reinterpret_cast<unsigned char *>(ROM_FILE_ADDR);
        if (startemulation(rom, romName, GAMESAVEDIR, ErrorMessage, HSTX))
        {
            process();
            stopemulation(romName, GAMESAVEDIR);
        }
        selectedRom[0] = 0;
        showSplash = false;
    }
    return 0;
}
