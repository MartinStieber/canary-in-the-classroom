# 🐤 canary-in-the-classroom

Monitor kvality vzduchu v učebnách (projekt do předmětu LET).
Jako kanárek v dole: dřív než se začneme dusit, dá vědět, že je čas otevřít okno.

## Jak to funguje

```
 [sensor-node] ◄──┐
 [sensor-node] ◄──┼── ESP-NOW dotaz ──  [central-station / panel s e-inkem]
 [sensor-node] ◄──┘   (panel se probudí, zeptá se, překreslí, usne)
```

- **sensor-node** (ESP32 + SCD41, z USB): měří CO₂, teplotu a vlhkost, počítá trend a historii 24 h a odpovídá na dotazy panelu.
- **central-station** (ESPink v3.6 + e-paper 7,5", baterie): většinu času spí, periodicky si vyžádá data od uzlů a zobrazí je.
- Iniciátorem komunikace je vždy panel. Definice protokolu je v `firmware/common/`.

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
