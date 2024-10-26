#include <stdio.h>
#include <string.h>

#include "FrensHelpers.h"
#include <pico.h>
#include <memory>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include <dvi/dvi.h>
#include <util/exclusive_proc.h>
#include <gamepad.h>
#include "hardware/watchdog.h"
#include "RomLister.h"
#include "menu.h"
#include "nespad.h"
#include "wiipad.h"

#include "font_8x8.h"
#include "mytypes.h"

#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8
#define FONT_N_CHARS 95
#define FONT_FIRST_ASCII 32
#define SCREEN_COLS 32
#define SCREEN_ROWS 29

#define STARTROW 3
#define ENDROW 24
#define PAGESIZE (ENDROW - STARTROW + 1)

#define VISIBLEPATHSIZE (SCREEN_COLS - 3)

extern util::ExclusiveProc exclProc_;
extern std::unique_ptr<dvi::DVI> dvi_;
void screenMode(int incr);

#define CBLACK 0
#define CWHITE 0x3f
#define CRED 3
#define CGREEN 0x40
#define CBLUE 0x54
#define CLIGHTBLUE 48
#define DEFAULT_FGCOLOR CBLACK // 60
#define DEFAULT_BGCOLOR CWHITE

static int fgcolor = DEFAULT_FGCOLOR;
static int bgcolor = DEFAULT_BGCOLOR;

struct charCell
{
    uint8_t fgcolor;
    uint8_t bgcolor;
    char charvalue;
};

#define SCREENBUFCELLS SCREEN_ROWS *SCREEN_COLS
static charCell *screenBuffer;
;

static WORD *WorkLineRom = nullptr;

// The Sega Master system color palette converted to RGB444
// so it can be used with the DVI library.
// from https://segaretro.org/Palette
const WORD SMSPaletteRGB444[64] = {
    0x0, 0x500, 0xA00, 0xF00, 0x50, 0x550, 0xA50, 0xF50,
    0xA0, 0x5A0, 0xAA0, 0xFA0, 0xF0, 0x5F0, 0xAF0, 0xFF0,
    0x5, 0x505, 0xA05, 0xF05, 0x55, 0x555, 0xA55, 0xF55,
    0xA5, 0x5A5, 0xAA5, 0xFA5, 0xF5, 0x5F5, 0xAF5, 0xFF5,
    0xA, 0x50A, 0xA0A, 0xF0A, 0x5A, 0x55A, 0xA5A, 0xF5A,
    0xAA, 0x5AA, 0xAAA, 0xFAA, 0xFA, 0x5FA, 0xAFA, 0xFFA,
    0xF, 0x50F, 0xA0F, 0xF0F, 0x5F, 0x55F, 0xA5F, 0xF5F,
    0xAF, 0x5AF, 0xAAF, 0xFAF, 0xFF, 0x5FF, 0xAFF, 0xFFF};

// static constexpr int X = 1 << 8;
// static constexpr int Y = 1 << 9;

char getcharslicefrom8x8font(char c, int rowInChar)
{
    sizeof(screenBuffer);
    return font_8x8[(c - FONT_FIRST_ASCII) + (rowInChar)*FONT_N_CHARS];
}

void RomSelect_DrawLine(int line, int selectedRow)
{
    WORD fgcolor, bgcolor;
    memset(WorkLineRom, 0, 640);

    for (auto i = 0; i < SCREEN_COLS; ++i)
    {
        int charIndex = i + line / FONT_CHAR_HEIGHT * SCREEN_COLS;
        int row = charIndex / SCREEN_COLS;
        uint c = screenBuffer[charIndex].charvalue;
        if (row == selectedRow)
        {
            fgcolor = SMSPaletteRGB444[screenBuffer[charIndex].bgcolor];
            bgcolor = SMSPaletteRGB444[screenBuffer[charIndex].fgcolor];
        }
        else
        {
            fgcolor = SMSPaletteRGB444[screenBuffer[charIndex].fgcolor];
            bgcolor = SMSPaletteRGB444[screenBuffer[charIndex].bgcolor];
        }

        int rowInChar = line % FONT_CHAR_HEIGHT;
        char fontSlice = getcharslicefrom8x8font(c, rowInChar); // font_8x8[(c - FONT_FIRST_ASCII) + (rowInChar)*FONT_N_CHARS];
        for (auto bit = 0; bit < 8; bit++)
        {
            if (fontSlice & 1)
            {
                *WorkLineRom = fgcolor;
            }
            else
            {
                *WorkLineRom = bgcolor;
            }
            fontSlice >>= 1;
            WorkLineRom++;
        }
    }
    return;
}

void drawline(int scanline, int selectedRow)
{
    // RomSelect_PreDrawLine(scanline);
    // RomSelect_DrawLine(scanline - 4, selectedRow);
    // InfoNES_PostDrawLine(scanline);

    auto b = dvi_->getLineBuffer();
    WorkLineRom = b->data() + 32;
    RomSelect_DrawLine(scanline - 4, selectedRow);
    dvi_->setLineBuffer(scanline, b);
}

static void putText(int x, int y, const char *text, int fgcolor, int bgcolor)
{

    if (text != nullptr)
    {
        auto index = y * SCREEN_COLS + x;
        auto maxLen = strlen(text);
        if (strlen(text) > SCREEN_COLS - x)
        {
            maxLen = SCREEN_COLS - x;
        }
        while (index < SCREENBUFCELLS && *text && maxLen > 0)
        {
            screenBuffer[index].charvalue = *text++;
            screenBuffer[index].fgcolor = fgcolor;
            screenBuffer[index].bgcolor = bgcolor;
            index++;
            maxLen--;
        }
    }
}

void DrawScreen(int selectedRow)
{
    for (auto line = 4; line < 236; line++)
    {
        drawline(line, selectedRow);
    }
}

void ClearScreen(charCell *screenBuffer, int color)
{
    for (auto i = 0; i < SCREENBUFCELLS; i++)
    {
        screenBuffer[i].bgcolor = color;
        screenBuffer[i].fgcolor = color;
        screenBuffer[i].charvalue = ' ';
    }
}

void displayRoms(Frens::RomLister romlister, int startIndex)
{
    char buffer[ROMLISTER_MAXPATH + 4];
    char s[SCREEN_COLS + 1];
    auto y = STARTROW;
    auto entries = romlister.GetEntries();
    ClearScreen(screenBuffer, bgcolor);
    strcpy(s, "- Pico-PeanutGB -");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 0, s, fgcolor, bgcolor);  
    strcpy(s, "Choose a rom to play:");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 1, s, fgcolor, bgcolor);
    for (int i = 1; i < SCREEN_COLS - 1; i++)
    {
        putText(i, STARTROW - 1, "-", fgcolor, bgcolor);
    }
    for (int i = 1; i < SCREEN_COLS - 1; i++)
    {
        putText(i, ENDROW + 1, "-", fgcolor, bgcolor);
    }
    strcpy(s, "A Select, B Back");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, ENDROW + 2, s, fgcolor, bgcolor);
    putText(SCREEN_COLS - strlen(SWVERSION), SCREEN_ROWS - 1,SWVERSION, fgcolor, bgcolor);
    for (auto index = startIndex; index < romlister.Count(); index++)
    {
        if (y <= ENDROW)
        {
            auto info = entries[index];
            if (info.IsDirectory)
            {
                snprintf(buffer, SCREEN_COLS - 1, "D %s", info.Path);
            }
            else
            {
                snprintf(buffer, SCREEN_COLS - 1, "R %s", info.Path);
            }

            putText(1, y, buffer, fgcolor, bgcolor);
            y++;
        }
    }
}

void DisplayFatalError(char *error)
{
    ClearScreen(screenBuffer, bgcolor);
    putText(0, 0, "Fatal error:", fgcolor, bgcolor);
    putText(1, 3, error, fgcolor, bgcolor);
    while (true)
    {
        auto frameCount = ProcessAfterFrameIsRendered(true);
        DrawScreen(-1);
    }
}

void DisplayEmulatorErrorMessage(char *error)
{
    DWORD PAD1_Latch, PAD1_Latch2, pdwSystem;
    ClearScreen(screenBuffer, bgcolor);
    putText(0, 0, "Error occured:", fgcolor, bgcolor);
    putText(0, 3, error, fgcolor, bgcolor);
    putText(0, ENDROW, "Press a button to continue.", fgcolor, bgcolor);
    while (true)
    {
        auto frameCount = ProcessAfterFrameIsRendered(true);
        DrawScreen(-1);
        processinput(true, &PAD1_Latch, &PAD1_Latch2, &pdwSystem, false);
        if (PAD1_Latch > 0)
        {
            return;
        }
    }
}

void showSplashScreen()
{
    DWORD PAD1_Latch, PAD1_Latch2, pdwSystem;
    char s[SCREEN_COLS];
    ClearScreen(screenBuffer, bgcolor);

    strcpy(s, "PicoPeanutGB");
    putText(SCREEN_COLS / 2 - (strlen(s)) / 2, 2, s, fgcolor, bgcolor);

    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 3, 2, "S", CRED, bgcolor);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 4, 2, "M", CGREEN, bgcolor);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 5, 2, "S", CBLUE, bgcolor);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 6, 2, "+", fgcolor, bgcolor);

    strcpy(s, "Game Boy emulator for RP2350");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 4, s, fgcolor, bgcolor);
   
    strcpy(s, "Based on");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 6, s, fgcolor, bgcolor);
    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 7, s, CLIGHTBLUE, bgcolor);
    strcpy(s, "deltabeard/Peanut-GB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 8, s, CLIGHTBLUE, bgcolor);
    strcpy(s, "Pico Port");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 10, s, fgcolor, bgcolor);
    strcpy(s, "@frenskefrens");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 11, s, CLIGHTBLUE, bgcolor);

    strcpy(s, "DVI Support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 13, s, fgcolor, bgcolor);
    strcpy(s, "@shuichi_takano");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 14, s, CLIGHTBLUE, bgcolor);

    strcpy(s, "(S)NES/WII controller support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 16, s, fgcolor, bgcolor);

    strcpy(s, "@PaintYourDragon @adafruit");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 17, s, CLIGHTBLUE, bgcolor);

    strcpy(s, "PCB Design");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 19, s, fgcolor, bgcolor);

    strcpy(s, "@johnedgarpark");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 20, s, CLIGHTBLUE, bgcolor);

    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 22, s, CLIGHTBLUE, bgcolor);
    strcpy(s, "fhoedemakers/Pico-PeanutGB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 23, s, CLIGHTBLUE, bgcolor);
    int startFrame = -1;
    while (true)
    {
        auto frameCount = ProcessAfterFrameIsRendered(true);
        if (startFrame == -1)
        {
            startFrame = frameCount;
        }
        DrawScreen(-1);
        processinput(true, &PAD1_Latch, &PAD1_Latch2, &pdwSystem, false);
        if (PAD1_Latch > 0 || (frameCount - startFrame) > 1000)
        {
             return;
        }
        if ((frameCount % 30) == 0)
        {
            for (auto i = 0; i < SCREEN_COLS; i++)
            {
                auto col = rand() % 63;
                putText(i, 0, " ", col, col);
                col = rand() % 63;
                putText(i, SCREEN_ROWS - 1, " ", col, col);
            }
            for (auto i = 1; i < SCREEN_ROWS - 1; i++)
            {
                auto col = rand() % 63;
                putText(0, i, " ", col, col);
                col = rand() % 63;
                putText(SCREEN_COLS - 1, i, " ", col, col);
            }
        }
    }
}

void screenSaver()
{
    DWORD PAD1_Latch, PAD1_Latch2, pdwSystem;
    WORD frameCount;
    while (true)
    {
        frameCount = ProcessAfterFrameIsRendered(true);
        DrawScreen(-1);
        processinput(true, &PAD1_Latch, &PAD1_Latch2, &pdwSystem, false);

        if (PAD1_Latch > 0)
        {
            return;
        }
        if ((frameCount % 3) == 0)
        {
            auto color = rand() % 63;
            auto row = rand() % SCREEN_ROWS;
            auto column = rand() % SCREEN_COLS;
            putText(column, row, " ", color, color);
        }
    }
}
void clearinput()
{
    DWORD PAD1_Latch, PAD1_Latch2, pdwSystem;
    while (1)
    {
        ProcessAfterFrameIsRendered(true);
        DrawScreen(-1);
        processinput(true, &PAD1_Latch, &PAD1_Latch2, &pdwSystem, true);
        if (PAD1_Latch == 0)
        {
            break;
        }
    }
}
// Global instances of local vars in romselect() some used in Lambda expression later on
static char *selectedRomOrFolder;
static uintptr_t FLASH_ADDRESS;
static bool errorInSavingRom = false;
static char *globalErrorMessage;


//
BYTE *dirbuffer;

void menu(uintptr_t NES_FILE_ADDR, char *errorMessage, bool isFatal, bool reset)
{
    FLASH_ADDRESS = NES_FILE_ADDR;
    int firstVisibleRowINDEX = 0;
    int selectedRow = STARTROW;
    char currentDir[FF_MAX_LFN];
    int totalFrames = -1;

    globalErrorMessage = errorMessage;
    FRESULT fr;
    DWORD PAD1_Latch, PAD1_Latch2, pdwSystem;

    int horzontalScrollIndex = 0;
    printf("Starting Menu\n");
    size_t menusize = SCREENBUFCELLS * sizeof(charCell);
    screenBuffer = (charCell *)malloc(menusize);
    printf("Allocated %d bytes for screenBuffer\n", menusize);
    size_t ramsize;
    // Borrow Emulator RAM buffer for screen.
    // screenBuffer = (charCell *)InfoNes_GetRAM(&ramsize);
    /// size_t chr_size;
    // Borrow ChrBuffer to store directory contents
    // void *buffer = InfoNes_GetChrBuf(&chr_size);
    size_t bufsize = 0x2000 *2;
    dirbuffer =(BYTE *) (BYTE *)malloc(bufsize);
    printf("Allocated %d bytes for dirbuffer\n", bufsize); 
    
    Frens::RomLister romlister(dirbuffer, bufsize);
    clearinput();
    if (strlen(errorMessage) > 0)
    {
        if (isFatal) // SD card not working, show error
        {
            DisplayFatalError(errorMessage);
        }
        else
        {
            DisplayEmulatorErrorMessage(errorMessage); // Emulator cannot start, show error
        }
    }
    else
    {
        // Show splash screen, only when not reset from emulation
        if ( reset == false )
        {
            showSplashScreen();
        } else sleep_ms(300);
    }
    romlister.list("/");
    displayRoms(romlister, firstVisibleRowINDEX);
    while (1)
    {

        auto frameCount = ProcessAfterFrameIsRendered(true);
        auto index = selectedRow - STARTROW + firstVisibleRowINDEX;
        auto entries = romlister.GetEntries();
        selectedRomOrFolder = (romlister.Count() > 0) ? entries[index].Path : nullptr;
        errorInSavingRom = false;
        DrawScreen(selectedRow);
        processinput(true,&PAD1_Latch, &PAD1_Latch2, &pdwSystem, false);

        if (PAD1_Latch > 0 || pdwSystem > 0)
        {
            totalFrames = frameCount; // Reset screenSaver
            // reset horizontal scroll of highlighted row
            horzontalScrollIndex = 0;
            putText(3, selectedRow, selectedRomOrFolder, fgcolor, bgcolor);
            putText(SCREEN_COLS - 1, selectedRow, " ", bgcolor, bgcolor);

            // if ((PAD1_Latch & Y) == Y)
            // {
            //     fgcolor++;
            //     if (fgcolor > 63)
            //     {
            //         fgcolor = 0;
            //     }
            //     printf("fgColor++ : %02d (%04x)\n", fgcolor, NesPalette[fgcolor]);
            //     displayRoms(romlister, firstVisibleRowINDEX);
            // }
            // else if ((PAD1_Latch & X) == X)
            // {
            //     bgcolor++;
            //     if (bgcolor > 63)
            //     {
            //         bgcolor = 0;
            //     }
            //     printf("bgColor++ : %02d (%04x)\n", bgcolor, NesPalette[bgcolor]);
            //     displayRoms(romlister, firstVisibleRowINDEX);
            // }
            // else
            if ((PAD1_Latch & UP) == UP && selectedRomOrFolder)
            {
                if (selectedRow > STARTROW)
                {
                    selectedRow--;
                }
                else
                {
                    if (firstVisibleRowINDEX > 0)
                    {
                        firstVisibleRowINDEX--;
                        displayRoms(romlister, firstVisibleRowINDEX);
                    }
                }
            }
            else if ((PAD1_Latch & DOWN) == DOWN && selectedRomOrFolder)
            {
                if (selectedRow < ENDROW && (index) < romlister.Count() - 1)
                {
                    selectedRow++;
                }
                else
                {
                    if (index < romlister.Count() - 1)
                    {
                        firstVisibleRowINDEX++;
                        displayRoms(romlister, firstVisibleRowINDEX);
                    }
                }
            }
            else if ((PAD1_Latch & LEFT) == LEFT && selectedRomOrFolder)
            {
                firstVisibleRowINDEX -= PAGESIZE;
                if (firstVisibleRowINDEX < 0)
                {
                    firstVisibleRowINDEX = 0;
                }
                selectedRow = STARTROW;
                displayRoms(romlister, firstVisibleRowINDEX);
            }
            else if ((PAD1_Latch & RIGHT) == RIGHT && selectedRomOrFolder)
            {
                if (firstVisibleRowINDEX + PAGESIZE < romlister.Count())
                {
                    firstVisibleRowINDEX += PAGESIZE;
                }
                selectedRow = STARTROW;
                displayRoms(romlister, firstVisibleRowINDEX);
            }
            else if ((PAD1_Latch & B) == B)
            {
                fr = f_getcwd(currentDir, 40);
                if (fr != FR_OK)
                {
                    printf("Cannot get current dir: %d\n", fr);
                }
                if (strcmp(currentDir, "/") != 0)
                {
                    romlister.list("..");
                    firstVisibleRowINDEX = 0;
                    selectedRow = STARTROW;
                    displayRoms(romlister, firstVisibleRowINDEX);
                }
            }
            else if ((PAD1_Latch & START) == START && (PAD1_Latch & SELECT) != SELECT)
            {
                // reboot and start emulator with currently loaded game
                // Create a file /START indicating not to reflash the already flashed game
                // The emulator will delete this file after loading the game
                FRESULT fr;
                FIL fil;
                printf("Creating /START\n");
                fr = f_open(&fil, "/START", FA_CREATE_ALWAYS | FA_WRITE);
                if (fr == FR_OK)
                {
                    auto bytes = f_puts("START", &fil);
                    printf("Wrote %d bytes\n", bytes);
                    fr = f_close(&fil);
                    if (fr != FR_OK)
                    {
                        printf("Cannot close file /START:%d\n", fr);
                    }
                }
                else
                {
                    printf("Cannot create file /START:%d\n", fr);
                }
                break; // reboot
            }
            else if ((PAD1_Latch & A) == A && selectedRomOrFolder)
            {
                if (entries[index].IsDirectory)
                {
                    romlister.list(selectedRomOrFolder);
                    firstVisibleRowINDEX = 0;
                    selectedRow = STARTROW;
                    displayRoms(romlister, firstVisibleRowINDEX);
                }
                else
                {
                    FRESULT fr;
                    FIL fil;
                    char curdir[256];

                    fr = f_getcwd(curdir, sizeof(curdir));
                    printf("Current dir: %s\n", curdir);
                    // Create file containing full path name currently loaded rom
                    // The contents of this file will be used by the emulator to flash and start the correct rom in main.cpp
                    printf("Creating %s\n", ROMINFOFILE);
                    fr = f_open(&fil, ROMINFOFILE, FA_CREATE_ALWAYS | FA_WRITE);
                    if (fr == FR_OK)
                    {
                        for (auto i = 0; i < strlen(curdir); i++)
                        {

                            int x = f_putc(curdir[i], &fil);
                            printf("%c", curdir[i]);
                            if (x < 0)
                            {
                                snprintf(globalErrorMessage, 40, "Error writing file %d", fr);
                                printf("%s\n", globalErrorMessage);
                                errorInSavingRom = true;
                                break;
                            }
                        }
                        f_putc('/', &fil);
                        printf("%c", '/');
                        for (auto i = 0; i < strlen(selectedRomOrFolder); i++)
                        {

                            int x = f_putc(selectedRomOrFolder[i], &fil);
                            printf("%c", selectedRomOrFolder[i]);
                            if (x < 0)
                            {
                                snprintf(globalErrorMessage, 40, "Error writing file %d", fr);
                                printf("%s\n", globalErrorMessage);
                                errorInSavingRom = true;
                                break;
                            }
                        }
                        printf("\n");
                    }
                    else
                    {
                        printf("Cannot create %s:%d\n", ROMINFOFILE, fr);
                        snprintf(globalErrorMessage, 40, "Cannot create %s:%d", ROMINFOFILE, fr);
                        errorInSavingRom = true;
                    }
                    f_close(&fil);
                    // break out of loop and reboot
                    // rom will be flashed and started by main.cpp
                    // Cannot flash here because of lockups (when using wii controller) and sound issues
                    break;
                }
            }
        }
        // scroll selected row horizontally if textsize exceeds rowlength
        if (selectedRomOrFolder)
        {
            if ((frameCount % 30) == 0)
            {
                if (strlen(selectedRomOrFolder + horzontalScrollIndex) >= VISIBLEPATHSIZE)
                {
                    horzontalScrollIndex++;
                }
                else
                {
                    horzontalScrollIndex = 0;
                }
                putText(3, selectedRow, selectedRomOrFolder + horzontalScrollIndex, fgcolor, bgcolor);
                putText(SCREEN_COLS - 1, selectedRow, " ", bgcolor, bgcolor);
            }
        }
        if (totalFrames == -1)
        {
            totalFrames = frameCount;
        }
        if ((frameCount - totalFrames) > 800)
        {
            printf("Starting screensaver\n");
            totalFrames = -1;
            screenSaver();
            displayRoms(romlister, firstVisibleRowINDEX);
        }
    } // while 1
      // Wait until user has released all buttons
    clearinput();
    
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
    wiipad_end();
#endif

    // Don't return from this function call, but reboot in order to get avoid several problems with sound and lockups (WII-pad)
    // After reboot the emulator will and flash start the selected game.
    printf("Rebooting...\n");
    watchdog_enable(100, 1);
    while (1)
        ;
    // Never return
}
