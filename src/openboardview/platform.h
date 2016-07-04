#pragma once

#include "imgui/imgui.h"
#include "utf8/utf8.h"

#ifdef _WIN32
#include <windows.h>
#define SDL_SCANCODE_LCTRL VK_CONTROL
#define SDL_SCANCODE_RCTRL VK_CONTROL
#define SDL_SCANCODE_KP_0 VK_NUMPAD0
#define SDL_SCANCODE_KP_2 VK_NUMPAD2
#define SDL_SCANCODE_KP_4 VK_NUMPAD4
#define SDL_SCANCODE_KP_5 VK_NUMPAD5
#define SDL_SCANCODE_KP_6 VK_NUMPAD6
#define SDL_SCANCODE_KP_8 VK_NUMPAD8
#define SDL_SCANCODE_KP_PERIOD VK_DECIMAL
#define SDL_SCANCODE_KP_PLUS VK_ADD
#define SDL_SCANCODE_KP_MINUS VK_SUBTRACT
#endif

#ifdef _MSC_VER // Visual Studio prefixes non-standard C functions with _
#define strdup _strdup
#endif

// Loads an entire file in to memory.  The returned buffer must be later freed by the caller.
char *file_as_buffer(size_t *buffer_size, const char *utf8_filename);

// Shows a file dialog (should hang the current thread) and returns the utf8 filename picked by the
// user or nullptr.  If non-null is returned, it must be later freed by the caller.
char *show_file_picker();

// these mirror the Windows .rc definitions:
#define ASSET_FILLED_CIRCLE 105
#define ASSET_FIRA_SANS 109
#define ASSET_EMPTY_CIRCLE 110

unsigned char *LoadAsset(int *asset_size, int asset_id);

#define NUM_GLOBAL_TEXTURES 10
extern ImTextureID TextureIDs[NUM_GLOBAL_TEXTURES];
