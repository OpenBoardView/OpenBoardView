#pragma once

#include "imgui/imgui.h"
#include "utf8/utf8.h"
#include <string>

#ifdef _WIN32
#include <windows.h>
#define SDLK_ESCAPE VK_ESCAPE
#define SDLK_RETURN VK_RETURN
#define SDL_SCANCODE_LCTRL VK_CONTROL
#define SDL_SCANCODE_RCTRL VK_CONTROL
#define SDL_SCANCODE_KP_0 VK_INSERT
#define SDL_SCANCODE_KP_2 VK_DOWN
#define SDL_SCANCODE_KP_4 VK_LEFT
#define SDL_SCANCODE_KP_5 VK_CLEAR
#define SDL_SCANCODE_KP_6 VK_RIGHT
#define SDL_SCANCODE_KP_8 VK_UP

#define SDLK_a 0x41
#define SDLK_b 0x42
#define SDLK_c 0x43
#define SDLK_d 0x44
#define SDLK_e 0x45
#define SDLK_f 0x46
#define SDLK_s 0x53
#define SDLK_d 0x44
#define SDLK_w 0x57
#define SDLK_x 0x58
#define SDLK_y 0x59
#define SDLK_z 0x5A

#define SDLK_l 0x4c
#define SDLK_k 0x4b
#define SDLK_c 0x43
#define SDLK_n 0x4e

#define SDLK_o 0x4F
#define SDLK_p 0x50
#define SDLK_q 0x51
#define SDLK_r 0x52

#define SDLK_SLASH VK_OEM2

#define SDLK_COMMA VK_OEM_COMMA
#define SDLK_KP_0 VK_INSERT
#define SDLK_KP_PERIOD VK_DELETE
#define SDLK_PERIOD VK_OEM_PERIOD
#define SDLK_EQUALS VK_OEM_PLUS
#define SDLK_SPACE VK_SPACE
#define SDLK_MINUS VK_OEM_MINUS
#define SDL_SCANCODE_KP_PERIOD VK_DECIMAL
#define SDL_SCANCODE_KP_PLUS VK_ADD
#define SDL_SCANCODE_KP_MINUS VK_SUBTRACT

#endif

#ifdef _MSC_VER // Visual Studio prefixes non-standard C functions with _
#define strdup _strdup
#endif

// Loads an entire file in to memory.  The returned buffer must be later freed
// by the caller.
char *file_as_buffer(size_t *buffer_size, const char *utf8_filename);

// Shows a file dialog (should hang the current thread) and returns the utf8
// filename picked by the
// user or nullptr.  If non-null is returned, it must be later freed by the
// caller.
char *show_file_picker();

// these mirror the Windows .rc definitions:
#define ASSET_FILLED_CIRCLE 105
#define ASSET_FIRA_SANS 109
#define ASSET_EMPTY_CIRCLE 110

unsigned char *LoadAsset(int *asset_size, int asset_id);
std::string get_asset_path(const char *asset);

#define NUM_GLOBAL_TEXTURES 10
extern ImTextureID TextureIDs[NUM_GLOBAL_TEXTURES];
