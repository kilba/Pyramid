#ifndef WND_H
#define WND_H

#include <windows.h>

typedef struct {
    HINSTANCE hinstance;
    HWND hwnd;
    HDC dc;
} WindowData;

WindowData bs_initWnd(int width, int height, char *title);
void bs_wndTick(void (*render)());

#endif /* WND_H */
