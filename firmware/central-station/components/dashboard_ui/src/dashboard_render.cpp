/**
 * @file dashboard_render.cpp
 * @brief Rozvržení a kreslení dashboardu.
 *
 * Obrazovka 800x480:
 *
 *   +--------------------------------------------------+
 *   | záhlaví: nadpis, výměny, baterie                 |
 *   +----------------+----------------+----------------+
 *   | dlaždice       | dlaždice       | dlaždice       |
 *   | (aktuální      | (...)          | (...)          |
 *   |  hodnoty)      |                |                |
 *   +----------------+----------------+----------------+
 *   | graf CO2 24 h  | graf CO2 24 h  | graf CO2 24 h  |
 *   +----------------+----------------+----------------+
 */
#include "dashboard_render.hpp"

#include <cstdio>

#include "epd_fonts.h"

namespace dash {
namespace {

/* -------------------------------------------------------------- rozvržení */

constexpr int kHeaderH = 40;
constexpr int kColW = EPD_WIDTH / static_cast<int>(kRoomCount);     /* 266 px */
constexpr int kGap = 6;                             /* mezera mezi sloupci */
constexpr int kTileY = kHeaderH + kGap;
constexpr int kTileH = 250;
constexpr int kChartY = kTileY + kTileH + 2;
constexpr int kChartH = EPD_HEIGHT - kChartY - 2;

/* ------------------------------------------------------------ prahy zobrazení */

constexpr int kBatteryLowPct = 20;                  /* pod touto hodnotou žlutý text */
constexpr std::uint32_t kAgeOldS = 30 * 60;         /* starší měření se zvýrazní červeně */
constexpr int kTrendSlowPpm = 40;                   /* nad: šikmá šipka */
constexpr int kTrendFastPpm = 150;                  /* nad: svislá šipka */
constexpr int kChartMaxPpm = 2000;                  /* horní okraj grafu */

/* ------------------------------------------------------------------ pomocné */

/** Vzhled dlaždice podle stavu učebny. */
struct Category {
    epd_color_t bg, fg;
    const char *label;
};

Category categoryOf(const Room &room)
{
    if (room.status() != Room::Status::Ok) {
        return { EPD_WHITE, EPD_BLACK, "BEZ DAT" };
    }
    if (room.measurement().co2Ppm >= kCo2LimitPpm) {
        return { EPD_RED, EPD_WHITE, "NAD LIMITEM" };
    }
    if (room.measurement().co2Ppm >= kCo2WarnPpm) {
        return { EPD_YELLOW, EPD_BLACK, "VYVĚTRAT" };
    }
    return { EPD_WHITE, EPD_BLACK, "V POŘÁDKU" };
}

/** Zapíše číslo s daným počtem desetinných míst a s desetinnou čárkou. */
void formatDecimal(char *buf, size_t n, float value, int decimals)
{
    snprintf(buf, n, "%.*f", decimals, value);
    for (char *p = buf; *p; p++) {
        if (*p == '.') *p = ',';
    }
}

/** Zapíše stáří měření v nejvhodnější jednotce ("před 5 min"). */
void formatAge(char *buf, size_t n, uint32_t seconds)
{
    if (seconds < 60)        snprintf(buf, n, "před %u s", static_cast<unsigned>(seconds));
    else if (seconds < 3600) snprintf(buf, n, "před %u min", static_cast<unsigned>(seconds / 60));
    else                     snprintf(buf, n, "před %u h", static_cast<unsigned>(seconds / 3600));
}

const char *trendArrow(int trendPpm)
{
    if (trendPpm > kTrendFastPpm)  return "↑";
    if (trendPpm > kTrendSlowPpm)  return "↗";
    if (trendPpm < -kTrendFastPpm) return "↓";
    if (trendPpm < -kTrendSlowPpm) return "↘";
    return "→";
}

/** Jeden řádek textu vycentrovaný na výšku v zadaném obdélníku. */
void drawLabel(epd_canvas_t *c, int x, int y, int w, int h, const char *text,
               const epd_font_t *font, epd_color_t color, epd_halign_t align)
{
    epd_gfx_text_box(c, x, y, w, h, text, font, color, align, EPD_ALIGN_MIDDLE);
}

/* ------------------------------------------------------------------ záhlaví */

void drawHeader(epd_canvas_t *c, const DashboardData &data)
{
    epd_gfx_fill_rect(c, 0, 0, c->width, kHeaderH, EPD_BLACK);
    drawLabel(c, 12, 0, 400, kHeaderH, "Kvalita vzduchu v učebnách",
              &epd_font_inter_bold_20, EPD_WHITE, EPD_ALIGN_LEFT);

    char volts[16], buf[96];
    formatDecimal(volts, sizeof(volts), data.battery.voltageV, 2);
    snprintf(buf, sizeof(buf), "výměny %u ✓  %u ✗   baterie %s V %d %%",
             static_cast<unsigned>(data.exchanges.ok), static_cast<unsigned>(data.exchanges.failed),
             volts, data.battery.percent);
    drawLabel(c, 0, 0, c->width - 12, kHeaderH, buf, &epd_font_inter_regular_16,
              data.battery.percent < kBatteryLowPct ? EPD_YELLOW : EPD_WHITE, EPD_ALIGN_RIGHT);
}

/* ----------------------------------------------------------------- dlaždice */

/** Obsah dlaždice bez platného měření. */
void drawTileNoData(epd_canvas_t *c, int x, int y, int w, int h,
                    const Room &room, epd_color_t fg)
{
    const bool sensorError = room.status() == Room::Status::SensorError;

    drawLabel(c, x, y + 40, w, 120, sensorError ? "chyba čidla" : "žádná data",
              &epd_font_inter_bold_32, EPD_RED, EPD_ALIGN_CENTER);

    /* stáří má smysl jen u uzlu, který odpovídá */
    if (sensorError) {
        char buf[32];
        formatAge(buf, sizeof(buf), room.measurement().ageS);
        drawLabel(c, x, y + h - 40, w, 32, buf, &epd_font_inter_regular_16, fg, EPD_ALIGN_CENTER);
    }
}

/** Obsah dlaždice s platným měřením. */
void drawTileValues(epd_canvas_t *c, int x, int y, int w,
                    const Measurement &m, const Category &cat)
{
    char buf[64];

    /* velké CO2: číslice vysoké 71 px = 14,5 mm, čitelné ze 3 m */
    snprintf(buf, sizeof(buf), "%u", m.co2Ppm);
    drawLabel(c, x, y + 40, w, 110, buf, &epd_font_digits_96, cat.fg, EPD_ALIGN_CENTER);

    /* jednotka a trend */
    snprintf(buf, sizeof(buf), "ppm CO₂   %s %+d / 15 min",
             trendArrow(m.co2TrendPpm), m.co2TrendPpm);
    drawLabel(c, x, y + 150, w, 28, buf, &epd_font_inter_bold_16, cat.fg, EPD_ALIGN_CENTER);

    epd_gfx_hline(c, x + 14, y + 184, w - 28, cat.fg);

    /* teplota a vlhkost */
    char temperature[16], humidity[16];
    formatDecimal(temperature, sizeof(temperature), m.temperatureC, 1);
    formatDecimal(humidity, sizeof(humidity), m.humidityPct, 0);
    snprintf(buf, sizeof(buf), "%s °C   %s %%", temperature, humidity);
    drawLabel(c, x, y + 188, w, 32, buf, &epd_font_inter_bold_24, cat.fg, EPD_ALIGN_CENTER);

    /* stáří měření; na červeném pozadí zůstává v barvě textu, aby bylo vidět */
    const bool highlightAge = m.ageS > kAgeOldS && cat.bg != EPD_RED;
    formatAge(buf, sizeof(buf), m.ageS);
    drawLabel(c, x, y + 220, w, 24, buf, &epd_font_inter_regular_16,
              highlightAge ? EPD_RED : cat.fg, EPD_ALIGN_CENTER);
}

void drawTile(epd_canvas_t *c, int x, int y, int w, int h, const Room &room)
{
    const Category cat = categoryOf(room);

    epd_gfx_fill_rect(c, x, y, w, h, cat.bg);
    epd_gfx_rect_thick(c, x, y, w, h, 2, EPD_BLACK);

    /* název učebny vlevo, slovní hodnocení vpravo */
    drawLabel(c, x + 10, y + 6, w - 20, 30, room.name(),
              &epd_font_inter_bold_24, cat.fg, EPD_ALIGN_LEFT);
    drawLabel(c, x + 10, y + 6, w - 20, 30, cat.label,
              &epd_font_inter_bold_16, cat.fg, EPD_ALIGN_RIGHT);

    if (room.status() == Room::Status::Ok) {
        drawTileValues(c, x, y, w, room.measurement(), cat);
    } else {
        drawTileNoData(c, x, y, w, h, room, cat.fg);
    }
}

/* --------------------------------------------------------------------- graf */

/** Vykreslovací plocha grafu (bez popisků os). */
struct Plot {
    int x, y, w, h;
};

/** Svislá souřadnice pro hodnotu v ppm; hodnoty nad rozsahem se oříznou. */
int plotY(const Plot &p, int ppm)
{
    if (ppm > kChartMaxPpm) ppm = kChartMaxPpm;
    return p.y + p.h - 1 - static_cast<int>(static_cast<long>(ppm) * (p.h - 1) / kChartMaxPpm);
}

/** Podbarvené zóny nad prahy a čárkované linie prahů. */
void drawChartZones(epd_canvas_t *c, const Plot &p)
{
    const int top = plotY(p, kChartMaxPpm);
    const int warn = plotY(p, kCo2WarnPpm);
    const int limit = plotY(p, kCo2LimitPpm);

    epd_gfx_fill_rect_pattern(c, p.x, top, p.w, limit - top,
                              EPD_RED, EPD_TRANSPARENT, EPD_PATTERN_SPARSE);
    epd_gfx_fill_rect_pattern(c, p.x, limit, p.w, warn - limit,
                              EPD_YELLOW, EPD_TRANSPARENT, EPD_PATTERN_DOTS);

    epd_gfx_hline_dashed(c, p.x, limit, p.w, 6, 3, EPD_RED);
    epd_gfx_hline_dashed(c, p.x, warn, p.w, 6, 3, EPD_BLACK);
}

/** Osy a jejich popisky. @p labelX je levý okraj popisků svislé osy. */
void drawChartAxes(epd_canvas_t *c, const Plot &p, int labelX)
{
    const int baseY = p.y + p.h - 1;

    epd_gfx_vline(c, p.x, p.y, p.h, EPD_BLACK);
    epd_gfx_hline(c, p.x, baseY, p.w, EPD_BLACK);

    /* svislá osa [ppm] */
    static constexpr struct { int ppm; const char *text; } yLabels[] = {
        { 1500, "1500" }, { 1000, "1000" }, { 400, "400" },
    };
    for (const auto &label : yLabels) {
        drawLabel(c, labelX, plotY(p, label.ppm) - 8, p.x - labelX - 4, 16,
                  label.text, &epd_font_inter_regular_12, EPD_BLACK, EPD_ALIGN_RIGHT);
    }

    /* časová osa: začátek, střed, konec */
    static constexpr struct { const char *text; epd_halign_t align; } xLabels[] = {
        { "−24 h", EPD_ALIGN_LEFT }, { "−12 h", EPD_ALIGN_CENTER }, { "teď", EPD_ALIGN_RIGHT },
    };
    for (int i = 0; i < 3; i++) {
        int tickX = p.x + i * (p.w - 1) / 2;
        epd_gfx_vline(c, tickX, baseY, 4, EPD_BLACK);
        epd_gfx_text_aligned(c, tickX, baseY + 4, xLabels[i].text,
                             &epd_font_inter_regular_12, EPD_BLACK, xLabels[i].align);
    }
}

/** Křivka historie tloušťky 2 px. Chybějící vzorky (0) křivku přeruší. */
void drawChartLine(epd_canvas_t *c, const Plot &p, const Room::History &history)
{
    bool hasPrev = false;
    int prevX = 0, prevY = 0;

    for (int i = 0; i < static_cast<int>(history.size()); i++) {
        if (history[i] == 0) {
            hasPrev = false;
            continue;
        }
        int x = p.x + 1 + static_cast<int>(static_cast<long>(i) * (p.w - 2) /
                                           (static_cast<int>(history.size()) - 1));
        int y = plotY(p, history[i]);
        if (hasPrev) {
            epd_gfx_line_thick(c, prevX, prevY, x, y, 2, EPD_BLACK);
        }
        hasPrev = true;
        prevX = x;
        prevY = y;
    }
}

void drawChart(epd_canvas_t *c, int x, int y, int w, int h, const Room &room)
{
    const int left = 34, bottom = 18;       /* místo pro popisky os */
    const Plot plot = {
        .x = x + left,
        .y = y + 6,
        .w = w - left - 6,
        .h = h - bottom - 6,
    };

    /* pořadí je důležité: křivka musí být nad zónami i osami */
    drawChartZones(c, plot);
    drawChartAxes(c, plot, x);
    drawChartLine(c, plot, room.history());
}

}  // namespace

/* --------------------------------------------------------------- obrazovky */

void render(epd_canvas_t &canvas, const DashboardData &data)
{
    epd_canvas_t *c = &canvas;

    epd_gfx_clear(c, EPD_WHITE);
    drawHeader(c, data);

    for (int i = 0; i < static_cast<int>(kRoomCount); i++) {
        /* 800 px nejde dělit třemi: poslední sloupec posuneme o zbylé 2 px */
        const int x = i * kColW + kGap / 2 + (i == static_cast<int>(kRoomCount) - 1 ? 2 : 0);
        const int w = kColW - kGap;
        drawTile(c, x, kTileY, w, kTileH, data.rooms[i]);
        drawChart(c, x, kChartY, w, kChartH, data.rooms[i]);
    }
}

void renderLowBattery(epd_canvas_t &canvas, float voltageV)
{
    epd_canvas_t *c = &canvas;

    epd_gfx_clear(c, EPD_RED);

    drawLabel(c, 0, 40, c->width, 160, "⚠",
              &epd_font_inter_bold_48, EPD_WHITE, EPD_ALIGN_CENTER);
    drawLabel(c, 0, 170, c->width, 80, "NABIJTE BATERII",
              &epd_font_inter_bold_48, EPD_WHITE, EPD_ALIGN_CENTER);

    char volts[16], buf[96];
    formatDecimal(volts, sizeof(volts), voltageV, 2);
    snprintf(buf, sizeof(buf), "napětí %s V · panel je v bezpečném režimu", volts);
    drawLabel(c, 0, 260, c->width, 60, buf,
              &epd_font_inter_regular_24, EPD_WHITE, EPD_ALIGN_CENTER);

    drawLabel(c, 0, 320, c->width, 60, "připojte USB-C nabíječku",
              &epd_font_inter_bold_24, EPD_YELLOW, EPD_ALIGN_CENTER);
}

}  // namespace dash
