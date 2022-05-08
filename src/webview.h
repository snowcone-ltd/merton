#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "matoya.h"

struct webview;

#if defined(_WIN32)

struct webview *webview_create(MTY_App *app, MTY_Window window, const char *tmp_dir, bool show);
void webview_destroy(struct webview **webview);
void webview_show(struct webview *ctx, bool show);
bool webview_is_showing(struct webview *ctx);
bool webview_became_ready(struct webview *ctx);
void webview_post_message(struct webview *ctx, const char *msg);
void webview_sync_messages(struct webview *ctx);
MTY_JSON *webview_get_message(struct webview *ctx);
void webview_update_size(struct webview *ctx);

#else

#define webview_create(app, window, tmp_dir, show) ((void *) 1)
#define webview_destroy(webview) {}
#define webview_show(ctx, show) {}
#define webview_is_showing(ctx) true
#define webview_became_ready(ctx) true
#define webview_post_message(ctx, msg) {}
#define webview_sync_messages(ctx) {}
#define webview_get_message(ctx) NULL
#define webview_update_size(ctx) {}

#endif
