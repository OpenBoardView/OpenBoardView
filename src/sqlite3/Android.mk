LOCAL_PATH := $(call my-dir)

###########################
#
# SQLite3
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := sqlite3

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	sqlite3.c

include $(BUILD_STATIC_LIBRARY)
