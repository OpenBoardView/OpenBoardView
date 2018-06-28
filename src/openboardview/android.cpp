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

unsigned char *LoadAsset(int *asset_size, int asset_id) {
	char *filename;
	if (asset_id == ASSET_FIRA_SANS)
		filename = "FiraSans-Medium.ttf";

	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	if (!file) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[OBV] Failed to load asset file '%s': %s", filename, SDL_GetError());
		return NULL;
	}

	size_t res_size = SDL_RWsize(file);

	unsigned char *res = new unsigned char[res_size+1]; // allow an extra character

	size_t nb_read_total = 0, nb_read = 1;
	unsigned char* buf = res;
	while (nb_read_total < res_size && nb_read != 0) {
		nb_read = SDL_RWread(file, buf, 1, (res_size - nb_read_total));
		nb_read_total += nb_read;
		buf += nb_read;
	}
	SDL_RWclose(file);
	if (nb_read_total != res_size) {
		free(res);
		return NULL;
	}

	*asset_size = nb_read_total;
	return res;
}
