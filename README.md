# 🐤 canary-in-the-classroom

Monitor kvality vzduchu v učebnách (projekt do předmětu LET).
Jako kanárek v dole: dřív než se začneme dusit, dá vědět, že je čas otevřít okno.

## Jak to funguje

```
 [sensor-node] ──┐
 [sensor-node] ──┼──►  [central-station]  ──►  zobrazení / upozornění
 [sensor-node] ──┘
```

- **sensor-node**: měří vzduch (CO₂, teplota, vlhkost, …) a posílá data centrále.
- **central-station**: sbírá data z jednotek, vyhodnocuje a zobrazuje / signalizuje.
- Obě strany sdílí definici protokolu v `firmware/common/`.

## Struktura repa

| Složka | Obsah |
|---|---|
| `firmware/central-station/` | Firmware centrální stanice |
| `firmware/sensor-node/` | Firmware měřicí jednotky |
| `firmware/common/` | Sdílený protokol a konstanty (jediný zdroj pravdy) |
| `hardware/` | Schémata zapojení, PCB, seznam součástek |
| `models/` | 3D modely a podklady pro tisk (krabičky, držáky) |
| `docs/` | Dokumentace, zadání, podklady |
| `presentation/` | Prezentace |

## Tým

| Oblast | Kdo |
|---|---|
| Firmware | Martin, Jakub Oriniak |
| 3D tisk / konstrukce | David Vavroušek, Štěpán Zelníček, Martin Vašíček |
| Prezentace | Romana Seidlerová |
| Dokumentace | Vladimíra Rošteková, Kateřina Hoffmannová |

## Jak buildit

Viz README v jednotlivých složkách firmwaru.

## Pravidla práce s gitem

Viz [CONTRIBUTING.md](CONTRIBUTING.md).
