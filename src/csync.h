#pragma once

#include <stdbool.h>

struct csync;

struct csync *csync_start(void);
void csync_stop(struct csync **csync);

void csync_fetch_core(struct csync *ctx, const char *file);
bool csync_poll_fetch_core(struct csync *ctx);

bool csync_check_core_hash(struct csync *ctx, const char *core, const char *file);
