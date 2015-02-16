LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ti8x
LOCAL_SRC_FILES := ti8x.c TI85.c Z80/Z80.c
LOCAL_LDLIBS    := -llog -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
