#include "FixedSlippageModel.h"

namespace backtest::simulation {

FixedSlippageModel::FixedSlippageModel(double slippage_bps) : slippage_bps_(slippage_bps) {}

double FixedSlippageModel::compute(const Order& order, double base_price,
                                   double /*bar_volume*/) {
    const double side_sign = (order.side == OrderSide::BUY) ? 1.0 : -1.0;
    return base_price * (1.0 + side_sign * slippage_bps_ / 10000.0);
}

}  // namespace backtest::simulation
