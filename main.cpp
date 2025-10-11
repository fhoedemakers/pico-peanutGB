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

#include "mytypes.h"
#include "gb.h"
#include "vumeter.h"
#ifndef CPUKFREQKHZ
// #define CPUKFREQKHZ 266000
#define CPUKFREQKHZ 252000
#endif

char *romName;

bool isFatalError = false;

static bool fps_enabled = false;
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

int sample_index = 0;

// Master gain control for external audio output (I2S / SPI).
// Higher values mean more attenuation: sample >> AUDIO_OUTPUT_GAIN_SHIFT.
// 4 was effectively used before (see l >> 4 in enqueue). If audio was too loud
// and distorted (likely clipping in DAC / amp), increase this value.
// Try 5 ( /32 ) or 6 ( /64 ) depending on headroom needed.
#ifndef AUDIO_OUTPUT_GAIN_SHIFT
#define AUDIO_OUTPUT_GAIN_SHIFT 5
#endif

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
#else

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
            nespadbuttons |= wiipad_read();
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
        if (p1 & SELECT)
        {
            if (pushed & START)
            {
                reset = true;
                printf("Reset pressed\n");
            }
        }
        if (p1 & START)
        {
            // Toggle frame rate display
            if (pushed & A)
            {
                fps_enabled = !fps_enabled;
                Frens::loadOverLay(); // reload overlay to show or hide fps
                // printf("FPS: %s\n", fps_enabled ? "ON" : "OFF");
            }
            if (pushed & UP)
            {
#if !HSTX
                Frens::screenMode(-1);
#endif
            }
            else if (pushed & DOWN)
            {
#if !HSTX
                Frens::screenMode(+1);
#endif
            }
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
    if (fps_enabled)
    {
        // calculate fps and round to nearest value (instead of truncating/floor)
        uint32_t tick_us = Frens::time_us() - start_tick_us;
        fps = (1000000 - 1) / tick_us + 1;
        start_tick_us = Frens::time_us();
        fpsString[0] = '0' + (fps / 10);
        fpsString[1] = '0' + (fps % 10);
    }

    return count;
}

WORD *__not_in_flash_func(dvi_getlinebuffer)(uint8_t line)
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
    if (fps_enabled)
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
            processaudio();
        }
        dvi_->setLineBuffer(line, currentLineBuffer_);
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#endif
    processaudio();
    prevline = line;
#endif
}

bool load_rom(char *, unsigned char *)
{
    return true;
}
void __not_in_flash_func(process)()
{

    uint32_t *sample_buffer = (uint32_t *)audio_stream;
    DWORD pdwPad1, pdwPad2, pdwSystem; // have only meaning in menu
    int fcount = 0;
    emu_init_lcd(&infogb_plot_line);
    uint32_t ti1, ti2;
    int frametime = 0;
    while (reset == false)
    {
        Frens::PaceFrames60fps(false);
        sample_index = 0;
        processinput(false, &pdwPad1, &pdwPad2, &pdwSystem, false, nullptr);
        ti1 = Frens::time_us();
        emu_run_frame();
        ti2 = Frens::time_us();
        frametime = ti2 - ti1;
        fcount++;
        ProcessAfterFrameIsRendered(false);
        for (int i = 0; i < 738; ++i)
        {
            uint32_t packed = sample_buffer[i];
            // Properly sign-extend the 16-bit samples.
            int16_t l = static_cast<int16_t>(packed >> 16);
            int16_t r = static_cast<int16_t>(packed & 0xFFFF);
            // Apply master attenuation to avoid clipping/distortion on output stage.
            EXT_AUDIO_ENQUEUE_SAMPLE(l >> AUDIO_OUTPUT_GAIN_SHIFT, r >> AUDIO_OUTPUT_GAIN_SHIFT);
#if ENABLE_VU_METER
            if (settings.flags.enableVUMeter)
            {
                addSampleToVUMeter(l);
            }
#endif
        }
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

    isFatalError = !Frens::initAll(selectedRom, CPUFreqKHz, MARGINTOP, MARGINBOTTOM, 512 * 8, false, true);
#if !HSTX
    if (settings.screenMode != ScreenMode::NOSCANLINE_1_1 && settings.screenMode != ScreenMode::SCANLINE_1_1)
    {
        // force NOSCANLINE_1_1 mode for GB, as the framebuffer is only 160x144 pixels
        settings.screenMode = ScreenMode::NOSCANLINE_1_1;
        Frens::savesettings();
    }
    scaleMode8_7_ = Frens::applyScreenMode(settings.screenMode);
#endif

    bool showSplash = true;
    while (true)
    {
        if (strlen(selectedRom) == 0 || reset == true)
        {
            menu("Pico-PeanutGB", ErrorMessage, isFatalError, showSplash, ".gb .gbc", selectedRom, "GB"); // never returns, but reboots upon selecting a game
        }

        reset = false;
        printf("Now playing: %s\n", selectedRom);

        printf("Initializing Game Boy Emulator\n");
        Frens::loadOverLay(); // load default overlay
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
