TARGET = windows
ARCH = %%Platform%%
BIN = mcompress.exe

OBJS = \
	main.obj

INCLUDES = \
	-I..\..\libmatoya\src

DEFS = \
	-DUNICODE \
	-DWIN32_LEAN_AND_MEAN

FLAGS = \
	/W4 \
	/MT \
	/MP \
	/volatile:iso \
	/wd4100 \
	/wd4201 \
	/nologo

LINK_FLAGS = \
	/subsystem:console \
	/nodefaultlib \
	/nologo

LIBS = \
	..\..\libmatoya\bin\$(TARGET)\$(ARCH)\matoya.lib \
	libvcruntime.lib \
	libucrt.lib \
	libcmt.lib \
	kernel32.lib \
	ole32.lib \
	ws2_32.lib \
	shlwapi.lib \
	shell32.lib \
	cabinet.lib \
	bcrypt.lib \
	crypt32.lib

!IFDEF DEBUG
FLAGS = $(FLAGS) /Ob0 /Zi /Oy-
LINK_FLAGS = $(LINK_FLAGS) /debug
!ELSE
FLAGS = $(FLAGS) /O2 /GS- /Gw /GL
LINK_FLAGS = $(LINK_FLAGS) /LTCG
!ENDIF

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS)

all: clean clear $(OBJS)
	link /out:$(BIN) $(LINK_FLAGS) *.obj $(LIBS)

clean:
	@-del /q *.obj 2>nul
	@-del /q *.exe 2>nul
	@-del /q *.ilk 2>nul
	@-del /q *.pdb 2>nul

clear:
	@cls
