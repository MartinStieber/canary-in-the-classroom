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

static const char *TAG = "central-station";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "canary panel, protocol v%d", LET_VERSION);

    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(epd_init(&cfg));

    epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);     /* 96 kB framebuffer, white */

    if (!c) {
        ESP_LOGE(TAG, "out of memory");
        return;
    }
    epd_gfx_clear(c, EPD_BLACK);  /* black background, white text */
    epd_gfx_text_box(c, 0, 0, c->width, c->height, "CHELLO WORLD!!!\nBěblbam", &epd_font_inter_bold_32,
                     EPD_WHITE, EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    ESP_ERROR_CHECK(epd_show(c->fb));                   /* ~15-20 s, then the panel sleeps */
    ESP_LOGI(TAG, "done, refresh took %u ms", (unsigned)epd_last_refresh_ms());
    epd_canvas_free(c);
    epd_deinit();                                       /* panel supply off */
    
}
