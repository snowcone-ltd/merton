PLATFORM = $(shell echo $(shell uname -s) | tr A-Z a-z)
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

ifeq ($(PLATFORM), linux)
TARGET = linux
LIBS = -lc -lm
LDFLAGS += -nodefaultlibs
endif

ifeq ($(PLATFORM), darwin)
TARGET = macosx
MIN_VER = 11.0

LIBS = \
	-framework AppKit \
	-framework QuartzCore \
	-framework Carbon \
	-framework GameController \
	-framework Metal \
	-framework IOKit \
	-framework AudioToolbox \
	-framework WebKit

FLAGS += -m$(TARGET)-version-min=$(MIN_VER)
endif

LIBS += ../libmatoya/bin/$(TARGET)/$(ARCH)/libmatoya.a

CFLAGS = $(INCLUDES) $(FLAGS)

all: clean clear zip
	make objs -j4

zip:
	compress/$(TARGET)/$(ARCH)/mcompress ui-zip.h UI_ZIP src/ui

objs: $(OBJS)
	$(CC) -o $(NAME) $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	@rm -rf $(NAME)
	@rm -rf $(OBJS)
	@rm -rf ui-zip.h

clear:
	@clear
