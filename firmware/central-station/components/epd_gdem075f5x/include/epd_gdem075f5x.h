/**
 * @file epd_gdem075f5x.h
 * @brief ESP-IDF driver for Good Display GDEM075F52 / GDEM075F53
 *        (7.5", 800x480, black/white/yellow/red, controller JD79665AA / JD79665CA).
 *
 * The init sequence comes from the Good Display demo shipped by LaskaKit for the
 * ESPink board (LaskaKit/ESPink, SW folder). Tested on LaskaKit ESPink v3.6 (ESP32-S3)
 * with GDEM075F53.
 *
 * Typical use:
 * @code
 *   epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
 *   ESP_ERROR_CHECK(epd_init(&cfg));
 *   epd_canvas_t *c = epd_canvas_create(EPD_ROT_0);
 *   epd_gfx_text(c, 20, 20, "Ahoj", &epd_font_inter_bold_32, EPD_BLACK);
 *   ESP_ERROR_CHECK(epd_show(c->fb));       // init + write + refresh + panel sleep
 *   epd_prepare_deep_sleep();               // panel supply off, pin held low
 *   esp_deep_sleep(10ULL * 60 * 1000000);
 * @endcode
 *
 * The driver is not thread-safe: call it from one task only.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "epd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hardware configuration. */
typedef struct {
    spi_host_device_t host;     /**< SPI host, e.g. SPI2_HOST */
    gpio_num_t mosi;            /**< SPI data (panel SDA/DIN) */
    gpio_num_t sclk;            /**< SPI clock */
    gpio_num_t cs;              /**< Chip select (active low) */
    gpio_num_t dc;              /**< Data/command: 0 = command, 1 = data */
    gpio_num_t rst;             /**< Reset (active low) */
    gpio_num_t busy;            /**< BUSY input: 0 = busy, 1 = idle */
    gpio_num_t power;           /**< Panel supply switch, HIGH = on. GPIO_NUM_NC if not present. */
    int spi_clock_hz;           /**< SPI clock, 10 MHz is known to work */
    uint32_t busy_timeout_ms;   /**< Max wait for BUSY; 0 = default (60 s) */
} epd_config_t;

/**
 * @brief Pinout of LaskaKit ESPink v3.x (ESP32-S3), taken from the LaskaKit demo header.
 * Verified on v3.6.
 */
#define EPD_CONFIG_ESPINK_V3() {          \
    .host = SPI2_HOST,                    \
    .mosi = GPIO_NUM_11,                  \
    .sclk = GPIO_NUM_12,                  \
    .cs   = GPIO_NUM_10,                  \
    .dc   = GPIO_NUM_48,                  \
    .rst  = GPIO_NUM_45,                  \
    .busy = GPIO_NUM_38,                  \
    .power = GPIO_NUM_47,                 \
    .spi_clock_hz = 10 * 1000 * 1000,     \
    .busy_timeout_ms = 0,                 \
}

/* ------------------------------------------------------------------ lifecycle */

/**
 * @brief Configure GPIOs, SPI bus + device and switch the panel supply on.
 *
 * If the SPI bus is already initialised (shared with another device), it is reused
 * and left alone by epd_deinit(). Calling epd_init() twice is a no-op.
 */
esp_err_t epd_init(const epd_config_t *cfg);

/** @brief Switch the panel supply off and release the SPI device (and bus, if we own it). */
esp_err_t epd_deinit(void);

/** @brief Switch the panel supply (ESPink transistor) on or off. On adds a 100 ms settle delay. */
void epd_power(bool on);

/**
 * @brief Prepare for MCU deep sleep: panel supply off and the power pin held low,
 *        so the supply stays off while the chip sleeps. epd_init() releases the hold.
 */
void epd_prepare_deep_sleep(void);

/* ------------------------------------------------------------------ high level */

/**
 * @brief One complete redraw: panel init, write framebuffer, refresh, panel sleep.
 *
 * Blocks for the whole refresh (roughly 12-20 s depending on temperature).
 * The panel is always put to sleep at the end, even after an error.
 *
 * @param fb EPD_FB_SIZE bytes in native format (see epd_gfx.h / epd_canvas_t)
 */
esp_err_t epd_show(const uint8_t *fb);

/**
 * @brief Fill the whole panel with one colour without needing a framebuffer.
 *
 * Handy for a periodic clean (e.g. white once a day) against ghosting.
 */
esp_err_t epd_show_color(epd_color_t color);

/** @brief Duration of the last refresh (BUSY low after command 0x12), in ms. For energy balance. */
uint32_t epd_last_refresh_ms(void);

/* ------------------------------------------------------------------ low level */

/** @brief Hardware reset, init sequence and power-on (0x04). Needed before every refresh. */
esp_err_t epd_panel_init(void);
/** @brief Send command 0x10 followed by EPD_FB_SIZE bytes of framebuffer. */
esp_err_t epd_write_frame(const uint8_t *fb);
/** @brief Display refresh (0x12) and wait for BUSY. */
esp_err_t epd_refresh(void);
/** @brief Power off (0x02) and deep sleep (0x07 0xA5). Wake-up needs epd_panel_init(). */
esp_err_t epd_panel_sleep(void);
/** @brief Send a raw command with parameters. */
esp_err_t epd_send(uint8_t cmd, const uint8_t *params, size_t len);

#ifdef __cplusplus
}
#endif
