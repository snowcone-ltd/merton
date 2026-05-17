#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core.h"
#include "matoya.h"

#define CONFIG_CORE_MAX 64
#define SYSTEM_NAME_MAX 64

// libmatoya removed its *TL (thread-local) path helpers. These shims preserve the
// "do not free" return semantics by copying into a rotating per-thread buffer pool,
// which keeps the heavily-nested call patterns in main.c readable.
#define CONFIG_PATH_TL_SLOTS 8

#if defined(_MSC_VER)
	#define CONFIG_TLOCAL __declspec(thread)
#else
	#define CONFIG_TLOCAL __thread
#endif

static CONFIG_TLOCAL char CONFIG_PATH_TL[CONFIG_PATH_TL_SLOTS][MTY_PATH_MAX];
static CONFIG_TLOCAL uint32_t CONFIG_PATH_TL_IDX;

static inline const char *config_path_tl_take(char *p)
{
	char *slot = CONFIG_PATH_TL[CONFIG_PATH_TL_IDX++ % CONFIG_PATH_TL_SLOTS];

	if (p) {
		snprintf(slot, MTY_PATH_MAX, "%s", p);
		MTY_Free(p);

	} else {
		slot[0] = '\0';
	}

	return slot;
}

static inline const char *config_path_tl_copy(const char *p)
{
	char *slot = CONFIG_PATH_TL[CONFIG_PATH_TL_IDX++ % CONFIG_PATH_TL_SLOTS];
	snprintf(slot, MTY_PATH_MAX, "%s", p ? p : "");
	return slot;
}

static inline const char *MTY_JoinPathTL(const char *p0, const char *p1)
{
	return config_path_tl_take(MTY_JoinPath(p0, p1));
}

static inline const char *MTY_GetPathPrefixTL(const char *path)
{
	return config_path_tl_take(MTY_GetPathPrefix(path));
}

static inline const char *MTY_ResolvePathTL(const char *path)
{
	char *r = MTY_ResolvePath(path);
	if (!r)
		return NULL;

	return config_path_tl_take(r);
}

static inline const char *MTY_GetFileNameTL(const char *path, bool with_ext)
{
	return with_ext
		? config_path_tl_copy(MTY_GetFileName(path))
		: config_path_tl_take(MTY_GetFileNameNoExt(path));
}

static inline const char *MTY_GetFileExtensionTL(const char *path)
{
	const char *p = path ? strrchr(path, '.') : NULL;
	return p ? p + 1 : "";
}

#define config_asset_dir()  MTY_JoinPathTL(MTY_GetProcessDir(), "merton-files")
#define config_path(path)   MTY_JoinPathTL(config_asset_dir(), path)
#define config_file()       config_path("config.json")
#define config_cores_dir()  config_path("cores")
#define config_save_dir()   config_path("saves")
#define config_state_dir()  config_path("state")
#define config_system_dir() config_path("system")
#define config_tmp_dir()    config_path("tmp")
#define config_ui_dir()     config_path("ui")

struct config {
	bool bg_pause;
	bool menu_pause;
	bool console;
	bool fullscreen;
	bool mute;
	bool square_pixels;
	bool int_scaling;
	uint32_t audio_buffer;
	uint32_t playback_rate;
	uint32_t scanlines;
	uint32_t sharpen;
	int32_t vsync;

	MTY_Filter filter;
	MTY_Frame window;

	char core[CORE_SYSTEM_MAX][CONFIG_CORE_MAX];
};
