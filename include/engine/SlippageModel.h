#pragma once

#include <string>

#include "Order.h"

namespace backtest::simulation {

class SlippageModel {
public:
    virtual ~SlippageModel() = default;

    virtual double compute(const Order& order, double base_price) = 0;
};

}  // namespace backtest::simulation
