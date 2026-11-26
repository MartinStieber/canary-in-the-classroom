/* Shows the bundled fonts, then shapes and patterns, then leaves the panel white. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "epd_gdem075f5x.h"
#include "showcase_ui.h"

static const char *TAG = "showcase";

void app_main(void)
{
    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(epd_init(&cfg));
    epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);
    if (!c) return;

    showcase_draw_fonts(c);
    ESP_ERROR_CHECK(epd_show(c->fb));
    ESP_LOGI(TAG, "fonts page shown (%u ms)", (unsigned)epd_last_refresh_ms());
    vTaskDelay(pdMS_TO_TICKS(30000));

    showcase_draw_shapes(c);
    ESP_ERROR_CHECK(epd_show(c->fb));
    ESP_LOGI(TAG, "shapes page shown (%u ms)", (unsigned)epd_last_refresh_ms());
    vTaskDelay(pdMS_TO_TICKS(30000));

    ESP_ERROR_CHECK(epd_show_color(EPD_WHITE));       /* clean panel for storage */
    epd_canvas_free(c);
    epd_deinit();
}
