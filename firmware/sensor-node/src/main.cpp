// Čidlový uzel: placeholder (blink).
// TODO: SCD41 přes I2C, trend 15 min, historie 24 h, detekce chyby čidla,
//       ESP-NOW odpovědi na požadavky panelu (viz ../../common/protocol.h).

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "protocol.h"

static const char *TAG = "sensor-node";

// Modrá LED na ESP32 DevKit bývá na GPIO2 (podle verze desky se může lišit).
#define BLINK_GPIO GPIO_NUM_2

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "canary sensor-node, protocol v%d", PROTOCOL_VERSION);

    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    int level = 0;
    while (1) {
        level = !level;
        gpio_set_level(BLINK_GPIO, level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
