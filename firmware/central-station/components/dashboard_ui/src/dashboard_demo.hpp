/**
 * @file dashboard_demo.hpp
 * @brief Interní rozhraní: vymyšlené hodnoty pro Dashboard::test().
 */
#pragma once

#include "dashboard_data.hpp"

namespace dash {

/** @brief Naplní data ukázkovými hodnotami z běžného školního dne. */
void fillDemoData(DashboardData &data);

/** @brief Upraví ukázková data tak, aby byly vidět chybové a krajní stavy. */
void applyErrorStates(DashboardData &data);

}  // namespace dash
