/* Host-side helpers (PC only): save a canvas as a BMP image that looks like the panel. */
#pragma once
#include "epd_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Write the framebuffer as an 8-bit indexed BMP (native orientation), scaled by @p scale. */
int epd_host_save_bmp(const epd_canvas_t *c, const char *path, int scale);

#ifdef __cplusplus
}
#endif
