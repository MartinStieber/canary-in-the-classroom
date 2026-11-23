/*
 * canary-in-the-classroom: sdílený protokol mezi sensor-node a central-station.
 *
 * !! Návrh. Domluvte se na něm dřív, než začnete psát kód. !!
 * Změny dělejte přes pull request, ať o nich druhá strana ví.
 */
#pragma once

#include <stdint.h>

#define PROTOCOL_VERSION 1

typedef enum {
    MSG_MEASUREMENT = 0x01,  /* node -> centrála: naměřená data */
    MSG_HEARTBEAT   = 0x02,  /* node -> centrála: jsem naživu */
    MSG_CONFIG      = 0x10,  /* centrála -> node: nastavení (interval apod.) */
} msg_type_t;

/* Naměřená data. Jednotky jsou pevně dané, aby se nepletly. */
typedef struct __attribute__((packed)) {
    uint8_t  version;       /* PROTOCOL_VERSION */
    uint8_t  type;          /* msg_type_t, zde MSG_MEASUREMENT */
    uint8_t  node_id;       /* ID jednotky (např. číslo učebny) */
    uint16_t seq;           /* pořadové číslo zprávy */
    uint16_t co2_ppm;       /* CO2 v ppm */
    int16_t  temp_cdeg;     /* teplota v setinách °C (2150 = 21.50 °C) */
    uint16_t humidity_cpct; /* vlhkost v setinách % (4500 = 45.00 %) */
    uint16_t battery_mv;    /* napětí baterie v mV (0 = napájeno z kabelu) */
} measurement_msg_t;

/* Prahové hodnoty CO2 pro signalizaci (orientační, upravte podle zadání). */
#define CO2_OK_MAX_PPM    1000
#define CO2_WARN_MAX_PPM  1500  /* nad tuto hodnotu: "otevři okno" */
