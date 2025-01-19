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

#ifndef CPUKFREQKHZ
#define CPUKFREQKHZ 266000
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

namespace
{
    constexpr uint32_t CPUFreqKHz = CPUKFREQKHZ; // 252000;
     dvi::DVI::LineBuffer *currentLineBuffer_{};
}

int sample_index = 0;

void __not_in_flash_func(processaudio)()
{

    int samples = 6; //  (739/144)

    // the audio_buffer is in fact a 32 bit array.
    // the first 16 bits are the left channel, the next 16 bits are the right channel
    uint32_t *sample_buffer = (uint32_t *)audio_stream;

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
        while (ct--)
        {

            // extract the left and right channel from the audio buffer
            uint32_t *p1 = &sample_buffer[sample_index];
            int l = *p1 >> 16;
            ;
            int r = *p1 & 0xFFFF;
            *p++ = {static_cast<short>(l), static_cast<short>(r)};
            sample_index++;
        }
        ring.advanceWritePointer(n);
        samples -= n;
    }
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
                // printf("FPS: %s\n", fps_enabled ? "ON" : "OFF");
            }
            if (pushed & UP)
            {
                Frens::screenMode(-1);
            }
            else if (pushed & DOWN)
            {
                Frens::screenMode(+1);
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
    auto count = dvi_->getFrameCounter();
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

WORD *__not_in_flash_func(dvi_getlinebuffer)()
{
    uint16_t *sbuffer;
#if NORENDER
    static WORD tmpbuffer[512];
    sbuffer = tmpbuffer;
#else
    auto b = dvi_->getLineBuffer();
    sbuffer = b->data() + (LEFTMARGIN);
    currentLineBuffer_ = b;
#endif
    return sbuffer;
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
            WORD *fpsBuffer = currentLineBuffer_->data() + FPSLEFTMARGIN;
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
#if FPSLEFTMARGIN < LEFTMARGIN
        if (line >= FPSEND)
        {
            WORD *fpsBuffer = currentLineBuffer_->data() + FPSLEFTMARGIN;
            for (auto i = 0; i < 16; i++)
            {
                *fpsBuffer++ = 0;
            }
        }
#endif
    }
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

    DWORD pdwPad1, pdwPad2, pdwSystem; // have only meaning in menu
    int fcount = 0;
    emu_init_lcd(&infogb_plot_line);
    uint32_t ti1, ti2;
    int frametime = 0;
    while (reset == false)
    {
        sample_index = 0;
        processinput(false, &pdwPad1, &pdwPad2, &pdwSystem, false, nullptr);
        ti1 = Frens::time_us();
        emu_run_frame();
        ti2 = Frens::time_us();
        frametime = ti2 - ti1;
        fcount++;
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

    // Set voltage and clock frequency
    vreg_set_voltage(VREG_VOLTAGE_1_20); // VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(CPUFreqKHz, true);

    stdio_init_all();
    printf("Starting DMG Gameboy program\n");
    printf("CPU freq: %d\n", clock_get_hz(clk_sys));
    printf("Starting Tinyusb subsystem\n");
    tusb_init();

    isFatalError = !Frens::initAll(selectedRom, CPUFreqKHz, MARGINTOP, MARGINBOTTOM, 512);

    bool showSplash = true;
    while (true)
    {
        if (strlen(selectedRom) == 0 || reset == true)
        {
            menu("Pico-Peanut-GB", ErrorMessage, isFatalError, showSplash, ".gb .gbc"); // never returns, but reboots upon selecting a game
        }
        reset = false;
        printf("Now playing: %s\n", selectedRom);

        printf("Initializing Game Boy Emulator\n");
        uint8_t *rom = reinterpret_cast<unsigned char *>(ROM_FILE_ADDR);
        if (startemulation(rom, romName, GAMESAVEDIR, ErrorMessage))
        {
            process();
            stopemulation(romName, GAMESAVEDIR);
        }
        selectedRom[0] = 0;
        showSplash = false;
    }
    return 0;
}
