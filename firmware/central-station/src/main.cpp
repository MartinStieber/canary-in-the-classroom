// Zobrazovací panel (ESPink v3.6): placeholder (blink).
// Cílový cyklus: probuzení -> ESP-NOW dotaz na uzly -> překreslení displeje -> deep sleep.
// TODO: ovladač e-paperu, ESP-NOW, měření baterie, deep sleep, počítadla výměn
//       (viz ../../common/let_proto.h).

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "let_proto.h"

static const char *TAG = "central-station";

// TODO: ověřit pin LED na ESPink v3.6 podle schématu. GPIO2 je jen zástupná hodnota.
#define BLINK_GPIO GPIO_NUM_2

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "canary panel, protocol v%d", LET_VERSION);

    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    int level = 0;
    while (1) {
        level = !level;
        gpio_set_level(BLINK_GPIO, level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
