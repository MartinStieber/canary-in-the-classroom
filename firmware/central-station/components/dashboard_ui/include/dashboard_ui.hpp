/**
 * @file dashboard_ui.hpp
 * @brief Dashboard kvality vzduchu pro e-paper 800x480 (GDEM075F53).
 *
 * @code
 *   static dash::Dashboard dashboard;
 *   epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
 *   ESP_ERROR_CHECK(dashboard.begin(cfg));
 *
 *   dash::Room &room = dashboard.data.rooms[0];
 *   room.setName("SE3.143");
 *   room.setMeasurement({.co2Ppm = 1234, .co2TrendPpm = 180,
 *                        .temperatureC = 23.4f, .humidityPct = 45.0f, .ageS = 3});
 *   dashboard.data.battery = {.voltageV = 3.92f, .percent = 78};
 *
 *   ESP_ERROR_CHECK(dashboard.refresh());
 *   dashboard.end();
 * @endcode
 */
#pragma once

#include "epd_gdem075f5x.h"
#include "epd_gfx.h"
#include "esp_err.h"

#include "dashboard_data.hpp"

namespace dash {

/** Testovací obrazovky pro Dashboard::test(). */
enum class TestScreen {
    Demo,           /**< Běžný provoz s ukázkovými hodnotami */
    ErrorStates,    /**< Offline uzel, chyba čidla, stará data, slabá baterie */
    LowBattery,     /**< Celoplošná výzva k nabití */
};

/**
 * @brief Dashboard na e-paperu.
 *
 * Hodnoty se zapisují do veřejného členu data, na displej se dostanou až
 * voláním refresh(). Zápis do data nepotřebuje begin().
 *
 * Displej je jen jeden, takže ho v danou chvíli může vlastnit jen jedna
 * instance. Objekt má přes 1 kB: deklarujte ho jako static, ne na zásobníku.
 *
 * Data jsou v obyčejné RAM, deep sleep je smaže.
 *
 * Třída není thread-safe: používejte ji jen z jednoho tasku (ne například
 * přímo z callbacku ESP-NOW).
 */
class Dashboard {
public:
    Dashboard() = default;
    ~Dashboard() { end(); }

    Dashboard(const Dashboard &) = delete;
    Dashboard &operator=(const Dashboard &) = delete;

    /**
     * @brief Inicializuje displej a alokuje framebuffer (EPD_FB_SIZE = 96 kB).
     *
     * Opakované volání nic nedělá.
     *
     * @param config  zapojení displeje, pro ESPink v3.x EPD_CONFIG_ESPINK_V3()
     * @return ESP_OK, ESP_ERR_INVALID_STATE pokud displej drží jiná instance,
     *         ESP_ERR_NO_MEM, nebo chyba z ovladače displeje
     */
    esp_err_t begin(const epd_config_t &config);

    /**
     * @brief Uvolní framebuffer, vypne napájení displeje a podrží ho vypnuté
     *        i během deep sleepu.
     *
     * Obraz na e-paperu zůstává viditelný. Bez begin() nic nedělá.
     */
    void end();

    /**
     * @brief Vykreslí data a překreslí displej.
     *
     * Blokuje po celou dobu překreslení (zhruba 12 až 20 s podle teploty).
     *
     * @return ESP_OK, ESP_ERR_INVALID_STATE bez begin(),
     *         nebo chyba z ovladače displeje
     */
    esp_err_t refresh();

    /**
     * @brief Zobrazí testovací obrazovku s vymyšlenými hodnotami (pro ladění).
     *
     * Členu data se nedotkne, takže další refresh() ukáže zase skutečné hodnoty.
     * Blokuje stejně jako refresh().
     *
     * @return stejné jako refresh(), navíc ESP_ERR_NO_MEM
     */
    esp_err_t test(TestScreen screen = TestScreen::Demo);

    /**
     * @brief Místo dashboardu zobrazí výzvu k nabití baterie.
     *
     * Blokuje stejně jako refresh().
     *
     * @param voltageV  napětí baterie vypsané na obrazovce [V]
     * @return stejné jako refresh()
     */
    esp_err_t showLowBattery(float voltageV);

    /** Zobrazované hodnoty. */
    DashboardData data;

private:
    /** Pošle obsah plátna na displej. */
    esp_err_t show();

    epd_canvas_t *canvas_ = nullptr;    /**< != nullptr právě mezi begin() a end() */
};

}  // namespace dash
