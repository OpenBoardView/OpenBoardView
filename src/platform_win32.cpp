#ifdef _WIN32

#include "platform.h"
#include "imgui.h"
#include <Windows.h>
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <winnls.h>

wchar_t *utf8_to_wide(const char *s) {
	size_t len = utf8len(s);
	wchar_t *buf = (wchar_t *)malloc(sizeof(wchar_t) * (1 + len));
	mbstowcs(buf, s, len);
	buf[len] = 0;
	return buf;
}

char *wide_to_utf8(const wchar_t *s) {
	size_t len = 0;
	wchar_t c;
	for (const wchar_t *ss = s; 0 != (c = *ss); ++ss) {
		if (c < 0x80) {
			len += 1;
		} else if (c < 0x800) {
			len += 2;
		} else if (c >= 0xd800 && c < 0xdc00) {
			// surrogate pair
			len += 4;
			++ss;
		} else if (c >= 0xdc00 && c < 0xe000) {
			// pair end -- invalid sequence
			return nullptr;
		} else {
			len += 3;
		}
	}
	char *buf = (char *)malloc(1 + len);
	wcstombs(buf, s, len);
	buf[len] = 0;
	return buf;
}

char *file_as_buffer(size_t *buffer_size, const char *utf8_filename) {
	wchar_t *wide_filename = utf8_to_wide(utf8_filename);
	HANDLE file = CreateFile(wide_filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
	                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	free(wide_filename);
	if (file == INVALID_HANDLE_VALUE) {
		*buffer_size = 0;
		return nullptr;
	}

	LARGE_INTEGER filesize;
	GetFileSizeEx(file, &filesize);
	uint32_t sz = (uint32_t)filesize.QuadPart;
	assert(filesize.QuadPart == sz);
	*buffer_size = sz;

	char *buf = (char *)malloc(sz);
	uint32_t numRead = 0;
	ReadFile(file, buf, sz, (LPDWORD)&numRead, NULL);
	assert(numRead == sz);

	return buf;
}

char *show_file_picker() {
	OPENFILENAME ofn;
	wchar_t filename[1024];
	filename[0] = 0;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (HWND)ImGui::GetIO().ImeWindowHandle;
	ofn.lpstrFilter = L"BRD Files\0*.brd\0All Files\0*.*\0\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	if (GetOpenFileName(&ofn)) {
		return wide_to_utf8(filename);
	}
	return nullptr;
}

// Not caring about XP. If this is a concern, find a replacement for CompareStringEx()
const std::vector<char> load_font(const std::string &name) {
	std::vector<char> data;
	HFONT fontHandle;

	wchar_t *wname = utf8_to_wide(name.c_str());

	fontHandle = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, name.empty() ? NULL : wname);
	if (!fontHandle) {
		std::cerr << "CreateFont failed" << std::endl;
		free(wname);
		return data;
	}
	HDC hdc = ::CreateCompatibleDC(NULL);
	if (hdc != NULL) {
		::SelectObject(hdc, fontHandle);

		int ncount = ::GetTextFaceW(hdc, 0, nullptr);
		LPWSTR fname = new wchar_t[ncount];
		::GetTextFaceW(hdc, ncount, fname);

		if (!name.empty() &&
		    ::CompareStringEx(NULL, NORM_IGNORECASE, wname, name.size(), fname, ncount - 1, NULL,
		                      NULL, 0) != CSTR_EQUAL) { // We didn't get the font we requested
			free(wname);
			return data;
		}

		free(wname);

		const size_t size = ::GetFontData(hdc, 0, 0, NULL, 0);
		if (size > 0) {
			char *buffer = new char[size];
			if (::GetFontData(hdc, 0, 0, buffer, size) == size) {
				data = std::vector<char>(buffer, buffer + size);
			}
			delete[] buffer;
		}
		::DeleteDC(hdc);
	}
	DeleteObject(fontHandle);
	return data;
}

#endif // _WIN32
