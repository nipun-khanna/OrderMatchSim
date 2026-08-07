#include "analytics/Dashboard.h"

#include <cstdint>

#include <fmt/format.h>

namespace oms {

std::string formatDashboard(const Analytics::Snapshot& s,
                            double trades_per_sec) {
  const std::string symbol = s.top_symbol.empty() ? "-" : s.top_symbol;

  std::string out = "======== MARKET ========\n\n";
  out += fmt::format("{:<20}{}\n", "Orders processed", s.orders_processed);
  out += fmt::format("{:<20}{}\n", "Trades", s.trades_executed);
  out += fmt::format("{:<20}{}\n", "Trades/sec",
                     static_cast<int64_t>(trades_per_sec + 0.5));
  out += fmt::format("{:<20}{}\n", "Top Symbol", symbol);
  out += fmt::format("{:<20}{:.2f}\n", fmt::format("VWAP ({})", symbol),
                     s.top_symbol_vwap_ticks / 100.0);
  out += fmt::format("{:<20}{}\n", "Avg order size",
                     static_cast<int64_t>(s.avg_order_size + 0.5));
  out += fmt::format("{:<20}{:.1f} ms\n", "Avg latency",
                     s.avg_latency_us / 1000.0);
  return out;
}

}
