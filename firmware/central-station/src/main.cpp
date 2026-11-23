// Zobrazovací panel (ESPink v3.6): kostra.
// Cyklus: probuzení -> ESP-NOW dotaz na uzly -> překreslení displeje -> deep sleep.
// TODO: piny EPD a měření baterie podle schématu ESPink v3.6 (viz README),
//       ESP-NOW komunikace, kategorie/barvy, graf 24 h, ochrana baterie.

#include <Arduino.h>
#include <esp_sleep.h>

#include "protocol.h"

// Interval mezi dotazy na uzly (zadání: max 10 min v době výuky).
static const uint64_t WAKE_INTERVAL_S = 10 * 60;

// RTC paměť přežije deep sleep: sem patří počítadla výměn a poslední data.
RTC_DATA_ATTR static uint32_t g_exchanges_ok = 0;
RTC_DATA_ATTR static uint32_t g_exchanges_failed = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("canary panel, protocol v%d, ok=%u fail=%u\n", PROTOCOL_VERSION,
                  (unsigned)g_exchanges_ok, (unsigned)g_exchanges_failed);

    // TODO: 1) zkontrolovat napětí baterie (podpětí -> výzva k nabití + bezpečný stav)
    //       2) ESP-NOW: poslat MSG_REQUEST_CURRENT každému uzlu, počkat na odpovědi
    //       3) rozhodnout, zda překreslit (změna kategorie/trendu, max 24 h bez překreslení)
    //       4) překreslit displej (12-20 s)

    esp_sleep_enable_timer_wakeup(WAKE_INTERVAL_S * 1000000ULL);
    Serial.println("sleeping");
    Serial.flush();
    esp_deep_sleep_start();
}

void loop() {
    // Nikdy se nedostaneme sem: po setup() jde panel spát.
}
