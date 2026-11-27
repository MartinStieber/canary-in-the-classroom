# dashboard_ui

Dashboard kvality vzduchu pro tři učebny na e-paperu 7,5" GDEM075F53 (800×480, černá/bílá/červená/žlutá). C++ komponenta pro ESP-IDF nad ovladačem [`epd_gdem075f5x`](../epd_gdem075f5x/).

Použití: **zapíšeš hodnoty do `dashboard.data` a zavoláš `refresh()`**. Kreslení i obsluhu displeje řeší knihovna.

## Rychlý start

V `CMakeLists.txt` své komponenty přidej `REQUIRES dashboard_ui`.

```cpp
#include "dashboard_ui.hpp"

static dash::Dashboard dashboard;   // static: objekt má přes 1 kB

extern "C" void app_main(void)
{
    epd_config_t cfg = EPD_CONFIG_ESPINK_V3();
    ESP_ERROR_CHECK(dashboard.begin(cfg));

    dash::Room &room = dashboard.data.rooms[0];
    room.setName("SE3.143");
    room.setMeasurement({
        .co2Ppm       = 1234,
        .co2TrendPpm  = 180,        // změna za 15 min
        .temperatureC = 23.4f,
        .humidityPct  = 45.0f,
        .ageS         = 3,
    });

    dashboard.data.battery   = {.voltageV = 3.92f, .percent = 78};
    dashboard.data.exchanges = {.ok = 1284, .failed = 7};

    ESP_ERROR_CHECK(dashboard.refresh());    // trvá cca 12-20 s
    dashboard.end();                         // vypne napájení displeje
}
```

## Jak do sebe třídy zapadají

```
Dashboard dashboard              ← ovládá displej: begin, refresh, test, end
 └── DashboardData data          ← všechno, co se má zobrazit
      ├── Room rooms[0]          ← název, stav, měření, historie
      ├── Room rooms[1]
      ├── Room rooms[2]
      ├── Battery battery
      └── ExchangeStats exchanges
```

`Dashboard` říká **kdy a jak** kreslit, `DashboardData` **co** kreslit a `Room` je jeden sloupec z těch dat.

### `Room &room` je reference, ne nová učebna

Všechny tři učebny už existují uvnitř `dashboard.data`, žádnou nevytváříš. `&` znamená referenci, tedy jiné jméno pro existující objekt. Podobá se pointeru, ale píše se s tečkou, nemůže být null a nedá se přesměrovat jinam.

```cpp
dash::Room &room = dashboard.data.rooms[0];
room.setName("SE3.143");                        // zapíše přímo do dashboardu

dashboard.data.rooms[0].setName("SE3.143");     // totéž bez zkratky
```

**Pozor na chybějící `&`.** Vznikne kopie. Přeloží se to bez chyby, ale na displeji se nic nezmění:

```cpp
dash::Room room = dashboard.data.rooms[0];      // KOPIE
room.setName("SE3.143");                        // změní jen kopii
```

### Proč je `DashboardData` zvlášť

Je to obyčejná struktura s hodnotami, bez práce s displejem:

- `test()` si vytvoří vlastní `DashboardData` s vymyšlenými hodnotami, takže skutečná data v `dashboard.data` zůstanou.
- Kreslicí kód dostane jen data a plátno. O hardwaru nic neví, takže jde přeložit i na PC.

## Ladění

`test()` zobrazí obrazovku s vymyšlenými hodnotami, uzly nejsou potřeba. Do `dashboard.data` nezapisuje.

```cpp
dashboard.test();                                   // běžný provoz
dashboard.test(dash::TestScreen::ErrorStates);      // offline, chyba čidla, stará data, slabá baterie
dashboard.test(dash::TestScreen::LowBattery);       // výzva k nabití
```

Každé volání překreslí displej, tedy trvá stejně jako `refresh()`.

## API

### `dash::Dashboard`

| Člen | Popis |
|---|---|
| `begin(config)` | Inicializuje displej a alokuje framebuffer (96 kB). |
| `end()` | Uvolní paměť, vypne napájení displeje a podrží ho vypnuté i v deep sleepu. Volá ho i destruktor. |
| `refresh()` | Vykreslí `data` a překreslí displej. |
| `test(screen)` | Testovací obrazovka. |
| `showLowBattery(voltageV)` | Celoplošná výzva k nabití místo dashboardu. |
| `data` | Zobrazované hodnoty (`dash::DashboardData`). |

Metody pracující s displejem vracejí `esp_err_t`.

### `dash::DashboardData`

| Člen | Typ | Obsah |
|---|---|---|
| `rooms` | `std::array<Room, 3>` | Učebny, sloupce zleva |
| `battery` | `Battery` | `voltageV`, `percent` |
| `exchanges` | `ExchangeStats` | `ok`, `failed` |

### `dash::Room`

| Metoda | Popis |
|---|---|
| `setName(name)` | Název (nejvýš 23 bajtů UTF-8, delší se ořízne). |
| `setMeasurement(m)` | Aktuální hodnoty; stav `Ok`. |
| `setSensorError(ageS)` | Uzel odpovídá, ale nemá platné měření („chyba čidla“). |
| `setOffline()` | Uzel neodpovídá („žádná data“). Výchozí stav. |
| `setHistory(values, count)` | Historie CO₂ pro graf. |
| `name()`, `status()`, `measurement()`, `history()` | Čtení hodnot. |

Podrobnosti jsou v [`dashboard_ui.hpp`](include/dashboard_ui.hpp) a [`dashboard_data.hpp`](include/dashboard_data.hpp).

## Pravidla

- **Data se na displej dostanou až s `refresh()`.** Zůstávají nastavená do dalšího přepsání, takže stačí měnit jen to nové.
- **Index učebny se nekontroluje.** Platí `0` až `dash::kRoomCount - 1`.
- **Jen jedna instance** může mít displej (`begin()` druhé vrátí `ESP_ERR_INVALID_STATE`).
- **Deep sleep data smaže.** Po probuzení nastav všechno znovu, včetně názvů a historie. Co má spánek přežít, ulož si v aplikaci (např. `RTC_DATA_ATTR`).
- **`refresh()` blokuje** po celou dobu překreslení.
- **Není thread-safe.** Používej z jednoho tasku, ne přímo z callbacku ESP-NOW.

## Historie pro graf

- 10min průměry, **nejstarší první**; poslední hodnota je „teď“.
- Graf pojme `dash::kHistoryLength` = 144 hodnot (24 h). Kratší pole se zarovná doprava, z delšího se vezme konec.
- `0` znamená „bez dat“, v křivce vznikne mezera. Osamocený vzorek mezi dvěma mezerami se nevykreslí.
- Osa končí na 2000 ppm, vyšší hodnoty se oříznou.
- `setHistory()` nahrazuje celou historii; nové hodnoty z uzlu si aplikace skládá sama.

## Co se zobrazuje

| CO₂ [ppm] | Dlaždice | Popisek |
|---|---|---|
| pod 1000 | bílá | V POŘÁDKU |
| 1000–1499 | žlutá | VYVĚTRAT |
| 1500 a víc | červená | NAD LIMITEM |
| offline / chyba čidla | bílá | BEZ DAT |

Dále se zvýrazní:

- měření starší než 30 min (červený text, kromě červené dlaždice),
- baterie pod 20 % (žlutý text v záhlaví).

Šipka trendu: `→` do ±40 ppm za 15 min, `↗`/`↘` nad 40, `↑`/`↓` nad 150.

Prahy CO₂ jsou v [`dashboard_data.hpp`](include/dashboard_data.hpp), ostatní na začátku [`dashboard_render.cpp`](src/dashboard_render.cpp). Popisky svislé osy grafu (400, 1000, 1500) jsou zapsané zvlášť v `drawChartAxes()`.

## Napojení na protokol ESP-NOW

Příklad převodu zprávy `let_resp_t` z [`let_proto.h`](../../../common/let_proto.h). Které příznaky znamenají chybu čidla, rozhoduje aplikace; tohle je jen návrh.

```cpp
void applyResponse(dash::DashboardData &data, const let_resp_t &resp)
{
    // node_id uzlů je 1-3, index učebny 0-2
    const int index = resp.hdr.node_id - 1;
    if (index < 0 || index >= static_cast<int>(dash::kRoomCount)) return;
    dash::Room &room = data.rooms[index];

    const uint16_t fatal = LET_E_SENSOR_NO_RESP | LET_E_CRC |
                           LET_E_NOT_READY | LET_E_OUT_OF_RANGE;
    if (resp.err_flags & fatal) {
        room.setSensorError(resp.age_s);
        return;
    }

    room.setMeasurement({
        .co2Ppm       = resp.co2_ppm,
        .co2TrendPpm  = resp.co2_delta_15m,
        .temperatureC = resp.temp_c_x100 / 100.0f,
        .humidityPct  = resp.rh_x100 / 100.0f,
        .ageS         = resp.age_s,
    });
}
```

## Struktura

```
dashboard_ui/
├── include/
│   ├── dashboard_ui.hpp          třída Dashboard
│   └── dashboard_data.hpp        Room, Measurement, DashboardData, konstanty
└── src/
    ├── dashboard_ui.cpp          obsluha displeje
    ├── dashboard_data.cpp        metody Room
    ├── dashboard_render.cpp/.hpp rozvržení a kreslení (bez hardwaru)
    └── dashboard_demo.cpp/.hpp   testovací data
```
