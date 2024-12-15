#include "menu.h"
#include "FrensHelpers.h"
#include <cstring>

// called by menu.cpp
// shows emulator specific splash screen
static int fgcolorSplash = DEFAULT_FGCOLOR;
static int bgcolorSplash =DEFAULT_BGCOLOR;
void splash()
{
    char s[SCREEN_COLS + 1];
    ClearScreen(bgcolorSplash);

    strcpy(s, "PicoPeanutGB");
    putText(SCREEN_COLS / 2 - (strlen(s)) / 2, 2, s, fgcolorSplash, bgcolorSplash);

    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 3, 2, "S", CRED, bgcolorSplash);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 4, 2, "M", CGREEN, bgcolorSplash);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 5, 2, "S", CBLUE, bgcolorSplash);
    // putText((SCREEN_COLS / 2 - (strlen(s)) / 2) + 6, 2, "+", fgcolorSplash, bgcolorSplash);

    strcpy(s, "Game Boy emulator for RP2350");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 4, s, fgcolorSplash, bgcolorSplash);
   
    strcpy(s, "Based on");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 6, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 7, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "deltabeard/Peanut-GB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 8, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "Pico Port");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 10, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "@frenskefrens");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 11, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "DVI Support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 13, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "@shuichi_takano");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 14, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "(S)NES/WII controller support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 16, s, fgcolorSplash, bgcolorSplash);

    strcpy(s, "@PaintYourDragon @adafruit");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 17, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "PCB Design");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 19, s, fgcolorSplash, bgcolorSplash);

    strcpy(s, "@johnedgarpark");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 20, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 22, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "fhoedemakers/Pico-PeanutGB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 23, s, CLIGHTBLUE, bgcolorSplash);
}