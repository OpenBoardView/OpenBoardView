# ndk r12 toolchain
NDK_TOOLCHAIN_VERSION := 4.9

# libc++ required for C++11, use static for older Android
APP_STL := c++_static
APP_CPPFLAGS += -std=c++11
APP_ABI := armeabi armeabi-v7a x86 # Warning: if you remove armeabi, adjust glad/Android.mk
