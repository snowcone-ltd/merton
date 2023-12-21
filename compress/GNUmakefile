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

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g3
else
FLAGS := $(FLAGS) -O3 -g0 -flto -fvisibility=hidden
LD_FLAGS := -flto
endif

TARGET = linux

LIBS = \
	-lc \
	-lm

LD_FLAGS = \
	-nodefaultlibs

STATIC_LIBS = \
	../../libmatoya/bin/$(TARGET)/$(ARCH)/libmatoya.a

CFLAGS = $(INCLUDES) $(FLAGS)

all: clean clear
	make objs -j4

objs: $(OBJS)
	$(CC) -o $(NAME) $(OBJS) $(STATIC_LIBS) $(LIBS) $(LD_FLAGS)

clean:
	@rm -rf android/build
	@rm -rf android/app/libs
	@rm -rf $(NAME)
	@rm -rf $(OBJS)

clear:
	@clear
