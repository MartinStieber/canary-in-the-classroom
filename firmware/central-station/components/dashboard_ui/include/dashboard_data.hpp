/**
 * @file dashboard_data.hpp
 * @brief Hodnoty zobrazované na dashboardu.
 *
 * Bez závislostí na ESP-IDF, aby šlo data i vykreslování přeložit na PC.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace dash {

/** Počet učeben (sloupců na displeji). */
inline constexpr std::size_t kRoomCount = 3;

/** Velikost bufferu pro název učebny v bajtech (UTF-8, včetně koncové nuly). */
inline constexpr std::size_t kRoomNameSize = 24;

/** Počet vzorků v grafu: 24 h po 10 minutách. */
inline constexpr std::size_t kHistoryLength = 144;

/** Od této koncentrace CO2 [ppm] je dlaždice žlutá ("VYVĚTRAT"). */
inline constexpr int kCo2WarnPpm = 1000;

/** Od této koncentrace CO2 [ppm] je dlaždice červená ("NAD LIMITEM"). */
inline constexpr int kCo2LimitPpm = 1500;

/** Aktuální měření jedné učebny. */
struct Measurement {
    std::uint16_t co2Ppm = 0;       /**< Koncentrace CO2 [ppm] */
    std::int16_t co2TrendPpm = 0;   /**< Změna CO2 za posledních 15 min [ppm] */
    float temperatureC = 0.0f;      /**< Teplota [°C] */
    float humidityPct = 0.0f;       /**< Relativní vlhkost [%] */
    std::uint32_t ageS = 0;         /**< Stáří měření [s] */
};

/** Stav baterie panelu. */
struct Battery {
    float voltageV = 0.0f;
    int percent = 0;
};

/** Počítadla výměn zpráv s čidlovými uzly. */
struct ExchangeStats {
    std::uint32_t ok = 0;
    std::uint32_t failed = 0;
};

/** Jedna učebna: název, stav, aktuální měření a historie CO2. */
class Room {
public:
    enum class Status : std::uint8_t {
        Offline,        /**< Uzel neodpovídá ("žádná data") */
        SensorError,    /**< Uzel odpovídá, ale nemá platné měření ("chyba čidla") */
        Ok,             /**< Platné měření */
    };

    /** CO2 [ppm], nejstarší hodnota první, 0 = bez dat. */
    using History = std::array<std::uint16_t, kHistoryLength>;

    /**
     * @brief Nastaví název. Řetězec se kopíruje, delší než kRoomNameSize - 1
     *        bajtů se ořízne. nullptr název smaže.
     */
    void setName(const char *name);

    /** @brief Nastaví aktuální měření a přepne učebnu do stavu Ok. */
    void setMeasurement(const Measurement &measurement);

    /** @brief Přepne do stavu SensorError. @p ageS je stáří posledního platného měření. */
    void setSensorError(std::uint32_t ageS);

    /** @brief Přepne do stavu Offline (výchozí stav). */
    void setOffline();

    /**
     * @brief Nahradí celou historii pro graf.
     *
     * Hodnoty jsou 10min průměry seřazené od nejstarší po nejnovější. Kratší
     * pole se zarovná doprava ("teď"), z delšího se použije jen konec.
     * Historie se nemění s novým měřením ani se změnou stavu.
     *
     * @param co2Ppm  pole hodnot, kopíruje se; nullptr historii smaže
     * @param count   počet hodnot; 0 historii smaže
     */
    void setHistory(const std::uint16_t *co2Ppm, std::size_t count);

    const char *name() const { return name_.data(); }
    Status status() const { return status_; }
    /** Platí celé jen ve stavu Ok; ve stavu SensorError jen ageS. */
    const Measurement &measurement() const { return measurement_; }
    const History &history() const { return history_; }

private:
    std::array<char, kRoomNameSize> name_{};
    Status status_ = Status::Offline;
    Measurement measurement_{};
    History history_{};
};

/** Všechno, co dashboard zobrazuje. */
struct DashboardData {
    std::array<Room, kRoomCount> rooms;     /**< Sloupce zleva doprava */
    Battery battery;
    ExchangeStats exchanges;
};

}  // namespace dash
