// ImGui SDL2 binding with OpenGL ES 3
// You can copy and use unmodified imgui_impl_* files in your project.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and
// ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui

#include "imgui.h"
#include "imgui_impl_sdl_gles2.h"

// SDL, glad
#if defined(_MSC_VER) || defined(__ANDROID__)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <glad/glad.h>

// Data
static SDL_Window *g_Window   = NULL;
static double g_Time          = 0.0f;
static bool g_MousePressed[3] = {false, false, false};
static float g_MouseWheel     = 0.0f;
static GLuint g_FontTexture   = 0;
static int g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;
static bool g_DrawElemWorkaround = false;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the
// ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplSdlGLES2_RenderDrawLists(ImDrawData *draw_data) {
	// Backup GL state
	GLint last_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_array_buffer;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);

	GLint last_blend_equation_rgb;
	glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
	GLint last_blend_equation_alpha;
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
	GLint last_viewport[4];
	glGetIntegerv(GL_VIEWPORT, last_viewport);

	GLboolean last_enable_blend        = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face    = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test   = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glActiveTexture(GL_TEXTURE0);

	// Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
	ImGuiIO &io     = ImGui::GetIO();
	float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// Setup orthographic projection matrix
	glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
	const float ortho_projection[4][4] = {
	    {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
	    {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
	    {0.0f, 0.0f, -1.0f, 0.0f},
	    {-1.0f, 1.0f, 0.0f, 1.0f},
	};

	glUseProgram(g_ShaderHandle);
	glUniform1i(g_AttribLocationTex, 0);
	glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList *cmd_list         = draw_data->CmdLists[n];
		const ImDrawIdx *idx_buffer_offset = 0;
		GLint vtx_buffer_offset            = 0;

		glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);

		if (g_DrawElemWorkaround) {
			ImVector<ImDrawVert> vertices;
			vertices.reserve(cmd_list->IdxBuffer.size());

			for (auto &id : cmd_list->IdxBuffer) vertices.push_back(cmd_list->VtxBuffer[id]);

			glBufferData(
			    GL_ARRAY_BUFFER, (GLsizeiptr)vertices.size() * sizeof(ImDrawVert), (GLvoid *)&vertices.front(), GL_STREAM_DRAW);
		} else {
			glBufferData(GL_ARRAY_BUFFER,
			             (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert),
			             (GLvoid *)&cmd_list->VtxBuffer.front(),
			             GL_STREAM_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			             (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx),
			             (GLvoid *)&cmd_list->IdxBuffer.front(),
			             GL_STREAM_DRAW);
		}

		for (const ImDrawCmd *pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
			if (pcmd->UserCallback) {
				pcmd->UserCallback(cmd_list, pcmd);
			} else {
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x,
				          (int)(fb_height - pcmd->ClipRect.w),
				          (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
				          (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				if (g_DrawElemWorkaround)
					glDrawArrays(
					    GL_TRIANGLES,
					    vtx_buffer_offset,
					    (GLsizei)pcmd->ElemCount); // WARNING: inefficient workaround for devices not supporting GL_UNSIGNED_INT
				else
					glDrawElements(GL_TRIANGLES,
					               (GLsizei)pcmd->ElemCount,
					               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
					               idx_buffer_offset);
			}
			if (g_DrawElemWorkaround)
				vtx_buffer_offset += pcmd->ElemCount;
			else
				idx_buffer_offset += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (last_enable_blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (last_enable_cull_face)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if (last_enable_depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (last_enable_scissor_test)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);

	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char *ImGui_ImplSdlGLES2_GetClipboardText(void *) {
	return SDL_GetClipboardText();
}

static void ImGui_ImplSdlGLES2_SetClipboardText(void *, const char *text) {
	SDL_SetClipboardText(text);
}

bool ImGui_ImplSdlGLES2_ProcessEvent(SDL_Event *event) {
	ImGuiIO &io = ImGui::GetIO();
	switch (event->type) {
		case SDL_MOUSEWHEEL: {
			if (event->wheel.y > 0) g_MouseWheel = 1;
			if (event->wheel.y < 0) g_MouseWheel = -1;
			return true;
		}
		case SDL_MOUSEBUTTONDOWN: {
			if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0]   = true;
			if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1]  = true;
			if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
			return true;
		}
		case SDL_FINGERDOWN: {
			// SDL_Log("Finger down !!");
			return true;
		}
		case SDL_TEXTINPUT: {
			io.AddInputCharactersUTF8(event->text.text);
			return true;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			// int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
			int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
			if (event->key.keysym.sym & (1 << 30)) {
				int tkey          = key | 0x100;
				io.KeysDown[tkey] = (event->type == SDL_KEYDOWN);
			}
			io.KeysDown[key] = (event->type == SDL_KEYDOWN);
			io.KeyShift      = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl       = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt        = ((SDL_GetModState() & KMOD_ALT) != 0);
			return true;
		}
	}
	return false;
}

void ImGui_ImplSdlGLES2_CreateFontsTexture() {
	ImGuiIO &io = ImGui::GetIO();

	// Build texture atlas
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(
	    &pixels,
	    &width,
	    &height); // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

	// Create OpenGL texture
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

	glBindTexture(GL_TEXTURE_2D, last_texture);
}

bool ImGui_ImplSdlGLES2_CreateDeviceObjects() {
	g_DrawElemWorkaround =
	    (SDL_GL_ExtensionSupported("GL_OES_element_index_uint") != SDL_TRUE) && (sizeof(ImDrawIdx) > sizeof(GLushort));
	if (g_DrawElemWorkaround)
		SDL_Log("[ImGui] Warning: GL_OES_element_index_uint not supported. Using glDrawArrays(). This will be slower.");

	// Backup GL state
	GLint last_texture, last_array_buffer;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);

	const GLchar *vertex_shader =
	    "precision mediump float;\n"
	    "attribute vec2 Position;\n"
	    "attribute vec2 UV;\n"
	    "attribute vec4 Color;\n"
	    "varying vec2 Frag_UV;\n"
	    "varying vec4 Frag_Color;\n"
	    "uniform mat4 ProjMtx;\n"
	    "void main()\n"
	    "{\n"
	    "	Frag_UV = UV;\n"
	    "	Frag_Color = Color;\n"
	    "	gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
	    "}\n";

	const GLchar *fragment_shader =
	    "precision mediump float;\n"
	    "varying vec2 Frag_UV;\n"
	    "varying vec4 Frag_Color;\n"
	    "uniform sampler2D Texture;\n"
	    "void main()\n"
	    "{\n"
	    "	gl_FragColor = Frag_Color * texture2D( Texture, Frag_UV.st);\n"
	    "}\n";

	g_ShaderHandle = glCreateProgram();
	g_VertHandle   = glCreateShader(GL_VERTEX_SHADER);
	g_FragHandle   = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
	glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
	glCompileShader(g_VertHandle);

	GLint logLength;
	glGetShaderiv(g_VertHandle, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 1) {
		GLchar *log = (GLchar *)malloc(logLength);
		glGetShaderInfoLog(g_VertHandle, logLength, &logLength, log);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VERTEX Shader compile log:\n%s", log);
		free(log);
	}

	glCompileShader(g_FragHandle);

	glGetShaderiv(g_FragHandle, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 1) {
		GLchar *log = (GLchar *)malloc(logLength);
		glGetShaderInfoLog(g_FragHandle, logLength, &logLength, log);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FRAGMENT Shader compile log:\n%s", log);
		free(log);
	}

	glAttachShader(g_ShaderHandle, g_VertHandle);
	glAttachShader(g_ShaderHandle, g_FragHandle);
	glLinkProgram(g_ShaderHandle);

	g_AttribLocationTex      = glGetUniformLocation(g_ShaderHandle, "Texture");
	g_AttribLocationProjMtx  = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
	g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
	g_AttribLocationUV       = glGetAttribLocation(g_ShaderHandle, "UV");
	g_AttribLocationColor    = glGetAttribLocation(g_ShaderHandle, "Color");

	glGenBuffers(1, &g_VboHandle);
	glGenBuffers(1, &g_ElementsHandle);

	glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
	glEnableVertexAttribArray(g_AttribLocationPosition);
	glEnableVertexAttribArray(g_AttribLocationUV);
	glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t) & (((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid *)OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(
	    g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid *)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

	ImGui_ImplSdlGLES2_CreateFontsTexture();

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);

	return true;
}

void ImGui_ImplSdlGLES2_InvalidateDeviceObjects() {
	if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
	if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
	g_VboHandle = g_ElementsHandle = 0;

	glDetachShader(g_ShaderHandle, g_VertHandle);
	glDeleteShader(g_VertHandle);
	g_VertHandle = 0;

	glDetachShader(g_ShaderHandle, g_FragHandle);
	glDeleteShader(g_FragHandle);
	g_FragHandle = 0;

	glDeleteProgram(g_ShaderHandle);
	g_ShaderHandle = 0;

	if (g_FontTexture) {
		glDeleteTextures(1, &g_FontTexture);
		ImGui::GetIO().Fonts->TexID = 0;
		g_FontTexture               = 0;
	}
}

bool ImGui_ImplSdlGLES2_Init(SDL_Window *window) {
	g_Window = window;

	ImGuiIO &io                   = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab]       = SDLK_TAB; // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow]    = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow]  = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp]     = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown]   = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home]       = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End]        = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete]     = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace]  = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter]      = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape]     = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A]          = SDLK_a;
	io.KeyMap[ImGuiKey_C]          = SDLK_c;
	io.KeyMap[ImGuiKey_V]          = SDLK_v;
	io.KeyMap[ImGuiKey_X]          = SDLK_x;
	io.KeyMap[ImGuiKey_Y]          = SDLK_y;
	io.KeyMap[ImGuiKey_Z]          = SDLK_z;

	io.RenderDrawListsFn = ImGui_ImplSdlGLES2_RenderDrawLists; // Alternatively you can set this to NULL and call
	                                                           // ImGui::GetDrawData() after ImGui::Render() to get the same
	                                                           // ImDrawData pointer.
	io.SetClipboardTextFn = ImGui_ImplSdlGLES2_SetClipboardText;
	io.GetClipboardTextFn = ImGui_ImplSdlGLES2_GetClipboardText;
	io.ClipboardUserData  = NULL;

	return true;
}

void ImGui_ImplSdlGLES2_Shutdown() {
	ImGui_ImplSdlGLES2_InvalidateDeviceObjects();
	ImGui::Shutdown();
}

void ImGui_ImplSdlGLES2_NewFrame() {
	if (!g_FontTexture) ImGui_ImplSdlGLES2_CreateDeviceObjects();

	ImGuiIO &io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	SDL_GetWindowSize(g_Window, &w, &h);
	io.DisplaySize             = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Setup time step
	Uint32 time         = SDL_GetTicks();
	double current_time = static_cast<double>(time / 1000);
	io.DeltaTime        = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
	g_Time              = current_time;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
	int mx, my;
	Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
	if (SDL_GetWindowFlags(g_Window) & SDL_WINDOW_MOUSE_FOCUS)
		io.MousePos =
		    ImVec2((float)mx, (float)my); // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
	else
		io.MousePos = ImVec2(-1, -1);

	io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0; // If a mouse press event came, always
	                                                                                       // pass it as "mouse held this frame", so
	                                                                                       // we don't miss click-release events
	                                                                                       // that are shorter than 1 frame.
	io.MouseDown[1]   = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2]   = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

	io.MouseWheel = g_MouseWheel;
	g_MouseWheel  = 0.0f;

	// Hide OS mouse cursor if ImGui is drawing it
	SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

	// Start the frame
	ImGui::NewFrame();
}
