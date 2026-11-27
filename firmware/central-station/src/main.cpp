// Zobrazovací panel (ESPink v3.6): placeholder (blink).
// Cílový cyklus: probuzení -> ESP-NOW dotaz na uzly -> překreslení displeje -> deep sleep.
// TODO: ovladač e-paperu, ESP-NOW, měření baterie, deep sleep, počítadla výměn
//       (viz ../../common/let_proto.h).

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "epd_gdem075f5x.h"
#include "epd_gfx.h"
#include "epd_fonts.h"

#include "let_proto.h"

#include "dashboard_ui.h"

static const char *TAG = "central-station";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "canary panel, protocol v%d", LET_VERSION);

    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(epd_init(&cfg));

    epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);
    if (!c) {
        ESP_LOGE(TAG, "out of memory");
        return;
    }

    static dash_view_t view;            // static: ~1 kB, ať nezatěžuje zásobník
    dashboard_demo_data(&view);
    dashboard_draw(c, &view);

    ESP_ERROR_CHECK(epd_show(c->fb));
    ESP_LOGI(TAG, "hotovo, %u ms", (unsigned)epd_last_refresh_ms());

    epd_canvas_free(c);
    epd_deinit();
}