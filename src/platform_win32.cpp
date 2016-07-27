#include "platform.h"
#include <stdint.h>
#include <Windows.h>

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

unsigned char *LoadAsset(int *asset_size, int asset_id) {
	HRSRC resinfo = FindResource(NULL, MAKEINTRESOURCE(asset_id), L"Asset");
	HGLOBAL res = LoadResource(NULL, resinfo);
	*asset_size = (int)SizeofResource(NULL, resinfo);
	unsigned char *data = (unsigned char *)LockResource(res);
	UnlockResource(res);
	return data;
}
