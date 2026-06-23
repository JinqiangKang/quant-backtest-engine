#include "RealisticMatchingEngine.h"

#include <sstream>
#include <utility>

namespace backtest::simulation {

namespace {

Fill make_invalid_fill() { return Fill{}; }

}  // namespace

RealisticMatchingEngine::RealisticMatchingEngine(std::unique_ptr<LatencyModel> latency,
                                                 std::unique_ptr<SlippageModel> slippage)
    : latency_model_(std::move(latency)), slippage_model_(std::move(slippage)) {}

RealisticMatchingEngine::RealisticMatchingEngine(std::shared_ptr<LatencyModel> latency,
                                                 std::shared_ptr<SlippageModel> slippage)
    : latency_model_(std::move(latency)), slippage_model_(std::move(slippage)) {}

Fill RealisticMatchingEngine::process(const Order& order, const Bar& current_bar) {
    if (order.status != OrderStatus::PENDING) {
        return make_invalid_fill();
    }

    const double base_price = current_bar.close;
    const double latency = latency_model_->compute(order);
    const double fill_price =
        slippage_model_->compute(order, base_price, current_bar.volume);

    std::ostringstream timestamp;
    timestamp << current_bar.datetime << '+' << latency << "ms";

    return Fill{"FILL-" + order.order_id,
                order.order_id,
                order.symbol,
                order.side,
                order.quantity,
                fill_price,
                timestamp.str()};
}

void RealisticMatchingEngine::set_latency_model(std::unique_ptr<LatencyModel> model) {
    latency_model_ = std::move(model);
}

void RealisticMatchingEngine::set_slippage_model(std::unique_ptr<SlippageModel> model) {
    slippage_model_ = std::move(model);
}

}  // namespace backtest::simulation
