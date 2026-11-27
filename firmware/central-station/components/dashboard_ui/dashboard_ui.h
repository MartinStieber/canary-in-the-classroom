#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "epd_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DASH_ROOMS        3
#define DASH_HISTORY_LEN  144
#define DASH_CO2_WARN     1000
#define DASH_CO2_LIMIT    1500

typedef struct {
    const char *name;
    bool online;
    bool sensor_error;
    uint16_t co2;
    int16_t trend;                      // změna za 15 min [ppm]
    float temperature;                  // °C
    float humidity;                     // %
    uint32_t age_s;
    uint16_t history[DASH_HISTORY_LEN]; // 10min průměry, 0 = bez dat
} dash_room_t;

typedef struct {
    dash_room_t rooms[DASH_ROOMS];
    float battery_v;
    int battery_pct;
    uint32_t exchanges_ok, exchanges_failed;
} dash_view_t;

void dashboard_draw(epd_canvas_t *c, const dash_view_t *v);
void dashboard_draw_low_battery(epd_canvas_t *c, float battery_v);
void dashboard_demo_data(dash_view_t *v);

#ifdef __cplusplus
}
#endif
