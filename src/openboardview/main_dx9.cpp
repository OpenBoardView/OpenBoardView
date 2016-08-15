/**
 * Open[Flex] Board View
 *
 * Copyright chloridite 2016
 * Copyright inflex 2016 (Paul Daniels)
 *
 * Git Fork: https://github.com/inflex/OpenBoardView
 *
 */
#include "BoardView.h"

#include "Renderers/imgui_impl_dx9.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include "platform.h"
#include "confparse.h"
#include "crtdbg.h"
#include "resource.h"
#include <dinput.h>
#include <direct.h>
#include <shlwapi.h>
#include <tchar.h>

// Data
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp;

// local functions
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif
uint32_t byte4swap(uint32_t x) {
	/*
	* used to convert RGBA -> ABGR etc
	*/
	return (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

	switch (msg) {
		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
				ImGui_ImplDX9_InvalidateDeviceObjects();
				g_d3dpp.BackBufferWidth  = LOWORD(lParam);
				g_d3dpp.BackBufferHeight = HIWORD(lParam);
				HRESULT hr               = g_pd3dDevice->Reset(&g_d3dpp);
				if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
				ImGui_ImplDX9_CreateDeviceObjects();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY: PostQuitMessage(0); return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	// Initialize comctl
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	char ss[1025];
	char *homepath;
	int err;
	size_t hpsz;
	// Confparse obvconfig;
	int sizex, sizey;
	bool use_exepath  = false;
	bool use_homepath = true;
	BoardView app{};
	CHAR history_file[MAX_PATH];
	CHAR conf_file[MAX_PATH];

	static const wchar_t *class_name = L"Openflex Boardview";

	/*
	 * To make OBV very easy to use and transportable among windows
	 * users, one method is to just make it do all its business in the
	 * folder that the EXE is launched from.  It's not "proper" but
	 * it is very simple and it works.
	 */
	HMODULE hModule = GetModuleHandleA(NULL);
	CHAR exepath[MAX_PATH];
	GetModuleFileNameA(hModule, exepath, MAX_PATH);

	/*
	 * Trim off the filename at the end of the path
	 */
	int l = strlen(exepath);
	while (--l) {
		if (exepath[l] == '\\') {
			exepath[l] = '\0';
			break;
		}
	}
	snprintf(history_file, sizeof(history_file), "%s\\obv.history", exepath);
	snprintf(conf_file, sizeof(conf_file), "%s\\obv.conf", exepath);

	/*
	 * Next, we check to see if there's an APPDATA folder that's
	 * already setup with our name on it.  This will be the case
	 * if OBV has been 'installed', even via the simple install.bat
	 * script.
	 */
	err = _dupenv_s(&homepath, &hpsz, "APPDATA");
	if (homepath) {
		struct stat st;
		int sr;
		snprintf(ss, sizeof(ss), "%s/openboardview", homepath);
		sr = stat(ss, &st);
		if (sr == -1) {
			//_mkdir(ss);
			// sr = stat(ss, &st);
		} else {
			snprintf(history_file, sizeof(history_file), "%s\\obv.history", homepath);
			snprintf(conf_file, sizeof(conf_file), "%s\\obv.conf", homepath);
		}
	}

	// Create application window
	HINSTANCE instance = GetModuleHandle(NULL);
	HICON icon         = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
	WNDCLASSEX wc      = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, instance, icon, NULL, NULL, NULL, class_name, NULL};
	RegisterClassEx(&wc);

	app.obvconfig.Load(conf_file);
	sizex = app.obvconfig.ParseInt("windowX", 900);
	sizey = app.obvconfig.ParseInt("windowY", 600);

	HWND hwnd = CreateWindow(class_name,
	                         _T("Openflex Board Viewer"),
	                         WS_OVERLAPPEDWINDOW,
	                         CW_USEDEFAULT,
	                         CW_USEDEFAULT,
	                         sizex,
	                         sizey,
	                         NULL,
	                         NULL,
	                         wc.hInstance,
	                         NULL);

	// Initialize Direct3D
	LPDIRECT3D9 pD3D;
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
		UnregisterClass(class_name, wc.hInstance);
		return 0;
	}
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed               = TRUE;
	g_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

	// Create the D3DDevice
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) <
	    0) {
		pD3D->Release();
		UnregisterClass(class_name, wc.hInstance);
		return 0;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(hwnd, g_pd3dDevice);

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see
	// extra_fonts/README.txt
	// for more details)
	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = NULL;

	/*
	 * Load the existing file loaded history
	 */
	if (history_file[0] != '\0') {
		app.fhistory.Set_filename(history_file);
		app.fhistory.Load();
	}

	/*
	 * Parse the settings in the configuration file
	 *
	 * There are some settings which have been parsed
	 * earlier but they apply to the startup of the
	 * main, as opposed to OBV itself.  This call
	 * parses all the things that'll influence OBV
	 */
	app.ConfigParse();

	/*
	 * Set the font based on the dpi
	 */
	for (auto name : {"Liberation Sans", "DejaVu Sans", "Arial", ""}) { // Empty string = use system default font
		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		const std::vector<char> ttf   = load_font(name);
		if (!ttf.empty()) {
			io.Fonts->AddFontFromMemoryTTF(const_cast<void *>(reinterpret_cast<const void *>(ttf.data())),
			                               ttf.size(),
			                               (app.obvconfig.ParseDouble("fontSize", 20.0f) * (app.dpi / 100.0)),
			                               &font_cfg);
			break;
		}
	}

	ImVec4 clear_col = ImColor(app.m_colors.backgroundColor);

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		ImGui_ImplDX9_NewFrame();
		app.Update();
		if (app.m_wantsQuit) {
			PostMessage(hwnd, WM_QUIT, 0, 0);
		}

		// Rendering
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(
		    (int)(clear_col.x * 255.0f), (int)(clear_col.y * 255.0f), (int)(clear_col.z * 255.0f), (int)(clear_col.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0) {
			ImGui::Render();
			g_pd3dDevice->EndScene();
		}
		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	}

	ImGui_ImplDX9_Shutdown();
	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (pD3D) pD3D->Release();
	UnregisterClass(class_name, wc.hInstance);

	return 0;
}
