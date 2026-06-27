#include "FixedLatencyModel.h"

namespace backtest::simulation {

FixedLatencyModel::FixedLatencyModel(double latency_ms) : latency_ms_(latency_ms) {}

double FixedLatencyModel::compute(const Order& /*order*/) { return latency_ms_; }

}  // namespace backtest::simulation
