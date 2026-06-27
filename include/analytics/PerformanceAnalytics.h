#pragma once

#include <string>
#include <vector>

#include "Fill.h"

namespace backtest::analytics {

struct EquityPoint {
    std::string datetime;
    double equity{0.0};
};

struct PerformanceMetrics {
    double total_return{0.0};
    double sharpe_ratio{0.0};
    double max_drawdown{0.0};
    double win_rate{0.0};
};

struct GapMetrics {
    double total_return_diff{0.0};
    double sharpe_reduction{0.0};
    double relative_decay{0.0};
};

class PerformanceAnalytics {
public:
    static PerformanceMetrics compute_metrics(const std::vector<Fill>& fills,
                                              const std::vector<EquityPoint>& equity_curve,
                                              double initial_cash);

    static GapMetrics compute_gap(const PerformanceMetrics& ideal,
                                  const PerformanceMetrics& realistic);

    static std::vector<EquityPoint> build_equity_curve(const std::vector<Fill>& fills,
                                                       double initial_cash);
};

}  // namespace backtest::analytics
