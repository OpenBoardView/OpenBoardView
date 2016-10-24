#pragma once

#include "imgui/imgui.h"
#include "utf8/utf8.h"

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
