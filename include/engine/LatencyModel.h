#pragma once

#include <string>

#include "Order.h"

namespace backtest::simulation {

class LatencyModel {
public:
    virtual ~LatencyModel() = default;

    virtual double compute(const Order& order) = 0;
};

}  // namespace backtest::simulation
