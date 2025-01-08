#pragma once

#include "Board.h"
#include "Searcher.h"
#include "SpellCorrector.h"
#include "annotations.h"
#include "confparse.h"
#include "history.h"
#include "imgui/imgui.h"
#include "UI/Keyboard/KeyBindings.h"
#include "GUI/Config.h"
#include "GUI/ColorScheme.h"
#include "GUI/Help/About.h"
#include "GUI/Help/Controls.h"
#include "GUI/Preferences/Color.h"
#include "GUI/Preferences/Keyboard.h"
#include "GUI/Preferences/Program.h"
#include "GUI/BackgroundImage.h"
#include "GUI/Preferences/BoardSettings/BoardSettings.h"
#include "PDFBridge/PDFBridge.h"
#include "PDFBridge/PDFBridgeEvince.h"
#include "PDFBridge/PDFBridgeSumatra.h"
#include "PDFBridge/PDFFile.h"
#include <cstdint>
#include <vector>

struct BRDPart;
class BRDFile;

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
		uint32_t bit   = (1u << (index & 0x1f));
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

// enum DrawChannel { kChannelImages = 0, kChannelFill, kChannelPolylines = 1, kChannelPins = 2, kChannelText = 3,
// kChannelAnnotations = 4, NUM_DRAW_CHANNELS = 5 };
enum DrawChannel {
	kChannelImages = 0,
	kChannelFill,
	kChannelPolylines,
	kChannelPins,
	kChannelText,
	kChannelAnnotations,
	NUM_DRAW_CHANNELS
};

enum FlipModes { flipModeVP = 0, flipModeMP = 1, NUM_FLIP_MODES };

struct BoardView {
	BRDFileBase *m_file;
	Board *m_board;
	BackgroundImage backgroundImage{m_current_side};

	Confparse obvconfig;
	FHistory fhistory;
	Searcher searcher;
	SpellCorrector scnets;
	SpellCorrector scparts;
	KeyBindings keybindings;
	Config config;
	Preferences::Program programPreferences{keybindings, obvconfig, config, *this};
	Preferences::Color colorPreferences{keybindings, obvconfig, m_colors};
	Preferences::Keyboard keyboardPreferences{keybindings, obvconfig};
	Preferences::BoardSettings boardSettings{keybindings, backgroundImage, pdfFile};

	Help::About helpAbout{keybindings};
	Help::Controls helpControls{keybindings};

#ifdef ENABLE_PDFBRIDGE_EVINCE
	PDFBridgeEvince pdfBridge;
#elif defined(_WIN32)
	PDFBridgeSumatra &pdfBridge = PDFBridgeSumatra::GetInstance(obvconfig);
#else
	PDFBridge pdfBridge; // Dummy implementation
#endif
	PDFFile pdfFile{pdfBridge};

	bool debug                   = false;
	int history_file_has_changed = 0;
	bool boardMinMaxDone      = false;

	bool reloadConfig  = false;
	bool reloadFonts  = false;
	int pinBlank       = 0;

	int ConfigParse(void);
	void CenterView(void);
	void Pan(int direction, int amount);
	void Zoom(float osd_x, float osd_y, float zoom);
	void Mirror(void);
	void DrawDiamond(ImDrawList *draw, ImVec2 c, double r, uint32_t color);
	void DrawHex(ImDrawList *draw, ImVec2 c, double r, uint32_t color);
	void DrawBox(ImDrawList *draw, ImVec2 c, double r, uint32_t color);
	template <class T>
	void ShowSearchResults(std::vector<T> results, char *search, int &limit, void (BoardView::*onSelect)(const char *));
	void SearchColumnGenerate(const std::string &title,
	                          std::pair<SharedVector<Component>, SharedVector<Net>> results,
	                          char *search,
	                          int limit);
	void Preferences(void);
	bool AnyItemVisible(void);
	void ThemeSetStyle(const char *name);

	void CenterZoomNet(std::string netname);

	void CenterZoomSearchResults(void);
	int EPCCheck(void);
	void OutlineGenFillDraw(ImDrawList *draw, int ydelta, double thickness);

	/* Context menu, sql stuff */
	Annotations m_annotations;
	void ContextMenu(void);
	int AnnotationIsHovered(void);
	bool AnnotationWasHovered     = false;
	bool m_annotationnew_retain   = false;
	bool m_annotationedit_retain  = false;
	int m_annotation_last_hovered = 0;
	int m_annotation_clicked_id   = 0;
	int m_hoverframes             = 0;
	ImVec2 m_previous_mouse_pos;

	/* Info/Side Pane */
	void ShowInfoPane(void);

	bool HighlightedPinIsHovered(void);
	std::shared_ptr<Pin> m_pinHighlightedHovered    = nullptr;
	std::shared_ptr<Pin> currentlyHoveredPin        = nullptr;
	std::shared_ptr<Component> currentlyHoveredPart = nullptr;

	ImVec2 m_showContextMenuPos;

	std::shared_ptr<Pin> m_pinSelected = nullptr;
	//	vector<Net *> m_netHiglighted;
	SharedVector<Pin> m_pinHighlighted;
	SharedVector<Component> m_partHighlighted;
	char m_cachedDrawList[sizeof(ImDrawList)];
	ImVector<char> m_cachedDrawCommands;
	SharedVector<Net> m_nets;
	int m_active_search_column = 0;
	char m_search[3][128];
	char m_netFilter[128];
	std::string m_lastFileOpenName;
	float m_dx; // display top-right coordinate?
	float m_dy;
	float m_mx; // board MID POINTS
	float m_my;
	float m_scale       = 1.0f;
	float m_scale_floor = 1.0f; // scale which displays the entire board
	float m_lastWidth;          // previously checked on-screen window size; use to redraw
	                            // when window is resized?
	float m_lastHeight;
	int m_rotation; // set to 0 for original orientation [0-4]
	int m_current_side;
	int m_boardWidth; // board size in what coordinates? thou?
	int m_boardHeight;
	float m_menu_height;
	float m_status_height;
	ImVec2 m_board_surface;
	ImVec2 m_info_surface;
	int m_dragging_token = 0; // 1 = board window, 2 = side pane

	ColorScheme m_colors;

	// TODO: save settings to disk
	// pinDiameter: diameter for all pins.  Unit scale: 1 = 0.025mm, boards are
	// done in "thou" (1/1000" = 0.0254mm)
	int m_pinDiameter     = 20;
	bool m_flipVertically = true;

	// Annotation layer specific
	bool m_annotationsVisible = true;

	// Do a hacky thing (memcpy the window draw list) to save us from rerendering
	// the board
	// The app will crash or break if this flag is not set when it should be.
	bool m_needsRedraw = true;
	bool m_draggingLastFrame;
	bool m_showContextMenu;
	//	bool m_showNetfilterSearch;
	bool m_showSearch;
	bool m_searchComponents = true;
	bool m_searchNets       = true;
	bool m_showNetList;
	bool m_showPartList;
	bool m_showPreferences;
	bool m_showColorPreferences;
	bool m_firstFrame = true;
	bool m_lastFileOpenWasInvalid;
	bool m_validBoard = false;
	bool m_wantsQuit;

	std::string m_error_msg;

	~BoardView();

	void ShowNetList(bool *p_open);
	void ShowPartList(bool *p_open);

	void Update();
	void HandleInput();
	void RenderOverlay();
	void DrawPartTooltips(ImDrawList *draw);
	void DrawPinTooltips(ImDrawList *draw);
	void DrawAnnotations(ImDrawList *draw);
	void DrawOutline(ImDrawList *draw);
	void DrawOutlinePoints(ImDrawList *draw);
	void DrawOutlineSegments(ImDrawList *draw);
	void DrawPins(ImDrawList *draw);
	void DrawParts(ImDrawList *draw);
	void DrawBoard();
	void DrawNetWeb(ImDrawList *draw);
	void LoadBoard(BRDFileBase *file);
	int LoadFile(const filesystem::path &filepath);
	ImVec2 CoordToScreen(float x, float y, float w = 1.0f);
	ImVec2 ScreenToCoord(float x, float y, float w = 1.0f);
	// void Move(float x, float y);
	void Rotate(int count);
	void DrawSelectedPins(ImDrawList *draw);
	void ResetSearch();
	void ClearAllHighlights(void);

	// Sets the center of the screen to (x,y) in board space
	void SetTarget(float x, float y);

	// Returns true if the part is shown on the currently displayed side of the
	// board.
	bool BoardElementIsVisible(const std::shared_ptr<BoardElement> be);
	bool IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io);
	// Returns true if the circle described by screen coordinates x, y, and radius
	// is visible in the
	// ImGuiIO screen rect.
	// bool IsVisibleScreen(float x, float y, float radius = 0.0f);

	bool PartIsHighlighted(const std::shared_ptr<Component> component);
	void FindNet(const char *net);
	void FindNetNoClear(const char *name);
	void FindComponent(const char *name);
	void FindComponentNoClear(const char *name);
	void SearchComponent(void);
	void SearchNetNoClear(const char *net);
	void SearchCompound(const char *item);
	void SearchCompoundNoClear(const char *item);
	std::pair<SharedVector<Component>, SharedVector<Net>> SearchPartsAndNets(const char *search, int limit);

	void SetLastFileOpenName(const std::string &name);
	void FlipBoard(int mode = 0);
	void HandlePDFBridgeSelection();
};
