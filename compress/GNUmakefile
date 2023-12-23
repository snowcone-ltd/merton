UNAME_S = $(shell uname -s)
ARCH = $(shell uname -m)
NAME = mcompress

OBJS = \
	main.o

INCLUDES = \
	-I../../libmatoya/src

FLAGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wno-unused-parameter \
	-std=c99 \
	-fPIC

#############
### LINUX ###
#############
ifeq ($(UNAME_S), Linux)

TARGET = linux

LIBS = \
	-lc \
	-lm

LD_FLAGS = \
	-nodefaultlibs

endif

#############
### APPLE ###
#############
ifeq ($(UNAME_S), Darwin)

ifndef TARGET
TARGET = macosx
endif

ifndef ARCH
ARCH = x86_64
endif

ifeq ($(TARGET), macosx)
MIN_VER = 10.15
else
MIN_VER = 13.0
FLAGS := $(FLAGS) -fembed-bitcode
endif

LIBS = \
	-framework CoreFoundation

FLAGS := $(FLAGS) \
	-m$(TARGET)-version-min=$(MIN_VER) \
	-isysroot $(shell xcrun --sdk $(TARGET) --show-sdk-path) \
	-arch $(ARCH)

endif

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g3
else
FLAGS := $(FLAGS) -O3 -g0 -flto -fvisibility=hidden
LD_FLAGS := $(LD_FLAGS) -flto
endif

STATIC_LIBS = \
	../../libmatoya/bin/$(TARGET)/$(ARCH)/libmatoya.a

CFLAGS = $(INCLUDES) $(FLAGS)

all: clean clear
	make objs -j4

objs: $(OBJS)
	$(CC) -o $(TARGET)/$(NAME) $(OBJS) $(STATIC_LIBS) $(LIBS) $(LD_FLAGS)

clean:
	@rm -rf android/build
	@rm -rf android/app/libs
	@rm -rf $(TARGET)/$(NAME)
	@rm -rf $(OBJS)

clear:
	@clear
