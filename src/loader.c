#include "loader.h"

#include "matoya.h"

#define CORE_FP
#include "core.h"
#include "rcore.h"

static MTY_SO *LOADER_SO;

static void loader_set_rcore(void)
{
	#define LOADER_SET_RCORE(sym, rsym) \
		sym = rsym

	LOADER_SET_RCORE(CoreLoad, rcore_load);
	LOADER_SET_RCORE(CoreUnload, rcore_unload);
	LOADER_SET_RCORE(CoreLoadGame, rcore_load_game);
	LOADER_SET_RCORE(CoreUnloadGame, rcore_unload_game);
	LOADER_SET_RCORE(CoreReset, rcore_reset);
	LOADER_SET_RCORE(CoreRun, rcore_run);
	LOADER_SET_RCORE(CoreSetButton, rcore_set_button);
	LOADER_SET_RCORE(CoreSetAxis, rcore_set_axis);
	LOADER_SET_RCORE(CoreGetState, rcore_get_state);
	LOADER_SET_RCORE(CoreSetState, rcore_set_state);
	LOADER_SET_RCORE(CoreGetNumDisks, rcore_get_num_disks);
	LOADER_SET_RCORE(CoreGetDisk, rcore_get_disk);
	LOADER_SET_RCORE(CoreSetDisk, rcore_set_disk);
	LOADER_SET_RCORE(CoreGetSaveData, rcore_get_save_data);
	LOADER_SET_RCORE(CoreGameIsLoaded, rcore_game_is_loaded);
	LOADER_SET_RCORE(CoreGetFrameRate, rcore_get_frame_rate);
	LOADER_SET_RCORE(CoreGetAspectRatio, rcore_get_aspect_ratio);
	LOADER_SET_RCORE(CoreSetLogFunc, rcore_set_log_func);
	LOADER_SET_RCORE(CoreSetAudioFunc, rcore_set_audio_func);
	LOADER_SET_RCORE(CoreSetVideoFunc, rcore_set_video_func);
	LOADER_SET_RCORE(CoreGetAllSettings, rcore_get_all_settings);
	LOADER_SET_RCORE(CoreSetSetting, rcore_set_setting);
	LOADER_SET_RCORE(CoreGetSetting, rcore_get_setting);
	LOADER_SET_RCORE(CoreResetSettings, rcore_reset_settings);
}

bool loader_load(const char *name)
{
	loader_reset();

	if (!name)
		return false;

	LOADER_SO = MTY_SOLoad(name);
	if (!LOADER_SO)
		return false;

	void *sym = (void *) MTY_SOGetSymbol(LOADER_SO, "retro_api_version");

	if (sym) {
		loader_reset();

	} else {
		#define LOADER_LOAD_SYM(sym) \
			sym = MTY_SOGetSymbol(LOADER_SO, #sym); \
			if (!sym) return false

		LOADER_LOAD_SYM(CoreLoad);
		LOADER_LOAD_SYM(CoreUnload);
		LOADER_LOAD_SYM(CoreLoadGame);
		LOADER_LOAD_SYM(CoreUnloadGame);
		LOADER_LOAD_SYM(CoreReset);
		LOADER_LOAD_SYM(CoreRun);
		LOADER_LOAD_SYM(CoreSetButton);
		LOADER_LOAD_SYM(CoreSetAxis);
		LOADER_LOAD_SYM(CoreGetState);
		LOADER_LOAD_SYM(CoreSetState);
		LOADER_LOAD_SYM(CoreGetNumDisks);
		LOADER_LOAD_SYM(CoreGetDisk);
		LOADER_LOAD_SYM(CoreSetDisk);
		LOADER_LOAD_SYM(CoreGetSaveData);
		LOADER_LOAD_SYM(CoreGameIsLoaded);
		LOADER_LOAD_SYM(CoreGetFrameRate);
		LOADER_LOAD_SYM(CoreGetAspectRatio);
		LOADER_LOAD_SYM(CoreSetLogFunc);
		LOADER_LOAD_SYM(CoreSetAudioFunc);
		LOADER_LOAD_SYM(CoreSetVideoFunc);
		LOADER_LOAD_SYM(CoreGetAllSettings);
		LOADER_LOAD_SYM(CoreSetSetting);
		LOADER_LOAD_SYM(CoreGetSetting);
		LOADER_LOAD_SYM(CoreResetSettings);
	}

	return true;
}

void loader_reset(void)
{
	loader_set_rcore();
	MTY_SOUnload(&LOADER_SO);
}
