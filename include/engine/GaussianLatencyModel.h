#pragma once

#include <random>

#include "LatencyModel.h"

namespace backtest::simulation {

class GaussianLatencyModel : public LatencyModel {
public:
    GaussianLatencyModel(double mean_ms, double std_dev_ms);

    double compute(const Order& order) override;

private:
    std::mt19937 rng_;
    std::normal_distribution<double> dist_;
};

}  // namespace backtest::simulation
