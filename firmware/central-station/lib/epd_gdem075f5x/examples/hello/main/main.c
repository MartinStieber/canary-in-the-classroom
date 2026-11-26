/* Minimal example: draw a few things and refresh the panel once. */
#include "esp_log.h"
#include "epd_gdem075f5x.h"
#include "epd_gfx.h"
#include "epd_fonts.h"

static const char *TAG = "hello";

void app_main(void)
{
    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(epd_init(&cfg));

    epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);     /* 96 kB framebuffer, white */
    if (!c) {
        ESP_LOGE(TAG, "out of memory");
        return;
    }

    epd_gfx_fill_rect(c, 0, 0, c->width, 60, EPD_BLACK);
    epd_gfx_text_box(c, 0, 0, c->width, 60, "Ahoj z ESPinku!", &epd_font_inter_bold_32,
                     EPD_WHITE, EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    epd_gfx_text(c, 40, 100, "Příliš žluťoučký kůň úpěl ďábelské ódy.", &epd_font_inter_regular_24, EPD_BLACK);
    epd_gfx_textf(c, 40, 150, &epd_font_inter_regular_24, EPD_RED, "Teplota %d °C, CO₂ %d ppm", 22, 845);

    epd_gfx_fill_round_rect(c, 40, 220, 340, 200, 20, EPD_YELLOW);
    epd_gfx_text_box(c, 40, 220, 340, 200, "845", &epd_font_digits_128, EPD_BLACK,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);
    epd_gfx_fill_circle(c, 590, 320, 100, EPD_RED);
    epd_gfx_text_box(c, 490, 220, 200, 200, "↗", &epd_font_inter_bold_48, EPD_WHITE,
                     EPD_ALIGN_CENTER, EPD_ALIGN_MIDDLE);

    ESP_ERROR_CHECK(epd_show(c->fb));                   /* ~15-20 s, then the panel sleeps */
    ESP_LOGI(TAG, "done, refresh took %u ms", (unsigned)epd_last_refresh_ms());

    epd_canvas_free(c);
    epd_deinit();                                       /* panel supply off */
}
