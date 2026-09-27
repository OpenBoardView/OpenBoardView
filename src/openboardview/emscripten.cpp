#ifdef __EMSCRIPTEN__

#include "platform.h"

#include <SDL.h>
#include <emscripten.h>
#include <sys/stat.h>

#include <cstdlib>

// Opens a browser file dialog and copies the chosen files into the in-memory
// file system with Module.obvStoreFiles() from shell.html. Returns the path of
// the board file, or an empty string when the user cancels. The user can pick
// more than one file because some formats, for example ASC, need them all.
// Needs ASYNCIFY to block the caller.
// clang-format off
EM_ASYNC_JS(char *, obv_pick_file, (int filterBoards), {
	const files = await new Promise((resolve) => {
		const input = document.createElement('input');
		input.type = 'file';
		input.multiple = true;
		if (filterBoards) {
			input.accept = '.asc,.bom,.brd,.bdv,.bvr,.cad,.cae,.cst,.pcbdoc,.fz,.pcb';
		}
		input.addEventListener('change', () => resolve(input.files));
		input.addEventListener('cancel', () => resolve([]));
		input.click();
	});
	try {
		const path = await Module.obvStoreFiles(files);
		return path ? stringToNewUTF8(path) : 0;
	} catch (e) {
		console.error(e);
		Module.setStatus('Cannot open the file. See the browser console.');
		setTimeout(() => Module.setStatus(''), 5000);
		return 0;
	}
});
// clang-format on

const filesystem::path show_file_picker(bool filterBoards) {
	char *path = obv_pick_file(filterBoards ? 1 : 0);
	if (path == nullptr) return {};
	filesystem::path result = filesystem::u8path(path);
	std::free(path);
	return result;
}

// The web build embeds one font at /fonts/default.ttf. Other fonts can be
// added at /fonts/<name>.ttf.
const std::string get_font_path(const std::string &name) {
	std::string path = name.empty() ? "/fonts/default.ttf" : "/fonts/" + name + ".ttf";
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return {};
	return path;
}

// Called from JavaScript (drag and drop, "Open" button) after it wrote a file
// into the in-memory file system. Reuses the SDL drop file path of the main loop.
extern "C" EMSCRIPTEN_KEEPALIVE void obv_open_file(const char *path) {
	SDL_Event event{};
	event.type      = SDL_DROPFILE;
	event.drop.file = SDL_strdup(path);
	SDL_PushEvent(&event);
}

#endif
