#pragma once

#include "SlippageModel.h"

namespace backtest::simulation {

class VolumeProportionalSlippageModel : public SlippageModel {
public:
    explicit VolumeProportionalSlippageModel(double slippage_per_pct_volume);

    double compute(const Order& order, double base_price,
                   double bar_volume = 0.0) override;

private:
    double slippage_per_pct_volume_;
};

}  // namespace backtest::simulation
