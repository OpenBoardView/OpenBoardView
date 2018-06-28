SRC_PATH := $(abspath $(call my-dir)/../..)

include $(SRC_PATH)/openboardview/Android.mk
include $(SRC_PATH)/SDL2/Android.mk
include $(SRC_PATH)/sqlite3/Android.mk
include jni/glad/Android.mk

include $(CLEAR_VARS)
