/**
 * @file dashboard_demo.cpp
 * @brief Testovací data pro ladění bez čidlových uzlů.
 */
#include "dashboard_demo.hpp"

#include <cmath>

namespace dash {
namespace {

constexpr int kMinutesPerDay = 24 * 60;
constexpr int kSampleMinutes = 10;
constexpr int kDemoNow = 12 * 60 + 10;      /* "teď" je 12:10, uprostřed hodiny */
constexpr int kSchoolStart = 7 * 60 + 30;
constexpr int kSchoolEnd = 18 * 60;
constexpr int kLessonMinutes = 100;
constexpr int kBreakMinutes = 10;

/** Simulované CO2: během vyučování stoupá, o přestávce klesá. */
std::uint16_t demoCo2(int room, int sample)
{
    static constexpr float peaks[kRoomCount] = { 1350.0f, 1050.0f, 2000.0f };

    const int minutesAgo = (static_cast<int>(kHistoryLength) - 1 - sample) * kSampleMinutes;
    const int minuteOfDay = (kDemoNow + kMinutesPerDay - minutesAgo) % kMinutesPerDay;
    const float base = 430.0f + 15.0f * sinf(static_cast<float>(sample) * 0.3f +
                                              static_cast<float>(room));

    const bool school = minuteOfDay >= kSchoolStart && minuteOfDay < kSchoolEnd;
    if (!school) return static_cast<std::uint16_t>(base);

    const int inBlock = (minuteOfDay - kSchoolStart) % (kLessonMinutes + kBreakMinutes);
    if (inBlock >= kLessonMinutes) return static_cast<std::uint16_t>(base + 250.0f);

    return static_cast<std::uint16_t>(base + (peaks[room] - base) *
                                                 (1.0f - expf(-inBlock / 45.0f)));
}

}  // namespace

void fillDemoData(DashboardData &data)
{
    static constexpr const char *names[kRoomCount] = { "SE3.143", "SE3.144", "SE3.145" };
    static constexpr float temperatures[kRoomCount] = { 22.4f, 21.8f, 24.1f };
    static constexpr float humidities[kRoomCount] = { 41.0f, 47.0f, 52.0f };

    for (std::size_t i = 0; i < kRoomCount; i++) {
        Room::History history;
        for (std::size_t s = 0; s < history.size(); s++) {
            history[s] = demoCo2(static_cast<int>(i), static_cast<int>(s));
        }

        const std::uint16_t now = history[history.size() - 1];
        const std::uint16_t previous = history[history.size() - 2];

        Room &room = data.rooms[i];
        room.setName(names[i]);
        room.setMeasurement({
            .co2Ppm = now,
            .co2TrendPpm = static_cast<std::int16_t>((now - previous) * 3 / 2),  /* 10 -> 15 min */
            .temperatureC = temperatures[i],
            .humidityPct = humidities[i],
            .ageS = static_cast<std::uint32_t>(120 + 60 * i),
        });

        /* simulovaný noční výpadek prostředního uzlu (2 h) */
        if (i == 1) {
            for (std::size_t s = 20; s < 32; s++) history[s] = 0;
        }
        room.setHistory(history.data(), history.size());
    }

    data.battery = { .voltageV = 3.92f, .percent = 78 };
    data.exchanges = { .ok = 1284, .failed = 7 };
}

void applyErrorStates(DashboardData &data)
{
    data.rooms[0].setOffline();
    data.rooms[1].setSensorError(4000);
    data.rooms[2].setMeasurement({
        .co2Ppm = 1100,
        .co2TrendPpm = -200,
        .temperatureC = 24.1f,
        .humidityPct = 52.0f,
        .ageS = 2000,
    });
    data.battery.percent = 10;
}

}  // namespace dash
