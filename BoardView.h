#pragma once

#include <stdint.h>
#include "imgui/imgui.h"

struct BRDPart;
struct BRDFile;

struct BitVec {
	uint32_t *m_bits;
	// length of the BitVec in bits
	uint32_t m_size;

	~BitVec();
	void Resize(uint32_t new_size);

	bool operator[](uint32_t index) {
		return 0 != (m_bits[index >> 5] & (1u << (index & 0x1f)));
	}

	void Set(uint32_t index, bool val) {
		uint32_t &slot = m_bits[index >> 5];
		uint32_t bit = (1u << (index & 0x1f));
		slot &= ~bit;
		if (val) {
			slot |= bit;
		}
	}

	void Clear() {
		uint32_t num_ints = m_size >> 5;
		for (uint32_t i = 0; i < num_ints; i++) {
			m_bits[i] = 0;
		}
	}
};

struct ColorScheme {
	uint32_t backgroundColor =	0xa0000000;
	uint32_t boxColor =			0xffcccccc;
	uint32_t partTextColor =	0xff808000;
	uint32_t boardOutline =		0xff00ffff;

	uint32_t pinHighlighted =	0xffffffff;
	uint32_t pinSelected =		0xff00ff00;
};

enum DrawChannel
{
	kChannelImages = 0,
	kChannelPolylines = 1,
	kChannelText = 2,
	NUM_DRAW_CHANNELS = 3
};

struct BoardView {
	BRDFile *m_file;
	int m_pinSelected;
	BitVec m_pinHighlighted;
	BitVec m_partHighlighted;
	char m_cachedDrawList[sizeof(ImDrawList)];
	ImVector<char> m_cachedDrawCommands;
	ImVector<char *> m_nets;
	char m_search[128];
	char m_netFilter[128];
	char *m_lastFileOpenName;
	float m_dx;
	float m_dy;
	float m_mx;
	float m_my;
	float m_scale = 1.0f;
	float m_lastWidth;
	float m_lastHeight;
	int m_rotation;
	int m_side;
	int m_boardWidth;
	int m_boardHeight;

	ColorScheme m_colors;

	// TODO: save settings to disk
	// pinDiameter: diameter for all pins.  Unit scale: 1 = 0.025mm
	int m_pinDiameter = 20;
	bool m_flipVertically = true;

	// Do a hacky thing (memcpy the window draw list) to save us from rerendering the board
	// The app will crash or break if this flag is not set when it should be.
	bool m_needsRedraw = true;
	bool m_draggingLastFrame;
	bool m_showNetfilterSearch;
	bool m_showComponentSearch;
	bool m_showNetList;
	bool m_firstFrame = true;
	bool m_lastFileOpenWasInvalid;
	bool m_wantsQuit;

	~BoardView();

	void ShowNetList(bool * p_open);

	void Update();
	void HandleInput();
	void RenderOverlay();
	void DrawOutline(ImDrawList * draw);
	void DrawPins(ImDrawList * draw);
	void DrawParts(ImDrawList * draw);
	void DrawBoard();
	void SetFile(BRDFile *file);
	ImVec2 CoordToScreen(float x, float y, float w = 1.0f);
	ImVec2 ScreenToCoord(float x, float y, float w = 1.0f);
	void Move(float x, float y);
	void Rotate(int count);

	// Sets the center of the screen to (x,y) in board space
	void SetTarget(float x, float y);

	// Returns true if the part is shown on the currently displayed side of the board.
	bool PartIsVisible(const BRDPart &part);
	// Returns true if the circle described by screen coordinates x, y, and radius is visible in the
	// ImGuiIO screen rect.
	bool IsVisibleScreen(float x, float y, float radius = 0.0f);

	bool PartIsHighlighted(int part_idx);
	void SetNetFilter(const char *net);
	void FindComponent(const char *name);
	void SetLastFileOpenName(char *name);
	void FlipBoard();
};
