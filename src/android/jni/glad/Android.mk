LOCAL_PATH := $(call my-dir)
SRC_PATH := $(abspath $(call my-dir)/../../..)

# Generate glad, only one time, says for armeabi
ifeq ($(TARGET_ARCH_ABI),armeabi)
$(info $(shell (${LOCAL_PATH}/gen_glad.sh "$(SRC_PATH)" "$(abspath ${LOCAL_PATH})")))
endif

###########################
#
# Glad
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := glad

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	src/glad.c

include $(BUILD_STATIC_LIBRARY)
