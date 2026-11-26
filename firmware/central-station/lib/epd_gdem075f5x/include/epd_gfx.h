/**
 * @file epd_gfx.h
 * @brief Drawing into a framebuffer for the 800x480 BWRY panel.
 *
 * Plain C without ESP-IDF dependencies: the same drawing code runs on the ESP32
 * and on a PC (see host/ for rendering previews to BMP).
 *
 * Coordinates are logical, i.e. after rotation. (0,0) is the top-left corner,
 * x grows to the right, y grows down. Everything is clipped to the canvas and to
 * the optional clip rectangle, so drawing outside is always safe.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "epd_types.h"
#include "epd_font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EPD_ROT_0 = 0,      /**< Landscape 800x480, FPC connector as the vendor intends */
    EPD_ROT_90,         /**< Portrait 480x800, rotated clockwise */
    EPD_ROT_180,        /**< Landscape, upside down */
    EPD_ROT_270,        /**< Portrait, rotated counter-clockwise */
} epd_rotation_t;

typedef struct {
    uint8_t *fb;                    /**< EPD_FB_SIZE bytes, native 2 bpp format */
    int16_t width, height;          /**< Logical size after rotation */
    epd_rotation_t rotation;
    int16_t clip_x0, clip_y0;       /**< Clip rectangle, inclusive start */
    int16_t clip_x1, clip_y1;       /**< Clip rectangle, exclusive end */
    bool owns_fb;
} epd_canvas_t;

typedef enum { EPD_ALIGN_LEFT, EPD_ALIGN_CENTER, EPD_ALIGN_RIGHT } epd_halign_t;
typedef enum { EPD_ALIGN_TOP, EPD_ALIGN_MIDDLE, EPD_ALIGN_BOTTOM } epd_valign_t;

/** Two-colour fill patterns, useful as "shades" on a 4-colour panel. */
typedef enum {
    EPD_PATTERN_CHECKER,    /**< 50 % checkerboard */
    EPD_PATTERN_DOTS,       /**< 1 pixel of fg in every 2x2 cell (25 %) */
    EPD_PATTERN_SPARSE,     /**< 1 pixel of fg in every 4x4 cell (~6 %) */
    EPD_PATTERN_HLINES,     /**< every other row */
    EPD_PATTERN_VLINES,     /**< every other column */
    EPD_PATTERN_DIAGONAL,   /**< 45° hatch, 1 px line every 4 px */
} epd_pattern_t;

/* ------------------------------------------------------------------ canvas */

/** @brief Allocate a framebuffer (malloc) and a canvas, cleared to white. NULL on failure. */
epd_canvas_t *epd_canvas_create(epd_rotation_t rot);
/** @brief Free a canvas from epd_canvas_create() (and its framebuffer). */
void epd_canvas_free(epd_canvas_t *c);
/** @brief Wrap an existing EPD_FB_SIZE buffer (e.g. static or in PSRAM). Content is untouched. */
void epd_canvas_init(epd_canvas_t *c, uint8_t *fb, epd_rotation_t rot);
/** @brief Change rotation (resets the clip rectangle). Existing content is not rotated. */
void epd_canvas_set_rotation(epd_canvas_t *c, epd_rotation_t rot);
/** @brief Restrict drawing to a rectangle (logical coordinates). */
void epd_canvas_set_clip(epd_canvas_t *c, int x, int y, int w, int h);
/** @brief Remove the clip rectangle. */
void epd_canvas_reset_clip(epd_canvas_t *c);

/* ------------------------------------------------------------------ primitives */

void epd_gfx_clear(epd_canvas_t *c, epd_color_t color);
void epd_gfx_pixel(epd_canvas_t *c, int x, int y, epd_color_t color);
/** @brief Read a pixel; returns EPD_TRANSPARENT outside the canvas. */
epd_color_t epd_gfx_get_pixel(const epd_canvas_t *c, int x, int y);

void epd_gfx_hline(epd_canvas_t *c, int x, int y, int w, epd_color_t color);
void epd_gfx_vline(epd_canvas_t *c, int x, int y, int h, epd_color_t color);
/** @brief Dashed horizontal line: @p dash px on, @p gap px off. */
void epd_gfx_hline_dashed(epd_canvas_t *c, int x, int y, int w, int dash, int gap, epd_color_t color);
/** @brief Dashed vertical line: @p dash px on, @p gap px off. */
void epd_gfx_vline_dashed(epd_canvas_t *c, int x, int y, int h, int dash, int gap, epd_color_t color);
void epd_gfx_line(epd_canvas_t *c, int x0, int y0, int x1, int y1, epd_color_t color);
/** @brief Line with a square pen of @p thickness px (good for charts). */
void epd_gfx_line_thick(epd_canvas_t *c, int x0, int y0, int x1, int y1, int thickness, epd_color_t color);

void epd_gfx_rect(epd_canvas_t *c, int x, int y, int w, int h, epd_color_t color);
/** @brief Rectangle outline with a border of @p thickness px drawn inwards. */
void epd_gfx_rect_thick(epd_canvas_t *c, int x, int y, int w, int h, int thickness, epd_color_t color);
void epd_gfx_fill_rect(epd_canvas_t *c, int x, int y, int w, int h, epd_color_t color);
void epd_gfx_round_rect(epd_canvas_t *c, int x, int y, int w, int h, int r, epd_color_t color);
void epd_gfx_fill_round_rect(epd_canvas_t *c, int x, int y, int w, int h, int r, epd_color_t color);
/** @brief Fill with a two-colour pattern. @p bg may be EPD_TRANSPARENT. Aligned to the canvas grid. */
void epd_gfx_fill_rect_pattern(epd_canvas_t *c, int x, int y, int w, int h,
                               epd_color_t fg, epd_color_t bg, epd_pattern_t pattern);

void epd_gfx_circle(epd_canvas_t *c, int cx, int cy, int r, epd_color_t color);
void epd_gfx_fill_circle(epd_canvas_t *c, int cx, int cy, int r, epd_color_t color);
void epd_gfx_triangle(epd_canvas_t *c, int x0, int y0, int x1, int y1, int x2, int y2, epd_color_t color);
void epd_gfx_fill_triangle(epd_canvas_t *c, int x0, int y0, int x1, int y1, int x2, int y2, epd_color_t color);

/* ------------------------------------------------------------------ bitmaps */

/**
 * @brief Draw a 1-bit bitmap (MSB first, each row padded to a whole byte).
 * Set bits are drawn in @p fg, clear bits in @p bg (EPD_TRANSPARENT = skip).
 */
void epd_gfx_bitmap1(epd_canvas_t *c, int x, int y, int w, int h, const uint8_t *data,
                     epd_color_t fg, epd_color_t bg);

/**
 * @brief Draw a native 2-bit image (4 px per byte, MSB first, each row padded to a whole
 * byte), as produced by tools/img2epd.py. @p transparent: colour to skip, or EPD_TRANSPARENT.
 */
void epd_gfx_bitmap2(epd_canvas_t *c, int x, int y, int w, int h, const uint8_t *data,
                     epd_color_t transparent);

/* ------------------------------------------------------------------ text */

/**
 * @brief Draw UTF-8 text. (x, y) is the top-left corner of the first line box,
 * the baseline is at y + font->ascent. '\n' starts a new line.
 * Characters missing in the font are drawn as '?'.
 * @return x coordinate after the last character of the last line.
 */
int epd_gfx_text(epd_canvas_t *c, int x, int y, const char *utf8,
                 const epd_font_t *font, epd_color_t color);

/** @brief Like epd_gfx_text(), but every line is aligned relative to @p x (left/center/right). */
void epd_gfx_text_aligned(epd_canvas_t *c, int x, int y, const char *utf8,
                          const epd_font_t *font, epd_color_t color, epd_halign_t align);

/**
 * @brief Draw text aligned inside a box. EPD_ALIGN_MIDDLE centres the cap height,
 * which looks right for numbers and capitals. Text is clipped to the box.
 */
void epd_gfx_text_box(epd_canvas_t *c, int x, int y, int w, int h, const char *utf8,
                      const epd_font_t *font, epd_color_t color,
                      epd_halign_t halign, epd_valign_t valign);

/** @brief printf-style epd_gfx_text() (output limited to 256 bytes). */
int epd_gfx_textf(epd_canvas_t *c, int x, int y, const epd_font_t *font, epd_color_t color,
                  const char *fmt, ...) __attribute__((format(printf, 6, 7)));

/** @brief Width in px of the widest line of @p utf8. */
int epd_text_width(const epd_font_t *font, const char *utf8);
/** @brief Total height in px of @p utf8 (number of lines x line height). */
int epd_text_height(const epd_font_t *font, const char *utf8);

/** @brief Decode one UTF-8 code point and advance @p s. Invalid bytes give U+FFFD. */
uint32_t epd_utf8_next(const char **s);

#ifdef __cplusplus
}
#endif
