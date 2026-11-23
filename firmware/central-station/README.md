# central-station (zobrazovací panel)

Firmware zobrazovacího panelu (Martin).

**Hardware:** LaskaKit ESPink v3.6 (ESP32-S3), e-paper 7,5" GDEM075F53 (800×480, černá/bílá/červená/žlutá), Li-Pol 4000 mAh.

**Úkol:** většinu času spí, periodicky se probudí, přes ESP-NOW si vyžádá data od čidlových uzlů, překreslí displej a znovu usne. Protokol viz `../common/protocol.h`.

## Build (PlatformIO)

```bash
cd firmware/central-station
pio run                 # build
pio run -t upload       # nahrát (ESPink přes USB-C)
pio device monitor      # sériový monitor
```

Používá board `esp32-s3-devkitc-1` (ESPink nemá vlastní definici v PlatformIO) a knihovnu GxEPD2.

## Co je potřeba doplnit podle schématu ESPink v3.6

- Piny e-paper (CS, DC, RST, BUSY, SPI), ovládání napájení displeje.
- Pin pro měření napětí baterie (a dělič).
- Správná třída displeje v GxEPD2 pro GDEM075F53 (4barevný 800×480), případně jiná knihovna, pokud GxEPD2 tento panel nepodporuje.
- Dokumentace: https://github.com/LaskaKit (repozitáře ESPink).

## Zapojení

Schéma zapojení viz `../../hardware/`.
