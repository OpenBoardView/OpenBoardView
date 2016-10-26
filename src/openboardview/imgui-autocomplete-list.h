#pragma once

struct ACLState {
	bool isPopupOpen      = false;
	int activeIdx         = -1;    // Index of currently 'active' item by use of up/down keys
	int clickedIdx        = -1;    // Index of popup item clicked with the mouse
	bool selectionChanged = false; // Flag to help focus the correct item when selecting active item
};

struct ACL {
	ACLState state;

	int InputCallback(ImGuiTextEditCallbackData *data);
	void SetInputFromActiveIndex(ImGuiTextEditCallbackData *data, int entryIndex);
	void DrawWindow(ImVec2 &popupPos, ImVec2 &popupSize, bool &isFocused);
	void DrawPopup(ImVec2 pos, ImVec2 size, bool &isFocused);
	void Draw(void);
};
