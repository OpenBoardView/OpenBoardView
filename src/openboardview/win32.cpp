#ifdef _WIN32

#include "platform.h" // Should be kept first
#include "imgui/imgui.h"
#include "utf8/utf8.h"
#include <assert.h>
#include <codecvt>
#include <iostream>
#include <locale>
#include <shlobj.h>
#include <shobjidl.h>
#include <stdint.h>
#include <winnls.h>
#ifdef ENABLE_SDL2
#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#endif

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

#if defined(_MSC_VER) && _MSC_VER <= 1900 // Should be fixed "in the next major version"
const std::wstring utf8_to_utf16(const std::string &text) {
	return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(text.c_str());
}
#else
const std::u16string utf8_to_utf16(const std::string &text) {
	return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(text.c_str());
}
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1900
const wchar_t *utf16_to_wchar(const std::wstring &text) {
#else
const wchar_t *utf16_to_wchar(const std::u16string &text) {
#endif
	return reinterpret_cast<const wchar_t *>(text.c_str());
}

// Mostly from https://msdn.microsoft.com/en-us/library/windows/desktop/ff485843(v=vs.85).aspx
// Windows Vista minimum
const std::string show_file_picker() {
	std::string file_path;
	IFileOpenDialog *pFileOpen;

	// Initializes the COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(hr)) return file_path;

	// Create the FileOpenDialog object.
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));
	if (SUCCEEDED(hr)) {
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr)) {
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// Display the file name to the user.
				if (SUCCEEDED(hr)) {
					file_path = wchar_to_utf8(pszFilePath);
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
	CoUninitialize();
	return file_path;
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

const std::string get_user_dir(const UserDir userdir) {
	int cdret = 0;
	std::string configPath;
	PWSTR envVar = nullptr;
	if (userdir == UserDir::Config)
		SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &envVar);
	else if (userdir == UserDir::Data)
		SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &envVar);

	if (envVar) {
		configPath = utf16_to_utf8(envVar);
		configPath += "\\OpenBoardView\\";
		auto configPathu16 = utf8_to_utf16(configPath);
		cdret = CreateDirectoryW(utf16_to_wchar(configPathu16), NULL); // Doesn't work recursively but it's not an issue here
	}
	CoTaskMemFree(envVar);

	if (configPath.empty() || (cdret == 0 && GetLastError() != ERROR_ALREADY_EXISTS)) configPath = ".\\"; // Fallback to current dir
	return configPath;
}

#endif
