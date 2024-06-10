#ifndef MENU_H
#define MENU_H
#define ROMSELECT
#define ROMINFOFILE "/currentloadedrom.txt"
#define SWVERSION "VX.X"
void processinput(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem, bool ignorepushed );
void menu(uintptr_t NES_FILE_ADDR, char *errorMessage, bool isFatalError, bool reset);
char getcharslicefrom8x8font(char c, int rowInChar);
int ProcessAfterFrameIsRendered();
#endif