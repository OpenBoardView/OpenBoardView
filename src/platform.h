#pragma once

#include "imgui.h"
#include "utf8.h"
#include <string>
#include <vector>

// Loads an entire file in to memory.  The returned buffer must be later freed by the caller.
char *file_as_buffer(size_t *buffer_size, const char *utf8_filename);

// Shows a file dialog (should hang the current thread) and returns the utf8 filename picked by the
// user or nullptr.  If non-null is returned, it must be later freed by the caller.
//
// Filetype filtering must be implemented separately in each and every one of these functions (this
// could be done in a more elegant way in the future, if the scope of the project expands to include
// lots of formats)
char *show_file_picker();

const std::string get_font_path(const std::string &name);
const std::vector<char> load_font(const std::string &name);
