/**
 * @file epd_types.h
 * @brief Panel geometry and colour codes shared by the driver and the graphics layer.
 *
 * This header is plain C (no ESP-IDF includes), so the graphics layer can also be
 * built on a PC for previews.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Native panel width in pixels (landscape). */
#define EPD_WIDTH    800
/** Native panel height in pixels (landscape). */
#define EPD_HEIGHT   480
/** Framebuffer size in bytes: 2 bits per pixel, 4 pixels per byte = 96 000 B. */
#define EPD_FB_SIZE  (EPD_WIDTH * EPD_HEIGHT / 4)

/**
 * @brief Colours, using the controller's own 2-bit codes.
 *
 * The framebuffer is sent to the panel without conversion, so these values are
 * exactly what ends up in the controller RAM.
 */
typedef enum {
    EPD_BLACK  = 0x0,
    EPD_WHITE  = 0x1,
    EPD_YELLOW = 0x2,
    EPD_RED    = 0x3,
    /** Only valid as a background colour (text, 1-bit bitmaps): leave pixels unchanged. */
    EPD_TRANSPARENT = 0xFF,
} epd_color_t;

#ifdef __cplusplus
}
#endif
