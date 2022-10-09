#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <wnd.h>

// Zero-initialised array of every key's state
bool keys[256] = { 0 };

WindowData wd;

int w, h;
double elapsed;
double delta_time;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

WindowData bs_initWnd(int width, int height, char *title) {
    w = width;
    h = height;

    const char g_szClassName[] = "myWindowClass";
    WNDCLASSEX wc;

    HINSTANCE hInstance = GetModuleHandle(0);

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "myWindowClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed11!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);

        printf("%ld\n", GetLastError());
        return wd;
    }

    wd.hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL);

    if(wd.hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);

        printf("%ld\n", GetLastError());
        return wd;
    }

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    const int pixel_format = 11;

    wd.dc = GetDC(wd.hwnd);
    wd.hinstance = hInstance;

    // int pf = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(wd.dc, pixel_format, &pfd);

    ShowWindow(wd.hwnd, SW_SHOW);
    UpdateWindow(wd.hwnd);

    // TODO: Get screen refresh rate
    SetTimer(wd.hwnd, 999, 1000 / 120, NULL);

    return wd;
}

/* --- INPUTS --- */
bool bs_isKeyDown(int key_code) {
    SHORT tabKeyState = GetAsyncKeyState(key_code);
    keys[key_code] = tabKeyState & 0x8000;
    return keys[key_code];
}

bool bs_isKeyOnce(int key_code) {
    SHORT tabKeyState = GetAsyncKeyState(key_code);

    bool prior = keys[key_code];
    keys[key_code] = tabKeyState & 0x8000;

    return keys[key_code] != prior;
}

bool bs_isKeyDownOnce(int key_code) {
    bool key = bs_isKeyOnce(key_code);
    return key & keys[key_code];
}

bool bs_isKeyUpOnce(int key_code) {
    bool key = bs_isKeyOnce(key_code);
    return key & !keys[key_code];
}

/* --- WINDOW FUNCTIONS --- */
void bs_tickSpeed(int fps) {
    int fpms = 1000 / fps;
    SetTimer(wd.hwnd, 999, fpms, NULL);
}

void bs_wndTick(void (*render)()) {
    MSG msg;

    double start, prev;
    start = (double)GetTickCount64() / 1000.0;

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        switch(msg.message) {
            case WM_QUIT:
                PostQuitMessage(0); 
                return;
            case WM_TIMER:
                break;
            /* Skip scene update on non-timed events */
            default:
                goto pass;
        }

        prev = elapsed;

        elapsed = (double)GetTickCount64() / 1000.0;
        elapsed -= start;

        delta_time = elapsed - prev;

        render();
        SwapBuffers(wd.dc);

        pass:
    }
}

/* --- GET VARIABLES --- */
double bs_elapsedTime() {
    return elapsed;
}

double bs_deltaTime() {
    return delta_time;
}
