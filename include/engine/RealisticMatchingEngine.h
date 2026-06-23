#pragma once

#include <memory>

#include "LatencyModel.h"
#include "MatchingEngine.h"
#include "SlippageModel.h"

namespace backtest::simulation {

class RealisticMatchingEngine : public MatchingEngine {
public:
    RealisticMatchingEngine(std::unique_ptr<LatencyModel> latency,
                            std::unique_ptr<SlippageModel> slippage);

    Fill process(const Order& order, const Bar& current_bar) override;

    void set_latency_model(std::unique_ptr<LatencyModel> model);
    void set_slippage_model(std::unique_ptr<SlippageModel> model);

private:
    std::unique_ptr<LatencyModel> latency_model_;
    std::unique_ptr<SlippageModel> slippage_model_;
};

}  // namespace backtest::simulation
