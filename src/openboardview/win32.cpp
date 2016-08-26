#ifdef _WIN32

#include "platform.h"
#include "imgui/imgui.h"
#include "utf8/utf8.h"
#include <assert.h>
#include <codecvt>
#include <iostream>
#include <locale>
#include <stdint.h>
#include <winnls.h>
#ifdef ENABLE_SDL2
#include <SDL2/SDL.h>
#endif

wchar_t *utf8_to_wide(const char *s) {
	size_t len   = utf8len(s);
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
	HANDLE file            = CreateFile(
	    wide_filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

	char *buf        = (char *)malloc(sz);
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
	ofn.hwndOwner   = (HWND)ImGui::GetIO().ImeWindowHandle;
	ofn.lpstrFilter = L"All Files\0*.*\0\0";
	ofn.lpstrFile   = filename;
	ofn.nMaxFile    = 1024;
	if (GetOpenFileName(&ofn)) {
		return wide_to_utf8(filename);
	}
	return nullptr;
}

const std::string utf16_to_utf8(const std::wstring &text) {
// See https://connect.microsoft.com/VisualStudio/feedback/details/1348277/link-error-when-using-std-codecvt-utf8-utf16-char16-t
#if defined(_MSC_VER) && _MSC_VER <= 1900 // Should be fixed "in the next major version"
	return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(
	    reinterpret_cast<const wchar_t *>(text.c_str()));
#else
	return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(
	    reinterpret_cast<const char16_t *>(text.c_str()));
#endif
}

const std::string wchar_to_utf8(const wchar_t *text) {
	return std::string(utf16_to_utf8(std::wstring(text)));
}

const std::u16string utf8_to_utf16(const std::string &text) {
#if defined(_MSC_VER) && _MSC_VER <= 1900 // Should be fixed "in the next major version"
	return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(text.c_str());
#else
	return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(text.c_str());
#endif
}

const wchar_t *utf16_to_wchar(const std::u16string &text) {
	return reinterpret_cast<const wchar_t *>(text.c_str());
}

const std::vector<char> load_font(const std::string &name) {
	std::vector<char> data;
	HFONT fontHandle;

	auto u16name = utf8_to_utf16(name);
	auto wname   = utf16_to_wchar(u16name);

	fontHandle = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, OUT_TT_ONLY_PRECIS, 0, 0, 0, wname);
	if (!fontHandle) {
		std::cerr << "CreateFont failed" << std::endl;
		return data;
	}
	HDC hdc = ::CreateCompatibleDC(NULL);
	if (hdc != NULL) {
		::SelectObject(hdc, fontHandle);

		int ncount = ::GetTextFaceW(hdc, 0, nullptr);
		if (ncount == 0) return data;

		LPWSTR fname = new wchar_t[ncount];
		ncount       = ::GetTextFaceW(hdc, ncount, fname);

		if (!name.empty() &&
		    ::CompareStringEx(NULL, NORM_IGNORECASE, wname, name.size(), fname, ncount - 1, NULL, NULL, 0) !=
		        CSTR_EQUAL) // We didn't get the font we requested
			return data;

		const size_t size = ::GetFontData(hdc, 0, 0, NULL, 0);
		if (size == GDI_ERROR) {
#ifdef ENABLE_SDL2
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Couldn't get \"%s\" font data: %lu", name.c_str(), GetLastError());
#else
			std::cout << "Couldn't get \"" << name << "\" font data: " << GetLastError() << std::endl;
#endif
		} else if (size > 0) {
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

#endif
