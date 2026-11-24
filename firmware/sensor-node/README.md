# sensor-node (čidlový uzel)

Firmware čidlového uzlu (Jakub Oriniak).

**Hardware:** ESP32 DevKit + LaskaKit SCD41 (CO₂, teplota, vlhkost, I2C přes konektor uSup), napájení z USB 5 V. Je stále zapnutý.

**Úkol:** měří CO₂/teplotu/vlhkost, počítá trend za 15 min, drží historii 24 h (10min průměry), hlásí chybu čidla a odpovídá na požadavky panelu přes ESP-NOW. Uzel sám nic neposílá. Protokol viz `../common/protocol.h`.

## Build (PlatformIO)

```bash
cd firmware/sensor-node
pio run                 # build
pio run -t upload       # nahrát
pio device monitor      # sériový monitor
```

Framework je ESP-IDF (přes PlatformIO). Aktuálně je to jen blink jako placeholder (LED na GPIO2). Sdílený protokol se includuje přes `src/CMakeLists.txt` (`../../common`).

Typické I2C piny ESP32 DevKit pro SCD41: SDA = GPIO21, SCL = GPIO22.

## Zapojení

Schéma zapojení viz `../../hardware/`.
