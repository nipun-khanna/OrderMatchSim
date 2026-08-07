#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "matching_engine/OrderBook.h"
#include "order_match.pb.h"

namespace oms {

class MatchingEngine {
 public:
  std::vector<ordermatch::TradeExecuted> process(
      const ordermatch::OrderCreated& order);

 private:
  std::unordered_map<std::string, OrderBook> books_;
};

}
