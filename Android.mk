LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
LOCAL_MODULE := libmatoya
LOCAL_SRC_FILES := ../libmatoya/bin/android/$(TARGET_ARCH_ABI)/libmatoya.a
include $(PREBUILT_STATIC_LIBRARY)

FLAGS = \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-switch \
	-Wno-atomic-alignment \
	-Wno-unused-function \
	-fPIC

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g
else
FLAGS := $(FLAGS) -O3 -fvisibility=hidden
ifdef LTO
FLAGS := $(FLAGS) -flto
endif
endif

LOCAL_MODULE_FILENAME := libmerton
LOCAL_MODULE := libmerton
LOCAL_STATIC_LIBRARIES := libmatoya
LOCAL_LDLIBS := -lGLESv3 -lEGL -landroid -laaudio -llog

LOCAL_C_INCLUDES = \
	. \
	src \
	src/app/deps \
	../libmatoya/src

DEFS = \
	-DGL_ES \
	-DMTY_GL_EXTERNAL \
	-D_POSIX_C_SOURCE=200112L

LOCAL_CFLAGS = $(DEFS) $(FLAGS)

LOCAL_SRC_FILES := \
	src/nes/cart.c \
	src/nes/apu.c \
	src/nes/sys.c \
	src/nes/cpu.c \
	src/nes/ppu.c \
	src/app/main.c

include $(BUILD_SHARED_LIBRARY)
