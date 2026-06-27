#pragma once

#include <string>

#include "Bar.h"
#include "Fill.h"
#include "Order.h"

namespace backtest::simulation {

class MatchingEngine {
public:
    virtual ~MatchingEngine() = default;

    virtual Fill process(const Order& order, const Bar& current_bar) = 0;
};

}  // namespace backtest::simulation
