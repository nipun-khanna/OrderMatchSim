#pragma once

#include <string>

#include "analytics/Analytics.h"

namespace oms {

std::string formatDashboard(const Analytics::Snapshot& snapshot,
                            double trades_per_sec);

}
