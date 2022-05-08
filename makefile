OS = windows
ARCH = %Platform%
BIN_NAME = merton.exe

OBJS = \
	src\main.obj \
	src\core.obj \
	src\webview.obj

RESOURCES = \
	assets\$(OS)\icon.res \
	assets\$(OS)\versioninfo.res

RFLAGS = \
	-Isrc \
	/nologo

FLAGS = \
	/W4 \
	/MT \
	/MP \
	/volatile:iso \
	/wd4152 \
	/wd4100 \
	/wd4201 \
	/nologo

INCLUDES = \
	-I. \
	-I..\libmatoya\src \
	-Isrc

DEFS = \
	-DWIN32_LEAN_AND_MEAN \
	-DUNICODE

LINK_FLAGS = \
	/subsystem:windows \
	/nodefaultlib \
	/manifest:embed \
	/manifestinput:assets\$(OS)\embed.manifest \
	/nologo

LIBS = \
	..\libmatoya\bin\$(OS)\$(ARCH)\matoya.lib \
	deps\WebView2LoaderStatic.lib \
	libvcruntime.lib \
	libucrt.lib \
	libcmt.lib \
	kernel32.lib \
	windowscodecs.lib \
	user32.lib \
	comdlg32.lib \
	shell32.lib \
	d3d9.lib \
	d3d11.lib \
	d3d12.lib \
	dxgi.lib \
	dxguid.lib \
	ole32.lib \
	uuid.lib \
	winmm.lib \
	shcore.lib \
	bcrypt.lib \
	userenv.lib \
	shlwapi.lib \
	advapi32.lib \
	opengl32.lib \
	ws2_32.lib \
	xinput.lib \
	gdi32.lib \
	imm32.lib \
	winhttp.lib \
	crypt32.lib \
	secur32.lib \
	hid.lib

!IFDEF DEBUG
FLAGS = $(FLAGS) /Oy- /Ob0 /Zi
LINK_FLAGS = $(LINK_FLAGS) /debug
!ELSE
FLAGS = $(FLAGS) /O2 /GS- /Gw /Gy
!IFDEF LTO
FLAGS = $(FLAGS) /GL
LINK_FLAGS = $(LINK_FLAGS) /LTCG
!ENDIF
!ENDIF

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS)

all: clean clear $(OBJS) $(RESOURCES)
	link /out:$(BIN_NAME) $(LINK_FLAGS) *.obj $(LIBS) $(RESOURCES)

clean:
	@del /q $(RESOURCES) 2>nul
	@del /q *.obj        2>nul
	@del /q *.exe        2>nul
	@del /q *.ilk        2>nul
	@del /q *.pdb        2>nul

clear:
	@cls
