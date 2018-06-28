LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

REVISION=$(shell (git rev-list HEAD --count)) # Count commits
BRANCH=$(shell (git name-rev --name-only HEAD)) # Get current branch
REPO=$(shell (git config remote.$(git config branch.${BRANCH}.remote).url|cut -d'/' -f 4)) # Get the user from a Github remote
OBV_BUILD=R$(REVISION) $(REPO)/$(BRANCH)

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/..

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../imgui/imgui.cpp \
	../imgui/imgui_draw.cpp \
	android.cpp \
	annotations.cpp \
	confparse.cpp \
	vectorhulls.cpp \
	history.cpp \
	FileFormats/ASCFile.cpp \
	FileFormats/BRD2File.cpp \
	FileFormats/BVRFile.cpp \
	FileFormats/BDVFile.cpp \
	BoardView.cpp \
	BRDBoard.cpp \
	FileFormats/BRDFile.cpp \
	FileFormats/CSTFile.cpp \
	FileFormats/FZFile.cpp \
	main_opengl.cpp \
	NetList.cpp \
	PartList.cpp \
	Searcher.cpp \
	SpellCorrector.cpp \
	unix.cpp \
	utils.cpp \
	Renderers/imgui_impl_sdl_gles2.cpp

LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_STATIC_LIBRARIES := glad sqlite3

LOCAL_LDLIBS := -llog -lz
# See https://code.google.com/p/android/issues/detail?id=68779
LOCAL_LDLIBS += -latomic

LOCAL_CFLAGS += -DImDrawIdx"=unsigned int" -DENABLE_GLES2 -DENABLE_SDL2 -DOBV_BUILD="\"$(OBV_BUILD)\""
LOCAL_CPPFLAGS += -fexceptions

include $(BUILD_SHARED_LIBRARY)
