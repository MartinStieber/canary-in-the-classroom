# central-station

Firmware centrální stanice (Martin).

- Přijímá data z jednotek `sensor-node` (viz `../common/protocol.h`).
- Vyhodnocuje kvalitu vzduchu a signalizuje / zobrazuje.

## Build

_Doplnit (toolchain, příkazy)._

## Zapojení

Schéma zapojení centrály viz `../../hardware/`.

## Tajemství

Zkopíruj `secrets.example.h` na `secrets.h` a vyplň hodnoty. `secrets.h` se necommituje.
