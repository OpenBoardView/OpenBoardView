// ImGui - standalone example application for DirectX 9
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "BoardView.h"

#include "imgui_impl_dx9.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include "crtdbg.h"
#include "platform.h"
#include <dinput.h>
#include <nowide/convert.hpp>
#include <tchar.h>

// Data
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp;

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// Initialize comctl
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	static const wchar_t *class_name = L"ImGui Example";

	// Create application window
	HINSTANCE instance = GetModuleHandle(NULL);
	HICON icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
	WNDCLASSEX wc = {sizeof(WNDCLASSEX),
	                 CS_CLASSDC,
	                 WndProc,
	                 0L,
	                 0L,
	                 instance,
	                 icon,
	                 NULL,
	                 NULL,
	                 NULL,
	                 class_name,
	                 NULL};
	RegisterClassEx(&wc);
	HWND hwnd =
	    CreateWindow(class_name, _T("Open Board Viewer"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
	                 CW_USEDEFAULT, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	DragAcceptFiles(hwnd, true);

	// Initialize Direct3D
	LPDIRECT3D9 pD3D;
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
		UnregisterClass(class_name, wc.hInstance);
		MessageBox(hwnd, L"Failed to initialise Direct3D", NULL, 0);
		return 0;
	}
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// Create the D3DDevice
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
	                       D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) {
		pD3D->Release();
		UnregisterClass(class_name, wc.hInstance);
		MessageBox(hwnd, L"Failed to create Direct3D device", NULL, 0);
		return 0;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(hwnd, g_pd3dDevice);

	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL; // Disable imgui.ini

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt
	// for more details)
	int ttf_size;
	unsigned char *ttf_data = LoadAsset(&ttf_size, ASSET_FIRA_SANS);
	ImFontConfig font_cfg{};
	font_cfg.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF(ttf_data, ttf_size, 20.0f, &font_cfg);
// io.Fonts->AddFontDefault();
// io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
// io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
// io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
// io.Fonts->GetGlyphRangesJapanese());
#if 0
	// Get current flag
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	// Turn on leak-checking bit.
	tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;

	// Set flag to the new value.
	_CrtSetDbgFlag(tmpFlag);
#endif

	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_col = ImColor(20, 20, 30);

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

		if (msg.message == WM_DROPFILES) {

			HDROP hDrop = (HDROP)msg.wParam;
			TCHAR *lpszFile = new TCHAR[MAX_PATH];
			UINT uFile = 0;

			uFile = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, NULL);

			if (uFile > 1) {
				app.ShowError("Multiple files not supported");
			} else {
				lpszFile[0] = '\0';
				if (DragQueryFile(hDrop, 0, lpszFile, MAX_PATH)) {
					app.OpenFile(nowide::narrow(lpszFile));
				}
			}

			DragFinish(hDrop);
		}

#if 0
		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window
		// automatically called "Debug"
		{
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float *)&clear_col);
			if (ImGui::Button("Test Window"))
				show_test_window ^= 1;
			if (ImGui::Button("Another Window"))
				show_another_window ^= 1;
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (show_another_window) {
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello");
			ImGui::End();
		}

#endif
#if 0
		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (show_test_window) {
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowTestWindow(&show_test_window);
		}
#endif
		app.Update();
		if (app.m_wantsQuit) {
			PostMessage(hwnd, WM_QUIT, 0, 0);
		}

		if (app.m_wantsTitleChange) {
			app.m_wantsTitleChange = false;

			TCHAR title[MAX_PATH + 100];
			TCHAR filename[MAX_PATH + 10];

			// Convert character encoding if required, and format the window title
			mbstowcs_s(NULL, filename, _countof(filename), app.m_lastFileOpenName.c_str(),
			           app.m_lastFileOpenName.length());
			_stprintf_s(title, _countof(title), L"%s - Open Board Viewer", filename);

			SetWindowText(hwnd, title);
		}

		// Rendering
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
		D3DCOLOR clear_col_dx =
		    D3DCOLOR_RGBA((int)(clear_col.x * 255.0f), (int)(clear_col.y * 255.0f),
		                  (int)(clear_col.z * 255.0f), (int)(clear_col.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0) {
			ImGui::Render();
			g_pd3dDevice->EndScene();
		}
		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	}

	ImGui_ImplDX9_Shutdown();
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
	if (pD3D)
		pD3D->Release();
	UnregisterClass(class_name, wc.hInstance);

	DragAcceptFiles(hwnd, false);

	return 0;
}
