/**
 * @file dashboard_render.hpp
 * @brief Interní rozhraní: vykreslení dashboardu do plátna (bez hardwaru).
 */
#pragma once

#include "epd_gfx.h"
#include "dashboard_data.hpp"

namespace dash {

/** @brief Vykreslí celý dashboard (přepíše celé plátno). */
void render(epd_canvas_t &canvas, const DashboardData &data);

/** @brief Vykreslí obrazovku "nabijte baterii" (přepíše celé plátno). */
void renderLowBattery(epd_canvas_t &canvas, float voltageV);

}  // namespace dash
