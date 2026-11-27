// Zobrazovací panel (ESPink v3.6).
// Cílový cyklus: probuzení -> ESP-NOW dotaz na uzly -> překreslení displeje -> deep sleep.
// Teď: jednorázově zobrazí testovací obrazovku dashboardu.
// TODO: ESP-NOW, měření baterie, deep sleep, počítadla výměn
//       (viz ../../common/let_proto.h).

#include "esp_log.h"

#include "let_proto.h"

#include "dashboard_ui.hpp"

static const char *TAG = "central-station";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "canary panel, protocol v%d", LET_VERSION);

    static dash::Dashboard dashboard;   // static: ~1 kB, ať nezatěžuje zásobník

    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(dashboard.begin(cfg));

    // později nahradit: naplnit dashboard.data z uzlů a zavolat refresh()
    ESP_ERROR_CHECK(dashboard.test(dash::TestScreen::LowBattery));

    dashboard.end();
}
