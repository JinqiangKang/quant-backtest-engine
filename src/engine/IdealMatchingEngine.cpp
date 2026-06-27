#include "IdealMatchingEngine.h"

namespace backtest::simulation {

namespace {

Fill make_invalid_fill() { return Fill{}; }

}  // namespace

Fill IdealMatchingEngine::process(const Order& order, const Bar& bar) {
    if (order.status != OrderStatus::PENDING) {
        return make_invalid_fill();
    }

    return Fill{"IDEAL-" + order.order_id,
                order.order_id,
                order.symbol,
                order.side,
                order.quantity,
                bar.close,
                bar.datetime};
}

}  // namespace backtest::simulation
