LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
LOCAL_MODULE := libmatoya
LOCAL_SRC_FILES := ../libmatoya/bin/android/$(TARGET_ARCH_ABI)/libmatoya.a
include $(PREBUILT_STATIC_LIBRARY)

FLAGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wno-unused-parameter \
	-std=c99 \
	-fPIC

ifdef DEBUG
LOCAL_CLFAGS := $(FLAGS) -O0 -g3
else
LOCAL_CFLAGS := $(FLAGS) -O3 -flto -fvisibility=hidden
LOCAL_LDFLAGS := -flto
endif

LOCAL_MODULE_FILENAME := libmerton
LOCAL_MODULE := libmerton
LOCAL_STATIC_LIBRARIES := libmatoya
LOCAL_LDLIBS := -lGLESv3 -lEGL -landroid -laaudio -llog

LOCAL_C_INCLUDES = \
	. \
	src \
	../libmatoya/src

LOCAL_SRC_FILES := \
	src/main.c \
	src/core.c

include $(BUILD_SHARED_LIBRARY)
