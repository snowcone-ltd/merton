#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#include "matoya.h"

#define CORE_FP
#define CORE_EXPORT_EXTERN
#include "core.h"

#include "app.h"
#include "config.h"
#include "csync.h"
#include "loader.h"
#include "ui-zip.h"

#define APP_TITLE_MAX 1024
#define MAIN_WINDOW_W 800
#define MAIN_WINDOW_H 600

#if defined(MTN_DEBUG)
	#define MTN_DEBUG_WEBVIEW true
#else
	#define MTN_DEBUG_WEBVIEW false
#endif

#if !defined(_WIN32)
static int32_t fopen_s(FILE **f, const char *name, const char *mode)
{
	*f = fopen(name, mode);

	return *f ? 0 : -1;
}
#endif

enum app_event_type {
	APP_EVENT_TITLE       = 1,
	APP_EVENT_LOAD_GAME   = 2,
	APP_EVENT_UNLOAD_GAME = 3,
	APP_EVENT_CONFIG      = 4,
	APP_EVENT_QUIT        = 5,
	APP_EVENT_PAUSE       = 6,
	APP_EVENT_GFX         = 7,
	APP_EVENT_CORE_OPT    = 8,
	APP_EVENT_CLEAR_OPTS  = 9,
	APP_EVENT_RESET       = 10,
	APP_EVENT_SAVE_STATE  = 11,
	APP_EVENT_LOAD_STATE  = 12,
	APP_EVENT_SET_DISK    = 13,
	APP_EVENT_HIDE_MENU   = 14,
	APP_EVENT_STATE       = 15,
};

struct audio_packet {
	uint32_t sample_rate;
	int16_t data[CORE_SAMPLES_MAX];
	size_t frames;
};

struct app_event {
	enum app_event_type type;
	bool rt;

	// These should be unioned
	struct config cfg;
	MTY_GFX gfx;
	uint32_t vsync;
	char title[APP_TITLE_MAX];
	char game[MTY_PATH_MAX];
	struct {
		char key[CORE_KEY_NAME_MAX];
		char val[CORE_OPT_NAME_MAX];
	} opt;
	bool fetch_core;
	uint8_t state_index;
	int8_t disk;
};

struct main {
	Core *core;
	struct csync *csync;

	char *game_path;
	char *content_name;
	CoreSystem system;
	MTY_App *app;
	MTY_JSON *jcfg;
	MTY_JSON *core_options;
	MTY_JSON *core_exts;
	MTY_Window window;
	MTY_Queue *rt_q;
	MTY_Queue *mt_q;
	MTY_Queue *a_q;
	struct config cfg;
	uint8_t last_save_index;
	uint8_t last_load_index;
	double core_fps;
	bool ui_visible;
	bool ui_debounce;
	bool running;
	bool paused;
	bool audio_init;
	bool resampler_init;
	bool show_ui;

	MTY_RenderDesc desc;

	char next_game[MTY_PATH_MAX];
};


// Maps

static const CoreButton NES_KEYBOARD_MAP[MTY_KEY_MAX] = {
	[MTY_KEY_SEMICOLON] = CORE_BUTTON_A,
	[MTY_KEY_L]         = CORE_BUTTON_B,
	[MTY_KEY_O]         = CORE_BUTTON_X,
	[MTY_KEY_P]         = CORE_BUTTON_Y,
	[MTY_KEY_Q]         = CORE_BUTTON_L,
	[MTY_KEY_LBRACKET]  = CORE_BUTTON_R,
	[MTY_KEY_LSHIFT]    = CORE_BUTTON_SELECT,
	[MTY_KEY_SPACE]     = CORE_BUTTON_START,
	[MTY_KEY_W]         = CORE_BUTTON_DPAD_U,
	[MTY_KEY_S]         = CORE_BUTTON_DPAD_D,
	[MTY_KEY_A]         = CORE_BUTTON_DPAD_L,
	[MTY_KEY_D]         = CORE_BUTTON_DPAD_R,
};


// Config

static struct config main_parse_config(const MTY_JSON *jcfg, MTY_JSON **core_options, MTY_JSON **core_exts)
{
	struct config cfg = {0};

	if (*core_options)
		MTY_JSONDestroy(core_options);

	if (*core_exts)
		MTY_JSONDestroy(core_exts);

	#define CFG_GET_BOOL(name, def) \
		if (!MTY_JSONObjGetBool(jcfg, #name, &cfg.name)) cfg.name = def

	#define CFG_GET_INT(name, def) \
		if (!MTY_JSONObjGetInt(jcfg, #name, &cfg.name)) cfg.name = def

	#define CFG_GET_UINT(name, def) \
		if (!MTY_JSONObjGetInt(jcfg, #name, (int32_t *) &cfg.name)) cfg.name = def

	#define CFG_GET_STR(name, size, def) \
		if (!MTY_JSONObjGetString(jcfg, #name, cfg.name, size)) snprintf(cfg.name, size, "%s", def);

	CFG_GET_BOOL(bg_pause, false);
	CFG_GET_BOOL(menu_pause, false);
	CFG_GET_BOOL(console, false);
	CFG_GET_BOOL(mute, false);
	CFG_GET_BOOL(square_pixels, false);
	CFG_GET_BOOL(int_scaling, false);
	CFG_GET_UINT(gfx, MTY_GetDefaultGFX());
	CFG_GET_UINT(filter, MTY_FILTER_LINEAR);
	CFG_GET_UINT(audio_buffer, 75);
	CFG_GET_UINT(playback_rate, 48000);
	CFG_GET_UINT(scanlines, 70);
	CFG_GET_UINT(sharpen, 0);
	CFG_GET_UINT(vsync, 0);
	CFG_GET_UINT(window.type, MTY_WINDOW_NORMAL);
	CFG_GET_UINT(window.size.w, 0);
	CFG_GET_UINT(window.size.h, 0);
	CFG_GET_INT(window.x, 0);
	CFG_GET_INT(window.y, 0);
	CFG_GET_STR(window.screen, MTY_SCREEN_MAX, "");

	CFG_GET_BOOL(fullscreen, cfg.window.type & MTY_WINDOW_FULLSCREEN);

	CFG_GET_STR(core.atari2600, CONFIG_CORE_MAX, "stella_libretro");
	CFG_GET_STR(core.gameboy, CONFIG_CORE_MAX, "mesen2");
	CFG_GET_STR(core.gba, CONFIG_CORE_MAX, "mgba");
	CFG_GET_STR(core.genesis, CONFIG_CORE_MAX, "genesis-plus-gx");
	CFG_GET_STR(core.ms, CONFIG_CORE_MAX, "mesen2");
	CFG_GET_STR(core.n64, CONFIG_CORE_MAX, "mupen64plus");
	CFG_GET_STR(core.nes, CONFIG_CORE_MAX, "mesen2");
	CFG_GET_STR(core.ps, CONFIG_CORE_MAX, "swanstation_libretro");
	CFG_GET_STR(core.snes, CONFIG_CORE_MAX, "mesen2");
	CFG_GET_STR(core.tg16, CONFIG_CORE_MAX, "mesen2");

	const MTY_JSON *obj = MTY_JSONObjGetItem(jcfg, "core_options");
	*core_options = obj ? MTY_JSONDuplicate(obj) : MTY_JSONObjCreate();

	obj = MTY_JSONObjGetItem(jcfg, "core_exts");
	if (obj) {
		*core_exts = MTY_JSONDuplicate(obj);

	} else {
		*core_exts = MTY_JSONObjCreate();
		MTY_JSONObjSetString(*core_exts, "atari2600", "a26");
		MTY_JSONObjSetString(*core_exts, "gameboy", "gb|gbc");
		MTY_JSONObjSetString(*core_exts, "gba", "gba");
		MTY_JSONObjSetString(*core_exts, "genesis", "gen|md|smd");
		MTY_JSONObjSetString(*core_exts, "ms", "sms|gg");
		MTY_JSONObjSetString(*core_exts, "n64", "n64|v64|z64");
		MTY_JSONObjSetString(*core_exts, "nes", "nes|fds|qd|unf|unif");
		MTY_JSONObjSetString(*core_exts, "ps", "");
		MTY_JSONObjSetString(*core_exts, "snes", "smc|sfc|bs");
		MTY_JSONObjSetString(*core_exts, "tg16", "pce");
	}

	return cfg;
}

static MTY_JSON *main_serialize_config(struct config *cfg, const MTY_JSON *core_options, const MTY_JSON *core_exts)
{
	MTY_JSON *jcfg = MTY_JSONObjCreate();

	#define CFG_SET_BOOL(name) \
		MTY_JSONObjSetBool(jcfg, #name, cfg->name)

	#define CFG_SET_NUMBER(name) \
		MTY_JSONObjSetNumber(jcfg, #name, cfg->name)

	#define CFG_SET_STR(name) \
		MTY_JSONObjSetString(jcfg, #name, cfg->name)

	CFG_SET_BOOL(bg_pause);
	CFG_SET_BOOL(menu_pause);
	CFG_SET_BOOL(console);
	CFG_SET_BOOL(mute);
	CFG_SET_BOOL(square_pixels);
	CFG_SET_BOOL(int_scaling);
	CFG_SET_NUMBER(gfx);
	CFG_SET_NUMBER(filter);
	CFG_SET_NUMBER(audio_buffer);
	CFG_SET_NUMBER(playback_rate);
	CFG_SET_NUMBER(scanlines);
	CFG_SET_NUMBER(sharpen);
	CFG_SET_NUMBER(vsync);
	CFG_SET_NUMBER(window.type);
	CFG_SET_NUMBER(window.size.w);
	CFG_SET_NUMBER(window.size.h);
	CFG_SET_NUMBER(window.x);
	CFG_SET_NUMBER(window.y);
	CFG_SET_STR(window.screen);

	CFG_SET_BOOL(fullscreen);

	CFG_SET_STR(core.atari2600);
	CFG_SET_STR(core.gameboy);
	CFG_SET_STR(core.gba);
	CFG_SET_STR(core.genesis);
	CFG_SET_STR(core.ms);
	CFG_SET_STR(core.n64);
	CFG_SET_STR(core.nes);
	CFG_SET_STR(core.ps);
	CFG_SET_STR(core.snes);
	CFG_SET_STR(core.tg16);

	MTY_JSONObjSetItem(jcfg, "core_options", MTY_JSONDuplicate(core_options));
	MTY_JSONObjSetItem(jcfg, "core_exts", MTY_JSONDuplicate(core_exts));

	return jcfg;
}

static void main_save_config(struct config *cfg, const MTY_JSON *core_options, const MTY_JSON *core_exts)
{
	MTY_JSON *jcfg = main_serialize_config(cfg, core_options, core_exts);

	MTY_JSONWriteFile(config_file(), jcfg);
	MTY_JSONDestroy(&jcfg);
}

static MTY_JSON *main_load_config(struct config *cfg, MTY_JSON **core_options, MTY_JSON **core_exts)
{
	MTY_JSON *jcfg = MTY_JSONReadFile(config_file());
	if (!jcfg)
		jcfg = MTY_JSONObjCreate();

	*cfg = main_parse_config(jcfg, core_options, core_exts);
	MTY_JSONDestroy(&jcfg);

	return main_serialize_config(cfg, *core_options, *core_exts);
}


// Core

static void main_push_app_event(const struct app_event *evt, void *opaque);

static void main_video(const void *buf, CoreColorFormat format,
	uint32_t width, uint32_t height, size_t pitch, void *opaque)
{
	struct main *ctx = opaque;

	// A NULL buffer means we should render the previous frame
	if (!buf)
		format = CORE_COLOR_FORMAT_UNKNOWN;

	ctx->desc.format =
		format == CORE_COLOR_FORMAT_BGRA ? MTY_COLOR_FORMAT_BGRA :
		format == CORE_COLOR_FORMAT_RGBA ? MTY_COLOR_FORMAT_RGBA :
		format == CORE_COLOR_FORMAT_B5G6R5 ? MTY_COLOR_FORMAT_BGR565 :
		format == CORE_COLOR_FORMAT_B5G5R5A1 ? MTY_COLOR_FORMAT_BGRA5551 :
		MTY_COLOR_FORMAT_UNKNOWN;

	if (ctx->desc.format != MTY_COLOR_FORMAT_UNKNOWN) {
		bool wide = format == CORE_COLOR_FORMAT_BGRA || format == CORE_COLOR_FORMAT_RGBA;
		ctx->desc.imageWidth = (uint32_t) (wide ? pitch / 4 : pitch / 2);
		ctx->desc.imageHeight = height;
		ctx->desc.cropWidth = width;
		ctx->desc.cropHeight = height;
	}

	// Effects
	ctx->desc.filter = ctx->cfg.filter;

	if (ctx->cfg.filter == MTY_FILTER_LINEAR && ctx->cfg.sharpen > 0) {
		ctx->desc.effects[0] = MTY_EFFECT_SHARPEN;
		ctx->desc.levels[0] = (float) ctx->cfg.sharpen / 100.0f;

	} else {
		ctx->desc.effects[0] = MTY_EFFECT_NONE;
	}

	if (ctx->cfg.scanlines > 0) {
		ctx->desc.effects[1] = MTY_EFFECT_SCANLINES;
		ctx->desc.levels[1] = (float) ctx->cfg.scanlines / 100.0f;

	} else {
		ctx->desc.effects[1] = MTY_EFFECT_NONE;
	}

	// Integer scaling
	if (ctx->cfg.int_scaling && ctx->desc.cropWidth > 0 && ctx->desc.cropHeight > 0) {
		MTY_Size size = MTY_WindowGetSize(ctx->app, ctx->window);

		uint32_t w_multi = size.w / ctx->desc.cropWidth;
		uint32_t h_multi = size.h / ctx->desc.cropHeight;

		ctx->desc.scale = (float) (w_multi < h_multi ? w_multi : h_multi);

	} else {
		ctx->desc.scale = 0;
	}

	// Square pixels
	ctx->desc.aspectRatio = !ctx->cfg.square_pixels ?
		CoreGetAspectRatio(ctx->core) : ctx->desc.cropHeight > 0 ?
		(float) ctx->desc.cropWidth / ctx->desc.cropHeight : 1;

	MTY_WindowDrawQuad(ctx->app, ctx->window, buf, &ctx->desc);
}

static void main_audio(const int16_t *buf, size_t frames, uint32_t sample_rate, void *opaque)
{
	// XXX This function should be thread safe in case the core (mupen64plus)
	// submits audio from a different thread. MTY_Queue is multi producer / single
	// consumer safe.

	MTY_Queue *q = opaque;

	struct audio_packet *pkt = MTY_QueueGetInputBuffer(q);

	if (pkt) {
		pkt->sample_rate = sample_rate;
		pkt->frames = frames;

		memcpy(pkt->data, buf, frames * 4);

		MTY_QueuePush(q, sizeof(struct audio_packet));
	}
}

static void main_log(const char *msg, void *opaque)
{
	printf("%s", msg);
}

static CoreSystem main_get_core_system(const char *key)
{
	if (!strcmp(key, "nes")) return CORE_SYSTEM_NES;
	if (!strcmp(key, "ms")) return CORE_SYSTEM_SMS;
	if (!strcmp(key, "tg16")) return CORE_SYSTEM_TG16;
	if (!strcmp(key, "gameboy")) return CORE_SYSTEM_GAMEBOY;
	if (!strcmp(key, "snes")) return CORE_SYSTEM_SNES;
	if (!strcmp(key, "n64")) return CORE_SYSTEM_N64;

	return CORE_SYSTEM_UNKNOWN;
}

static CoreSystem main_get_cdrom_system(struct main *ctx, const char *name,
	const char **core)
{
	*core = CONFIG_GET_CORE(&ctx->cfg, "tg16");
	CoreSystem sys = CORE_SYSTEM_TG16;

	MTY_FileList *list = MTY_GetFileList(MTY_GetPathPrefix(name), ".bin");
	if (!list)
		return sys;

	size_t header_size = 38 * 1024;
	size_t ps_offset = 37 * 1024;
	char *buf = calloc(1, header_size + 1);

	for (uint32_t x = 0; x < list->len && sys == CORE_SYSTEM_TG16; x++) {
		MTY_FileDesc *desc = &list->files[x];

		if (desc->dir)
			continue;

		FILE *f = NULL;
		if (fopen_s(&f, desc->path, "rb") != 0)
			continue;

		size_t n = fread(buf, 1, header_size, f);

		if (n > 0) {
			for (size_t y = 0; y < n; y++)
				if (buf[y] == 0)
					buf[y] = 1;

			// PLAYSTATION is about 37KB in
			if (strstr(buf + ps_offset, "PLAYSTATION")) {
				*core = CONFIG_GET_CORE(&ctx->cfg, "ps");
				sys = CORE_SYSTEM_UNKNOWN;
			}

			// SEGADISCSYSTEM is close to the beginning
			buf[1024] = 0;

			if (strstr(buf, "SEGADISCSYSTEM")) {
				*core = CONFIG_GET_CORE(&ctx->cfg, "genesis");
				sys = CORE_SYSTEM_UNKNOWN;
			}
		}

		fclose(f);
	}

	free(buf);
	MTY_FreeFileList(&list);

	return sys;
}

static CoreSystem main_get_system_by_ext(struct main *ctx, const char *name,
	const char **core)
{
	*core = NULL;

	const char *ext = MTY_GetFileExtension(name);

	if (!strcmp(ext, "cue"))
		return main_get_cdrom_system(ctx, name, core);

	uint64_t iter = 0;

	for (const char *key = NULL; MTY_JSONObjGetNextKey(ctx->core_exts, &iter, &key);) {
		char exts[SYSTEM_EXTS_MAX];

		if (MTY_JSONObjGetString(ctx->core_exts, key, exts, SYSTEM_EXTS_MAX)) {
			const char *substr = MTY_Strcasestr(exts, ext);
			size_t end = strlen(ext);

			if (substr && (substr[end] == '\0' || substr[end] == '|')) {
				*core = CONFIG_GET_CORE(&ctx->cfg, key);
				return main_get_core_system(key);
			}
		}
	}

	return CORE_SYSTEM_UNKNOWN;
}

static void main_set_core_options(struct main *ctx)
{
	uint64_t iter = 0;

	for (const char *key = NULL; MTY_JSONObjGetNextKey(ctx->core_options, &iter, &key);) {
		char val[CORE_OPT_NAME_MAX];

		if (MTY_JSONObjGetString(ctx->core_options, key, val, CORE_OPT_NAME_MAX))
			CoreSetSetting(ctx->core, key, val);
	}
}

static void *main_read_sdata(Core *core, const char *content_name, size_t *size)
{
	const char *name = MTY_SprintfDL("%s.srm", content_name);

	return MTY_ReadFile(MTY_JoinPath(config_save_dir(), name), size);
}

static void main_save_sdata(Core *core, const char *content_name)
{
	if (!content_name)
		return;

	size_t size = 0;
	void *sdata = CoreGetSaveData(core, &size);
	if (sdata) {
		const char *name = MTY_SprintfDL("%s.srm", content_name);
		const char *dir = config_save_dir();

		MTY_Mkdir(dir);
		MTY_WriteFile(MTY_JoinPath(dir, name), sdata, size);
		MTY_Free(sdata);
	}
}

static void main_unload(struct main *ctx)
{
	main_save_sdata(ctx->core, ctx->content_name);
	CoreUnload(&ctx->core);
	loader_reset();

	uint16_t tmp[8][8] = {0};
	main_video(tmp, CORE_COLOR_FORMAT_B5G6R5, 8, 8, 8 * 2, ctx);
	MTY_WindowPresent(ctx->app, ctx->window);

	MTY_Free(ctx->game_path);
	MTY_Free(ctx->content_name);
	ctx->system = CORE_SYSTEM_UNKNOWN;
	ctx->game_path = NULL;
	ctx->content_name = NULL;

	ctx->resampler_init = false;

	struct app_event evt = {.type = APP_EVENT_TITLE};
	snprintf(evt.title, APP_TITLE_MAX, "%s", APP_NAME);
	main_push_app_event(&evt, ctx);
}

static void main_load_game(struct main *ctx, const char *name, bool fetch_core)
{
	const char *core = NULL;
	CoreSystem system = main_get_system_by_ext(ctx, name, &core);
	if (!core)
		return;

	main_unload(ctx);

	const char *cname = MTY_SprintfDL("%s.%s", core, MTY_GetSOExtension());
	const char *core_path = MTY_JoinPath(config_cores_dir(), cname);
	const char *content_name = MTY_GetFileName(name, false);

	bool file_ok = MTY_FileExists(core_path) &&
		csync_check_core_hash(ctx->csync, core, core_path);

	// If core is on the system and the most recent hash matches, try to use it
	if (file_ok) {
		ctx->core = loader_load(core_path, config_system_dir());
		if (!ctx->core)
			return;

		main_set_core_options(ctx);

		CoreSetLogFunc(ctx->core, main_log, &ctx);
		CoreSetAudioFunc(ctx->core, main_audio, ctx->a_q);
		CoreSetVideoFunc(ctx->core, main_video, ctx);

		size_t sdata_size = 0;
		void *sdata = main_read_sdata(ctx->core, content_name, &sdata_size);
		bool success = CoreLoadGame(ctx->core, system, name, sdata, sdata_size);
		MTY_Free(sdata);

		if (!success)
			return;

		ctx->system = system;
		ctx->game_path = MTY_Strdup(name);
		ctx->content_name = MTY_Strdup(content_name);
		ctx->core_fps = CoreGetFrameRate(ctx->core);

		struct app_event evt = {.type = APP_EVENT_TITLE};
		snprintf(evt.title, APP_TITLE_MAX, "%s - %s", APP_NAME, ctx->content_name);
		main_push_app_event(&evt, ctx);

	// Get the core from the internet
	} else if (fetch_core) {
		snprintf(ctx->next_game, MTY_PATH_MAX, "%s", name);
		csync_fetch_core(ctx->csync, cname);
	}
}


// WebView

static bool main_ui_is_steam(void)
{
	return MTY_WebViewIsSteam();
}

static void main_ui_show(MTY_App *app, MTY_Window window, bool show)
{
	MTY_WebViewShow(app, window, show);
}

static bool main_ui_visible(MTY_App *app, MTY_Window window)
{
	return MTY_WebViewIsVisible(app, window);
}

static void main_ui_init(MTY_App *app, MTY_Window window)
{
	char *dir = (char *) MTY_GetProcessDir();

	for (size_t x = 0; x < strlen(dir); x++)
		if (dir[x] == '\\')
			dir[x] = '/';

	// The Steam WebView needs to know about the location of the SO
	const char *fdir = main_ui_is_steam() ? MTY_JoinPath("deps", "steam") :
		config_tmp_dir();

	MTY_WindowSetWebView(app, window, fdir, MTN_DEBUG_WEBVIEW);

	const char *url = MTY_SprintfDL("file:///%s/merton-files/ui/index.html", dir);

	// Development location
	if (MTY_FileExists(MTY_JoinPath(dir, MTY_JoinPath("src", MTY_JoinPath("ui", "index.html"))))) {
		url = MTY_SprintfDL("file:///%s/src/ui/index.html", dir);

	// Production location, bootstrap from UI_ZIP if necessary
	} else {
		const char *ui = config_ui_dir();
		char *id = MTY_ReadFile(MTY_JoinPath(ui, "id.txt"), NULL);

		if (!id || strcmp(id, UI_ZIP_ID)) {
			MTY_Mkdir(ui);

			for (size_t x = 0; x < UI_ZIP_LEN; x++) {
				size_t size = 0;
				void *out = MTY_Decompress(UI_ZIP[x].buf, UI_ZIP[x].size, &size);

				if (out) {
					MTY_WriteFile(MTY_JoinPath(ui, UI_ZIP[x].name), out, size);
					MTY_Free(out);
				}
			}

			MTY_WriteFile(MTY_JoinPath(ui, "id.txt"), UI_ZIP_ID, strlen(UI_ZIP_ID));
		}

		MTY_Free(id);
	}

	MTY_WebViewNavigate(app, window, url, true);
	MTY_WebViewSetInputPassthrough(app, window, true);
}

static void main_post_ui_files(MTY_App *app, MTY_Window window, const char *dir)
{
	MTY_FileList *list = MTY_GetFileList(dir, NULL);

	if (list) {
		MTY_JSON *jmsg = MTY_JSONObjCreate();
		MTY_JSON *jlist = MTY_JSONArrayCreate(list->len);

		MTY_JSONObjSetString(jmsg, "type", "files");
		MTY_JSONObjSetString(jmsg, "path", dir);
		MTY_JSONObjSetItem(jmsg, "list", jlist);

		for (uint32_t x = 0; x < list->len; x++) {
			MTY_FileDesc *desc = &list->files[x];

			MTY_JSON *fobj = MTY_JSONObjCreate();
			MTY_JSONObjSetBool(fobj, "dir", desc->dir);
			MTY_JSONObjSetString(fobj, "name", desc->name);

			MTY_JSONArraySetItem(jlist, x, fobj);
		}

		MTY_FreeFileList(&list);

		char *jstr = MTY_JSONSerialize(jmsg);
		MTY_WebViewSendText(app, window, jstr);

		MTY_Free(jstr);
		MTY_JSONDestroy(&jmsg);
	}
}

static void main_post_ui_controller(MTY_App *app, MTY_Window window, const MTY_ControllerEvent *c)
{
	MTY_JSON *msg = MTY_JSONObjCreate();
	MTY_JSONObjSetString(msg, "type", "controller");
	MTY_JSONObjSetBool(msg, "b", c->buttons[MTY_CBUTTON_B]);
	MTY_JSONObjSetBool(msg, "a", c->buttons[MTY_CBUTTON_A]);
	MTY_JSONObjSetBool(msg, "u", c->buttons[MTY_CBUTTON_DPAD_UP]);
	MTY_JSONObjSetBool(msg, "d", c->buttons[MTY_CBUTTON_DPAD_DOWN]);
	MTY_JSONObjSetBool(msg, "l", c->buttons[MTY_CBUTTON_DPAD_LEFT]);
	MTY_JSONObjSetBool(msg, "r", c->buttons[MTY_CBUTTON_DPAD_RIGHT]);
	MTY_JSONObjSetBool(msg, "ls", c->buttons[MTY_CBUTTON_LEFT_SHOULDER]);
	MTY_JSONObjSetBool(msg, "rs", c->buttons[MTY_CBUTTON_RIGHT_SHOULDER]);

	char *jmsg = MTY_JSONSerialize(msg);
	MTY_WebViewSendText(app, window, jmsg);

	MTY_Free(jmsg);
	MTY_JSONDestroy(&msg);
}


// WebView (Main)

static void main_post_ui_state(struct main *ctx)
{
	// Configuration
	MTY_JSON *msg = MTY_JSONObjCreate();
	MTY_JSONObjSetString(msg, "type", "state");

	MTY_JSON *cfg = MTY_JSONDuplicate(ctx->jcfg);
	MTY_JSONObjSetItem(msg, "cfg", cfg);

	// Core options
	MTY_JSON *core_opts = MTY_JSONObjCreate();
	MTY_JSONObjSetItem(msg, "core_opts", core_opts);

	uint32_t vlen = 0;
	const CoreSetting *vars = CoreGetAllSettings(ctx->core, &vlen);

	for (uint32_t x = 0; x < vlen; x++) {
		CoreSystem system = vars[x].system;
		CoreSettingType type = vars[x].type;
		const char *desc = vars[x].desc;
		const char *key = vars[x].key;

		if (system != CORE_SYSTEM_UNKNOWN && system != ctx->system)
			continue;

		const char *cur = CoreGetSetting(ctx->core, key);
		if (!cur)
			continue;

		MTY_JSON *opt_item = MTY_JSONObjCreate();
		MTY_JSONObjSetString(cfg, key, cur);
		MTY_JSONObjSetItem(core_opts, key, opt_item);
		MTY_JSONObjSetString(opt_item, "name", desc);
		MTY_JSONObjSetString(opt_item, "type",
			type == CORE_SETTING_BOOL ? "checkbox" : "dropdown");

		MTY_JSON *opt_list = MTY_JSONArrayCreate(vars[x].nopts);
		MTY_JSONObjSetItem(opt_item, "list", opt_list);

		for (uint32_t y = 0; y < vars[x].nopts; y++) {
			const char *item = vars[x].opts[y];
			MTY_JSONArraySetString(opt_list, y, item);
		}
	}

	// Other native state
	bool has_disks = false;
	MTY_JSON *nstate = MTY_JSONObjCreate();
	MTY_JSONObjSetItem(msg, "nstate", nstate);
	MTY_JSONObjSetBool(nstate, "pause", ctx->paused);
	MTY_JSONObjSetBool(nstate, "running", CoreGameIsLoaded(ctx->core));
	MTY_JSONObjSetNumber(nstate, "save-state", ctx->last_save_index);
	MTY_JSONObjSetNumber(nstate, "load-state", ctx->last_load_index);
	MTY_JSONObjSetBool(nstate, "has_disks", has_disks);
	MTY_JSONObjSetNumber(nstate, "num_disks", 0);
	MTY_JSONObjSetNumber(nstate, "disk", -1);
	MTY_JSONObjSetBool(nstate, "allow_window_adjustments", !main_ui_is_steam());

	char *jmsg = MTY_JSONSerialize(msg);
	MTY_WebViewSendText(ctx->app, ctx->window, jmsg);

	MTY_Free(jmsg);
	MTY_JSONDestroy(&msg);
}

static void main_handle_ui_event(struct main *ctx, const char *text)
{
	MTY_JSON *j = MTY_JSONParse(text);
	if (!j)
		return;

	#define JBUF_SIZE 128

	char jbuf[JBUF_SIZE];
	if (!MTY_JSONObjGetString(j, "type", jbuf, JBUF_SIZE))
		goto except;

	// Actions
	if (!strcmp(jbuf, "action") || !strcmp(jbuf, "nstate")) {
		if (!MTY_JSONObjGetString(j, "name", jbuf, JBUF_SIZE))
			goto except;

		if (!strcmp(jbuf, "load-rom")) {
			char basedir[MTY_PATH_MAX] = {0};
			if (!MTY_JSONObjGetString(j, "basedir", basedir, MTY_PATH_MAX))
				goto except;

			char fname[MTY_PATH_MAX] = {0};
			if (!MTY_JSONObjGetString(j, "fname", fname, MTY_PATH_MAX))
				goto except;

			struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .fetch_core = true, .rt = true};
			snprintf(evt.game, MTY_PATH_MAX, "%s", MTY_JoinPath(basedir, fname));
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "unload-rom")) {
			struct app_event evt = {.type = APP_EVENT_UNLOAD_GAME, .rt = true};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "pause")) {
			struct app_event evt = {.type = APP_EVENT_PAUSE};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "reset")) {
			struct app_event evt = {.type = APP_EVENT_RESET, .rt = true};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "reset-window")) {
			MTY_Frame frame = MTY_MakeDefaultFrame(0, 0, MAIN_WINDOW_W, MAIN_WINDOW_H, 1.0f);
			MTY_WindowSetFrame(ctx->app, ctx->window, &frame);

			MTY_JSONObjSetBool(ctx->jcfg, "fullscreen", false);
			struct app_event evt = {.type = APP_EVENT_CONFIG};
			evt.cfg = main_parse_config(ctx->jcfg, &ctx->core_options, &ctx->core_exts);
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "quit")) {
			struct app_event evt = {.type = APP_EVENT_QUIT};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "hide-menu")) {
			struct app_event evt = {.type = APP_EVENT_HIDE_MENU};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "reload")) {
			if (!ctx->game_path)
				goto except;

			struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
			evt.fetch_core = true;
			snprintf(evt.game, MTY_PATH_MAX, "%s", ctx->game_path);
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "core-reset")) {
			struct app_event evt = {.type = APP_EVENT_CLEAR_OPTS, .rt = true};
			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "save-state")) {
			struct app_event evt = {.type = APP_EVENT_SAVE_STATE, .rt = true};

			if (!MTY_JSONObjGetInt8(j, "value", (int8_t *) &evt.state_index))
				goto except;

			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "load-state")) {
			struct app_event evt = {.type = APP_EVENT_LOAD_STATE, .rt = true};

			if (!MTY_JSONObjGetInt8(j, "value", (int8_t *) &evt.state_index))
				goto except;

			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "disk")) {
			struct app_event evt = {.type = APP_EVENT_SET_DISK, .rt = true};

			if (!MTY_JSONObjGetInt8(j, "value", &evt.disk))
				goto except;

			main_push_app_event(&evt, ctx);

		} else if (!strcmp(jbuf, "files")) {
			char basedir[MTY_PATH_MAX] = {0};
			MTY_JSONObjGetString(j, "basedir", basedir, MTY_PATH_MAX);

			char dir[MTY_PATH_MAX] = {0};
			MTY_JSONObjGetString(j, "dir", dir, MTY_PATH_MAX);

			if (!basedir[0])
				snprintf(basedir, MTY_PATH_MAX, "%s", MTY_GetProcessDir());

			const char *rdir = MTY_ResolvePath(dir[0] ? MTY_JoinPath(basedir, dir) : basedir);
			if (!rdir)
				rdir = MTY_GetProcessDir();

			main_post_ui_files(ctx->app, ctx->window, rdir);
		}

	// Configuration change
	} else if (!strcmp(jbuf, "cfg")) {
		if (!MTY_JSONObjGetString(j, "name", jbuf, JBUF_SIZE))
			goto except;

		const MTY_JSON *jval = MTY_JSONObjGetItem(j, "value");
		if (!jval)
			goto except;

		switch (MTY_JSONGetType(jval)) {
			case MTY_JSON_BOOL: {
				bool val = false;
				MTY_JSONBool(jval, &val);
				MTY_JSONObjSetBool(ctx->jcfg, jbuf, val);
				break;
			}
			case MTY_JSON_NUMBER: {
				double val = 0;
				MTY_JSONNumber(jval, &val);
				MTY_JSONObjSetNumber(ctx->jcfg, jbuf, val);
				break;
			}
			case MTY_JSON_STRING: {
				char val[JBUF_SIZE];
				MTY_JSONString(jval, val, JBUF_SIZE);
				MTY_JSONObjSetString(ctx->jcfg, jbuf, val);
				break;
			}
			default:
				break;
		}

		struct app_event evt = {.type = APP_EVENT_CONFIG};
		evt.cfg = main_parse_config(ctx->jcfg, &ctx->core_options, &ctx->core_exts);
		main_push_app_event(&evt, ctx);

	// Core options change
	} else if (!strcmp(jbuf, "core_opts")) {
		struct app_event evt = {.type = APP_EVENT_CORE_OPT, .rt = true};

		if (!MTY_JSONObjGetString(j, "name", evt.opt.key, CORE_KEY_NAME_MAX))
			goto except;

		if (!MTY_JSONObjGetString(j, "value", evt.opt.val, CORE_OPT_NAME_MAX))
			goto except;

		main_push_app_event(&evt, ctx);
	}

	except:

	MTY_JSONDestroy(&j);
}


// App events

static void main_push_app_event(const struct app_event *evt, void *opaque)
{
	struct main *ctx = opaque;

	MTY_Queue *q = evt->rt ? ctx->rt_q : ctx->mt_q;

	struct app_event *qbuf = MTY_QueueGetInputBuffer(q);
	if (qbuf) {
		*qbuf = *evt;
		MTY_QueuePush(q, sizeof(struct app_event));
	}
}

static void main_refresh_gfx(struct main *ctx, MTY_GFX gfx, uint32_t vsync)
{
	MTY_WindowSetGFX(ctx->app, ctx->window, gfx, vsync > 0);

	double rr = vsync > 0 ? MTY_WindowGetRefreshRate(ctx->app, ctx->window) : 0;
	MTY_WindowSetSyncInterval(ctx->app, ctx->window, lrint((rr / 60.0) * 100.0));
}

static void main_poll_app_events(struct main *ctx, MTY_Queue *q)
{
	for (struct app_event *evt = NULL; MTY_QueueGetOutputBuffer(q, 0, (void **) &evt, NULL);) {
		switch (evt->type) {
			case APP_EVENT_CONFIG:
				// Fullscreen/windowed transitions
				if (evt->cfg.fullscreen != ctx->cfg.fullscreen)
					MTY_WindowSetFullscreen(ctx->app, ctx->window, evt->cfg.fullscreen);

				// Graphics API change
				if (evt->cfg.gfx != ctx->cfg.gfx || evt->cfg.vsync != ctx->cfg.vsync) {
					struct app_event gevt = {.type = APP_EVENT_GFX, .rt = true};
					gevt.gfx = evt->cfg.gfx;
					gevt.vsync = evt->cfg.vsync;
					main_push_app_event(&gevt, ctx);
				}

				// Console
				if (evt->cfg.console != ctx->cfg.console) {
					if (evt->cfg.console) {
						MTY_OpenConsole(APP_NAME);

					} else {
						MTY_CloseConsole();
					}
				}

				// Audio
				if (evt->cfg.audio_buffer != ctx->cfg.audio_buffer ||
					evt->cfg.playback_rate != ctx->cfg.playback_rate ||
					evt->cfg.vsync != ctx->cfg.vsync)
				{
					ctx->audio_init = false;
					ctx->resampler_init = false;
				}

				ctx->cfg = evt->cfg;
				break;
			case APP_EVENT_QUIT:
				ctx->running = false;
				break;
			case APP_EVENT_GFX:
				main_refresh_gfx(ctx, evt->gfx, evt->vsync);
				break;
			case APP_EVENT_PAUSE:
				ctx->paused = !ctx->paused;
				break;
			case APP_EVENT_TITLE:
				MTY_WindowSetTitle(ctx->app, ctx->window, evt->title);
				break;
			case APP_EVENT_LOAD_GAME: {
				main_load_game(ctx, evt->game, evt->fetch_core);
				ctx->paused = false;

				struct app_event hevt = {.type = APP_EVENT_HIDE_MENU};
				main_push_app_event(&hevt, ctx);
				break;
			}
			case APP_EVENT_RESET: {
				CoreReset(ctx->core);
				ctx->paused = false;

				struct app_event hevt = {.type = APP_EVENT_HIDE_MENU};
				main_push_app_event(&hevt, ctx);
				break;
			}
			case APP_EVENT_UNLOAD_GAME: {
				main_unload(ctx);
				break;
			}
			case APP_EVENT_CLEAR_OPTS: {
				MTY_JSONDestroy(&ctx->core_options);
				ctx->core_options = MTY_JSONObjCreate();
				CoreResetSettings(ctx->core);

				struct app_event sevt = {.type = APP_EVENT_STATE};
				main_push_app_event(&sevt, ctx);
				break;
			}
			case APP_EVENT_CORE_OPT:
				MTY_JSONObjSetString(ctx->core_options, evt->opt.key, evt->opt.val);
				CoreSetSetting(ctx->core, evt->opt.key, evt->opt.val);
				break;
			case APP_EVENT_SAVE_STATE: {
				if (!ctx->content_name || !CoreGameIsLoaded(ctx->core))
					break;

				size_t size = 0;
				void *state = CoreGetState(ctx->core, &size);

				if (state) {
					ctx->last_save_index = evt->state_index;

					const char *path = config_state_dir();
					MTY_Mkdir(path);

					const char *name = MTY_SprintfDL("%s.state%u", ctx->content_name, evt->state_index);
					MTY_WriteFile(MTY_JoinPath(path, name), state, size);

					free(state);
				}

				struct app_event sevt = {.type = APP_EVENT_HIDE_MENU};
				main_push_app_event(&sevt, ctx);
				break;
			}
			case APP_EVENT_LOAD_STATE: {
				if (!ctx->content_name || !CoreGameIsLoaded(ctx->core))
					break;

				const char *name = MTY_SprintfDL("%s.state%u", ctx->content_name, evt->state_index);

				size_t size = 0;
				void *state = MTY_ReadFile(MTY_JoinPath(config_state_dir(), name), &size);

				if (state) {
					if (CoreSetState(ctx->core, state, size))
						ctx->last_load_index = evt->state_index;

					MTY_Free(state);
				}

				struct app_event sevt = {.type = APP_EVENT_HIDE_MENU};
				main_push_app_event(&sevt, ctx);
				break;
			}
			case APP_EVENT_SET_DISK:
				CoreInsertDisc(ctx->core, "");
				break;
			case APP_EVENT_HIDE_MENU:
				main_ui_show(ctx->app, ctx->window, false);
				break;
			case APP_EVENT_STATE:
				main_post_ui_state(ctx);
				break;
		}

		if (!evt->rt)
			main_post_ui_state(ctx);

		MTY_QueuePop(q);
	}
}


// Audio thread

struct at_state {
	uint32_t sample_rate;
	uint32_t target_rate;
	bool correct_high;
	bool correct_low;
};

static bool main_audio_init(uint32_t rate, uint32_t buffer, MTY_Resampler **rsp, MTY_Audio **audio)
{
	MTY_ResamplerDestroy(rsp);
	MTY_AudioDestroy(audio);

	*audio = MTY_AudioCreate(rate, buffer, buffer * 2, 2, NULL, false);
	if (!audio)
		return false;

	*rsp = MTY_ResamplerCreate();

	return true;
}

static void main_audio_packet(struct main *ctx, struct at_state *s, MTY_Resampler *rsp,
	MTY_Audio *audio, const struct audio_packet *pkt)
{
	// TODO Instead of dividing by 60.0, it would be better to know the actual refresh rate
	uint32_t scaled_rate = !ctx->cfg.vsync ? ctx->cfg.playback_rate :
		lrint((ctx->core_fps / 60.0) * ctx->cfg.playback_rate);

	// Reset resampler on sample rate changes
	if (s->sample_rate != pkt->sample_rate || !ctx->resampler_init) {
		MTY_ResamplerReset(rsp);
		MTY_AudioReset(audio);

		s->sample_rate = pkt->sample_rate;
		s->target_rate = scaled_rate;
		s->correct_high = s->correct_low = false;

		ctx->resampler_init = true;
	}

	// Submit the audio
	if (!ctx->cfg.mute) {
		size_t out_frames = 0;
		const int16_t *rsp_buf = MTY_Resample(rsp, (float) s->target_rate / s->sample_rate,
			pkt->data, pkt->frames, &out_frames);

		MTY_AudioQueue(audio, rsp_buf, (uint32_t) out_frames);
	}

	// Correct buffer drift by tweaking the output sample rate
	uint32_t low = lrint(ctx->cfg.audio_buffer / 1.5);
	uint32_t mid = ctx->cfg.audio_buffer;
	uint32_t high = ctx->cfg.audio_buffer + (ctx->cfg.audio_buffer - low);
	uint32_t queued = MTY_AudioGetQueued(audio);

	if (queued <= mid)
		s->correct_high = false;

	if (queued >= mid)
		s->correct_low = false;

	if (!s->correct_high && !s->correct_low) {
		if (queued >= high) {
			s->correct_high = true;
			s->target_rate = lrint(scaled_rate * 0.993);

		} else if (queued <= low) {
			s->correct_low = true;
			s->target_rate = lrint(scaled_rate * 1.007);

		} else {
			s->target_rate = scaled_rate;
		}
	}
}

static void *main_audio_thread(void *opaque)
{
	struct main *ctx = opaque;

	MTY_Audio *audio = NULL;
	MTY_Resampler *rsp = NULL;

	struct at_state s = {0};

	while (ctx->running) {
		// Reinit audio & resampler on the fly
		if (!ctx->audio_init) {
			ctx->audio_init = main_audio_init(ctx->cfg.playback_rate, ctx->cfg.audio_buffer,
				&rsp, &audio);

			if (!ctx->audio_init)
				break;
		}

		// Dequeue audio data from the core
		for (struct audio_packet *pkt = NULL;;) {
			if (!MTY_QueueGetOutputBuffer(ctx->a_q, 10, (void **) &pkt, NULL))
				break;

			main_audio_packet(ctx, &s, rsp, audio, pkt);
			MTY_QueuePop(ctx->a_q);
		}
	}

	MTY_ResamplerDestroy(&rsp);
	MTY_AudioDestroy(&audio);

	return NULL;
}


// Render thread

static void *main_render_thread(void *opaque)
{
	struct main *ctx = opaque;

	main_refresh_gfx(ctx, ctx->cfg.gfx, ctx->cfg.vsync);

	MTY_Time stamp = MTY_GetTime();

	while (ctx->running) {
		// Poll all events on the render thread
		main_poll_app_events(ctx, ctx->rt_q);

		// Poll a core fetch, then run game
		if (csync_poll_fetch_core(ctx->csync)) {
			struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
			snprintf(evt.game, MTY_PATH_MAX, "%s", ctx->next_game);
			main_push_app_event(&evt, ctx);
		}

		// Run video
		bool loaded = CoreGameIsLoaded(ctx->core);

		bool active = !ctx->paused &&
			((!ctx->cfg.bg_pause || MTY_WindowIsActive(ctx->app, ctx->window)) &&
			(!ctx->cfg.menu_pause || !ctx->ui_visible));

		if (active && loaded) {
			CoreRun(ctx->core);

		} else if (loaded) {
			main_video(NULL, CORE_COLOR_FORMAT_UNKNOWN, 0, 0, 0, ctx);

		} else {
			MTY_WindowClear(ctx->app, ctx->window, 0, 0, 0, 1);
		}

		// Frame timing when vsync is off
		double rem = 1000.0 / CoreGetFrameRate(ctx->core) - MTY_TimeDiff(stamp, MTY_GetTime());

		if (ctx->cfg.vsync == 0 && rem > 0)
			MTY_PreciseSleep(rem, 4.0);

		stamp = MTY_GetTime();

		// Present the frame
		MTY_WindowPresent(ctx->app, ctx->window);
	}

	MTY_WindowSetGFX(ctx->app, ctx->window, MTY_GFX_NONE, false);

	main_unload(ctx);

	return NULL;
}


// Main thread

static void main_event_func(const MTY_Event *evt, void *opaque)
{
	struct main *ctx = opaque;

	bool toggle_menu = false;

	switch (evt->type) {
		case MTY_EVENT_CLOSE:
		case MTY_EVENT_QUIT:
			ctx->running = false;
			break;
		case MTY_EVENT_DROP: {
			struct app_event devt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
			devt.fetch_core = true;
			snprintf(devt.game, MTY_PATH_MAX, "%s", evt->drop.name);
			main_push_app_event(&devt, ctx);
			break;
		}
		case MTY_EVENT_WEBVIEW_KEY:
		case MTY_EVENT_KEY: {
			// Native window only
			if (evt->type == MTY_EVENT_KEY) {
				MTY_AppShowCursor(ctx->app, false);

				CoreButton button = NES_KEYBOARD_MAP[evt->key.key];
				if (button != 0)
					CoreSetButton(ctx->core, 0, button, evt->key.pressed);
			}

			toggle_menu = evt->key.pressed && evt->key.key == MTY_KEY_ESCAPE && !ctx->ui_visible;
			break;
		}
		case MTY_EVENT_MOTION:
			MTY_AppShowCursor(ctx->app, true);
			break;
		case MTY_EVENT_CONTROLLER: {
			MTY_AppShowCursor(ctx->app, false);

			const MTY_ControllerEvent *c = &evt->controller;

			if (!ctx->ui_visible) {
				CoreSetButton(ctx->core, 0, CORE_BUTTON_A, c->buttons[MTY_CBUTTON_A]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_B, c->buttons[MTY_CBUTTON_B]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_X, c->buttons[MTY_CBUTTON_X]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_Y, c->buttons[MTY_CBUTTON_Y]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_SELECT, c->buttons[MTY_CBUTTON_BACK]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_START, c->buttons[MTY_CBUTTON_START]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_L, c->buttons[MTY_CBUTTON_LEFT_SHOULDER]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_R, c->buttons[MTY_CBUTTON_RIGHT_SHOULDER]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_DPAD_U, c->buttons[MTY_CBUTTON_DPAD_UP]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_DPAD_D, c->buttons[MTY_CBUTTON_DPAD_DOWN]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_DPAD_L, c->buttons[MTY_CBUTTON_DPAD_LEFT]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_DPAD_R, c->buttons[MTY_CBUTTON_DPAD_RIGHT]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_L2, c->buttons[MTY_CBUTTON_LEFT_TRIGGER]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_R2, c->buttons[MTY_CBUTTON_RIGHT_TRIGGER]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_L3, c->buttons[MTY_CBUTTON_LEFT_THUMB]);
				CoreSetButton(ctx->core, 0, CORE_BUTTON_R3, c->buttons[MTY_CBUTTON_RIGHT_THUMB]);

				CoreSetAxis(ctx->core, 0, CORE_AXIS_LX, c->axes[MTY_CAXIS_THUMB_LX].value);
				CoreSetAxis(ctx->core, 0, CORE_AXIS_LY, c->axes[MTY_CAXIS_THUMB_LY].value);
				CoreSetAxis(ctx->core, 0, CORE_AXIS_RX, c->axes[MTY_CAXIS_THUMB_RX].value);
				CoreSetAxis(ctx->core, 0, CORE_AXIS_RY, c->axes[MTY_CAXIS_THUMB_RY].value);

			} else {
				main_post_ui_controller(ctx->app, ctx->window, c);
			}

			bool pressed = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER];

			toggle_menu = pressed && ctx->ui_debounce != pressed;
			ctx->ui_debounce = pressed;
			break;
		}
		case MTY_EVENT_WEBVIEW_READY:
			if (ctx->show_ui)
				main_ui_show(ctx->app, ctx->window, true);

			main_post_ui_state(ctx);
			break;
		case MTY_EVENT_WEBVIEW_TEXT:
			main_handle_ui_event(ctx, evt->webviewText);
			break;
		default:
			break;
	}

	if (toggle_menu)
		main_ui_show(ctx->app, ctx->window, !ctx->ui_visible);
}

static bool main_app_func(void *opaque)
{
	struct main *ctx = opaque;

	ctx->ui_visible = main_ui_visible(ctx->app, ctx->window);

	main_poll_app_events(ctx, ctx->mt_q);

	return ctx->running;
}

static void main_mty_log_callback(const char *msg, void *opaque)
{
	printf("%s\n", msg);
}

int32_t main(int32_t argc, char **argv)
{
	setlocale(LC_ALL, ".utf8");

	MTY_Mkdir(config_asset_dir());

	// Get the function pointers assigned
	loader_reset();

	struct main ctx = {0};
	ctx.running = true;
	ctx.core_fps = 60;

	ctx.jcfg = main_load_config(&ctx.cfg, &ctx.core_options, &ctx.core_exts);

	if (ctx.cfg.console)
		MTY_OpenConsole(APP_NAME);

	ctx.csync = csync_start();

	MTY_SetTimerResolution(1);
	MTY_SetLogFunc(main_mty_log_callback, NULL);

	ctx.rt_q = MTY_QueueCreate(50, sizeof(struct app_event));
	ctx.mt_q = MTY_QueueCreate(50, sizeof(struct app_event));
	ctx.a_q = MTY_QueueCreate(15, sizeof(struct audio_packet));

	if (argc >= 2) {
		struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
		evt.fetch_core = true;
		snprintf(evt.game, MTY_PATH_MAX, "%s", argv[1]);
		main_push_app_event(&evt, &ctx);

	} else {
		ctx.show_ui = true;
	}

	ctx.app = MTY_AppCreate(0, main_app_func, main_event_func, &ctx);
	MTY_AppSetTimeout(ctx.app, 1);

	MTY_Frame frame = ctx.cfg.window;
	if (frame.size.w == 0) {
		frame = MTY_MakeDefaultFrame(0, 0, MAIN_WINDOW_W, MAIN_WINDOW_H, 1.0f);

		if (main_ui_is_steam())
			frame.type = MTY_WINDOW_FULLSCREEN;
	}

	ctx.window = MTY_WindowCreate(ctx.app, APP_NAME, &frame, 0);
	if (ctx.window == -1)
		goto except;

	MTY_WindowSetMinSize(ctx.app, ctx.window, 256, 240);

	main_ui_init(ctx.app, ctx.window);

	MTY_Thread *rt = MTY_ThreadCreate(main_render_thread, &ctx);
	MTY_Thread *at = MTY_ThreadCreate(main_audio_thread, &ctx);
	MTY_AppRun(ctx.app);
	MTY_ThreadDestroy(&at);
	MTY_ThreadDestroy(&rt);

	ctx.cfg.window = MTY_WindowGetFrame(ctx.app, ctx.window);
	ctx.cfg.fullscreen = ctx.cfg.window.type & MTY_WINDOW_FULLSCREEN;
	main_save_config(&ctx.cfg, ctx.core_options, ctx.core_exts);

	except:

	MTY_RevertTimerResolution(1);
	MTY_AppDestroy(&ctx.app);
	MTY_QueueDestroy(&ctx.rt_q);
	MTY_QueueDestroy(&ctx.mt_q);
	MTY_QueueDestroy(&ctx.a_q);
	MTY_JSONDestroy(&ctx.core_options);
	MTY_JSONDestroy(&ctx.core_exts);
	MTY_JSONDestroy(&ctx.jcfg);

	csync_stop(&ctx.csync);

	return 0;
}

#if defined(_WIN32)

#include <windows.h>
#include <combaseapi.h>

int32_t WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow)
{
	hInstance; hPrevInstance; lpCmdLine; nCmdShow;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	return main(__argc, __argv);
}
#endif
