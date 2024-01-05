#pragma once

struct core *rcore_load(const char *name, const char *system_dir, const char *save_dir);
void rcore_unload(struct core **core);
bool rcore_load_game(struct core *ctx, enum core_system system, const char *path);
void rcore_unload_game(struct core *ctx);
void rcore_reset_game(struct core *ctx);
void rcore_run_frame(struct core *ctx);
void rcore_set_button(struct core *ctx, uint8_t player, enum core_button button, bool pressed);
void rcore_set_axis(struct core *ctx, uint8_t player, enum core_axis axis, int16_t value);
void *rcore_get_state(struct core *ctx, size_t *size);
bool rcore_set_state(struct core *ctx, const void *state, size_t size);
uint8_t rcore_get_num_disks(struct core *ctx);
int8_t rcore_get_disk(struct core *ctx);
bool rcore_set_disk(struct core *ctx, int8_t disk, const char *path);
void *rcore_get_sram(struct core *ctx, size_t *size);
bool rcore_game_is_loaded(struct core *ctx);
double rcore_get_frame_rate(struct core *ctx);
float rcore_get_aspect_ratio(struct core *ctx);
void rcore_set_log_func(struct core *ctx, CORE_LOG_FUNC func, void *opaque);
void rcore_set_audio_func(struct core *ctx, CORE_AUDIO_FUNC func, void *opaque);
void rcore_set_video_func(struct core *ctx, CORE_VIDEO_FUNC func, void *opaque);
const struct core_setting *rcore_get_settings(struct core *ctx, uint32_t *len);
void rcore_set_setting(struct core *ctx, const char *key, const char *val);
const char *rcore_get_setting(struct core *ctx, const char *key);
void rcore_clear_settings(struct core *ctx);
