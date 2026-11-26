#include "epd_gfx.h"

#include <stdlib.h>
#include <string.h>

#define SWAP(a, b) do { int _t = (a); (a) = (b); (b) = _t; } while (0)

/* ------------------------------------------------------------------ canvas */

void epd_canvas_set_rotation(epd_canvas_t *c, epd_rotation_t rot)
{
    c->rotation = rot;
    bool portrait = (rot == EPD_ROT_90 || rot == EPD_ROT_270);
    c->width  = portrait ? EPD_HEIGHT : EPD_WIDTH;
    c->height = portrait ? EPD_WIDTH : EPD_HEIGHT;
    epd_canvas_reset_clip(c);
}

void epd_canvas_init(epd_canvas_t *c, uint8_t *fb, epd_rotation_t rot)
{
    memset(c, 0, sizeof(*c));
    c->fb = fb;
    epd_canvas_set_rotation(c, rot);
}

epd_canvas_t *epd_canvas_create(epd_rotation_t rot)
{
    epd_canvas_t *c = malloc(sizeof(*c));
    uint8_t *fb = malloc(EPD_FB_SIZE);
    if (!c || !fb) {
        free(c);
        free(fb);
        return NULL;
    }
    epd_canvas_init(c, fb, rot);
    c->owns_fb = true;
    epd_gfx_clear(c, EPD_WHITE);
    return c;
}

void epd_canvas_free(epd_canvas_t *c)
{
    if (!c) return;
    if (c->owns_fb) free(c->fb);
    free(c);
}

void epd_canvas_set_clip(epd_canvas_t *c, int x, int y, int w, int h)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > c->width ? c->width : x + w;
    int y1 = y + h > c->height ? c->height : y + h;
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    c->clip_x0 = x0; c->clip_y0 = y0; c->clip_x1 = x1; c->clip_y1 = y1;
}

void epd_canvas_reset_clip(epd_canvas_t *c)
{
    c->clip_x0 = 0; c->clip_y0 = 0;
    c->clip_x1 = c->width; c->clip_y1 = c->height;
}

/* ------------------------------------------------------------------ pixels */

static inline void native_xy(const epd_canvas_t *c, int x, int y, int *px, int *py)
{
    switch (c->rotation) {
    default:
    case EPD_ROT_0:   *px = x;                  *py = y;                  break;
    case EPD_ROT_90:  *px = EPD_WIDTH - 1 - y;  *py = x;                  break;
    case EPD_ROT_180: *px = EPD_WIDTH - 1 - x;  *py = EPD_HEIGHT - 1 - y; break;
    case EPD_ROT_270: *px = y;                  *py = EPD_HEIGHT - 1 - x; break;
    }
}

static inline bool in_clip(const epd_canvas_t *c, int x, int y)
{
    return x >= c->clip_x0 && x < c->clip_x1 && y >= c->clip_y0 && y < c->clip_y1;
}

void epd_gfx_pixel(epd_canvas_t *c, int x, int y, epd_color_t color)
{
    if (color == EPD_TRANSPARENT || !in_clip(c, x, y)) return;
    int px, py;
    native_xy(c, x, y, &px, &py);
    size_t idx = ((size_t)py * EPD_WIDTH + px) >> 2;
    int shift = 6 - 2 * (px & 3);
    c->fb[idx] = (uint8_t)((c->fb[idx] & ~(0x3 << shift)) | (((unsigned)color & 0x3) << shift));
}

epd_color_t epd_gfx_get_pixel(const epd_canvas_t *c, int x, int y)
{
    if (x < 0 || y < 0 || x >= c->width || y >= c->height) return EPD_TRANSPARENT;
    int px, py;
    native_xy(c, x, y, &px, &py);
    size_t idx = ((size_t)py * EPD_WIDTH + px) >> 2;
    return (epd_color_t)((c->fb[idx] >> (6 - 2 * (px & 3))) & 0x3);
}

void epd_gfx_clear(epd_canvas_t *c, epd_color_t color)
{
    if (color == EPD_TRANSPARENT) return;
    unsigned v = (unsigned)color & 0x3;
    memset(c->fb, (int)((v << 6) | (v << 4) | (v << 2) | v), EPD_FB_SIZE);
}

/* ------------------------------------------------------------------ lines */

void epd_gfx_hline(epd_canvas_t *c, int x, int y, int w, epd_color_t color)
{
    if (w < 0) { x += w + 1; w = -w; }
    if (y < c->clip_y0 || y >= c->clip_y1) return;
    int x0 = x < c->clip_x0 ? c->clip_x0 : x;
    int x1 = x + w > c->clip_x1 ? c->clip_x1 : x + w;
    for (int i = x0; i < x1; i++) epd_gfx_pixel(c, i, y, color);
}

void epd_gfx_vline(epd_canvas_t *c, int x, int y, int h, epd_color_t color)
{
    if (h < 0) { y += h + 1; h = -h; }
    if (x < c->clip_x0 || x >= c->clip_x1) return;
    int y0 = y < c->clip_y0 ? c->clip_y0 : y;
    int y1 = y + h > c->clip_y1 ? c->clip_y1 : y + h;
    for (int j = y0; j < y1; j++) epd_gfx_pixel(c, x, j, color);
}

void epd_gfx_hline_dashed(epd_canvas_t *c, int x, int y, int w, int dash, int gap, epd_color_t color)
{
    if (dash <= 0) return;
    if (gap < 0) gap = 0;
    for (int i = 0; i < w; i += dash + gap)
        epd_gfx_hline(c, x + i, y, (i + dash > w) ? w - i : dash, color);
}

void epd_gfx_vline_dashed(epd_canvas_t *c, int x, int y, int h, int dash, int gap, epd_color_t color)
{
    if (dash <= 0) return;
    if (gap < 0) gap = 0;
    for (int j = 0; j < h; j += dash + gap)
        epd_gfx_vline(c, x, y + j, (j + dash > h) ? h - j : dash, color);
}

static void line_impl(epd_canvas_t *c, int x0, int y0, int x1, int y1, int t, epd_color_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int off = (t - 1) / 2;
    for (;;) {
        if (t <= 1) epd_gfx_pixel(c, x0, y0, color);
        else epd_gfx_fill_rect(c, x0 - off, y0 - off, t, t, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void epd_gfx_line(epd_canvas_t *c, int x0, int y0, int x1, int y1, epd_color_t color)
{
    if (y0 == y1) { if (x1 < x0) SWAP(x0, x1); epd_gfx_hline(c, x0, y0, x1 - x0 + 1, color); return; }
    if (x0 == x1) { if (y1 < y0) SWAP(y0, y1); epd_gfx_vline(c, x0, y0, y1 - y0 + 1, color); return; }
    line_impl(c, x0, y0, x1, y1, 1, color);
}

void epd_gfx_line_thick(epd_canvas_t *c, int x0, int y0, int x1, int y1, int thickness, epd_color_t color)
{
    line_impl(c, x0, y0, x1, y1, thickness < 1 ? 1 : thickness, color);
}

/* ------------------------------------------------------------------ rectangles */

void epd_gfx_fill_rect(epd_canvas_t *c, int x, int y, int w, int h, epd_color_t color)
{
    if (w <= 0 || h <= 0) return;
    for (int j = y; j < y + h; j++) epd_gfx_hline(c, x, j, w, color);
}

void epd_gfx_rect(epd_canvas_t *c, int x, int y, int w, int h, epd_color_t color)
{
    epd_gfx_rect_thick(c, x, y, w, h, 1, color);
}

void epd_gfx_rect_thick(epd_canvas_t *c, int x, int y, int w, int h, int t, epd_color_t color)
{
    if (w <= 0 || h <= 0 || t <= 0) return;
    if (2 * t >= w || 2 * t >= h) { epd_gfx_fill_rect(c, x, y, w, h, color); return; }
    epd_gfx_fill_rect(c, x, y, w, t, color);
    epd_gfx_fill_rect(c, x, y + h - t, w, t, color);
    epd_gfx_fill_rect(c, x, y + t, t, h - 2 * t, color);
    epd_gfx_fill_rect(c, x + w - t, y + t, t, h - 2 * t, color);
}

static bool pattern_on(epd_pattern_t p, int x, int y)
{
    switch (p) {
    case EPD_PATTERN_CHECKER:  return ((x + y) & 1) == 0;
    case EPD_PATTERN_DOTS:     return (x & 1) == 0 && (y & 1) == 0;
    case EPD_PATTERN_SPARSE:   return (x & 3) == 0 && (y & 3) == 0;
    case EPD_PATTERN_HLINES:   return (y & 1) == 0;
    case EPD_PATTERN_VLINES:   return (x & 1) == 0;
    case EPD_PATTERN_DIAGONAL: return ((x + y) & 3) == 0;
    default:                   return true;
    }
}

void epd_gfx_fill_rect_pattern(epd_canvas_t *c, int x, int y, int w, int h,
                               epd_color_t fg, epd_color_t bg, epd_pattern_t pattern)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            epd_gfx_pixel(c, i, j, pattern_on(pattern, i, j) ? fg : bg);
}

/* ------------------------------------------------------------------ circles */

/* Quadrant outline helper; corners: 1 = top-left, 2 = top-right, 4 = bottom-right, 8 = bottom-left */
static void circle_quadrants(epd_canvas_t *c, int cx, int cy, int r, int corners, epd_color_t color)
{
    int f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
    while (x < y) {
        if (f >= 0) { y--; ddy += 2; f += ddy; }
        x++; ddx += 2; f += ddx;
        if (corners & 4) { epd_gfx_pixel(c, cx + x, cy + y, color); epd_gfx_pixel(c, cx + y, cy + x, color); }
        if (corners & 2) { epd_gfx_pixel(c, cx + x, cy - y, color); epd_gfx_pixel(c, cx + y, cy - x, color); }
        if (corners & 8) { epd_gfx_pixel(c, cx - y, cy + x, color); epd_gfx_pixel(c, cx - x, cy + y, color); }
        if (corners & 1) { epd_gfx_pixel(c, cx - y, cy - x, color); epd_gfx_pixel(c, cx - x, cy - y, color); }
    }
}

/* Filled halves helper; sides: 1 = right half, 2 = left half; delta stretches vertically */
static void fill_circle_halves(epd_canvas_t *c, int cx, int cy, int r, int sides, int delta, epd_color_t color)
{
    int f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
    int px = x, py = y;
    delta++;
    while (x < y) {
        if (f >= 0) { y--; ddy += 2; f += ddy; }
        x++; ddx += 2; f += ddx;
        if (x < y + 1) {
            if (sides & 1) epd_gfx_vline(c, cx + x, cy - y, 2 * y + delta, color);
            if (sides & 2) epd_gfx_vline(c, cx - x, cy - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (sides & 1) epd_gfx_vline(c, cx + py, cy - px, 2 * px + delta, color);
            if (sides & 2) epd_gfx_vline(c, cx - py, cy - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

void epd_gfx_circle(epd_canvas_t *c, int cx, int cy, int r, epd_color_t color)
{
    if (r < 0) return;
    epd_gfx_pixel(c, cx, cy + r, color);
    epd_gfx_pixel(c, cx, cy - r, color);
    epd_gfx_pixel(c, cx + r, cy, color);
    epd_gfx_pixel(c, cx - r, cy, color);
    circle_quadrants(c, cx, cy, r, 0xF, color);
}

void epd_gfx_fill_circle(epd_canvas_t *c, int cx, int cy, int r, epd_color_t color)
{
    if (r < 0) return;
    epd_gfx_vline(c, cx, cy - r, 2 * r + 1, color);
    fill_circle_halves(c, cx, cy, r, 3, 0, color);
}

void epd_gfx_round_rect(epd_canvas_t *c, int x, int y, int w, int h, int r, epd_color_t color)
{
    int max_r = (w < h ? w : h) / 2;
    if (r > max_r) r = max_r;
    if (r <= 0) { epd_gfx_rect(c, x, y, w, h, color); return; }
    epd_gfx_hline(c, x + r, y, w - 2 * r, color);
    epd_gfx_hline(c, x + r, y + h - 1, w - 2 * r, color);
    epd_gfx_vline(c, x, y + r, h - 2 * r, color);
    epd_gfx_vline(c, x + w - 1, y + r, h - 2 * r, color);
    circle_quadrants(c, x + r, y + r, r, 1, color);
    circle_quadrants(c, x + w - r - 1, y + r, r, 2, color);
    circle_quadrants(c, x + w - r - 1, y + h - r - 1, r, 4, color);
    circle_quadrants(c, x + r, y + h - r - 1, r, 8, color);
}

void epd_gfx_fill_round_rect(epd_canvas_t *c, int x, int y, int w, int h, int r, epd_color_t color)
{
    int max_r = (w < h ? w : h) / 2;
    if (r > max_r) r = max_r;
    if (r <= 0) { epd_gfx_fill_rect(c, x, y, w, h, color); return; }
    epd_gfx_fill_rect(c, x + r, y, w - 2 * r, h, color);
    fill_circle_halves(c, x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    fill_circle_halves(c, x + r, y + r, r, 2, h - 2 * r - 1, color);
}

/* ------------------------------------------------------------------ triangles */

void epd_gfx_triangle(epd_canvas_t *c, int x0, int y0, int x1, int y1, int x2, int y2, epd_color_t color)
{
    epd_gfx_line(c, x0, y0, x1, y1, color);
    epd_gfx_line(c, x1, y1, x2, y2, color);
    epd_gfx_line(c, x2, y2, x0, y0, color);
}

void epd_gfx_fill_triangle(epd_canvas_t *c, int x0, int y0, int x1, int y1, int x2, int y2, epd_color_t color)
{
    /* sort by y */
    if (y0 > y1) { SWAP(y0, y1); SWAP(x0, x1); }
    if (y1 > y2) { SWAP(y2, y1); SWAP(x2, x1); }
    if (y0 > y1) { SWAP(y0, y1); SWAP(x0, x1); }

    if (y0 == y2) {                     /* degenerate: one row */
        int a = x0, b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        epd_gfx_hline(c, a, y0, b - a + 1, color);
        return;
    }

    int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0;
    int dx12 = x2 - x1, dy12 = y2 - y1;
    long sa = 0, sb = 0;
    int last = (y1 == y2) ? y1 : y1 - 1;
    int y;

    for (y = y0; y <= last; y++) {
        int a = x0 + (int)(sa / dy01);
        int b = x0 + (int)(sb / dy02);
        sa += dx01; sb += dx02;
        if (a > b) SWAP(a, b);
        epd_gfx_hline(c, a, y, b - a + 1, color);
    }
    sa = (long)dx12 * (y - y1);
    sb = (long)dx02 * (y - y0);
    for (; y <= y2; y++) {
        int a = x1 + (int)(sa / dy12);
        int b = x0 + (int)(sb / dy02);
        sa += dx12; sb += dx02;
        if (a > b) SWAP(a, b);
        epd_gfx_hline(c, a, y, b - a + 1, color);
    }
}

/* ------------------------------------------------------------------ bitmaps */

void epd_gfx_bitmap1(epd_canvas_t *c, int x, int y, int w, int h, const uint8_t *data,
                     epd_color_t fg, epd_color_t bg)
{
    int stride = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        const uint8_t *row = data + (size_t)j * stride;
        for (int i = 0; i < w; i++) {
            bool on = row[i >> 3] & (0x80 >> (i & 7));
            epd_gfx_pixel(c, x + i, y + j, on ? fg : bg);
        }
    }
}

void epd_gfx_bitmap2(epd_canvas_t *c, int x, int y, int w, int h, const uint8_t *data,
                     epd_color_t transparent)
{
    int stride = (w + 3) / 4;
    for (int j = 0; j < h; j++) {
        const uint8_t *row = data + (size_t)j * stride;
        for (int i = 0; i < w; i++) {
            epd_color_t col = (epd_color_t)((row[i >> 2] >> (6 - 2 * (i & 3))) & 0x3);
            if (col != transparent) epd_gfx_pixel(c, x + i, y + j, col);
        }
    }
}
