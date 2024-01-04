#include "loader.h"

#include "matoya.h"

#define CORE_FP
#include "core.h"
#include "rcore.h"

static MTY_SO *LOADER_SO;

static void loader_set_rcore(void)
{
	#define LOADER_SET_RCORE(sym) \
		sym = r##sym

	LOADER_SET_RCORE(core_load);
	LOADER_SET_RCORE(core_unload);
	LOADER_SET_RCORE(core_load_game);
	LOADER_SET_RCORE(core_unload_game);
	LOADER_SET_RCORE(core_reset_game);
	LOADER_SET_RCORE(core_run_frame);
	LOADER_SET_RCORE(core_set_button);
	LOADER_SET_RCORE(core_set_axis);
	LOADER_SET_RCORE(core_get_state);
	LOADER_SET_RCORE(core_set_state);
	LOADER_SET_RCORE(core_get_num_disks);
	LOADER_SET_RCORE(core_get_disk);
	LOADER_SET_RCORE(core_set_disk);
	LOADER_SET_RCORE(core_get_sram);
	LOADER_SET_RCORE(core_game_is_loaded);
	LOADER_SET_RCORE(core_get_frame_rate);
	LOADER_SET_RCORE(core_get_aspect_ratio);
	LOADER_SET_RCORE(core_set_log_func);
	LOADER_SET_RCORE(core_set_audio_func);
	LOADER_SET_RCORE(core_set_video_func);
	LOADER_SET_RCORE(core_get_settings);
	LOADER_SET_RCORE(core_set_setting);
	LOADER_SET_RCORE(core_get_setting);
	LOADER_SET_RCORE(core_clear_settings);
}

bool loader_load(const char *name, bool libretro)
{
	if (libretro) {
		loader_set_rcore();

	} else {
		loader_unload();

		LOADER_SO = MTY_SOLoad(name);
		if (!LOADER_SO)
			return false;

		#define LOADER_LOAD_SYM(sym) \
			sym = MTY_SOGetSymbol(LOADER_SO, #sym); \
			if (!sym) return false

		LOADER_LOAD_SYM(core_load);
		LOADER_LOAD_SYM(core_unload);
		LOADER_LOAD_SYM(core_load_game);
		LOADER_LOAD_SYM(core_unload_game);
		LOADER_LOAD_SYM(core_reset_game);
		LOADER_LOAD_SYM(core_run_frame);
		LOADER_LOAD_SYM(core_set_button);
		LOADER_LOAD_SYM(core_set_axis);
		LOADER_LOAD_SYM(core_get_state);
		LOADER_LOAD_SYM(core_set_state);
		LOADER_LOAD_SYM(core_get_num_disks);
		LOADER_LOAD_SYM(core_get_disk);
		LOADER_LOAD_SYM(core_set_disk);
		LOADER_LOAD_SYM(core_get_sram);
		LOADER_LOAD_SYM(core_game_is_loaded);
		LOADER_LOAD_SYM(core_get_frame_rate);
		LOADER_LOAD_SYM(core_get_aspect_ratio);
		LOADER_LOAD_SYM(core_set_log_func);
		LOADER_LOAD_SYM(core_set_audio_func);
		LOADER_LOAD_SYM(core_set_video_func);
		LOADER_LOAD_SYM(core_get_settings);
		LOADER_LOAD_SYM(core_set_setting);
		LOADER_LOAD_SYM(core_get_setting);
		LOADER_LOAD_SYM(core_clear_settings);
	}

	return true;
}

void loader_unload(void)
{
	loader_set_rcore();
	MTY_SOUnload(&LOADER_SO);
}
