#include "BoardAppearance.h"

#include "imgui/imgui.h"

#include "GUI/DPI.h"
#include "GUI/widgets.h"

namespace Preferences {

BoardAppearance::BoardAppearance(Confparse &obvconfig, Config &config)
	: obvconfig(obvconfig), config(config) {
}

void BoardAppearance::render() {
	RightAlignedText("Annotation flag size", DPI(250));
	ImGui::SameLine();
	ImGui::InputInt("##annotationBoxSize", &config.annotationBoxSize);

	RightAlignedText("Annotation flag offset", DPI(250));
	ImGui::SameLine();
	ImGui::InputInt("##annotationBoxOffset", &config.annotationBoxOffset);

	RightAlignedText("Pin-1/A1 count threshold", DPI(250));
	ImGui::SameLine();
	if (ImGui::InputInt("##pinA1threshold", &config.pinA1threshold)) {
		if (config.pinA1threshold < 1) config.pinA1threshold = 1;
	}

	RightAlignedText("Pin select masks", DPI(250));
	ImGui::SameLine();
	ImGui::Checkbox("##pinSelectMasks", &config.pinSelectMasks);

	RightAlignedText("Pin halo", DPI(250));
	ImGui::SameLine();
	ImGui::Checkbox("##pinHalo", &config.pinHalo);

	RightAlignedText("Halo diameter", DPI(250));
	ImGui::SameLine();
	ImGui::InputFloat("##haloDiameter", &config.pinHaloDiameter);

	RightAlignedText("Halo thickness", DPI(250));
	ImGui::SameLine();
	ImGui::InputFloat("##haloThickness", &config.pinHaloThickness);

	RightAlignedText("Show net web", DPI(250));
	ImGui::SameLine();
	ImGui::Checkbox("##showNetWeb", &config.showNetWeb);

	RightAlignedText("Net web thickness", DPI(250));
	ImGui::SameLine();
	ImGui::InputInt("##netWebThickness", &config.netWebThickness);

	RightAlignedText("Fill parts", DPI(250));
	ImGui::SameLine();
	ImGui::Checkbox("##fillParts", &config.fillParts);

	ImGui::SameLine();
	RightAlignedText("Fill board", DPI(150));
	ImGui::SameLine();
	ImGui::Checkbox("##boardFill", &config.boardFill);

	RightAlignedText("Board fill spacing", DPI(250));
	ImGui::SameLine();
	ImGui::InputInt("##boardFillSpacing", &config.boardFillSpacing);

	RightAlignedText("Show parts name", DPI(250));
	ImGui::SameLine();
	ImGui::Checkbox("##showPartName", &config.showPartName);

	ImGui::SameLine();
	RightAlignedText("Show pins name", DPI(150));
	ImGui::SameLine();
	ImGui::Checkbox("##showPinName", &config.showPinName);
}

} // namespace Preferences
