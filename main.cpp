/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <hardware/flash.h>
#include <memory>
#include <math.h>
#include <dvi/dvi.h>
#include <util/dump_bin.h>
#include <util/exclusive_proc.h>
#include <util/work_meter.h>
#include <tusb.h>

#include "mytypes.h"
#include "gamepad.h"
#include "menu.h"
#include "nespad.h"
#include "wiipad.h"
#include "FrensHelpers.h"
#include "gb.h"
#include "ff.h"

#ifndef DVICONFIG
// #define DVICONFIG dviConfig_PicoDVI
// #define DVICONFIG dviConfig_PicoDVISock
#define DVICONFIG dviConfig_PimoroniDemoDVSock
#endif

#define ERRORMESSAGESIZE 40
#define GAMESAVEDIR "/SAVES"
util::ExclusiveProc exclProc_;
std::unique_ptr<dvi::DVI> dvi_;

char *ErrorMessage;
bool isFatalError = false;
static FATFS fs;
char *romName;

static bool fps_enabled = true;
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
bool frametimeenabled = false;

#ifndef NORENDER
#define NORENDER 0 // 0 is render frames in emulation loop
#endif

namespace
{
    constexpr uint32_t CPUFreqKHz = 252000; // 252000;

    constexpr dvi::Config dviConfig_PicoDVI = {
        .pinTMDS = {10, 12, 14},
        .pinClock = 8,
        .invert = true,
    };
    // Breadboard with Adafruit compononents
    constexpr dvi::Config dviConfig_PicoDVISock = {
        .pinTMDS = {12, 18, 16},
        .pinClock = 14,
        .invert = false,
    };
    // Pimoroni Digital Video, SD Card & Audio Demo Board
    constexpr dvi::Config dviConfig_PimoroniDemoDVSock = {
        .pinTMDS = {8, 10, 12},
        .pinClock = 6,
        .invert = true,
    };
    // Adafruit Feather RP2040 DVI
    constexpr dvi::Config dviConfig_AdafruitFeatherDVI = {
        .pinTMDS = {18, 20, 22},
        .pinClock = 16,
        .invert = true,
    };
    // Waveshare RP2040-PiZero DVI
    constexpr dvi::Config dviConfig_WaveShareRp2040 = {
        .pinTMDS = {26, 24, 22},
        .pinClock = 28,
        .invert = false,
    };

    enum class ScreenMode
    {
        SCANLINE_8_7,
        NOSCANLINE_8_7,
        SCANLINE_1_1,
        NOSCANLINE_1_1,
        MAX,
    };
    ScreenMode screenMode_{};

    bool scaleMode8_7_ = true;

    void applyScreenMode()
    {
        bool scanLine = false;
        printf("Screen mode: %d\n", static_cast<int>(screenMode_));
        switch (screenMode_)
        {
        case ScreenMode::SCANLINE_1_1:
            scaleMode8_7_ = false;
            scanLine = true;
            break;

        case ScreenMode::SCANLINE_8_7:
            scaleMode8_7_ = true;
            scanLine = true;
            break;

        case ScreenMode::NOSCANLINE_1_1:
            scaleMode8_7_ = false;
            scanLine = false;
            break;

        case ScreenMode::NOSCANLINE_8_7:
            scaleMode8_7_ = true;
            scanLine = false;
            break;
        }

        dvi_->setScanLine(scanLine);
    }
    void screenMode(int incr)
    {
        screenMode_ = static_cast<ScreenMode>((static_cast<int>(screenMode_) + incr) & 3);
        applyScreenMode();
    }
}

namespace
{
    dvi::DVI::LineBuffer *currentLineBuffer_{};
}

void __not_in_flash_func(drawWorkMeterUnit)(int timing,
                                            [[maybe_unused]] int span,
                                            uint32_t tag)
{
    if (timing >= 0 && timing < 640)
    {
        auto p = currentLineBuffer_->data();
        p[timing] = tag; // tag = color
    }
}

void __not_in_flash_func(drawWorkMeter)(int line)
{
    if (!currentLineBuffer_)
    {
        return;
    }

    memset(currentLineBuffer_->data(), 0, 64);
    memset(&currentLineBuffer_->data()[320 - 32], 0, 64);
    (*currentLineBuffer_)[160] = 0;
    if (line == 4)
    {
        for (int i = 1; i < 10; ++i)
        {
            (*currentLineBuffer_)[16 * i] = 31;
        }
    }

    constexpr uint32_t clocksPerLine = 800 * 10;
    constexpr uint32_t meterScale = 160 * 65536 / (clocksPerLine * 2);
    util::WorkMeterEnum(meterScale, 1, drawWorkMeterUnit);
}

bool initSDCard()
{
    FRESULT fr;
    TCHAR str[40];
    sleep_ms(1000);

    printf("Mounting SDcard");
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "SD card mount error: %d", fr);
        printf("\n%s\n", ErrorMessage);
        return false;
    }
    printf("\n");

    fr = f_chdir("/");
    if (fr != FR_OK)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot change dir to / : %d", fr);
        printf("\n%s\n", ErrorMessage);
        return false;
    }
    // for f_getcwd to work, set
    //   #define FF_FS_RPATH		2
    // in drivers/fatfs/ffconf.h
    fr = f_getcwd(str, sizeof(str));
    if (fr != FR_OK)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot get current dir: %d", fr);
        printf("\n%s\n", ErrorMessage);
        return false;
    }
    printf("Current directory: %s\n", str);
    printf("Creating directory %s\n", GAMESAVEDIR);
    fr = f_mkdir(GAMESAVEDIR);
    if (fr != FR_OK)
    {
        if (fr == FR_EXIST)
        {
            printf("Directory already exists.\n");
        }
        else
        {
            snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot create dir %s: %d", GAMESAVEDIR, fr);
            printf("%s\n", ErrorMessage);
            return false;
        }
    }
    return true;
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
            int l = *p1 >> 16;;
            int r = *p1 & 0xFFFF;
            *p++ = {static_cast<short>(l), static_cast<short>(r)};
            sample_index++;
        }
        ring.advanceWritePointer(n);
        samples -= n;
    }
}
uint32_t time_us()
{
    absolute_time_t t = get_absolute_time();
    return to_us_since_boot(t);
}

void __not_in_flash_func(core1_main)()
{
    printf("core1 started\n");
    while (true)
    {
        dvi_->registerIRQThisCore();
        dvi_->waitForValidLine();

        dvi_->start();
        while (!exclProc_.isExist())
        {
            if (scaleMode8_7_)
            {
                dvi_->convertScanBuffer12bppScaled16_7(34, 32, 288 * 2);
                // 34 + 252 + 34
                // 32 + 576 + 32
            }
            else
            {
                dvi_->convertScanBuffer12bpp();
            }
        }

        dvi_->unregisterIRQThisCore();
        dvi_->stop();

        exclProc_.processOrWaitIfExist();
    }
}

static DWORD prevButtons[2]{};
static DWORD prevButtonssystem[2]{};
static DWORD prevOtherButtons[2]{};

static int rapidFireMask[2]{};
static int rapidFireCounter = 0;
void processinput(bool fromMenu, DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem, bool ignorepushed)
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
#if NES_PIN_CLK != -1
            nespadbuttons = nespad_state;
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
        // if (gp.buttons & io::GamePadState::Button::SELECT) printf("SELECT\n");
        // if (gp.buttons & io::GamePadState::Button::START) printf("START\n");
        // input.pad[i] = smsbuttons;
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
                // fps_enabled = !fps_enabled;
                // printf("FPS: %s\n", fps_enabled ? "ON" : "OFF");
                printf("FPS: %d- %d\n", fps, dvi_->getFrameCounter());
                frametimeenabled = !frametimeenabled;
            }
            if (pushed & UP)
            {
                screenMode(-1);
            }
            else if (pushed & DOWN)
            {
                screenMode(+1);
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
#if LED_GPIO_PIN != -1
    gpio_put(LED_GPIO_PIN, onOff);
#endif
#if NES_PIN_CLK != -1
    nespad_read_finish(); // Sets global nespad_state var
#endif
    // nespad_read_finish(); // Sets global nespad_state var
    tuh_task();
    // Frame rate calculation
    if (fps_enabled)
    {
        // calculate fps and round to nearest value (instead of truncating/floor)
        uint32_t tick_us = time_us() - start_tick_us;
        fps = (1000000 - 1) / tick_us + 1;
        start_tick_us = time_us();
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
    if (line - 1 != prevline)
    {
        // printf("Line: %d - previous: %d \n", line, prevline);
        // prevline = line;
        auto b = dvi_->getLineBuffer();
        WORD *buffer = b->data();
        WORD *currentLineBuffer = currentLineBuffer_->data();
        // Copy previous line buffer to current line buffer
        // for (int i = 0; i < 512; i++)
        // {
        //     buffer[i] = currentLineBuffer[i];
        // }
        __builtin_memcpy(buffer, currentLineBuffer, 512 * sizeof(currentLineBuffer[0]));
        //__builtin_memset(buffer, 0, 512);
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
    int minframes = 0, maxframes = 0;
    int frametime = 0;
    bool print = false;
    while (reset == false)
    {
        sample_index = 0;
        processinput(false, &pdwPad1, &pdwPad2, &pdwSystem, false);
        ti1 = time_us();
        emu_run_frame();
        ti2 = time_us();
        frametime = (ti2 - ti1) / 1000;
        print = false;
        if (minframes == 0 || frametime < minframes)
        {
            minframes = frametime;
            print = true;
        }
        if (frametime > maxframes)
        {
            maxframes = frametime;
            print = true;
        }
        // if ( print ) {
        //     printf("Min frame time:%d Max frame time  %d ms\n", minframes, maxframes);
        // }
        // printf("Frame time: %d ms\n", frametime);
        // if ( frametime > 10) {
        //     printf("Break\n");
        // }
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
    char errMSG[ERRORMESSAGESIZE];
    errMSG[0] = selectedRom[0] = 0;
    int fileSize = 0;
    bool isGameBoyColor = false;
    FIL fil;
    FIL fil2;
    FRESULT fr;
    FRESULT fr2;
    size_t tmpSize;

    ErrorMessage = errMSG;

    // Set voltage and clock frequency
    vreg_set_voltage(VREG_VOLTAGE_1_20); // VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(CPUFreqKHz, true);

    stdio_init_all();
    sleep_ms(500);
    printf("Starting Game Boy Emulator\n");

#if LED_GPIO_PIN != -1
    gpio_init(LED_GPIO_PIN);
    gpio_set_dir(LED_GPIO_PIN, GPIO_OUT);
    gpio_put(LED_GPIO_PIN, 1);
#endif

    // usb initialise
    printf("USB Initialising\n");
    tusb_init();
    if ((isFatalError = !initSDCard()) == false)
    {
        // Load info about current game and determine file size.
        printf("Reading current game from %s (if exists).\n", ROMINFOFILE);
        fr = f_open(&fil, ROMINFOFILE, FA_READ);
        if (fr == FR_OK)
        {
            size_t r;
            fr = f_read(&fil, selectedRom, sizeof(selectedRom), &r);

            if (fr != FR_OK)
            {
                snprintf(ErrorMessage, 40, "Cannot read %s:%d\n", ROMINFOFILE, fr);
                selectedRom[0] = 0;
                printf(ErrorMessage);
            }
            else
            {
                // determine file size
                selectedRom[r] = 0;
                isGameBoyColor = Frens::cstr_endswith(selectedRom, ".gc");
                printf("Current game: %s\n", selectedRom);
                printf("Console is %s\n", isGameBoyColor ? "Game Boy" : "Game Boy Color");
                printf("Determine filesize of %s\n", selectedRom);
                fr2 = f_open(&fil2, selectedRom, FA_READ);
                if (fr2 == FR_OK)
                {
                    fileSize = (int)f_size(&fil2);
                    printf("File size: %d Bytes (0x%x)\n", fileSize, fileSize);
                }
                else
                {
                    snprintf(ErrorMessage, 40, "Cannot open rom %d", fr2);
                    printf("%s\n", ErrorMessage);
                    selectedRom[0] = 0;
                }
                f_close(&fil2);
            }
        }
        else
        {
            snprintf(ErrorMessage, 40, "Cannot open %s:%d\n", ROMINFOFILE, fr);
            printf(ErrorMessage);
        }
        f_close(&fil);
    }
    // When a game is started from the menu, the menu will reboot the device.
    // After reboot the emulator will start the selected game.
    if (watchdog_caused_reboot() && isFatalError == false && selectedRom[0] != 0)
    {
        // Determine loaded rom
        printf("Rebooted by menu\n");

        printf("Starting (%d) %s\n", strlen(selectedRom), selectedRom);
        printf("Checking for /START file. (Is start pressed in Menu?)\n");
        fr = f_unlink("/START");
        if (fr == FR_NO_FILE)
        {
            printf("Start not pressed, flashing rom.\n ");
            // Allocate buffer for flashing. Borrow emulator memory for this.
            size_t bufsize = 0x2000;
            BYTE *buffer = (BYTE *)malloc(bufsize);

            auto ofs = GB_FILE_ADDR - XIP_BASE;
            printf("write %s rom to flash %x\n", selectedRom, ofs);
            fr = f_open(&fil, selectedRom, FA_READ);

            UINT bytesRead;
            if (fr == FR_OK)
            {
                // filesize already known.
                if ((fileSize / 512) & 1)
                {
                    printf("Skipping header\n");
                    fr = f_lseek(&fil, 512);
                    if (fr != FR_OK)
                    {
                        snprintf(ErrorMessage, 40, "Error skipping header: %d", fr);
                        printf("%s\n", ErrorMessage);
                        selectedRom[0] = 0;
                    }
                }
                if (fr == FR_OK)
                {

                    for (;;)
                    {
                        fr = f_read(&fil, buffer, bufsize, &bytesRead);
                        if (fr == FR_OK)
                        {
                            if (bytesRead == 0)
                            {
                                break;
                            }
                            printf("Flashing %d bytes to flash address %x\n", bytesRead, ofs);
                            printf("  -> Erasing...");

                            // Disable interupts, erase, flash and enable interrupts
                            uint32_t ints = save_and_disable_interrupts();
                            flash_range_erase(ofs, bufsize);
                            printf("\n  -> Flashing...");
                            flash_range_program(ofs, buffer, bufsize);
                            restore_interrupts(ints);
                            //

                            printf("\n");
                            ofs += bufsize;
                        }
                        else
                        {
                            snprintf(ErrorMessage, 40, "Error reading rom: %d", fr);
                            printf("Error reading rom: %d\n", fr);
                            selectedRom[0] = 0;
                            break;
                        }
                    }
                }
                f_close(&fil);
            }
            else
            {
                snprintf(ErrorMessage, 40, "Cannot open rom %d", fr);
                printf("%s\n", ErrorMessage);
                selectedRom[0] = 0;
            }
            free(buffer);
        }
        else
        {
            if (fr != FR_OK)
            {
                snprintf(ErrorMessage, 40, "Cannot delete /START file %d", fr);
                printf("%s\n", ErrorMessage);
                selectedRom[0] = 0;
            }
            else
            {
                printf("Start pressed in menu, not flashing rom.\n");
            }
        }
    }
    else
    {
        selectedRom[0] = 0;
    }
    //
    printf("Initialising DVI\n");
    dvi_ = std::make_unique<dvi::DVI>(pio0, &DVICONFIG,
                                      dvi::getTiming640x480p60Hz());
    //    dvi_->setAudioFreq(48000, 25200, 6144);
    dvi_->setAudioFreq(44100, 28000, 6272);
    // Gameboy emulator needs a larger audio buffer to avoid audio stutter, so we allocate a larger buffer. (was 256)
    dvi_->allocateAudioBuffer(512);
    //    dvi_->setExclusiveProc(&exclProc_);

    // Adjust the top and bottom to center the emulator screen
    dvi_->getBlankSettings().top = MARGINTOP * 2;
    dvi_->getBlankSettings().bottom = MARGINBOTTOM * 2;
    // dvi_->setScanLine(true);

    applyScreenMode();
#if NES_PIN_CLK != -1
    nespad_begin(CPUFreqKHz, NES_PIN_CLK, NES_PIN_DATA, NES_PIN_LAT);
#endif
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
    wiipad_begin();
#endif
    // 空サンプル詰めとく
    dvi_->getAudioRingBuffer().advanceWritePointer(255);

    multicore_launch_core1(core1_main);
    // smsp_gamedata_set(argv[1]);
    // Check the type of ROM
    // sms.console = strcmp(strrchr(argv[1], '.'), ".gg") ? CONSOLE_SMS : CONSOLE_GG;
    // sms.console = CONSOLE_SMS; // For now, we only support SMS
    //

    while (true)
    {
        if (strlen(selectedRom) == 0 || reset == true)
        {
            // reset margin to give menu more screen space
            dvi_->getBlankSettings().top = 4 * 2;
            dvi_->getBlankSettings().bottom = 4 * 2;
            screenMode_ = ScreenMode::NOSCANLINE_8_7;
            applyScreenMode();
            menu(GB_FILE_ADDR, ErrorMessage, isFatalError, reset);
        }
        reset = false;
        printf("Now playing: %s\n", selectedRom);

        printf("Initialising Game Boy\n");
        uint8_t *rom = reinterpret_cast<unsigned char *>(GB_FILE_ADDR);
        if (startemulation(rom, ErrorMessage))
        {
            process();
        }
        selectedRom[0] = 0;
    }
    return 0;
}
