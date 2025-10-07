UNAME_S = $(shell uname -s)
ARCH = $(shell uname -m)
NAME = merton

OBJS = \
	src/csync.o \
	src/rcore.o \
	src/loader.o \
	src/main.o

INCLUDES = \
	-I. \
	-Isrc \
	-I../libmatoya/src

FLAGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wno-unused-parameter \
	-Wno-switch \
	-std=c17

ifdef DEBUG
FLAGS += -O0 -g3
else
FLAGS += -O3 -g0 -flto -fvisibility=hidden
LDFLAGS = -flto=auto
endif

### WASM ###
# github.com/WebAssembly/wasi-sdk/releases -> ~/wasi-sdk

ifdef WASM

WASI_SDK = $(HOME)/wasi-sdk

CC = $(WASI_SDK)/bin/clang

TARGET = web
ARCH := wasm32

LDFLAGS += \
	-Wl,--allow-undefined \
	-Wl,--export-table \
	-Wl,--import-memory,--export-memory,--max-memory=1073741824 \
	-Wl,-z,stack-size=$$((8 * 1024 * 1024))

FLAGS += \
	--sysroot=$(WASI_SDK)/share/wasi-sysroot \
	--target=wasm32-wasi-threads \
	-pthread

else

### LINUX ###
ifeq ($(UNAME_S), Linux)

TARGET = linux

LIBS = \
	-lgcc_s \
	-lc \
	-lm

LDFLAGS += \
	-nodefaultlibs

endif

### APPLE ###
ifeq ($(UNAME_S), Darwin)

ifndef TARGET
TARGET = macosx
endif

ifndef ARCH
ARCH = x86_64
endif

ifeq ($(TARGET), macosx)
MIN_VER = 11.0
else
MIN_VER = 13.0
FLAGS += -fembed-bitcode
endif

LIBS = \
	-framework AppKit \
	-framework QuartzCore \
	-framework Carbon \
	-framework GameController \
	-framework Metal \
	-framework IOKit \
	-framework AudioToolbox \
	-framework WebKit

FLAGS += \
	-m$(TARGET)-version-min=$(MIN_VER) \
	-isysroot $(shell xcrun --sdk $(TARGET) --show-sdk-path) \
	-arch $(ARCH)

LDFLAGS += \
	-arch $(ARCH)

endif
endif

STATIC_LIBS = \
	../libmatoya/bin/$(TARGET)/$(ARCH)/libmatoya.a

CFLAGS = $(INCLUDES) $(FLAGS)

all: clean clear zip
	make objs -j4

zip:
	compress/$(TARGET)/$(ARCH)/mcompress ui-zip.h UI_ZIP src/ui

objs: $(OBJS)
	$(CC) -o $(NAME) $(OBJS) $(STATIC_LIBS) $(LIBS) $(LDFLAGS)

### ANDROID ###
# developer.android.com/ndk/downloads -> ~/android-ndk

ifndef ANDROID_NDK_ROOT
ANDROID_NDK_ROOT = $(HOME)/android-ndk
endif

ifndef ABI
ABI = all
endif

android: clean clear $(SHADERS)
	@$(ANDROID_NDK_ROOT)/ndk-build -j4 \
		APP_BUILD_SCRIPT=Android.mk \
		APP_PLATFORM=android-28 \
		APP_ABI=$(ABI) \
		NDK_PROJECT_PATH=. \
		NDK_OUT=android/build \
		NDK_LIBS_OUT=android/app/libs \
		--no-print-directory

clean:
	@rm -rf android/build
	@rm -rf android/app/libs
	@rm -rf $(NAME)
	@rm -rf $(OBJS)
	@rm -rf ui-zip.h

clear:
	@clear
