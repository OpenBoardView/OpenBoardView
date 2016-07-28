#include "platform.h"
#include <Windows.h>
#include <nowide/convert.hpp>
#include <stdint.h>
#include <string>

std::string show_file_picker() {
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
		return nowide::narrow(filename);
	}
	return std::string("");
}

unsigned char *LoadAsset(int *asset_size, int asset_id) {
	HRSRC resinfo = FindResource(NULL, MAKEINTRESOURCE(asset_id), L"Asset");
	HGLOBAL res = LoadResource(NULL, resinfo);
	*asset_size = (int)SizeofResource(NULL, resinfo);
	unsigned char *data = (unsigned char *)LockResource(res);
	UnlockResource(res);
	return data;
}
