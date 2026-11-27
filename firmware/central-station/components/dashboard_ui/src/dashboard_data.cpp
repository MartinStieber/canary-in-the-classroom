#include "dashboard_data.hpp"

#include <algorithm>
#include <cstdio>

namespace dash {

void Room::setName(const char *name)
{
    std::snprintf(name_.data(), name_.size(), "%s", name ? name : "");
}

void Room::setMeasurement(const Measurement &measurement)
{
    measurement_ = measurement;
    status_ = Status::Ok;
}

void Room::setSensorError(std::uint32_t ageS)
{
    measurement_ = {};
    measurement_.ageS = ageS;
    status_ = Status::SensorError;
}

void Room::setOffline()
{
    measurement_ = {};
    status_ = Status::Offline;
}

void Room::setHistory(const std::uint16_t *co2Ppm, std::size_t count)
{
    history_.fill(0);
    if (!co2Ppm) return;

    /* delší pole: zajímá nás jen nejnovější konec; kratší: zarovnat doprava */
    const std::size_t used = std::min(count, history_.size());
    std::copy_n(co2Ppm + (count - used), used, history_.end() - used);
}

}  // namespace dash
