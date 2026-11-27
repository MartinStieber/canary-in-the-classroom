#include "dashboard_ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "epd_fonts.h"

#define HEADER_H   40
#define COL_W      (EPD_WIDTH / DASH_ROOMS)     /* 266 px */
#define TILE_H     250
#define GAP        6

typedef struct { epd_color_t bg, fg; const char *label; } category_t;

static category_t category(const dash_room_t *r)
{
    if (!r->online || r->sensor_error) return (category_t){ EPD_WHITE, EPD_BLACK, "BEZ DAT" };
    if (r->co2 >= DASH_CO2_LIMIT)      return (category_t){ EPD_RED, EPD_WHITE, "NAD LIMITEM" };
    if (r->co2 >= DASH_CO2_WARN)       return (category_t){ EPD_YELLOW, EPD_BLACK, "VYVĚTRAT" };
    return (category_t){ EPD_WHITE, EPD_BLACK, "V POŘÁDKU" };
}

/* "%.1f" with a decimal comma */
static void fmt_decimal(char *buf, size_t n, const char *fmt, float v)
{
    snprintf(buf, n, fmt, v);
    for (char *p = buf; *p; p++) if (*p == '.') *p = ',';
}

static void fmt_age(char *buf, size_t n, uint32_t s)
{
    if (s < 60)            snprintf(buf, n, "před %u s", (unsigned)s);
    else if (s < 3600)     snprintf(buf, n, "před %u min", (unsigned)(s / 60));
    else                   snprintf(buf, n, "před %u h", (unsigned)(s / 3600));
}

static const char *trend_arrow(int t)
{
    if (t > 150) return "↑";
    if (t > 40)  return "↗";
    if (t < -150) return "↓";
    if (t < -40) return "↘";
    return "→";
}

static void draw_header(epd_canvas_t *c, const dash_view_t *v)
{
    char buf[96];
    epd_gfx_fill_rect(c, 0, 0, c->width, HEADER_H, EPD_BLACK);
    epd_gfx_text_box(c, 12, 0, 400, HEADER_H, "Kvalita vzduchu v učebnách", &epd_font_inter_bold_20,
                     EPD_WHITE, EPD_ALIGN_LEFT, EPD_ALIGN_MIDDLE);

    char volts[16];
    fmt_decimal(volts, sizeof(volts), "%.2f", v->battery_v);
    snprintf(buf, sizeof(buf), "výměny %u ✓  %u ✗   baterie %s V %d %%",
             (unsigned)v->exchanges_ok, (unsigned)v->exchanges_failed, volts, v->battery_pct);
    epd_gfx_text_box(c, 0, 0, c->width - 12, HEADER_H, buf, &epd_font_inter_regular_16,
                     v->battery_pct < 20 ? EPD_YELLOW : EPD_WHITE, EPD_ALIGN_RIGHT, EPD_ALIGN_MIDDLE);
}

static void draw_tile(epd_canvas_t *c, int x, int y, int w, int h, const dash_room_t *r)
{
    char buf[64];
    category_t cat = category(r);

    epd_gfx_fill_rect(c, x, y, w, h, cat.bg);
    epd_gfx_rect_thick(c, x, y, w, h, 2, EPD_BLACK);

    /* room name + category */
    epd_gfx_text_box(c, x + 10, y + 6, w - 20, 30, r->name, &epd_font_inter_bold_24, cat.fg,
                     EPD_ALIGN_LEFT, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, x + 10, y + 6, w - 20, 30, cat.label, &epd_font_inter_bold_16, cat.fg,
                     EPD_ALIGN_RIGHT, EPD_ALIGN_MIDDLE);

    if (!r->online || r->sensor_error) {
        epd_gfx_text_box(c, x, y + 40, w, 120, r->sensor_error ? "chyba čidla" : "žádná data",
                         &epd_font_inter_bold_32, EPD_RED, EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
        if (r->online) {
            fmt_age(buf, sizeof(buf), r->age_s);
            epd_gfx_text_box(c, x, y + h - 40, w, 32, buf, &epd_font_inter_regular_16, cat.fg,
                             EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
        }
        return;
    }

    /* big CO2 value - 71 px digits = 14.5 mm, readable from 3 m */
    snprintf(buf, sizeof(buf), "%u", r->co2);
    epd_gfx_text_box(c, x, y + 40, w, 110, buf, &epd_font_digits_96, cat.fg,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    /* unit + trend */
    snprintf(buf, sizeof(buf), "ppm CO₂   %s %+d / 15 min", trend_arrow(r->trend), r->trend);
    epd_gfx_text_box(c, x, y + 150, w, 28, buf, &epd_font_inter_bold_16, cat.fg,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    /* separator + temperature / humidity */
    epd_gfx_hline(c, x + 14, y + 184, w - 28, cat.fg);
    char t[16], rh[16];
    fmt_decimal(t, sizeof(t), "%.1f", r->temperature);
    fmt_decimal(rh, sizeof(rh), "%.0f", r->humidity);
    snprintf(buf, sizeof(buf), "%s °C   %s %%", t, rh);
    epd_gfx_text_box(c, x, y + 188, w, 32, buf, &epd_font_inter_bold_24, cat.fg,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    fmt_age(buf, sizeof(buf), r->age_s);
    epd_gfx_text_box(c, x, y + 220, w, 24, buf, &epd_font_inter_regular_16,
                     (r->age_s > 30 * 60 && cat.bg != EPD_RED) ? EPD_RED : cat.fg,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
}

static void draw_chart(epd_canvas_t *c, int x, int y, int w, int h, const dash_room_t *r)
{
    const int left = 34, bottom = 18;              /* space for axis labels */
    int px = x + left, pw = w - left - 6, py = y + 6, ph = h - bottom - 6;
    const int ymax = 2000;

    /* y position for a ppm value */
#define Y_OF(ppm) (py + ph - 1 - (int)((long)((ppm) > ymax ? ymax : (ppm)) * (ph - 1) / ymax))

    /* zones: light shading above the limits */
    epd_gfx_fill_rect_pattern(c, px, Y_OF(ymax), pw, Y_OF(DASH_CO2_LIMIT) - Y_OF(ymax),
                              EPD_RED, EPD_TRANSPARENT, EPD_PATTERN_SPARSE);
    epd_gfx_fill_rect_pattern(c, px, Y_OF(DASH_CO2_LIMIT), pw, Y_OF(DASH_CO2_WARN) - Y_OF(DASH_CO2_LIMIT),
                              EPD_YELLOW, EPD_TRANSPARENT, EPD_PATTERN_DOTS);

    /* thresholds */
    epd_gfx_hline_dashed(c, px, Y_OF(DASH_CO2_LIMIT), pw, 6, 3, EPD_RED);
    epd_gfx_hline_dashed(c, px, Y_OF(DASH_CO2_WARN), pw, 6, 3, EPD_BLACK);

    /* axes + labels */
    epd_gfx_vline(c, px, py, ph, EPD_BLACK);
    epd_gfx_hline(c, px, py + ph - 1, pw, EPD_BLACK);
    static const struct { int v; const char *s; } yl[] = { { 1500, "1500" }, { 1000, "1000" }, { 400, "400" } };
    for (int i = 0; i < 3; i++) {
        epd_gfx_text_box(c, x, Y_OF(yl[i].v) - 8, left - 4, 16, yl[i].s, &epd_font_inter_regular_12,
                         EPD_BLACK, EPD_ALIGN_RIGHT, EPD_ALIGN_MIDDLE);
    }
    static const char *xl[] = { "−24 h", "−12 h", "teď" };
    for (int i = 0; i < 3; i++) {
        int lx = px + i * (pw - 1) / 2;
        epd_gfx_vline(c, lx, py + ph - 1, 4, EPD_BLACK);
        epd_gfx_text_aligned(c, lx, py + ph + 3, xl[i], &epd_font_inter_regular_12, EPD_BLACK,
                             i == 0 ? EPD_ALIGN_LEFT : i == 2 ? EPD_ALIGN_RIGHT : EPD_ALIGN_CENTER);
    }

    /* data: 2 px line, gaps where no value */
    int prev_x = -1, prev_y = -1;
    for (int i = 0; i < DASH_HISTORY_LEN; i++) {
        uint16_t v = r->history[i];
        if (!v) { prev_x = -1; continue; }
        int cx = px + 1 + (int)((long)i * (pw - 2) / (DASH_HISTORY_LEN - 1));
        int cy = Y_OF(v);
        if (prev_x >= 0) epd_gfx_line_thick(c, prev_x, prev_y, cx, cy, 2, EPD_BLACK);
        prev_x = cx; prev_y = cy;
    }
#undef Y_OF
}

void dashboard_draw(epd_canvas_t *c, const dash_view_t *v)
{
    epd_gfx_clear(c, EPD_WHITE);
    draw_header(c, v);
    for (int i = 0; i < DASH_ROOMS; i++) {
        int x = i * COL_W + GAP / 2 + (i == DASH_ROOMS - 1 ? 2 : 0);
        int w = COL_W - GAP;
        draw_tile(c, x, HEADER_H + GAP, w, TILE_H, &v->rooms[i]);
        draw_chart(c, x, HEADER_H + GAP + TILE_H + 2, w, EPD_HEIGHT - HEADER_H - TILE_H - GAP - 4,
                   &v->rooms[i]);
    }
}

void dashboard_draw_low_battery(epd_canvas_t *c, float battery_v)
{
    char buf[96], volts[16];
    epd_gfx_clear(c, EPD_WHITE);
    epd_gfx_fill_rect(c, 0, 0, c->width, c->height, EPD_RED);
    epd_gfx_text_box(c, 0, 40, c->width, 160, "⚠", &epd_font_inter_bold_48, EPD_WHITE,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, 0, 170, c->width, 80, "NABIJTE BATERII", &epd_font_inter_bold_48, EPD_WHITE,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    fmt_decimal(volts, sizeof(volts), "%.2f", battery_v);
    snprintf(buf, sizeof(buf), "napětí %s V · panel je v bezpečném režimu", volts);
    epd_gfx_text_box(c, 0, 260, c->width, 60, buf, &epd_font_inter_regular_24, EPD_WHITE,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    epd_gfx_text_box(c, 0, 320, c->width, 60, "připojte USB-C nabíječku", &epd_font_inter_bold_24,
                     EPD_YELLOW, EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
}

/* ------------------------------------------------------------------ fake data */

static uint16_t demo_co2(int room, int i)
{
    /* i = 0..143, 10 min steps, ending "now" at 12:10, in the middle of a lesson: lessons drive CO2 up, breaks down */
    int minute_of_day = (12 * 60 + 10 + 24 * 60 - (DASH_HISTORY_LEN - 1 - i) * 10) % (24 * 60);
    float base = 430.0f + 15.0f * sinf((float)i * 0.3f + (float)room);
    bool lesson = minute_of_day >= 7 * 60 + 30 && minute_of_day < 18 * 60;
    if (!lesson) return (uint16_t)base;
    int in_block = (minute_of_day - (7 * 60 + 30)) % 110;      /* 100 min lesson + 10 min break */
    float peak = room == 0 ? 1350.0f : room == 1 ? 1050.0f : 2000.0f;
    float level = in_block < 100 ? base + (peak - base) * (1.0f - expf(-in_block / 45.0f))
                                 : base + 250.0f;
    return (uint16_t)level;
}

void dashboard_demo_data(dash_view_t *v)
{
    static const char *names[DASH_ROOMS] = { "SE3.143", "SE3.144", "SE3.145" };
    static const float temps[DASH_ROOMS] = { 22.4f, 21.8f, 24.1f };
    static const float hums[DASH_ROOMS] = { 41.0f, 47.0f, 52.0f };

    memset(v, 0, sizeof(*v));
    for (int r = 0; r < DASH_ROOMS; r++) {
        dash_room_t *room = &v->rooms[r];
        room->name = names[r];
        room->online = true;
        for (int i = 0; i < DASH_HISTORY_LEN; i++) room->history[i] = demo_co2(r, i);
        room->co2 = room->history[DASH_HISTORY_LEN - 1];
        room->trend = (int16_t)((room->co2 - room->history[DASH_HISTORY_LEN - 2]) * 3 / 2);
        room->temperature = temps[r];
        room->humidity = hums[r];
        room->age_s = 120 + 60 * r;
    }
    /* simulate a 2 h outage of the middle node during the night */
    for (int i = 20; i < 32; i++) v->rooms[1].history[i] = 0;

    v->battery_v = 3.92f;
    v->battery_pct = 78;
    v->exchanges_ok = 1284;
    v->exchanges_failed = 7;
}
