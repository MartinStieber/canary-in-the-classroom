/*
 * canary-in-the-classroom: sdílený protokol ESP-NOW mezi panelem (central-station)
 * a čidlovými uzly (sensor-node).
 *
 * !! Návrh. Domluvte se na něm dřív, než začnete psát kód. !!
 * Změny dělejte přes pull request, ať o nich druhá strana ví.
 *
 * Princip (podle zadání): iniciátorem je vždy PANEL, uzly jen odpovídají.
 * Spící panel nic nepřijme, takže uzly samy nic neposílají.
 *
 *   panel  --MSG_REQUEST_CURRENT-->  uzel
 *   panel  <--MSG_RESPONSE_CURRENT-- uzel
 *
 *   panel  --MSG_REQUEST_HISTORY-->  uzel   (po výpadku / pro graf za 24 h)
 *   panel  <--MSG_RESPONSE_HISTORY-- uzel
 *
 * Limit ESP-NOW: payload max 250 B.
 */
#pragma once

#include <stdint.h>

#define PROTOCOL_VERSION 1

/* Aby nás nerušil druhý tým (stejný HW): vlastní magic v každé zprávě
 * a vlastní WiFi kanál. Hodnoty si domluvte a změňte na unikátní. */
#define PROTOCOL_MAGIC   0xCA  /* "canary" */
#define ESPNOW_CHANNEL   1

typedef enum {
    MSG_REQUEST_CURRENT  = 0x01,  /* panel -> uzel */
    MSG_RESPONSE_CURRENT = 0x02,  /* uzel -> panel */
    MSG_REQUEST_HISTORY  = 0x03,  /* panel -> uzel */
    MSG_RESPONSE_HISTORY = 0x04,  /* uzel -> panel */
} msg_type_t;

/* Bity ve status poli odpovědi. */
#define STATUS_SENSOR_ERROR  (1u << 0)  /* SCD41 neodpovídá / chybná data */
#define STATUS_WARMING_UP    (1u << 1)  /* ještě nejsou platná data po startu */
#define STATUS_TREND_VALID   (1u << 2)  /* je k dispozici 15 min historie pro trend */

/* Společná hlavička každé zprávy. */
typedef struct __attribute__((packed)) {
    uint8_t  magic;      /* PROTOCOL_MAGIC */
    uint8_t  version;    /* PROTOCOL_VERSION */
    uint8_t  type;       /* msg_type_t */
    uint8_t  node_id;    /* 0..2 = učebna (pořadí v zadání) */
    uint16_t seq;        /* číslo požadavku; odpověď ho opakuje */
} msg_header_t;

/* Aktuální data. Jednotky jsou pevně dané, aby se nepletly. */
typedef struct __attribute__((packed)) {
    msg_header_t h;
    uint16_t co2_ppm;        /* CO2 v ppm */
    int16_t  temp_cdeg;      /* teplota v setinách °C (2150 = 21.50 °C) */
    uint16_t humidity_cpct;  /* vlhkost v setinách % (4500 = 45.00 %) */
    int16_t  trend_ppm_15m;  /* změna CO2 za posledních 15 min v ppm */
    uint16_t age_s;          /* stáří měření v sekundách (v okamžiku odpovědi) */
    uint8_t  status;         /* STATUS_* */
    uint8_t  history_count;  /* kolik 10min vzorků má uzel k dispozici (0..144) */
} response_current_t;

/* Požadavek na historii: 10minutové průměry CO2, index 0 = nejstarší. */
typedef struct __attribute__((packed)) {
    msg_header_t h;
    uint8_t  start;          /* od kterého vzorku */
    uint8_t  count;          /* kolik vzorků (max HISTORY_CHUNK_MAX) */
} request_history_t;

#define HISTORY_SAMPLES_24H  144   /* 24 h / 10 min */
#define HISTORY_CHUNK_MAX    100   /* 2 B * 100 + hlavička < 250 B */

typedef struct __attribute__((packed)) {
    msg_header_t h;
    uint8_t  start;
    uint8_t  count;
    uint16_t co2_ppm[HISTORY_CHUNK_MAX];  /* 0xFFFF = chybějící vzorek */
} response_history_t;

/* Kategorie podle zadání. */
#define CO2_OK_MAX_PPM    1000  /* do 1000: v pořádku */
#define CO2_WARN_MAX_PPM  1500  /* 1000-1500: vyvětrat, nad 1500: nad limitem */
