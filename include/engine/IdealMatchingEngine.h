#pragma once

#include "MatchingEngine.h"

namespace backtest::simulation {

class IdealMatchingEngine : public MatchingEngine {
public:
    Fill process(const Order& order, const Bar& bar) override;
};

}  // namespace backtest::simulation
