#include "BackgroundImage.h"

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "platform.h"

namespace Preferences {

BackgroundImage::BackgroundImage(::BackgroundImage &backgroundImage) : backgroundImage(backgroundImage) {
}

void BackgroundImage::errorPopup() {
	if (ImGui::BeginPopupModal("Error##PreferencesBackgroundImage")) {

		ImGui::Text("There was an error while opening the selected image file(s)");
		for (const auto &filename : erroredFiles) {
			ImGui::Text("%s", filename.c_str());
		}
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			erroredFiles.clear(); // clear error flag once popup is closed;
		}

		ImGui::EndPopup();
	}
}

void BackgroundImage::imageSettings(const std::string &name, Image &image) {
	ImGui::Text("%s image settings", name.c_str());
	std::string filename = image.file.string();
	if (ImGui::InputText((name + " file").c_str(), &filename)) {
		image.file = filename;
	}
	ImGui::SameLine();
	if (ImGui::Button(("Browse##" + name).c_str())) {
		auto path = show_file_picker();
		if (!path.empty()) {
			image.file = path;
			filename = path.string();
			std::string error = image.reload();
			if (!error.empty()) {
				erroredFiles.push_back(error);
			}
		}
	}

	ImGui::InputInt(("X offset##" + name).c_str(), &image.offsetX);
	ImGui::SameLine();
	ImGui::InputInt(("Y offset##" + name).c_str(), &image.offsetY);

	ImGui::InputFloat(("X scaling##" + name).c_str(), &image.scalingX);
	ImGui::SameLine();
	ImGui::InputFloat(("Y scaling##" + name).c_str(), &image.scalingY);

	ImGui::Checkbox(("Flip horizontal##" + name).c_str(), &image.mirrorX);
	ImGui::SameLine();
	ImGui::Checkbox(("Flip vertical##" + name).c_str(), &image.mirrorY);

	ImGui::SliderFloat(("Transparency##" + name).c_str(), &image.transparency, 0.0f, 1.0f);
}

void BackgroundImage::save() {
	backgroundImage.writeToConfig(backgroundImage.configFilepath);
	std::string error = backgroundImage.topImage.reload();
	if (!error.empty()) {
		erroredFiles.push_back(error);
	}
	error = backgroundImage.bottomImage.reload();
	if (!error.empty()) {
		erroredFiles.push_back(error);
	}
}

void BackgroundImage::cancel() {
	backgroundImage = backgroundImageCopy;
	backgroundImage.reload(); // don't care if there is an error here since the user cancelled
}

void BackgroundImage::clear() {
	backgroundImage.topImage = {};
	backgroundImage.bottomImage = {};
	backgroundImage.reload();
}

void BackgroundImage::render(bool shown) {
	static bool wasShown = false;

	if (shown) {
		if (!wasShown) { // Panel just got opened
			erroredFiles.clear(); // make sure to clean the errors
			backgroundImageCopy = backgroundImage; // Make a copy to be able to restore if cancelled
		}

		imageSettings("Top", backgroundImage.topImage);

		ImGui::Separator();
		imageSettings("Bottom", backgroundImage.bottomImage);
	}


	if (!erroredFiles.empty()) {
		ImGui::OpenPopup("Error##PreferencesBackgroundImage"); // Open error popup if there was an error
	}
	errorPopup(); // Render error popup if opened

	wasShown = shown;
}

} // namespace Preferences

