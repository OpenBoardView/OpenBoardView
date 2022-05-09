#include "platform.h"

#include "BackgroundImage.h"

#include "imgui/imgui.h"
#include "confparse.h"


#if __cplusplus < 201703L
constexpr const BackgroundImage::Side BackgroundImage::defaultSide; // c++ <17 need definition in .cpp
#endif

BackgroundImage::BackgroundImage(const Side &side) : side(&side) {
}

BackgroundImage::BackgroundImage(const int &side) : side(reinterpret_cast<const Side*>(&side)) {
}

void BackgroundImage::loadFromConfig(const filesystem::path &filepath) {
	configFilepath = filepath; // save filepath for latter use with writeToConfig

	if (!filesystem::exists(filepath)) // Config file doesn't exist, do not attempt to read or write it and load images
		return;

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	Confparse confparse{};
	confparse.Load(filepath);

	std::string topImageFilename{confparse.ParseStr("TopImageFile", "")};
	topImage = Image{topImageFilename.empty() ? filesystem::path{} : configDir/filesystem::u8path(topImageFilename)};
	topImage.offsetX = confparse.ParseInt("TopImageOffsetX", 0);
	topImage.offsetY = confparse.ParseInt("TopImageOffsetY", 0);
	topImage.scalingX = confparse.ParseDouble("TopImageScalingX", 1.0);
	topImage.scalingY = confparse.ParseDouble("TopImageScalingY", 1.0);
	topImage.mirrorX = confparse.ParseBool("TopImageMirrorX", false);
	topImage.mirrorY = confparse.ParseBool("TopImageMirrorY", false);
	topImage.transparency = confparse.ParseDouble("TopImageTransparency", 0.0);

	std::string bottomImageFilename{confparse.ParseStr("BottomImageFile", "")};
	bottomImage = Image{bottomImageFilename.empty() ? filesystem::path{} : configDir/filesystem::u8path(bottomImageFilename)};
	bottomImage.offsetX = confparse.ParseInt("BottomImageOffsetX", 0);
	bottomImage.offsetY = confparse.ParseInt("BottomImageOffsetY", 0);
	bottomImage.scalingX = confparse.ParseDouble("BottomImageScalingX", 1.0);
	bottomImage.scalingY = confparse.ParseDouble("BottomImageScalingY", 1.0);
	bottomImage.mirrorX = confparse.ParseBool("BottomImageMirrorX", false);
	bottomImage.mirrorY = confparse.ParseBool("BottomImageMirrorY", false);
	bottomImage.transparency = confparse.ParseDouble("BottomImageTransparency", 0.0);

	writeToConfig(filepath);
	reload();
}

void BackgroundImage::writeToConfig(const filesystem::path &filepath) {
	if (filepath.empty()) // No destination file to save to
		return;

	std::error_code ec;
	auto confparse = Confparse{};
	confparse.Load(filepath);

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	if (!topImage.file.empty()) {
		auto topImagePath = filesystem::relative(topImage.file, configDir, ec);
		if (ec) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error writing top image path: %d - %s", ec.value(), ec.message().c_str());
		} else {
			confparse.WriteStr("TopImageFile", topImagePath.generic_string().c_str());
		}
	}
	confparse.WriteInt("TopImageOffsetX", topImage.offsetX);
	confparse.WriteInt("TopImageOffsetY", topImage.offsetY);
	confparse.WriteFloat("TopImageScalingX", topImage.scalingX);
	confparse.WriteFloat("TopImageScalingY", topImage.scalingY);
	confparse.WriteBool("TopImageMirrorX", topImage.mirrorX);
	confparse.WriteBool("TopImageMirrorY", topImage.mirrorY);
	confparse.WriteFloat("TopImageTransparency", topImage.transparency);

	if (!bottomImage.file.empty()) {
		auto bottomImagePath = filesystem::relative(bottomImage.file, configDir, ec);
		if (ec) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error writing bottom image path: %d - %s", ec.value(), ec.message().c_str());
		} else {
			confparse.WriteStr("BottomImageFile", bottomImagePath.generic_string().c_str());
		}
	}
	confparse.WriteInt("BottomImageOffsetX", bottomImage.offsetX);
	confparse.WriteInt("BottomImageOffsetY", bottomImage.offsetY);
	confparse.WriteFloat("BottomImageScalingX", bottomImage.scalingX);
	confparse.WriteFloat("BottomImageScalingY", bottomImage.scalingY);
	confparse.WriteBool("BottomImageMirrorX", bottomImage.mirrorX);
	confparse.WriteBool("BottomImageMirrorY", bottomImage.mirrorY);
	confparse.WriteFloat("BottomImageTransparency", bottomImage.transparency);
}

std::string BackgroundImage::reload() {
	error = topImage.reload();
	if (!error.empty()) {
		error += "\n";
	}
	error += bottomImage.reload();

	return error;
}

const Image &BackgroundImage::selectedImage() const {
	switch (*side) {
		case Side::TOP:
			return topImage;
		case Side::BOTTOM:
		default:
			return bottomImage;
	}
}

void BackgroundImage::render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) {
	if (!enabled)
		return;

	if (ImGui::BeginPopupModal("Error while loading background image")) {
		ImGui::Text("There was an error while opening background image file(s)");
		ImGui::Text("%s", error.c_str());
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
			error.clear(); // clear errors once popup is closed
		}
		ImGui::EndPopup();
	}
	if (!error.empty()) {
		ImGui::OpenPopup("Error while loading background image"); // Open error popup if there was an error
	}

	selectedImage().render(draw, p_min, p_max, rotation);
}

float BackgroundImage::x0() const {
	return selectedImage().x0();
}

float BackgroundImage::y0() const {
	return selectedImage().y0();
}

float BackgroundImage::x1() const {
	return selectedImage().x1();
}

float BackgroundImage::y1() const {
	return selectedImage().y1();
}

