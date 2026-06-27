#pragma once

#include "SlippageModel.h"

namespace backtest::simulation {

class FixedSlippageModel : public SlippageModel {
public:
    explicit FixedSlippageModel(double slippage_bps);

    double compute(const Order& order, double base_price,
                   double bar_volume = 0.0) override;

private:
    double slippage_bps_;
};

}  // namespace backtest::simulation
