#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

struct core_variable {
	uint32_t nopts;
	char key[CORE_KEY_NAME_MAX];
	char desc[CORE_DESC_MAX];
	char opts[CORE_OPTS_MAX][CORE_OPT_NAME_MAX];
};

typedef void (*CORE_LOG_FUNC)(const char *msg, void *opaque);
typedef void (*CORE_AUDIO_FUNC)(const int16_t *buf, size_t frames, void *opaque);
typedef void (*CORE_VIDEO_FUNC)(const void *buf, uint32_t width, uint32_t height,
	size_t pitch, void *opaque);

struct core *core_load(const char *name, const char *asset_dir);
void core_unload(struct core **core);
bool core_load_game(struct core *ctx, const char *path);
void core_unload_game(struct core *ctx);
void core_reset_game(struct core *ctx);
void core_run_frame(struct core *ctx);
void core_set_button(struct core *ctx, uint8_t player, enum core_button button, bool pressed);
void core_set_axis(struct core *ctx, uint8_t player, enum core_axis axis, int16_t value);
void *core_get_state(struct core *ctx, size_t *size);
bool core_set_state(struct core *ctx, const void *state, size_t size);
bool core_has_disk_interface(struct core *ctx);
uint8_t core_get_num_disks(struct core *ctx);
int8_t core_get_disk(struct core *ctx);
bool core_set_disk(struct core *ctx, int8_t disk);
bool core_load_disk(struct core *ctx, uint8_t disk, const char *path);
void *core_get_sram(struct core *ctx, size_t *size);
bool core_set_sram(struct core *ctx, const void *sram, size_t size);
const char *core_get_save_dir(struct core *ctx);
const char *core_get_game_path(struct core *ctx);
bool core_game_is_loaded(struct core *ctx);
uint32_t core_get_sample_rate(struct core *ctx);
double core_get_frame_rate(struct core *ctx);
float core_get_aspect_ratio(struct core *ctx);
enum core_color_format core_get_color_format(struct core *ctx);
void core_set_log_func(CORE_LOG_FUNC func, void *opaque);
void core_set_audio_func(struct core *ctx, CORE_AUDIO_FUNC func, void *opaque);
void core_set_video_func(struct core *ctx, CORE_VIDEO_FUNC func, void *opaque);
const struct core_variable *core_get_variables(struct core *ctx, uint32_t *len);
void core_set_variable(struct core *ctx, const char *key, const char *val);
const char *core_get_variable(struct core *ctx, const char *key);
void core_clear_variables(struct core *ctx);
