# central-station (zobrazovací panel)

Firmware zobrazovacího panelu (Martin).

**Hardware:** LaskaKit ESPink v3.6 (ESP32-S3), e-paper 7,5" GDEM075F53 (800×480, černá/bílá/červená/žlutá), Li-Pol 4000 mAh.

**Úkol:** většinu času spí, periodicky se probudí, přes ESP-NOW si vyžádá data od čidlových uzlů, překreslí displej a znovu usne. Protokol viz `../common/let_proto.h` a PDF specifikace ve stejné složce.

## Build (PlatformIO)

```bash
cd firmware/central-station
pio run                 # build
pio run -t upload       # nahrát (ESPink přes USB-C)
pio device monitor      # sériový monitor
```

Framework je ESP-IDF (přes PlatformIO), board `esp32-s3-devkitc-1` (ESPink nemá vlastní definici). Aktuálně je to jen blink jako placeholder. Sdílený protokol se includuje přes `src/CMakeLists.txt` (`../../common`).

## Co je potřeba doplnit podle schématu ESPink v3.6

- Pin LED (v `main.c` je zástupné GPIO2), velikost flash.
- Piny e-paperu (CS, DC, RST, BUSY, SPI), ovládání napájení displeje.
- Pin pro měření napětí baterie (a dělič).
- Ovladač displeje GDEM075F53 (4barevný 800×480) pro ESP-IDF. Knihovny jako GxEPD2 jsou jen pro Arduino, takže bude potřeba C ovladač nebo port.
- Dokumentace: https://github.com/LaskaKit (repozitáře ESPink).

## Zapojení

Schéma zapojení viz `../../hardware/`.
