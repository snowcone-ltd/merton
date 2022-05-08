#include "webview.h"

#include <stdio.h>

#define COBJMACROS
#include <windows.h>
#include <ole2.h>

#include "deps/WebView2.h"

struct webview_handler0 {
	ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler handler;
	void *opaque;
};

struct webview_handler1 {
	ICoreWebView2CreateCoreWebView2ControllerCompletedHandler handler;
	void *opaque;
};

struct webview_handler2 {
	ICoreWebView2WebMessageReceivedEventHandler handler;
	void *opaque;
};

struct webview {
	MTY_App *app;
	MTY_Window window;
	MTY_Queue *pushq;
	MTY_Queue *popq;
	ICoreWebView2Controller2 *controller;
	ICoreWebView2 *webview;
	struct webview_handler0 handler0;
	struct webview_handler1 handler1;
	struct webview_handler2 handler2;
	bool show;
	bool ready;
	bool was_ready;
};


// Generic COM shims

static HRESULT STDMETHODCALLTYPE main_query_interface(void *This,
	REFIID riid, _COM_Outptr_ void **ppvObject)
{
	*ppvObject = NULL;

	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE main_add_ref(void *This)
{
	return 1;
}

static ULONG STDMETHODCALLTYPE main_release(void *This)
{
	return 0;
}


// ICoreWebView2WebMessageReceivedEventHandler

static HRESULT STDMETHODCALLTYPE main_h2_invoke(ICoreWebView2WebMessageReceivedEventHandler *This,
	ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args)
{
	struct webview_handler2 *handler = (struct webview_handler2 *) This;
	struct webview *ctx = handler->opaque;

	WCHAR *wstr = NULL;
	HRESULT e = ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(args, &wstr);

	if (e == S_OK) {
		char *str = MTY_WideToMultiD(wstr);

		if (!strcmp(str, "__WEBVIEW_READY")) {
			ctx->ready = true;
			ctx->was_ready = false;

			if (ctx->show)
				webview_show(ctx, true);

			webview_sync_messages(ctx);

		} else {
			MTY_JSON *j = MTY_JSONParse(str);

			if (j)
				MTY_QueuePushPtr(ctx->popq, j, 0);
		}

		MTY_Free(str);
	}

	return e;
}


// ICoreWebView2CreateCoreWebView2ControllerCompletedHandler

static HRESULT STDMETHODCALLTYPE main_h1_invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This,
	HRESULT errorCode, ICoreWebView2Controller *controller)
{
	struct webview_handler1 *handler = (struct webview_handler1 *) This;
	struct webview *ctx = handler->opaque;

	HRESULT e = ICoreWebView2Controller2_QueryInterface(controller, &IID_ICoreWebView2Controller2, &ctx->controller);
	if (e != S_OK)
		return e;

	webview_show(ctx, false);

	ICoreWebView2Controller2_get_CoreWebView2(ctx->controller, &ctx->webview);

	COREWEBVIEW2_COLOR bg = {0};
	ICoreWebView2Controller2_put_DefaultBackgroundColor(ctx->controller, bg);

	webview_update_size(ctx);

	EventRegistrationToken token = {0};
	ICoreWebView2_add_WebMessageReceived(ctx->webview,
		(ICoreWebView2WebMessageReceivedEventHandler *) &ctx->handler2, &token);

	char *dir = (char *) MTY_GetProcessDir();

	for (size_t x = 0; x < strlen(dir); x++)
		if (dir[x] == '\\')
			dir[x] = '/';

	const WCHAR *full_dirw = MTY_MultiToWideDL(MTY_SprintfDL("file:///%s/src/ui/index.html", dir));

	ICoreWebView2_Navigate(ctx->webview, full_dirw);

	return S_OK;
}


// ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler

static HRESULT STDMETHODCALLTYPE main_h0_invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This,
	HRESULT errorCode, ICoreWebView2Environment *env)
{
	struct webview_handler0 *handler = (struct webview_handler0 *) This;
	struct webview *ctx = handler->opaque;

	HWND hwnd = MTY_WindowGetNative(ctx->app, ctx->window);

	return ICoreWebView2Environment_CreateCoreWebView2Controller(env, hwnd,
		(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *) &ctx->handler1);
}


// Vtables

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl VTBL0 = {
	.QueryInterface = main_query_interface,
	.AddRef = main_add_ref,
	.Release = main_release,
	.Invoke = main_h0_invoke,
};

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl VTBL1 = {
	.QueryInterface = main_query_interface,
	.AddRef = main_add_ref,
	.Release = main_release,
	.Invoke = main_h1_invoke,
};

static ICoreWebView2WebMessageReceivedEventHandlerVtbl VTBL2 = {
	.QueryInterface = main_query_interface,
	.AddRef = main_add_ref,
	.Release = main_release,
	.Invoke = main_h2_invoke,
};


// Public

void webview_show(struct webview *ctx, bool show)
{
	if (!ctx || !ctx->controller)
		return;

	ICoreWebView2Controller2_put_IsVisible(ctx->controller, show);

	if (show)
		ICoreWebView2Controller2_MoveFocus(ctx->controller, COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

bool webview_is_showing(struct webview *ctx)
{
	if (!ctx || !ctx->controller)
		return false;

	BOOL visible = FALSE;
	ICoreWebView2Controller2_get_IsVisible(ctx->controller, &visible);

	return visible;
}

bool webview_became_ready(struct webview *ctx)
{
	if (!ctx)
		return false;

	bool became_ready = ctx->ready && !ctx->was_ready;

	ctx->was_ready = ctx->ready;

	return became_ready;
}

void webview_update_size(struct webview *ctx)
{
	if (!ctx || !ctx->controller)
		return;

	MTY_Size size = MTY_WindowGetSize(ctx->app, ctx->window);

	RECT bounds = {0};
	bounds.right = size.w;
	bounds.bottom = size.h;

	ICoreWebView2Controller2_put_Bounds(ctx->controller, bounds);
}

void webview_post_message(struct webview *ctx, const char *msg)
{
	if (!ctx)
		return;

	MTY_QueuePushPtr(ctx->pushq, MTY_MultiToWideD(msg), 0);
}

void webview_sync_messages(struct webview *ctx)
{
	if (!ctx || !ctx->ready || !ctx->webview)
		return;

	for (WCHAR *wmsg = NULL; MTY_QueuePopPtr(ctx->pushq, 0, &wmsg, NULL);) {
		ICoreWebView2_PostWebMessageAsString(ctx->webview, wmsg);
		MTY_Free(wmsg);
	}
}

MTY_JSON *webview_get_message(struct webview *ctx)
{
	if (!ctx)
		return NULL;

	MTY_JSON *j = NULL;
	if (MTY_QueuePopPtr(ctx->popq, 0, &j, NULL))
		return j;

	return NULL;
}

struct webview *webview_create(MTY_App *app, MTY_Window window, const char *tmp_dir, bool show)
{
	struct webview *ctx = MTY_Alloc(1, sizeof(struct webview));

	ctx->app = app;
	ctx->window = window;
	ctx->show = show;

	ctx->pushq = MTY_QueueCreate(50, 0);
	ctx->popq = MTY_QueueCreate(50, 0);

	ctx->handler0.handler.lpVtbl = &VTBL0;
	ctx->handler0.opaque = ctx;
	ctx->handler1.handler.lpVtbl = &VTBL1;
	ctx->handler1.opaque = ctx;
	ctx->handler2.handler.lpVtbl = &VTBL2;
	ctx->handler2.opaque = ctx;

	HRESULT e = CreateCoreWebView2EnvironmentWithOptions(NULL, MTY_MultiToWideDL(tmp_dir), NULL,
		(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *) &ctx->handler0);

	if (e != S_OK)
		webview_destroy(&ctx);

	return ctx;
}

static void webview_flush_func(void *ptr)
{
	MTY_JSON *j = ptr;

	MTY_JSONDestroy(&j);
}

void webview_destroy(struct webview **webview)
{
	if (!webview || !*webview)
		return;

	struct webview *ctx = *webview;

	if (ctx->controller)
		ICoreWebView2Controller2_Release(ctx->controller);

	if (ctx->popq)
		MTY_QueueFlush(ctx->popq, webview_flush_func);

	if (ctx->pushq)
		MTY_QueueFlush(ctx->popq, webview_flush_func);

	MTY_QueueDestroy(&ctx->popq);
	MTY_QueueDestroy(&ctx->pushq);

	MTY_Free(ctx);
	*webview = NULL;
}
