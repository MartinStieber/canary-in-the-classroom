#include "epd_gfx.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ UTF-8 */

uint32_t epd_utf8_next(const char **ps)
{
    const unsigned char *s = (const unsigned char *)*ps;
    uint32_t cp;
    int extra;

    if (s[0] < 0x80)              { cp = s[0];        extra = 0; }
    else if ((s[0] & 0xE0) == 0xC0) { cp = s[0] & 0x1F; extra = 1; }
    else if ((s[0] & 0xF0) == 0xE0) { cp = s[0] & 0x0F; extra = 2; }
    else if ((s[0] & 0xF8) == 0xF0) { cp = s[0] & 0x07; extra = 3; }
    else { *ps += 1; return 0xFFFD; }

    for (int i = 1; i <= extra; i++) {
        if ((s[i] & 0xC0) != 0x80) {         /* truncated sequence (also stops at '\0') */
            *ps += i;
            return 0xFFFD;
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    *ps += 1 + extra;
    return cp;
}

/* ------------------------------------------------------------------ glyph lookup */

const epd_glyph_t *epd_font_glyph(const epd_font_t *font, uint32_t cp)
{
    for (uint16_t i = 0; i < font->range_count; i++) {
        const epd_font_range_t *r = &font->ranges[i];
        if (cp < r->first) break;            /* ranges are sorted */
        if (cp <= r->last) return &font->glyphs[r->glyph + (cp - r->first)];
    }
    return NULL;
}

int epd_font_kern(const epd_font_t *font, uint32_t left, uint32_t right)
{
    if (!font->kern || left > 0xFFFF || right > 0xFFFF) return 0;
    uint32_t key = (left << 16) | right;
    int lo = 0, hi = (int)font->kern_count - 1;
    while (lo <= hi) {                       /* pairs are sorted by (left, right) */
        int mid = (lo + hi) / 2;
        const epd_kern_pair_t *k = &font->kern[mid];
        uint32_t v = ((uint32_t)k->left << 16) | k->right;
        if (v == key) return k->adjust;
        if (v < key) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

static const epd_glyph_t *glyph_or_fallback(const epd_font_t *font, uint32_t cp)
{
    const epd_glyph_t *g = epd_font_glyph(font, cp);
    return g ? g : epd_font_glyph(font, '?');
}

/* ------------------------------------------------------------------ measuring */

/* Width of one line starting at s (up to '\n' or end); *end set to the terminator. */
static int line_width(const epd_font_t *font, const char *s, const char **end)
{
    int w = 0;
    uint32_t prev = 0;
    while (*s && *s != '\n') {
        uint32_t cp = epd_utf8_next(&s);
        const epd_glyph_t *g = glyph_or_fallback(font, cp);
        if (!g) continue;
        w += epd_font_kern(font, prev, cp) + g->advance;
        prev = cp;
    }
    if (end) *end = s;
    return w;
}

int epd_text_width(const epd_font_t *font, const char *utf8)
{
    int max = 0;
    const char *s = utf8;
    for (;;) {
        int w = line_width(font, s, &s);
        if (w > max) max = w;
        if (*s != '\n') break;
        s++;
    }
    return max;
}

static int line_count(const char *s)
{
    int n = 1;
    for (; *s; s++) if (*s == '\n') n++;
    return n;
}

int epd_text_height(const epd_font_t *font, const char *utf8)
{
    return line_count(utf8) * font->line_height;
}

/* ------------------------------------------------------------------ drawing */

static void draw_glyph(epd_canvas_t *c, int pen_x, int baseline, const epd_font_t *font,
                       const epd_glyph_t *g, epd_color_t color)
{
    const uint8_t *bits = font->bitmap + g->offset;
    int x0 = pen_x + g->x_off, y0 = baseline + g->y_off;

    /* skip glyphs entirely outside the clip rectangle */
    if (x0 >= c->clip_x1 || y0 >= c->clip_y1 ||
        x0 + g->width <= c->clip_x0 || y0 + g->height <= c->clip_y0) return;

    uint32_t bit = 0;
    for (int j = 0; j < g->height; j++) {
        for (int i = 0; i < g->width; i++, bit++) {
            if (bits[bit >> 3] & (0x80 >> (bit & 7)))
                epd_gfx_pixel(c, x0 + i, y0 + j, color);
        }
    }
}

/* Draws one line, returns pen x after it; *end points at '\n' or '\0'. */
static int draw_line(epd_canvas_t *c, int x, int baseline, const char *s, const char **end,
                     const epd_font_t *font, epd_color_t color)
{
    uint32_t prev = 0;
    while (*s && *s != '\n') {
        uint32_t cp = epd_utf8_next(&s);
        const epd_glyph_t *g = glyph_or_fallback(font, cp);
        if (!g) continue;
        x += epd_font_kern(font, prev, cp);
        if (g->width && g->height) draw_glyph(c, x, baseline, font, g, color);
        x += g->advance;
        prev = cp;
    }
    if (end) *end = s;
    return x;
}

int epd_gfx_text(epd_canvas_t *c, int x, int y, const char *utf8,
                 const epd_font_t *font, epd_color_t color)
{
    int baseline = y + font->ascent, pen = x;
    const char *s = utf8;
    for (;;) {
        pen = draw_line(c, x, baseline, s, &s, font, color);
        if (*s != '\n') break;
        s++;
        baseline += font->line_height;
    }
    return pen;
}

void epd_gfx_text_aligned(epd_canvas_t *c, int x, int y, const char *utf8,
                          const epd_font_t *font, epd_color_t color, epd_halign_t align)
{
    int baseline = y + font->ascent;
    const char *s = utf8;
    for (;;) {
        int w = line_width(font, s, NULL);
        int lx = align == EPD_ALIGN_CENTER ? x - w / 2 : align == EPD_ALIGN_RIGHT ? x - w : x;
        draw_line(c, lx, baseline, s, &s, font, color);
        if (*s != '\n') break;
        s++;
        baseline += font->line_height;
    }
}

void epd_gfx_text_box(epd_canvas_t *c, int x, int y, int w, int h, const char *utf8,
                      const epd_font_t *font, epd_color_t color,
                      epd_halign_t halign, epd_valign_t valign)
{
    int n = line_count(utf8);
    int first_baseline;
    switch (valign) {
    default:
    case EPD_ALIGN_TOP:
        first_baseline = y + font->ascent;
        break;
    case EPD_ALIGN_BOTTOM:
        first_baseline = y + h - font->descent - (n - 1) * font->line_height;
        break;
    case EPD_ALIGN_MIDDLE: {
        int block = (n - 1) * font->line_height + font->cap_height;
        first_baseline = y + (h - block) / 2 + font->cap_height;
        break;
    }
    }

    int ax = halign == EPD_ALIGN_CENTER ? x + w / 2 : halign == EPD_ALIGN_RIGHT ? x + w : x;

    /* intersect clip with the box, restore afterwards */
    int16_t cx0 = c->clip_x0, cy0 = c->clip_y0, cx1 = c->clip_x1, cy1 = c->clip_y1;
    int nx0 = x > cx0 ? x : cx0, ny0 = y > cy0 ? y : cy0;
    int nx1 = x + w < cx1 ? x + w : cx1, ny1 = y + h < cy1 ? y + h : cy1;
    if (nx1 > nx0 && ny1 > ny0) {
        c->clip_x0 = nx0; c->clip_y0 = ny0; c->clip_x1 = nx1; c->clip_y1 = ny1;
        epd_gfx_text_aligned(c, ax, first_baseline - font->ascent, utf8, font, color, halign);
    }
    c->clip_x0 = cx0; c->clip_y0 = cy0; c->clip_x1 = cx1; c->clip_y1 = cy1;
}

int epd_gfx_textf(epd_canvas_t *c, int x, int y, const epd_font_t *font, epd_color_t color,
                  const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return epd_gfx_text(c, x, y, buf, font, color);
}
