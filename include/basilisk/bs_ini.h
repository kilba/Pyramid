#ifndef BS_WND_H
#define BS_WND_H

#include <bs_types.h>
#include <windows.h>
#define BS_NUM_KEYS 348

#define BS_VK_ERR(call, msg) \
    do { \
        if ((call) != VK_SUCCESS) { \
            bs_throwVk(msg, call); \
        } \
    } while(0)

typedef struct {
    bs_U32 command_buffers;
} bs_HandleOffsets;

typedef struct {
	bs_vec2 resolution;

	double elapsed;
	double delta_time;

    bs_U32 swapchain_frame;
    bool key_states[BS_NUM_KEYS + 1];
} bs_WindowFrame;

bs_HandleOffsets* bs_handleOffsets();
bs_WindowFrame* bs_frameData();
void* bs_vkDevice();
void* bs_vkPhysicalDevice();
void* bs_vkRenderPass();
void* bs_vkCmdPool();
void* bs_vkGraphicsQueue();
bs_U32 bs_handleOffset();
void bs_addVkHandle(void* handle);
void* bs_swapchainImgViews();
bs_U32 bs_numSwapchainImgs();
void* bs_vkHandle(bs_U32 index);
bs_U32 bs_swapchainImage();

bs_ivec2 bs_swapchainExtents();
void bs_throw(const char* message);
void bs_throwVk(const char* message, bs_U32 result);
void bs_ini(bs_U32 width, bs_U32 height, const char* name);
void bs_run(void (*tick)());
void bs_exit();
double bs_deltaTime();
double bs_elapsedTime();
bs_vec2 bs_resolution();
bool bs_keyDown(bs_U32 code);
bool bs_keyDownOnce(bs_U32 code);
bool bs_keyUpOnce(bs_U32 code);
void bs_wndTitle(const char* title);
void bs_wndTitlef(const char* format, ...);
HWND bs_hwnd();
bs_PerformanceData bs_queryPerformance();
double bs_queryPerformanceResult(bs_PerformanceData data);

#define 	BS_KEY_UNKNOWN   -1
#define 	BS_KEY_SPACE   32
#define 	BS_KEY_APOSTROPHE 39 // '
#define 	BS_KEY_COMMA   44   // ,
#define 	BS_KEY_MINUS   45   // -
#define 	BS_KEY_PERIOD  46   // .
#define 	BS_KEY_SLASH   47   // /
#define 	BS_KEY_0   48
#define 	BS_KEY_1   49
#define 	BS_KEY_2   50
#define 	BS_KEY_3   51
#define 	BS_KEY_4   52
#define 	BS_KEY_5   53
#define 	BS_KEY_6   54
#define 	BS_KEY_7   55
#define 	BS_KEY_8   56
#define 	BS_KEY_9   57
#define 	BS_KEY_SEMICOLON 59 // ;
#define 	BS_KEY_EQUAL   61   // =
#define 	BS_KEY_A   65
#define 	BS_KEY_B   66
#define 	BS_KEY_C   67
#define 	BS_KEY_D   68
#define 	BS_KEY_E   69
#define 	BS_KEY_F   70
#define 	BS_KEY_G   71
#define 	BS_KEY_H   72
#define 	BS_KEY_I   73
#define 	BS_KEY_J   74
#define 	BS_KEY_K   75
#define 	BS_KEY_L   76
#define 	BS_KEY_M   77
#define 	BS_KEY_N   78
#define 	BS_KEY_O   79
#define 	BS_KEY_P   80
#define 	BS_KEY_Q   81
#define 	BS_KEY_R   82
#define 	BS_KEY_S   83
#define 	BS_KEY_T   84
#define 	BS_KEY_U   85
#define 	BS_KEY_V   86
#define 	BS_KEY_W   87
#define 	BS_KEY_X   88
#define 	BS_KEY_Y   89
#define 	BS_KEY_Z   90
#define 	BS_KEY_LEFT_BRACKET 91  // [
#define 	BS_KEY_BACKSLASH    92  // '\'
#define 	BS_KEY_RIGHT_BRACKET 93 // ]
#define 	BS_KEY_GRAVE_ACCENT  96 // `
#define 	BS_KEY_WORLD_1   161    // non-US #1
#define 	BS_KEY_WORLD_2   162    // non-US #2
#define 	BS_KEY_ESCAPE   256
#define 	BS_KEY_ENTER   257
#define 	BS_KEY_TAB   258
#define 	BS_KEY_BACKSPACE   259
#define 	BS_KEY_INSERT   260
#define 	BS_KEY_DELETE   261
#define 	BS_KEY_RIGHT   262
#define 	BS_KEY_LEFT   263
#define 	BS_KEY_DOWN   264
#define 	BS_KEY_UP   265
#define 	BS_KEY_PAGE_UP   266
#define 	BS_KEY_PAGE_DOWN   267
#define 	BS_KEY_HOME   268
#define 	BS_KEY_END   269
#define 	BS_KEY_CAPS_LOCK   280
#define 	BS_KEY_SCROLL_LOCK   281
#define 	BS_KEY_NUM_LOCK   282
#define 	BS_KEY_PRINT_SCREEN   283
#define 	BS_KEY_PAUSE   284
#define 	BS_KEY_F1   290
#define 	BS_KEY_F2   291
#define 	BS_KEY_F3   292
#define 	BS_KEY_F4   293
#define 	BS_KEY_F5   294
#define 	BS_KEY_F6   295
#define 	BS_KEY_F7   296
#define 	BS_KEY_F8   297
#define 	BS_KEY_F9   298
#define 	BS_KEY_F10   299
#define 	BS_KEY_F11   300
#define 	BS_KEY_F12   301
#define 	BS_KEY_F13   302
#define 	BS_KEY_F14   303
#define 	BS_KEY_F15   304
#define 	BS_KEY_F16   305
#define 	BS_KEY_F17   306
#define 	BS_KEY_F18   307
#define 	BS_KEY_F19   308
#define 	BS_KEY_F20   309
#define 	BS_KEY_F21   310
#define 	BS_KEY_F22   311
#define 	BS_KEY_F23   312
#define 	BS_KEY_F24   313
#define 	BS_KEY_F25   314
#define 	BS_KEY_KP_0   320
#define 	BS_KEY_KP_1   321
#define 	BS_KEY_KP_2   322
#define 	BS_KEY_KP_3   323
#define 	BS_KEY_KP_4   324
#define 	BS_KEY_KP_5   325
#define 	BS_KEY_KP_6   326
#define 	BS_KEY_KP_7   327
#define 	BS_KEY_KP_8   328
#define 	BS_KEY_KP_9   329
#define 	BS_KEY_KP_DECIMAL   330
#define 	BS_KEY_KP_DIVIDE   331
#define 	BS_KEY_KP_MULTIPLY   332
#define 	BS_KEY_KP_SUBTRACT   333
#define 	BS_KEY_KP_ADD   334
#define 	BS_KEY_KP_ENTER   335
#define 	BS_KEY_KP_EQUAL   336
#define 	BS_KEY_LEFT_SHIFT   340
#define 	BS_KEY_LEFT_CONTROL   341
#define 	BS_KEY_LEFT_ALT   342
#define 	BS_KEY_LEFT_SUPER   343
#define 	BS_KEY_RIGHT_SHIFT   344
#define 	BS_KEY_RIGHT_CONTROL   345
#define 	BS_KEY_RIGHT_ALT   346
#define 	BS_KEY_RIGHT_SUPER   347
#define 	BS_KEY_CTRL   348

#endif // BS_WND_H