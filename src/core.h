#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(CORE_EXPORT)
	#if defined(_MSC_VER)
		#define EXPORT __declspec(dllexport)
	#else
		#define EXPORT __attribute__((visibility("default")))
	#endif
#elif defined(CORE_EXPORT_EXTERN)
	#define EXPORT extern
#else
	#define EXPORT
#endif

#if defined(CORE_FP)
	#define FP(func) (*func)
#else
	#define FP(func) func
#endif

#define CORE_PLAYERS_MAX   8
#define CORE_DESC_MAX      128
#define CORE_OPTS_MAX      128
#define CORE_KEY_NAME_MAX  64
#define CORE_OPT_NAME_MAX  64

#define CORE_FRAMES_MAX    0x4000
#define CORE_SAMPLES_MAX   (CORE_FRAMES_MAX * 2)

struct core;

enum core_button {
	CORE_BUTTON_A      = 1,
	CORE_BUTTON_B      = 2,
	CORE_BUTTON_X      = 3,
	CORE_BUTTON_Y      = 4,
	CORE_BUTTON_L      = 5,
	CORE_BUTTON_R      = 6,
	CORE_BUTTON_L2     = 7,
	CORE_BUTTON_R2     = 8,
	CORE_BUTTON_L3     = 9,
	CORE_BUTTON_R3     = 10,
	CORE_BUTTON_DPAD_U = 11,
	CORE_BUTTON_DPAD_D = 12,
	CORE_BUTTON_DPAD_L = 13,
	CORE_BUTTON_DPAD_R = 14,
	CORE_BUTTON_SELECT = 15,
	CORE_BUTTON_START  = 16,
	CORE_BUTTON_MAX,
};

enum core_axis {
	CORE_AXIS_LX = 1,
	CORE_AXIS_LY = 2,
	CORE_AXIS_RX = 3,
	CORE_AXIS_RY = 4,
	CORE_AXIS_MAX,
};

enum core_color_format {
	CORE_COLOR_FORMAT_UNKNOWN  = 0,
	CORE_COLOR_FORMAT_BGRA     = 1,
	CORE_COLOR_FORMAT_B5G6R5   = 2,
	CORE_COLOR_FORMAT_B5G5R5A1 = 3,
};

enum core_system {
	CORE_SYSTEM_UNKNOWN = 0,
	CORE_SYSTEM_NES     = 1,
	CORE_SYSTEM_SMS     = 2,
	CORE_SYSTEM_TG16    = 3,
	CORE_SYSTEM_GAMEBOY = 4,
	CORE_SYSTEM_SNES    = 5,
};

struct core_setting {
	uint32_t nopts;
	char key[CORE_KEY_NAME_MAX];
	char desc[CORE_DESC_MAX];
	char opts[CORE_OPTS_MAX][CORE_OPT_NAME_MAX];
};

typedef void (*CORE_LOG_FUNC)(const char *msg, void *opaque);
typedef void (*CORE_AUDIO_FUNC)(const int16_t *buf, size_t frames, uint32_t sample_rate, void *opaque);
typedef void (*CORE_VIDEO_FUNC)(const void *buf, enum core_color_format format,
	uint32_t width, uint32_t height, size_t pitch, void *opaque);

#ifdef __cplusplus
extern "C" {
#endif

EXPORT struct core *
FP(core_load)(const char *name, const char *system_dir, const char *save_dir);

EXPORT void
FP(core_unload)(struct core **core);

EXPORT bool
FP(core_load_game)(struct core *ctx, enum core_system system, const char *path);

EXPORT void
FP(core_unload_game)(struct core *ctx);

EXPORT void
FP(core_reset_game)(struct core *ctx);

EXPORT void
FP(core_run_frame)(struct core *ctx);

EXPORT void
FP(core_set_button)(struct core *ctx, uint8_t player, enum core_button button, bool pressed);

EXPORT void
FP(core_set_axis)(struct core *ctx, uint8_t player, enum core_axis axis, int16_t value);

EXPORT void *
FP(core_get_state)(struct core *ctx, size_t *size);

EXPORT bool
FP(core_set_state)(struct core *ctx, const void *state, size_t size);

EXPORT uint8_t
FP(core_get_num_disks)(struct core *ctx);

EXPORT int8_t
FP(core_get_disk)(struct core *ctx);

EXPORT bool
FP(core_set_disk)(struct core *ctx, int8_t disk, const char *path);

EXPORT void *
FP(core_get_sram)(struct core *ctx, size_t *size);

EXPORT bool
FP(core_game_is_loaded)(struct core *ctx);

EXPORT double
FP(core_get_frame_rate)(struct core *ctx);

EXPORT float
FP(core_get_aspect_ratio)(struct core *ctx);

EXPORT void
FP(core_set_log_func)(struct core *ctx, CORE_LOG_FUNC func, void *opaque);

EXPORT void
FP(core_set_audio_func)(struct core *ctx, CORE_AUDIO_FUNC func, void *opaque);

EXPORT void
FP(core_set_video_func)(struct core *ctx, CORE_VIDEO_FUNC func, void *opaque);

EXPORT const struct core_setting *
FP(core_get_settings)(struct core *ctx, uint32_t *len);

EXPORT void
FP(core_set_setting)(struct core *ctx, const char *key, const char *val);

EXPORT const char *
FP(core_get_setting)(struct core *ctx, const char *key);

EXPORT void
FP(core_clear_settings)(struct core *ctx);

#ifdef __cplusplus
}
#endif
