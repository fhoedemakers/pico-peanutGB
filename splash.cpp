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

    strcpy(s, "Game Boy and Game Boy Color emulator");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 4, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "for RP2350");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 5, s, fgcolorSplash, bgcolorSplash);

    strcpy(s, "Based on");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 7, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 8, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "deltabeard/Peanut-GB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 9, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "Pico Port");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 11, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "@frenskefrens");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 12, s, CLIGHTBLUE, bgcolorSplash);
#if !HSTX
    strcpy(s, "DVI Support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 14, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "@shuichi_takano");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 15, s, CLIGHTBLUE, bgcolorSplash);
#else
    strcpy(s, "HSTX video driver _ I2S audio___");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 14, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "__@fliperama86____@frenskefrens__");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 15, s, CLIGHTBLUE, bgcolorSplash);
#endif
    strcpy(s, "(S)NES/WII controller support");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 17, s, fgcolorSplash, bgcolorSplash);

    strcpy(s, "@PaintYourDragon @adafruit");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 18, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "PCB Design");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 20, s, fgcolorSplash, bgcolorSplash);

    strcpy(s, "@johnedgarpark Dynamight");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 21, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "3D Case design,artwork & metadata");
    putText(2, 23, s, fgcolorSplash, bgcolorSplash);
    strcpy(s, "DynaMight");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 24, s, CLIGHTBLUE, bgcolorSplash);

    strcpy(s, "https://github.com/");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 27, s, CLIGHTBLUE, bgcolorSplash);
    strcpy(s, "fhoedemakers/Pico-PeanutGB");
    putText(SCREEN_COLS / 2 - strlen(s) / 2, 28, s, CLIGHTBLUE, bgcolorSplash);
}