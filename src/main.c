#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "matoya.h"

#include "app.h"
#include "core.h"
#include "config.h"
#include "ui-zip.h"

#define APP_TITLE_MAX 1024
#define MAIN_WINDOW_W 800
#define MAIN_WINDOW_H 600

#if defined(MTN_DEBUG)
	#define MTN_DEBUG_WEBVIEW true
#else
	#define MTN_DEBUG_WEBVIEW false
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
	APP_EVENT_VSYNC       = 15,
	APP_EVENT_STATE       = 16,
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
	struct core *core;

	char *content_name;
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
	bool menu_visible;
	bool menu_debounce;
	bool running;
	bool paused;
	bool audio_init;
	bool show_ui;

	MTY_RenderDesc desc;

	struct {
		uint32_t req;
		char file[MTY_PATH_MAX];
		char name[MTY_PATH_MAX];
	} core_fetch;
};


// Maps

static const enum core_button NES_KEYBOARD_MAP[MTY_KEY_MAX] = {
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


// Helpers

static const char *main_asset_dir(void)
{
	return MTY_JoinPath(MTY_GetProcessDir(), "merton-files");
}

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

	CFG_GET_STR(core.atari2600, CONFIG_CORE_MAX, "stella");
	CFG_GET_STR(core.gameboy, CONFIG_CORE_MAX, "sameboy");
	CFG_GET_STR(core.gba, CONFIG_CORE_MAX, "mgba");
	CFG_GET_STR(core.genesis, CONFIG_CORE_MAX, "genesis_plus_gx");
	CFG_GET_STR(core.ms, CONFIG_CORE_MAX, "genesis_plus_gx");
	CFG_GET_STR(core.n64, CONFIG_CORE_MAX, "mupen64plus_next");
	CFG_GET_STR(core.nes, CONFIG_CORE_MAX, "mesen");
	CFG_GET_STR(core.ps, CONFIG_CORE_MAX, "swanstation");
	CFG_GET_STR(core.snes, CONFIG_CORE_MAX, "mesen-s");
	CFG_GET_STR(core.tg16, CONFIG_CORE_MAX, "mednafen_pce");

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
		MTY_JSONObjSetString(*core_exts, "ms", "sms");
		MTY_JSONObjSetString(*core_exts, "n64", "n64|v64|z64");
		MTY_JSONObjSetString(*core_exts, "nes", "nes|fds|qd|unf|unif");
		MTY_JSONObjSetString(*core_exts, "ps", "cue");
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

	MTY_JSONWriteFile(MTY_JoinPath(main_asset_dir(), "config.json"), jcfg);
	MTY_JSONDestroy(&jcfg);
}

static MTY_JSON *main_load_config(struct config *cfg, MTY_JSON **core_options, MTY_JSON **core_exts)
{
	MTY_JSON *jcfg = MTY_JSONReadFile(MTY_JoinPath(main_asset_dir(), "config.json"));
	if (!jcfg)
		jcfg = MTY_JSONObjCreate();

	*cfg = main_parse_config(jcfg, core_options, core_exts);
	MTY_JSONDestroy(&jcfg);

	return main_serialize_config(cfg, *core_options, *core_exts);
}


// Dynamic core fetching

static void main_push_app_event(const struct app_event *evt, void *opaque);

static const char *main_get_platform(void)
{
	uint32_t platform = MTY_GetPlatform();

	switch (platform & 0xFF000000) {
		case MTY_OS_WINDOWS: return "windows";
		case MTY_OS_MACOS:   return "macos";
		case MTY_OS_ANDROID: return "android";
		case MTY_OS_LINUX:   return "linux";
		case MTY_OS_WEB:     return "web";
		case MTY_OS_IOS:     return "ios";
		case MTY_OS_TVOS:    return "tvos";
	}

	return "unknown";
}

static void main_fetch_core(struct main *ctx, const char *file, const char *name)
{
	snprintf(ctx->core_fetch.name, MTY_PATH_MAX, "%s", name);
	snprintf(ctx->core_fetch.file, MTY_PATH_MAX, "%s", file);

	const char *url = MTY_SprintfDL("https://snowcone.ltd/cores/%s/x86_64/%s", main_get_platform(), file);
	MTY_HttpAsyncRequest(&ctx->core_fetch.req, url, "GET", NULL, NULL, 0, NULL, 10000, false);
}

static void main_poll_core_fetch(struct main *ctx)
{
	uint16_t status = 0;
	void *so = NULL;
	size_t size = 0;

	MTY_Async async = MTY_HttpAsyncPoll(ctx->core_fetch.req, &so, &size, &status);

	if (async == MTY_ASYNC_OK) {
		if (status == 200) {
			const char *base = MTY_JoinPath(main_asset_dir(), "cores");
			MTY_Mkdir(base);

			const char *path = MTY_JoinPath(base, ctx->core_fetch.file);

			if (MTY_WriteFile(path, so, size)) {
				struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
				snprintf(evt.game, MTY_PATH_MAX, "%s", ctx->core_fetch.name);
				main_push_app_event(&evt, ctx);
			}
		}

		MTY_HttpAsyncClear(&ctx->core_fetch.req);
	}
}


// Core

static void main_video(const void *buf, uint32_t width, uint32_t height, size_t pitch, void *opaque)
{
	struct main *ctx = opaque;

	// A NULL buffer means we should render the previous frame
	enum core_color_format format = buf ? core_get_color_format(ctx->core) :
		CORE_COLOR_FORMAT_UNKNOWN;

	ctx->desc.format =
		format == CORE_COLOR_FORMAT_BGRA ? MTY_COLOR_FORMAT_BGRA :
		format == CORE_COLOR_FORMAT_B5G6R5 ? MTY_COLOR_FORMAT_BGR565 :
		format == CORE_COLOR_FORMAT_B5G5R5A1 ? MTY_COLOR_FORMAT_BGRA5551 :
		MTY_COLOR_FORMAT_UNKNOWN;

	if (ctx->desc.format != MTY_COLOR_FORMAT_UNKNOWN) {
		ctx->desc.imageWidth = (uint32_t) (format == CORE_COLOR_FORMAT_BGRA ? pitch / 4 : pitch / 2);
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
	if (ctx->cfg.int_scaling && ctx->desc.imageWidth > 0 && ctx->desc.imageHeight > 0) {
		MTY_Size size = MTY_WindowGetSize(ctx->app, ctx->window);

		uint32_t w_multi = size.w / ctx->desc.imageWidth;
		uint32_t h_multi = size.h / ctx->desc.imageHeight;

		ctx->desc.scale = (float) (w_multi < h_multi ? w_multi : h_multi);

	} else {
		ctx->desc.scale = 0;
	}

	// Square pixels
	ctx->desc.aspectRatio = !ctx->cfg.square_pixels ?
		core_get_aspect_ratio(ctx->core) : ctx->desc.imageHeight > 0 ?
		(float) ctx->desc.imageWidth / ctx->desc.imageHeight : 1;

	MTY_WindowDrawQuad(ctx->app, ctx->window, buf, &ctx->desc);
}

static void main_audio(const int16_t *buf, size_t frames, void *opaque)
{
	struct main *ctx = opaque;

	struct audio_packet *pkt = MTY_QueueGetInputBuffer(ctx->a_q);

	if (pkt) {
		pkt->sample_rate = core_get_sample_rate(ctx->core);
		pkt->frames = frames;

		memcpy(pkt->data, buf, frames * 4);

		MTY_QueuePush(ctx->a_q, sizeof(struct audio_packet));
	}
}

static void main_log(const char *msg, void *opaque)
{
	printf("%s", msg);
}

static void main_get_system_by_ext(struct main *ctx, const char *name,
	const char **core, const char **system)
{
	*core = NULL;
	*system = NULL;

	const char *ext = MTY_GetFileExtension(name);

	uint64_t iter = 0;

	for (const char *key = NULL; MTY_JSONObjGetNextKey(ctx->core_exts, &iter, &key);) {
		char exts[SYSTEM_EXTS_MAX];

		if (MTY_JSONObjGetString(ctx->core_exts, key, exts, SYSTEM_EXTS_MAX)) {
			const char *substr = MTY_Strcasestr(exts, ext);
			size_t end = strlen(ext);

			if (substr && (substr[end] == '\0' || substr[end] == '|')) {
				*core = CONFIG_GET_CORE(&ctx->cfg, key);
				*system = key;
				break;
			}
		}
	}
}

static void main_set_core_options(struct main *ctx)
{
	uint64_t iter = 0;

	for (const char *key = NULL; MTY_JSONObjGetNextKey(ctx->core_options, &iter, &key);) {
		char val[CORE_OPT_NAME_MAX];

		if (MTY_JSONObjGetString(ctx->core_options, key, val, CORE_OPT_NAME_MAX))
			core_set_variable(ctx->core, key, val);
	}
}

static void main_read_sram(struct core *core, const char *content_name)
{
	const char *name = MTY_SprintfDL("%s.srm", content_name);

	size_t size = 0;
	void *sram = MTY_ReadFile(MTY_JoinPath(core_get_save_dir(core), name), &size);
	if (sram) {
		core_set_sram(core, sram, size);
		MTY_Free(sram);
	}
}

static void main_save_sram(struct core *core, const char *content_name)
{
	if (!content_name)
		return;

	size_t size = 0;
	void *sram = core_get_sram(core, &size);
	if (sram) {
		const char *name = MTY_SprintfDL("%s.srm", content_name);
		const char *dir = core_get_save_dir(core);

		MTY_Mkdir(dir);
		MTY_WriteFile(MTY_JoinPath(dir, name), sram, size);
		MTY_Free(sram);
	}
}

static void main_unload(struct main *ctx, bool unload_core)
{
	main_save_sram(ctx->core, ctx->content_name);

	if (unload_core) {
		core_unload(&ctx->core);

	} else {
		core_unload_game(ctx->core);
	}

	MTY_Free(ctx->content_name);
	ctx->content_name = NULL;
}

static void main_load_game(struct main *ctx, const char *name, bool fetch_core)
{
	const char *core = NULL;
	const char *system = NULL;
	main_get_system_by_ext(ctx, name, &core, &system);
	if (!core)
		return;

	main_unload(ctx, true);

	const char *cname = MTY_SprintfDL("%s.%s", core, MTY_GetSOExtension());
	const char *core_path = MTY_JoinPath(MTY_JoinPath(main_asset_dir(), "cores"), cname);

	// If core is on the system, try to use it
	if (MTY_FileExists(core_path)) {
		ctx->core = core_load(core_path, main_asset_dir());
		if (!ctx->core)
			return;

		main_set_core_options(ctx);

		core_set_log_func(main_log, &ctx);
		core_set_audio_func(ctx->core, main_audio, ctx);
		core_set_video_func(ctx->core, main_video, ctx);

		if (!core_load_game(ctx->core, name))
			return;

		ctx->content_name = MTY_Strdup(MTY_GetFileName(name, false));
		main_read_sram(ctx->core, ctx->content_name);

		struct app_event evt = {.type = APP_EVENT_TITLE};
		snprintf(evt.title, APP_TITLE_MAX, "%s - %s", APP_NAME, ctx->content_name);
		main_push_app_event(&evt, ctx);

	// Get the core from the internet
	} else if (fetch_core) {
		main_fetch_core(ctx, cname, name);
	}
}


// App events

static void main_post_webview_state(struct main *ctx)
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
	const struct core_variable *vars = core_get_variables(ctx->core, &vlen);

	for (uint32_t x = 0; x < vlen; x++) {
		const char *desc = vars[x].desc;
		const char *key = vars[x].key;

		const char *cur = core_get_variable(ctx->core, key);
		if (!cur)
			cur = vars[x].opts[0];

		MTY_JSON *opt_item = MTY_JSONObjCreate();
		MTY_JSONObjSetString(cfg, key, cur);
		MTY_JSONObjSetItem(core_opts, key, opt_item);
		MTY_JSONObjSetString(opt_item, "name", desc);

		MTY_JSON *opt_list = MTY_JSONArrayCreate(vars[x].nopts);
		MTY_JSONObjSetItem(opt_item, "list", opt_list);

		for (uint32_t y = 0; y < vars[x].nopts; y++) {
			const char *item = vars[x].opts[y];
			MTY_JSONArraySetString(opt_list, y, item);
		}
	}

	// Other native state
	bool has_disks = core_has_disk_interface(ctx->core);
	MTY_JSON *nstate = MTY_JSONObjCreate();
	MTY_JSONObjSetItem(msg, "nstate", nstate);
	MTY_JSONObjSetBool(nstate, "pause", ctx->paused);
	MTY_JSONObjSetBool(nstate, "running", core_game_is_loaded(ctx->core));
	MTY_JSONObjSetNumber(nstate, "save-state", ctx->last_save_index);
	MTY_JSONObjSetNumber(nstate, "load-state", ctx->last_load_index);
	MTY_JSONObjSetBool(nstate, "has_disks", has_disks);
	MTY_JSONObjSetNumber(nstate, "num_disks", core_get_num_disks(ctx->core));
	MTY_JSONObjSetNumber(nstate, "disk", has_disks ? core_get_disk(ctx->core) : -1);

	char *jmsg = MTY_JSONSerialize(msg);
	MTY_WebViewSendText(ctx->app, ctx->window, jmsg);

	MTY_Free(jmsg);
	MTY_JSONDestroy(&msg);
}

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

static void main_poll_app_events(struct main *ctx, MTY_Queue *q)
{
	for (struct app_event *evt = NULL; MTY_QueueGetOutputBuffer(q, 0, (void **) &evt, NULL);) {
		switch (evt->type) {
			case APP_EVENT_CONFIG:
				// Fullscreen/windowed transitions
				if (evt->cfg.fullscreen != ctx->cfg.fullscreen)
					MTY_WindowSetFullscreen(ctx->app, ctx->window, evt->cfg.fullscreen);

				// Graphics API change
				if (evt->cfg.gfx != ctx->cfg.gfx) {
					struct app_event gevt = {.type = APP_EVENT_GFX, .rt = true};
					gevt.gfx = evt->cfg.gfx;
					gevt.vsync = evt->cfg.vsync;
					main_push_app_event(&gevt, ctx);
				}

				// VSync change
				if (evt->cfg.vsync != ctx->cfg.vsync) {
					struct app_event vevt = {.type = APP_EVENT_VSYNC, .rt = true};
					vevt.vsync = evt->cfg.vsync;
					main_push_app_event(&vevt, ctx);
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
				}

				ctx->cfg = evt->cfg;
				break;
			case APP_EVENT_QUIT:
				ctx->running = false;
				break;
			case APP_EVENT_GFX:
				MTY_WindowSetGFX(ctx->app, ctx->window, evt->gfx, evt->vsync > 0);
				MTY_WindowSetSyncInterval(ctx->app, ctx->window, evt->vsync);
				break;
			case APP_EVENT_VSYNC:
				MTY_WindowSetSyncInterval(ctx->app, ctx->window, evt->vsync);
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
				core_reset_game(ctx->core);
				ctx->paused = false;

				struct app_event hevt = {.type = APP_EVENT_HIDE_MENU};
				main_push_app_event(&hevt, ctx);
				break;
			}
			case APP_EVENT_UNLOAD_GAME: {
				main_unload(ctx, false);

				struct app_event tevt = {.type = APP_EVENT_TITLE};
				snprintf(tevt.title, APP_TITLE_MAX, "%s", APP_NAME);
				main_push_app_event(&tevt, ctx);
				break;
			}
			case APP_EVENT_CLEAR_OPTS: {
				MTY_JSONDestroy(&ctx->core_options);
				ctx->core_options = MTY_JSONObjCreate();
				core_clear_variables(ctx->core);

				struct app_event sevt = {.type = APP_EVENT_STATE};
				main_push_app_event(&sevt, ctx);
				break;
			}
			case APP_EVENT_CORE_OPT:
				MTY_JSONObjSetString(ctx->core_options, evt->opt.key, evt->opt.val);
				core_set_variable(ctx->core, evt->opt.key, evt->opt.val);
				break;
			case APP_EVENT_SAVE_STATE: {
				if (!ctx->content_name || !core_game_is_loaded(ctx->core))
					break;

				size_t size = 0;
				void *state = core_get_state(ctx->core, &size);

				if (state) {
					ctx->last_save_index = evt->state_index;

					const char *path = MTY_JoinPath(main_asset_dir(), "state");
					MTY_Mkdir(path);

					const char *name = MTY_SprintfDL("%s.state%u", ctx->content_name, evt->state_index);
					MTY_WriteFile(MTY_JoinPath(path, name), state, size);

					free(state);
				}
				break;
			}
			case APP_EVENT_LOAD_STATE: {
				if (!ctx->content_name || !core_game_is_loaded(ctx->core))
					break;

				const char *path = MTY_JoinPath(main_asset_dir(), "state");
				MTY_Mkdir(path);

				const char *name = MTY_SprintfDL("%s.state%u", ctx->content_name, evt->state_index);

				size_t size = 0;
				void *state = MTY_ReadFile(MTY_JoinPath(path, name), &size);

				if (state) {
					if (core_set_state(ctx->core, state, size))
						ctx->last_load_index = evt->state_index;

					MTY_Free(state);
				}
				break;
			}
			case APP_EVENT_SET_DISK:
				core_set_disk(ctx->core, evt->disk);
				break;
			case APP_EVENT_HIDE_MENU:
				MTY_WebViewShow(ctx->app, ctx->window, false);
				break;
			case APP_EVENT_STATE:
				main_post_webview_state(ctx);
				break;
		}

		if (!evt->rt)
			main_post_webview_state(ctx);

		MTY_QueuePop(q);
	}
}


// Audio thread

static void *main_audio_thread(void *opaque)
{
	struct main *ctx = opaque;

	uint32_t pcm_buffer = ctx->cfg.audio_buffer;
	uint32_t playback_rate = ctx->cfg.playback_rate;

	MTY_Audio *audio = NULL;
	MTY_Resampler *rsp = NULL;

	uint32_t sample_rate = 0;
	uint32_t target_rate = 0;
	bool correct_high = false;
	bool correct_low = false;

	while (ctx->running) {
		// Reinit audio & resampler on the fly
		if (!ctx->audio_init) {
			pcm_buffer = ctx->cfg.audio_buffer;
			playback_rate = ctx->cfg.playback_rate;
			sample_rate = 0;

			if (rsp)
				MTY_ResamplerDestroy(&rsp);

			if (audio)
				MTY_AudioDestroy(&audio);

			audio = MTY_AudioCreate(playback_rate, pcm_buffer, pcm_buffer * 2, 2, NULL, false);
			if (!audio)
				break;

			if (!rsp)
				rsp = MTY_ResamplerCreate();

			ctx->audio_init = true;
		}

		// Dequeue audio data from the core
		for (struct audio_packet *pkt = NULL; MTY_QueueGetOutputBuffer(ctx->a_q, 10, (void **) &pkt, NULL);) {
			// TODO TARGET_RATE should likely be some kind of modified sample rate based on
			// the actual FPS vs. the core's target FPS. If using adaptive sync, these numbers should
			// always match
			#define TARGET_RATE(rate) (rate)

			// Reset resampler on sample rate changes
			if (sample_rate != pkt->sample_rate) {
				MTY_ResamplerReset(rsp);

				sample_rate = pkt->sample_rate;
				target_rate = lrint(TARGET_RATE(playback_rate));
				correct_high = correct_low = false;
			}

			// Submit the audio
			if (!ctx->cfg.mute) {
				size_t out_frames = 0;
				const int16_t *rsp_buf = MTY_Resample(rsp, (float) target_rate / sample_rate,
					pkt->data, pkt->frames, &out_frames);

				MTY_AudioQueue(audio, rsp_buf, (uint32_t) out_frames);
			}

			// Correct buffer drift by tweaking the output sample rate
			uint32_t low = pcm_buffer / 2;
			uint32_t mid = pcm_buffer;
			uint32_t high = pcm_buffer + low;
			uint32_t queued = MTY_AudioGetQueued(audio);

			if (queued <= mid)
				correct_high = false;

			if (queued >= mid)
				correct_low = false;

			if (!correct_high && !correct_low) {
				if (queued >= high) {
					correct_high = true;
					target_rate = lrint(TARGET_RATE(playback_rate) * 0.995);

				} else if (queued <= low) {
					correct_low = true;
					target_rate = lrint(TARGET_RATE(playback_rate) * 1.005);

				} else {
					target_rate = lrint(TARGET_RATE(playback_rate));
				}
			}

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

	MTY_WindowSetGFX(ctx->app, ctx->window, ctx->cfg.gfx, ctx->cfg.vsync > 0);
	MTY_WindowSetSyncInterval(ctx->app, ctx->window, ctx->cfg.vsync);

	while (ctx->running) {
		MTY_Time stamp = MTY_GetTime();

		main_poll_app_events(ctx, ctx->rt_q);
		main_poll_core_fetch(ctx);

		bool loaded = core_game_is_loaded(ctx->core);

		bool active = !ctx->paused &&
			((!ctx->cfg.bg_pause || MTY_WindowIsActive(ctx->app, ctx->window)) &&
			(!ctx->cfg.menu_pause || !ctx->menu_visible));

		if (active && loaded) {
			core_run_frame(ctx->core);

		} else {
			if (loaded) {
				main_video(NULL, 0, 0, 0, ctx);

			} else {
				MTY_WindowClear(ctx->app, ctx->window, 0, 0, 0, 1);
			}
		}

		MTY_WindowPresent(ctx->app, ctx->window);

		double diff = MTY_TimeDiff(stamp, MTY_GetTime());

		if (ctx->cfg.vsync == 0)
			MTY_PreciseSleep(1000.0 / core_get_frame_rate(ctx->core) - diff, 4.0);
	}

	MTY_WindowSetGFX(ctx->app, ctx->window, MTY_GFX_NONE, false);

	main_unload(ctx, true);

	return NULL;
}


// Main thread

static void main_handle_webview_text(struct main *ctx, const char *text)
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
			const char *name = core_get_game_path(ctx->core);

			if (name) {
				struct app_event evt = {.type = APP_EVENT_LOAD_GAME, .rt = true};
				evt.fetch_core = true;
				snprintf(evt.game, MTY_PATH_MAX, "%s", name);
				main_push_app_event(&evt, ctx);
			}

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

			if (!basedir[0])
				snprintf(basedir, MTY_PATH_MAX, "%s", MTY_GetProcessDir());

			const char *jdir = basedir;

			char dir[MTY_PATH_MAX] = {0};
			MTY_JSONObjGetString(j, "dir", dir, MTY_PATH_MAX);

			if (dir[0])
				jdir = MTY_JoinPath(basedir, dir);

			jdir = MTY_ResolvePath(jdir);
			if (!jdir)
				jdir = MTY_GetProcessDir();

			MTY_FileList *list = MTY_GetFileList(jdir, NULL);
			if (list) {
				MTY_JSON *jmsg = MTY_JSONObjCreate();
				MTY_JSON *jlist = MTY_JSONArrayCreate(list->len);

				MTY_JSONObjSetString(jmsg, "type", "files");
				MTY_JSONObjSetString(jmsg, "path", jdir);
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
				MTY_WebViewSendText(ctx->app, ctx->window, jstr);

				MTY_Free(jstr);
				MTY_JSONDestroy(&jmsg);
			}
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

				enum core_button button = NES_KEYBOARD_MAP[evt->key.key];
				if (button != 0)
					core_set_button(ctx->core, 0, button, evt->key.pressed);
			}

			toggle_menu = evt->key.pressed && evt->key.key == MTY_KEY_ESCAPE && !ctx->menu_visible;
			break;
		}
		case MTY_EVENT_MOTION:
			MTY_AppShowCursor(ctx->app, true);
			break;
		case MTY_EVENT_CONTROLLER: {
			MTY_AppShowCursor(ctx->app, false);

			const MTY_ControllerEvent *c = &evt->controller;

			if (!ctx->menu_visible) {

				#define REV_AXIS(axis) \
					((axis) == INT16_MAX ? INT16_MIN : (axis) == INT16_MIN ? INT16_MAX : -(axis))

				core_set_button(ctx->core, 0, CORE_BUTTON_A, c->buttons[MTY_CBUTTON_B]);
				core_set_button(ctx->core, 0, CORE_BUTTON_B, c->buttons[MTY_CBUTTON_A]);
				core_set_button(ctx->core, 0, CORE_BUTTON_X, c->buttons[MTY_CBUTTON_Y]);
				core_set_button(ctx->core, 0, CORE_BUTTON_Y, c->buttons[MTY_CBUTTON_X]);
				core_set_button(ctx->core, 0, CORE_BUTTON_SELECT, c->buttons[MTY_CBUTTON_BACK]);
				core_set_button(ctx->core, 0, CORE_BUTTON_START, c->buttons[MTY_CBUTTON_START]);
				core_set_button(ctx->core, 0, CORE_BUTTON_L, c->buttons[MTY_CBUTTON_LEFT_SHOULDER]);
				core_set_button(ctx->core, 0, CORE_BUTTON_R, c->buttons[MTY_CBUTTON_RIGHT_SHOULDER]);
				core_set_button(ctx->core, 0, CORE_BUTTON_DPAD_U, c->buttons[MTY_CBUTTON_DPAD_UP]);
				core_set_button(ctx->core, 0, CORE_BUTTON_DPAD_D, c->buttons[MTY_CBUTTON_DPAD_DOWN]);
				core_set_button(ctx->core, 0, CORE_BUTTON_DPAD_L, c->buttons[MTY_CBUTTON_DPAD_LEFT]);
				core_set_button(ctx->core, 0, CORE_BUTTON_DPAD_R, c->buttons[MTY_CBUTTON_DPAD_RIGHT]);
				core_set_button(ctx->core, 0, CORE_BUTTON_L2, c->buttons[MTY_CBUTTON_LEFT_TRIGGER]);
				core_set_button(ctx->core, 0, CORE_BUTTON_R2, c->buttons[MTY_CBUTTON_RIGHT_TRIGGER]);
				core_set_button(ctx->core, 0, CORE_BUTTON_L3, c->buttons[MTY_CBUTTON_LEFT_THUMB]);
				core_set_button(ctx->core, 0, CORE_BUTTON_R3, c->buttons[MTY_CBUTTON_RIGHT_THUMB]);

				core_set_axis(ctx->core, 0, CORE_AXIS_LX, c->axes[MTY_CAXIS_THUMB_LX].value);
				core_set_axis(ctx->core, 0, CORE_AXIS_LY, REV_AXIS(c->axes[MTY_CAXIS_THUMB_LY].value));
				core_set_axis(ctx->core, 0, CORE_AXIS_RX, c->axes[MTY_CAXIS_THUMB_RX].value);
				core_set_axis(ctx->core, 0, CORE_AXIS_RY, REV_AXIS(c->axes[MTY_CAXIS_THUMB_RY].value));

			} else {
				MTY_JSON *msg = MTY_JSONObjCreate();
				MTY_JSONObjSetString(msg, "type", "controller");
				MTY_JSONObjSetBool(msg, "b", c->buttons[MTY_CBUTTON_B]);
				MTY_JSONObjSetBool(msg, "a", c->buttons[MTY_CBUTTON_A]);
				MTY_JSONObjSetBool(msg, "u", c->buttons[MTY_CBUTTON_DPAD_UP]);
				MTY_JSONObjSetBool(msg, "d", c->buttons[MTY_CBUTTON_DPAD_DOWN]);
				MTY_JSONObjSetBool(msg, "l", c->buttons[MTY_CBUTTON_DPAD_LEFT]);
				MTY_JSONObjSetBool(msg, "r", c->buttons[MTY_CBUTTON_DPAD_RIGHT]);

				char *jmsg = MTY_JSONSerialize(msg);
				MTY_WebViewSendText(ctx->app, ctx->window, jmsg);

				MTY_Free(jmsg);
				MTY_JSONDestroy(&msg);
			}

			bool pressed = c->buttons[MTY_CBUTTON_LEFT_TRIGGER];

			toggle_menu = pressed && ctx->menu_debounce != pressed;
			ctx->menu_debounce = pressed;
			break;
		}
		case MTY_EVENT_WEBVIEW_READY:
			if (ctx->show_ui)
				MTY_WebViewShow(ctx->app, ctx->window, true);

			main_post_webview_state(ctx);
			break;
		case MTY_EVENT_WEBVIEW_TEXT:
			main_handle_webview_text(ctx, evt->webviewText);
			break;
		default:
			break;
	}

	if (toggle_menu)
		MTY_WebViewShow(ctx->app, ctx->window, !ctx->menu_visible);
}

static bool main_app_func(void *opaque)
{
	struct main *ctx = opaque;

	ctx->menu_visible = MTY_WebViewIsVisible(ctx->app, ctx->window);

	main_poll_app_events(ctx, ctx->mt_q);

	return ctx->running;
}

static void main_mty_log_callback(const char *msg, void *opaque)
{
	printf("%s\n", msg);
}

static void main_set_webview(struct main *ctx)
{
	char *dir = (char *) MTY_GetProcessDir();

	for (size_t x = 0; x < strlen(dir); x++)
		if (dir[x] == '\\')
			dir[x] = '/';

	// The Steam WebView needs to know about the location of the SO
	const char *fdir = MTY_WebViewIsSteam() ? MTY_JoinPath("deps", "steam") :
		MTY_JoinPath(main_asset_dir(), "tmp");

	MTY_WindowSetWebView(ctx->app, ctx->window, fdir, MTN_DEBUG_WEBVIEW);

	const char *url = MTY_SprintfDL("file:///%s/merton-files/ui/index.html", dir);

	// Development location
	if (MTY_FileExists(MTY_JoinPath(dir, MTY_JoinPath("src", MTY_JoinPath("ui", "index.html"))))) {
		url = MTY_SprintfDL("file:///%s/src/ui/index.html", dir);

	// Production location, bootstrap from UI_ZIP if necessary
	} else {
		const char *ui = MTY_JoinPath(main_asset_dir(), "ui");
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

	MTY_WebViewNavigate(ctx->app, ctx->window, url, true);
	MTY_WebViewSetInputPassthrough(ctx->app, ctx->window, true);
}

int32_t main(int32_t argc, char **argv)
{
	MTY_HttpAsyncCreate(4);
	MTY_Mkdir(main_asset_dir());

	struct main ctx = {0};
	ctx.running = true;

	ctx.jcfg = main_load_config(&ctx.cfg, &ctx.core_options, &ctx.core_exts);

	if (ctx.cfg.console)
		MTY_OpenConsole(APP_NAME);

	MTY_SetTimerResolution(1);
	MTY_SetLogFunc(main_mty_log_callback, NULL);

	ctx.rt_q = MTY_QueueCreate(50, sizeof(struct app_event));
	ctx.mt_q = MTY_QueueCreate(50, sizeof(struct app_event));
	ctx.a_q = MTY_QueueCreate(5, sizeof(struct audio_packet));

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
	if (frame.size.w == 0)
		frame = MTY_MakeDefaultFrame(0, 0, MAIN_WINDOW_W, MAIN_WINDOW_H, 1.0f);

	ctx.window = MTY_WindowCreate(ctx.app, APP_NAME, &frame, 0);
	if (ctx.window == -1)
		goto except;

	MTY_WindowSetMinSize(ctx.app, ctx.window, 256, 240);

	main_set_webview(&ctx);

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

	MTY_HttpAsyncDestroy();

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
