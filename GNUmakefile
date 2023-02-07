UNAME_S = $(shell uname -s)
ARCH = $(shell uname -m)
BIN_NAME = merton

.m.o:
	$(CC) $(OCFLAGS)  -c -o $@ $<

OBJS = \
	src/main.o \
	src/core.o

FLAGS = \
	-Wall \
	-Wextra \
	-Wno-unused-function \
	-Wno-unused-result \
	-Wno-unused-value \
	-Wno-unused-parameter

INCLUDES = \
	-I. \
	-Isrc \
	-Isrc/app/deps \
	-I../libmatoya/src

DEFS = \
	-D_POSIX_C_SOURCE=200112L

############
### WASM ###
############
ifdef WASM

WASI_SDK = $(HOME)/wasi-sdk-12.0

LD_FLAGS := \
	-Wl,--allow-undefined \
	-Wl,--export-table \
	-Wl,-z,stack-size=$$((8 * 1024 * 1024))

CC = $(WASI_SDK)/bin/clang --sysroot=$(WASI_SDK)/share/wasi-sysroot
CXX = $(CC)

OS = web
ARCH := wasm32

else
#############
### LINUX ###
#############
ifeq ($(UNAME_S), Linux)

LD_FLAGS = \
	-nodefaultlibs

LIBS = \
	-ldl \
	-lpthread \
	-lm \
	-lc \
	-lgcc_s \
	-latomic

OS = linux
endif

#############
### MACOS ###
#############
ifeq ($(UNAME_S), Darwin)

LIBS = \
	-lc \
	-framework AppKit \
	-framework QuartzCore \
	-framework Carbon \
	-framework OpenGL \
	-framework Metal \
	-framework IOKit \
	-framework AudioToolbox \
	-framework Security \
	-framework WebKit

OS = macosx

FLAGS := $(FLAGS) \
	-mmacosx-version-min=10.14 \
	-isysroot $(shell xcrun --sdk macosx --show-sdk-path) \
	-arch x86_64
endif
endif

LIBS := ../libmatoya/bin/$(OS)/$(ARCH)/libmatoya.a $(LIBS)

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g
else
FLAGS := $(FLAGS) -O3 -fvisibility=hidden
ifdef LTO
FLAGS := $(FLAGS) -flto
LD_FLAGS := $(LD_FLAGS) -flto
endif
endif

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS) -std=c99
OCFLAGS = $(CFLAGS) -fobjc-arc

all: clean clear
	make objs -j4

web: all
	ln -sf ../libmatoya
	python3 -m http.server

objs: $(OBJS)
	$(CC) -o $(BIN_NAME) $(OBJS) $(LIBS) $(LD_FLAGS)

###############
### ANDROID ###
###############

### Downloads ###
# sudo apt install openjdk-11-jre-headless
# https://developer.android.com/ndk/downloads -> Put in ~/android-ndk-xxx
# https://developer.android.com/studio#command-tools -> Put in ~/android-home/cmdline-tools

### Licenses ###
# Run 'ANDROID_SDK_ROOT/tools/bin/sdkmanager --licenses' to accept licenses

### Windows ###
# https://developer.android.com/studio/run/win-usb -> This may help
# https://developer.android.com/studio/releases/platform-tools
# If using WSL, adb.exe must be run to connect to the device on the Windows size,
# then WSL can communicate with it over the local network

ANDROID_PROJECT = android
ANDROID_PACKAGE = tmp.domain.$(BIN_NAME)

ANDROID_NDK = $(HOME)/android-ndk-r21d
export ANDROID_HOME = $(HOME)/android-home
export ANDROID_SDK_ROOT = $(ANDROID_HOME)/cmdline-tools

gradle:
	@java \
		-classpath $(ANDROID_PROJECT)/gradle/wrapper/gradle-wrapper.jar \
		org.gradle.wrapper.GradleWrapperMain \
		-p $(ANDROID_PROJECT) \
		$(CMD)

ndk:
	@mkdir -p $(ANDROID_PROJECT)/app/libs
	@$(ANDROID_NDK)/ndk-build -j4 \
		NDK_PROJECT_PATH=. \
		APP_BUILD_SCRIPT=Android.mk \
		APP_OPTIM=release \
		APP_PLATFORM=android-26 \
		NDK_OUT=$(ANDROID_PROJECT)/build \
		NDK_LIBS_OUT=$(ANDROID_PROJECT)/app/libs \
		--no-print-directory \
		| grep -v 'fcntl(): Operation not supported'

adb-run:
	@$(ANDROID_HOME)/platform-tools/adb logcat -c
	@$(ANDROID_HOME)/platform-tools/adb \
		shell am start -n $(ANDROID_PACKAGE)/$(ANDROID_PACKAGE).MainActivity

adb-kill:
	@$(ANDROID_HOME)/platform-tools/adb shell am force-stop $(ANDROID_PACKAGE)

adb-log:
	@$(ANDROID_HOME)/platform-tools/adb logcat $(ANDROID_PACKAGE) | grep ' E \|MTY'

android: clear
	@make --no-print-directory adb-kill
	@make --no-print-directory ndk
	@make --no-print-directory gradle CMD=installDebug
	@make --no-print-directory adb-run
	@make --no-print-directory adb-log

clean:
	@rm -rf libmatoya
	@rm -rf $(ANDROID_PROJECT)/build
	@rm -rf $(BIN_NAME)
	@rm -rf $(OBJS)

clear:
	@clear
