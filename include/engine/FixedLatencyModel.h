#pragma once

#include "LatencyModel.h"

namespace backtest::simulation {

class FixedLatencyModel : public LatencyModel {
public:
    explicit FixedLatencyModel(double latency_ms);

    double compute(const Order& order) override;

private:
    double latency_ms_;
};

}  // namespace backtest::simulation
