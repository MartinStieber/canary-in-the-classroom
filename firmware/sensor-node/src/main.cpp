// Čidlový uzel: kostra. SCD41 v periodickém měření, výpis na sériovou linku.
// TODO: trend za 15 min, historie 24 h (10min průměry), detekce chyby čidla,
//       ESP-NOW odpovědi na požadavky panelu (viz ../common/protocol.h).

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>

#include "protocol.h"

// Výchozí I2C piny ESP32 DevKit. LaskaKit SCD41 se připojuje kabelem uSup.
static const int PIN_SDA = 21;
static const int PIN_SCL = 22;

static SensirionI2cScd4x scd4x;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("canary sensor-node, protocol v" + String(PROTOCOL_VERSION));

    Wire.begin(PIN_SDA, PIN_SCL);
    scd4x.begin(Wire, SCD41_I2C_ADDR_62);

    // Po resetu zastavíme případné běžící měření a nastartujeme znovu.
    scd4x.stopPeriodicMeasurement();
    delay(500);
    int16_t err = scd4x.startPeriodicMeasurement();
    if (err) {
        Serial.printf("SCD41 start error: %d\n", err);
    }
}

void loop() {
    // SCD41 vydává nové měření každých ~5 s.
    delay(5000);

    uint16_t co2 = 0;
    float temp = 0, hum = 0;
    int16_t err = scd4x.readMeasurement(co2, temp, hum);
    if (err) {
        Serial.printf("SCD41 read error: %d\n", err);
        return;
    }
    Serial.printf("CO2 %u ppm, %.2f C, %.1f %%RH\n", co2, temp, hum);
}
