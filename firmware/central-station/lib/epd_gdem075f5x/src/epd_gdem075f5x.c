#include "epd_gdem075f5x.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "soc/soc_caps.h"

static const char *TAG = "epd";

#define CHUNK                    4096      /* SPI transfer size / DMA bounce buffer */
#define DEFAULT_BUSY_TIMEOUT_MS  60000

static epd_config_t s_cfg;
static spi_device_handle_t s_dev;
static uint8_t *s_dma_buf;
static bool s_inited;
static bool s_own_bus;
static bool s_cfg_valid;      /* s_cfg holds real pins (survives epd_deinit) */
static uint32_t s_last_refresh_ms;

/* ------------------------------------------------------------------ SPI helpers */

static esp_err_t tx(const uint8_t *data, size_t len, int dc_level)
{
    gpio_set_level(s_cfg.dc, dc_level);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(s_dev, &t);
}

/* Parameters go out in one burst after the command. The vendor demo toggles CS
 * after every byte; the controller accepts both (tested). */
esp_err_t epd_send(uint8_t cmd, const uint8_t *params, size_t len)
{
    if (!s_inited) return ESP_ERR_INVALID_STATE;
    if (len > CHUNK) return ESP_ERR_INVALID_SIZE;
    s_dma_buf[0] = cmd;
    esp_err_t err = tx(s_dma_buf, 1, 0);
    if (err != ESP_OK || len == 0) return err;
    memcpy(s_dma_buf, params, len);
    return tx(s_dma_buf, len, 1);
}

#define SEND(cmd, ...) do {                                         \
        static const uint8_t _p[] = { __VA_ARGS__ };                \
        esp_err_t _e = epd_send((cmd), _p, sizeof(_p));             \
        if (_e != ESP_OK) return _e;                                \
    } while (0)

/* BUSY: 0 = busy, 1 = idle */
static esp_err_t wait_idle(uint32_t *elapsed_ms)
{
    uint32_t timeout = s_cfg.busy_timeout_ms ? s_cfg.busy_timeout_ms : DEFAULT_BUSY_TIMEOUT_MS;
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(s_cfg.busy) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if ((esp_timer_get_time() - start) / 1000 >= timeout) {
            ESP_LOGE(TAG, "BUSY timeout after %u ms", (unsigned)timeout);
            return ESP_ERR_TIMEOUT;
        }
    }
    if (elapsed_ms) *elapsed_ms = (uint32_t)((esp_timer_get_time() - start) / 1000);
    return ESP_OK;
}

/* ------------------------------------------------------------------ lifecycle */

void epd_power(bool on)
{
    if (!s_cfg_valid || s_cfg.power == GPIO_NUM_NC) return;
    gpio_set_level(s_cfg.power, on ? 1 : 0);
    if (on) vTaskDelay(pdMS_TO_TICKS(100));
}

void epd_prepare_deep_sleep(void)
{
    if (!s_cfg_valid || s_cfg.power == GPIO_NUM_NC) return;
    gpio_set_level(s_cfg.power, 0);
    gpio_hold_en(s_cfg.power);
#if !SOC_GPIO_SUPPORT_HOLD_SINGLE_IO_IN_DSLP
    gpio_deep_sleep_hold_en();
#endif
}

esp_err_t epd_init(const epd_config_t *cfg)
{
    if (s_inited) return ESP_OK;
    if (!cfg) return ESP_ERR_INVALID_ARG;
    s_cfg = *cfg;
    s_cfg_valid = true;
    esp_err_t err;

    uint64_t out_mask = (1ULL << cfg->dc) | (1ULL << cfg->rst);
    if (cfg->power != GPIO_NUM_NC) {
        gpio_hold_dis(cfg->power);              /* released after deep sleep */
        out_mask |= 1ULL << cfg->power;
    }
    gpio_config_t out = { .mode = GPIO_MODE_OUTPUT, .pin_bit_mask = out_mask };
    if ((err = gpio_config(&out)) != ESP_OK) return err;
    gpio_set_level(cfg->rst, 1);

    gpio_config_t in = { .mode = GPIO_MODE_INPUT, .pin_bit_mask = 1ULL << cfg->busy };
    if ((err = gpio_config(&in)) != ESP_OK) return err;

    s_dma_buf = heap_caps_malloc(CHUNK, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_dma_buf) return ESP_ERR_NO_MEM;

    spi_bus_config_t bus = {
        .mosi_io_num = cfg->mosi,
        .miso_io_num = -1,
        .sclk_io_num = cfg->sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CHUNK,
    };
    err = spi_bus_initialize(cfg->host, &bus, SPI_DMA_CH_AUTO);
    if (err == ESP_OK) {
        s_own_bus = true;
    } else if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "SPI bus already initialised, sharing it");
        s_own_bus = false;
    } else {
        goto fail;
    }

    spi_device_interface_config_t dev = {
        .clock_speed_hz = cfg->spi_clock_hz > 0 ? cfg->spi_clock_hz : 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = cfg->cs,
        .queue_size = 1,
    };
    if ((err = spi_bus_add_device(cfg->host, &dev, &s_dev)) != ESP_OK) {
        if (s_own_bus) spi_bus_free(cfg->host);
        goto fail;
    }

    s_inited = true;
    epd_power(true);
    return ESP_OK;

fail:
    free(s_dma_buf);
    s_dma_buf = NULL;
    return err;
}

esp_err_t epd_deinit(void)
{
    if (!s_inited) return ESP_OK;
    epd_power(false);
    spi_bus_remove_device(s_dev);
    if (s_own_bus) spi_bus_free(s_cfg.host);
    free(s_dma_buf);
    s_dma_buf = NULL;
    s_inited = false;
    return ESP_OK;
}

/* ------------------------------------------------------------------ low level */

esp_err_t epd_panel_init(void)
{
    if (!s_inited) return ESP_ERR_INVALID_STATE;

    /* Hardware reset (vendor timing: >=20 ms, RST low >=10 ms, high >=10 ms) */
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(s_cfg.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(s_cfg.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    esp_err_t err = wait_idle(NULL);
    if (err != ESP_OK) return err;

    /* Init sequence from the Good Display / LaskaKit demo, unchanged */
    SEND(0x00, 0x0F, 0x29);                                       /* panel setting */
    SEND(0x06, 0x0F, 0x8B, 0x93, 0xA1);                           /* booster soft start */
    SEND(0x41, 0x00);                                             /* temperature sensor */
    SEND(0x50, 0x37);                                             /* VCOM / data interval */
    SEND(0x60, 0x02, 0x02);                                       /* TCON */
    SEND(0x61, EPD_WIDTH >> 8, EPD_WIDTH & 0xFF,
               EPD_HEIGHT >> 8, EPD_HEIGHT & 0xFF);               /* resolution */
    SEND(0x62, 0x98, 0x98, 0x98, 0x75, 0xCA, 0xB2, 0x98, 0x7E);
    SEND(0x65, 0x00, 0x00, 0x00, 0x00);                           /* gate/source start */
    SEND(0xE7, 0x1C);
    SEND(0xE3, 0x00);
    SEND(0xE9, 0x01);
    SEND(0x30, 0x08);                                             /* frame rate */

    if ((err = epd_send(0x04, NULL, 0)) != ESP_OK) return err;    /* power on */
    return wait_idle(NULL);
}

esp_err_t epd_write_frame(const uint8_t *fb)
{
    if (!fb) return ESP_ERR_INVALID_ARG;
    esp_err_t err = epd_send(0x10, NULL, 0);
    for (size_t off = 0; err == ESP_OK && off < EPD_FB_SIZE; off += CHUNK) {
        size_t n = EPD_FB_SIZE - off < CHUNK ? EPD_FB_SIZE - off : CHUNK;
        memcpy(s_dma_buf, fb + off, n);
        err = tx(s_dma_buf, n, 1);
    }
    return err;
}

static esp_err_t write_color(epd_color_t color)
{
    uint8_t c = (uint8_t)color & 0x3;
    uint8_t b = (c << 6) | (c << 4) | (c << 2) | c;
    esp_err_t err = epd_send(0x10, NULL, 0);
    if (err != ESP_OK) return err;
    memset(s_dma_buf, b, CHUNK);
    for (size_t off = 0; err == ESP_OK && off < EPD_FB_SIZE; off += CHUNK) {
        size_t n = EPD_FB_SIZE - off < CHUNK ? EPD_FB_SIZE - off : CHUNK;
        err = tx(s_dma_buf, n, 1);
    }
    return err;
}

esp_err_t epd_refresh(void)
{
    SEND(0x12, 0x00);
    esp_err_t err = wait_idle(&s_last_refresh_ms);
    if (err == ESP_OK) ESP_LOGI(TAG, "refresh took %u ms", (unsigned)s_last_refresh_ms);
    return err;
}

esp_err_t epd_panel_sleep(void)
{
    SEND(0x02, 0x00);                         /* power off */
    esp_err_t err = wait_idle(NULL);
    esp_err_t err2 = epd_send(0x07, (const uint8_t[]){ 0xA5 }, 1);   /* deep sleep */
    return err != ESP_OK ? err : err2;
}

/* ------------------------------------------------------------------ high level */

esp_err_t epd_show(const uint8_t *fb)
{
    esp_err_t err = epd_panel_init();
    if (err == ESP_OK) err = epd_write_frame(fb);
    if (err == ESP_OK) err = epd_refresh();
    epd_panel_sleep();        /* always - the vendor warns that skipping it shortens panel life */
    return err;
}

esp_err_t epd_show_color(epd_color_t color)
{
    esp_err_t err = epd_panel_init();
    if (err == ESP_OK) err = write_color(color);
    if (err == ESP_OK) err = epd_refresh();
    epd_panel_sleep();
    return err;
}

uint32_t epd_last_refresh_ms(void)
{
    return s_last_refresh_ms;
}
