#pragma once

#include <stdbool.h>

bool loader_load(const char *name, bool libretro);
void loader_unload(void);
