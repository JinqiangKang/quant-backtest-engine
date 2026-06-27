#include "VolumeProportionalSlippageModel.h"

namespace backtest::simulation {

VolumeProportionalSlippageModel::VolumeProportionalSlippageModel(
    double slippage_per_pct_volume)
    : slippage_per_pct_volume_(slippage_per_pct_volume) {}

double VolumeProportionalSlippageModel::compute(const Order& order, double base_price,
                                                double bar_volume) {
    const double volume_ratio =
        (bar_volume > 0.0) ? (order.quantity / bar_volume) : 0.0;
    const double slippage = volume_ratio * slippage_per_pct_volume_ * 100.0;
    const double side_sign = (order.side == OrderSide::BUY) ? 1.0 : -1.0;
    return base_price * (1.0 + side_sign * slippage / 10000.0);
}

}  // namespace backtest::simulation
