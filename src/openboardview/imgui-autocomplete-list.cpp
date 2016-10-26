#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "imgui-autocomplete-list.h"
#include "imgui/imgui.h"

const int ENTRY_COUNT            = 10;
const char *ENTRIES[ENTRY_COUNT] = {
    "Entry 0", "Entry 1", "Entry 2", "Entry 3", "Entry 4", "Entry 5", "Entry 6", "Entry 7", "Entry 8", "Entry 9"};

//-----------------------------------------------------------
void ACL::SetInputFromActiveIndex(ImGuiTextEditCallbackData *data, int entryIndex) {
	// ACLState& state = *reinterpret_cast<ACLState*>( data->UserData );

	const char *entry   = ENTRIES[entryIndex];
	const size_t length = strlen(entry);

	memmove(data->Buf, entry, length + 1);

	data->BufTextLen = (int)length;
	data->BufDirty   = true;
}

//-----------------------------------------------------------
int ACL::InputCallback(ImGuiTextEditCallbackData *data) {
	ACLState &state = *reinterpret_cast<ACLState *>(data->UserData);

	switch (data->EventFlag) {
		case ImGuiInputTextFlags_CallbackCompletion:

			if (state.isPopupOpen && state.activeIdx != -1) {
				// Tab was pressed, grab the item's text
				SetInputFromActiveIndex(data, state.activeIdx);
			}

			state.isPopupOpen = false;
			state.activeIdx   = -1;
			state.clickedIdx  = -1;

			break;

		case ImGuiInputTextFlags_CallbackHistory:

			state.isPopupOpen = true;

			if (data->EventKey == ImGuiKey_UpArrow && state.activeIdx > 0) {
				state.activeIdx--;
				state.selectionChanged = true;
			} else if (data->EventKey == ImGuiKey_DownArrow && state.activeIdx < (ENTRY_COUNT - 1)) {
				state.activeIdx++;
				state.selectionChanged = true;
			}

			break;

		case ImGuiInputTextFlags_CallbackAlways:

			if (state.clickedIdx != -1) {
				// The user has clicked an item, grab the item text
				SetInputFromActiveIndex(data, state.clickedIdx);

				// Hide the popup
				state.isPopupOpen = false;
				state.activeIdx   = -1;
				state.clickedIdx  = -1;
			}

			break;

		case ImGuiInputTextFlags_CallbackCharFilter: break;
	}

	return 0;
}

//-----------------------------------------------------------
void ACL::DrawWindow(ImVec2 &popupPos, ImVec2 &popupSize, bool &isFocused) {
	ImGuiWindowFlags winFlags = 0;

	// Set this flag in order to be able to show the popup on top
	// of the window. This is only reakky necessarry if you want to
	// show the popup over the window region
	if (state.isPopupOpen) winFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

	/// Begin main window
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("DeveloperConsole", nullptr, winFlags)) {
		ImGui::End();
		return;
	}

	/// Draw any window content here
	{
		ImGui::BeginChild(
		    "scrollRegion", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);

		// Draw entries
		for (int i = 0; i < 3; i++) ImGui::Text("Foo %d", i);

		ImGui::EndChild();
	}

	/// Input Box
	{
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways |
		                            ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackCompletion |
		                            ImGuiInputTextFlags_CallbackHistory;

		const size_t INPUT_BUF_SIZE          = 256;
		static char inputBuf[INPUT_BUF_SIZE] = {0};

		if (ImGui::InputText("Input", inputBuf, INPUT_BUF_SIZE, flags, &ACL::InputCallback, &state)) {
			ImGui::SetKeyboardFocusHere(-1);

			if (state.isPopupOpen && state.activeIdx != -1) {
				// This means that enter was pressed whilst a
				// the popup was open and we had an 'active' item.
				// So we copy the entry to the input buffer here
				const char *entry   = ENTRIES[state.activeIdx];
				const size_t length = strlen(entry);

				memmove(inputBuf, entry, length + 1);
			} else {
				// Handle text input here
				inputBuf[0] = '\0';
			}

			// Hide popup
			state.isPopupOpen = false;
			state.activeIdx   = -1;
		}

		// Restore focus to the input box if we just clicked an item
		if (state.clickedIdx != -1) {
			ImGui::SetKeyboardFocusHere(-1);

			// NOTE: We do not reset the 'clickedIdx' here because
			// we want to let the callback handle it in order to
			// modify the buffer, therefore we simply restore keyboard input instead
			state.isPopupOpen = false;
		}

		// Get input box position, so we can place the popup under it
		popupPos = ImGui::GetItemRectMin();

		// Based on Omar's developer console demo: Retain focus on the input box
		if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		// Grab the position for the popup
		popupSize = ImVec2(ImGui::GetItemRectSize().x - 60, ImGui::GetItemsLineHeightWithSpacing() * 4);
		popupPos.y += ImGui::GetItemRectSize().y;
	}

	isFocused = ImGui::IsRootWindowFocused();

	ImGui::End();
}

//-----------------------------------------------------------
void ACL::DrawPopup(ImVec2 pos, ImVec2 size, bool &isFocused) {
	if (!state.isPopupOpen) return;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                         ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::Begin("input_popup", nullptr, flags);
	ImGui::PushAllowKeyboardFocus(false);

	for (int i = 0; i < ENTRY_COUNT; i++) {
		// Track if we're drawing the active index so we
		// can scroll to it if it has changed
		bool isIndexActive = state.activeIdx == i;

		if (isIndexActive) {
			// Draw the currently 'active' item differently
			// ( used appropriate colors for your own style )
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 0, 0, 1));
		}

		ImGui::PushID(i);
		if (ImGui::Selectable(ENTRIES[i], isIndexActive)) {
			// And item was clicked, notify the input
			// callback so that it can modify the input buffer
			state.clickedIdx = i;
		}
		ImGui::PopID();

		if (isIndexActive) {
			if (state.selectionChanged) {
				// Make sure we bring the currently 'active' item into view.
				ImGui::SetScrollHere();
				state.selectionChanged = false;
			}

			ImGui::PopStyleColor(1);
		}
	}

	isFocused = ImGui::IsRootWindowFocused();

	ImGui::PopAllowKeyboardFocus();
	ImGui::End();
	ImGui::PopStyleVar(1);
}

//-----------------------------------------------------------
void ACL::Draw() {
	/*
	static ACLState state =
	{
	    false,  // Not open initially
	    -1,     // No index active
	    -1,     // No index clicked
	    false
	};
	*/

	ImVec2 popupPos, popupSize;
	bool isWindowFocused, isPopupFocused;

	// Draw the main window
	DrawWindow(popupPos, popupSize, isWindowFocused);

	// Draw the popup window
	DrawPopup(popupPos, popupSize, isPopupFocused);

	// If neither of the windows has focus, hide the popup
	if (!isWindowFocused && !isPopupFocused) {
		state.isPopupOpen = false;
	}
}
