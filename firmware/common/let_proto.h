/*
 * canary-in-the-classroom: protokol ESP-NOW mezi panelem (central-station)
 * a čidlovými uzly (sensor-node), verze 1.
 *
 * Přepis hlavičkového souboru ze specifikace
 * "Specifikace zpráv ESP-NOW panel ↔ uzel.pdf" (v téže složce).
 * Význam polí, pravidla pro uzel i příklady zpráv jsou v PDF.
 *
 * Tento soubor používají oba firmwary. Měňte ho jen po domluvě a přes pull
 * request. Při změně formátu zvyšte LET_VERSION a upravte i PDF.
 *
 * Shrnutí:
 *   - Iniciátorem je vždy panel. Uzel odpovídá jen na platný REQ.
 *   - panel --REQ-->  uzel
 *     panel <--RESP-- uzel
 *     panel <--HIST-- uzel  (0 až 2 zprávy, podle RESP.hist_chunks)
 *   - Všechny vícebajtové hodnoty jsou little-endian.
 *   - Přijatá data kopírujte do lokální struktury přes memcpy, nepřetypovávejte
 *     ukazatel (příjmový buffer nemusí být zarovnaný).
 *   - Délku zprávy HIST počítejte makrem LET_HIST_LEN(n), ne přes sizeof.
 */
#pragma once
#include <stdint.h>

#define LET_MAGIC      0x4C54u  // 'LT', v bajtech 54 4C
#define LET_VERSION    1
#define LET_TEAM_ID    1
#define LET_NODE_PANEL 0        // node_id panelu; uzly mají 1-3 (učebna)

enum { LET_MSG_REQ = 0x01, LET_MSG_RESP = 0x02, LET_MSG_HIST = 0x03 };

// REQ flags
#define LET_F_WANT_HIST   (1u << 0)  // poslat historii
#define LET_F_FULL_RESYNC (1u << 1)  // poslat celou historii

// RESP err_flags
#define LET_E_SENSOR_NO_RESP (1u << 0)  // SCD41 neodpovídá na I2C
#define LET_E_CRC            (1u << 1)  // data ze SCD41 mají špatné CRC
#define LET_E_NOT_READY      (1u << 2)  // od startu zatím žádné platné měření
#define LET_E_OUT_OF_RANGE   (1u << 3)  // CO2 < 300 nebo > 5000 ppm
#define LET_E_STALE          (1u << 4)  // age_s > 60
#define LET_I_HIST_PARTIAL   (1u << 5)  // historie má méně než 144 slotů (informativní)

#define LET_HIST_SLOTS   144  // 24 h po 10 min
#define LET_HIST_PER_MSG 100

// Společná hlavička každé zprávy (8 B).
typedef struct __attribute__((packed)) {
    uint16_t magic;    // LET_MAGIC, jinou hodnotu zahodit
    uint8_t  version;  // LET_VERSION
    uint8_t  type;     // LET_MSG_*
    uint8_t  team_id;  // LET_TEAM_ID
    uint8_t  node_id;  // v REQ cílový uzel, v RESP a HIST odesílající uzel
    uint16_t req_id;   // panel zvyšuje s každým REQ, uzel kopíruje do RESP i HIST
} let_hdr_t;

// REQ: panel -> uzel (18 B).
typedef struct __attribute__((packed)) {
    let_hdr_t hdr;
    uint8_t  flags;          // LET_F_*
    uint8_t  reserved;       // 0
    uint32_t known_boot_id;  // boot_id uzlu, který panel zná; 0 = nezná
    uint32_t have_seq;       // poslední slot historie, který už panel má; 0 = žádný
} let_req_t;

// RESP: uzel -> panel (38 B).
typedef struct __attribute__((packed)) {
    let_hdr_t hdr;
    uint32_t boot_id;        // náhodné číslo ze startu uzlu (esp_random()), nikdy 0
    uint32_t uptime_s;       // sekundy od startu uzlu
    uint32_t unix_time;      // skutečný čas, pokud ho uzel zná; jinak 0
    uint16_t co2_ppm;        // poslední platné CO2 [ppm]
    int16_t  temp_c_x100;    // teplota x 100 [°C], 23,45 °C -> 2345
    uint16_t rh_x100;        // vlhkost x 100 [%], 45,67 % -> 4567
    int16_t  co2_delta_15m;  // CO2 nyní minus CO2 před 15 min [ppm]
    uint16_t age_s;          // sekundy od posledního platného měření
    uint16_t err_flags;      // LET_E_* / LET_I_*, 0 = v pořádku
    uint32_t hist_last_seq;  // seq nejnovějšího uzavřeného 10min slotu; 0 = zatím žádný
    uint8_t  hist_chunks;    // počet zpráv HIST, které následují (0-2)
    uint8_t  reserved;       // 0
} let_resp_t;

// HIST: uzel -> panel (20 + 2*n B, max. 220 B).
typedef struct __attribute__((packed)) {
    let_hdr_t hdr;
    uint32_t boot_id;      // stejné jako v RESP
    uint32_t first_seq;    // seq první hodnoty v této zprávě
    uint8_t  count;        // počet hodnot n, 1-100
    uint8_t  chunk_idx;    // pořadí této zprávy, od 0
    uint8_t  chunk_total;  // celkový počet zpráv HIST (= hist_chunks z RESP)
    uint8_t  reserved;     // 0
    uint16_t co2_avg[LET_HIST_PER_MSG];  // 10min průměry [ppm], nejstarší první; 0 = slot bez platného měření
} let_hist_t;

#define LET_HIST_HDR_LEN 20
#define LET_HIST_LEN(n)  (LET_HIST_HDR_LEN + 2 * (n))

#ifdef __cplusplus
#define LET_SASSERT(c, m) static_assert(c, m)
#else
#define LET_SASSERT(c, m) _Static_assert(c, m)
#endif
LET_SASSERT(sizeof(let_hdr_t) == 8, "let_hdr_t size");
LET_SASSERT(sizeof(let_req_t) == 18, "let_req_t size");
LET_SASSERT(sizeof(let_resp_t) == 38, "let_resp_t size");
LET_SASSERT(sizeof(let_hist_t) == 220, "let_hist_t size");
