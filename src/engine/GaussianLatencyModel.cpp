#include "GaussianLatencyModel.h"

#include <algorithm>

namespace backtest::simulation {

GaussianLatencyModel::GaussianLatencyModel(double mean_ms, double std_dev_ms)
    : rng_(std::random_device{}()), dist_(mean_ms, std_dev_ms) {}

double GaussianLatencyModel::compute(const Order& /*order*/) {
    const double latency = dist_(rng_);
    return std::max(0.0, latency);
}

}  // namespace backtest::simulation
