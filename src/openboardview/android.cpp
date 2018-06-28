#include "platform.h"
#include "BoardView.h"
#include <SDL.h>
#include <jni.h>

/*
 * Because we can't call C++ inside a JNI function
 */
void loadFileWrapper(char* path) {
	std::string paths = std::string(path);
	free(path);
	app.LoadFile(paths);
}

extern "C" {
/*
 * Class:     org_openboardview_openboardview_OBVActivity
 * Method:    openFileWrapper
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_openboardview_openboardview_OBVActivity_openFileWrapper
  (JNIEnv * env, jobject o, jstring filePath) {
	const char *utf = env->GetStringUTFChars(filePath, 0);
	if (utf) {
		char *path = SDL_strdup(utf);
		env->ReleaseStringUTFChars(filePath, utf);
		loadFileWrapper(path);
	}
}

}

const std::string show_file_picker() {
	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();
	jclass activity = env->FindClass("org/openboardview/openboardview/OBVActivity");
	jmethodID openFilePicker= env->GetStaticMethodID(activity, "openFilePicker", "()V");

	env->CallStaticVoidMethod(activity, openFilePicker);

	env->DeleteLocalRef(activity);
	return std::string(); // We have to wait for the result, it will call the above JNI function
}

std::string get_asset_path(const char* asset) {
	std::string path = "/sdcard/openboardview";
	path += "/";
	path += asset;
	return path;
}

// Dummy, there is no proper way to search for or enumerate fonts so force Droid Sans
const std::string get_font_path(const std::string &name) {
	return "/system/fonts/DroidSans.ttf";
}

// Don't care about userdir and put everything in the same dir
// Either appplication external storage if available or internal storage
const std::string get_user_dir(const UserDir userdir) {
	std::string path;
	auto extState = SDL_AndroidGetExternalStorageState();
	if (extState == SDL_ANDROID_EXTERNAL_STORAGE_WRITE)
		path = std::string(SDL_AndroidGetExternalStoragePath());
	else
		path = std::string(SDL_AndroidGetInternalStoragePath());
	if (create_dirs(path))
		return path + "/";
	else
		return "./";
}
