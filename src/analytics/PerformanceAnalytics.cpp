#include "PerformanceAnalytics.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace backtest::analytics {

namespace {

constexpr double kTradingDaysPerYear = 252.0;

std::string normalize_datetime(const std::string& timestamp) {
    const std::size_t plus_pos = timestamp.find('+');
    if (plus_pos != std::string::npos) {
        return timestamp.substr(0, plus_pos);
    }
    return timestamp;
}

double compute_max_drawdown(const std::vector<EquityPoint>& equity_curve) {
    if (equity_curve.empty()) {
        return 0.0;
    }

    double peak = equity_curve.front().equity;
    double max_drawdown = 0.0;

    for (const EquityPoint& point : equity_curve) {
        peak = std::max(peak, point.equity);
        if (peak > 0.0) {
            const double drawdown = (peak - point.equity) / peak;
            max_drawdown = std::max(max_drawdown, drawdown);
        }
    }

    return max_drawdown;
}

double compute_sharpe_ratio(const std::vector<EquityPoint>& equity_curve) {
    if (equity_curve.size() < 2) {
        return 0.0;
    }

    std::vector<double> daily_returns;
    daily_returns.reserve(equity_curve.size() - 1);

    for (std::size_t i = 1; i < equity_curve.size(); ++i) {
        const double prev_equity = equity_curve[i - 1].equity;
        if (prev_equity <= 0.0) {
            continue;
        }
        daily_returns.push_back((equity_curve[i].equity - prev_equity) / prev_equity);
    }

    if (daily_returns.empty()) {
        return 0.0;
    }

    const double mean_return =
        std::accumulate(daily_returns.begin(), daily_returns.end(), 0.0) /
        static_cast<double>(daily_returns.size());

    double variance = 0.0;
    for (const double ret : daily_returns) {
        const double diff = ret - mean_return;
        variance += diff * diff;
    }
    variance /= static_cast<double>(daily_returns.size());
    const double std_dev = std::sqrt(variance);

    if (std_dev <= 0.0) {
        return 0.0;
    }

    const double annualized_return = mean_return * kTradingDaysPerYear;
    const double annualized_vol = std_dev * std::sqrt(kTradingDaysPerYear);
    return annualized_return / annualized_vol;
}

// Simplified win rate: count SELL fills where execution price exceeds
// the symbol's average cost at the time of sale.
double compute_win_rate(const std::vector<Fill>& fills) {
    struct SymbolState {
        double quantity{0.0};
        double average_price{0.0};
    };

    std::unordered_map<std::string, SymbolState> states;
    std::size_t closed_trades = 0;
    std::size_t winning_trades = 0;

    for (const Fill& fill : fills) {
        if (fill.quantity <= 0.0) {
            continue;
        }

        SymbolState& state = states[fill.symbol];

        if (fill.side == OrderSide::BUY) {
            const double trade_value = fill.quantity * fill.price;
            const double new_quantity = state.quantity + fill.quantity;
            state.average_price =
                (state.quantity * state.average_price + trade_value) / new_quantity;
            state.quantity = new_quantity;
            continue;
        }

        if (state.quantity <= 0.0) {
            continue;
        }

        ++closed_trades;
        if (fill.price > state.average_price) {
            ++winning_trades;
        }

        state.quantity = std::max(0.0, state.quantity - fill.quantity);
        if (state.quantity == 0.0) {
            state.average_price = 0.0;
        }
    }

    if (closed_trades == 0) {
        return 0.0;
    }

    return static_cast<double>(winning_trades) / static_cast<double>(closed_trades);
}

}  // namespace

PerformanceMetrics PerformanceAnalytics::compute_metrics(
    const std::vector<Fill>& fills, const std::vector<EquityPoint>& equity_curve,
    double initial_cash) {
    PerformanceMetrics metrics;

    if (equity_curve.empty()) {
        metrics.win_rate = compute_win_rate(fills);
        return metrics;
    }

    const double final_equity = equity_curve.back().equity;
    if (initial_cash > 0.0) {
        metrics.total_return = (final_equity - initial_cash) / initial_cash;
    }

    metrics.sharpe_ratio = compute_sharpe_ratio(equity_curve);
    metrics.max_drawdown = compute_max_drawdown(equity_curve);
    metrics.win_rate = compute_win_rate(fills);

    return metrics;
}

GapMetrics PerformanceAnalytics::compute_gap(const PerformanceMetrics& ideal,
                                             const PerformanceMetrics& realistic) {
    GapMetrics gap;
    gap.total_return_diff = ideal.total_return - realistic.total_return;
    gap.sharpe_reduction = ideal.sharpe_ratio - realistic.sharpe_ratio;

    if (std::abs(ideal.total_return) > 1e-12) {
        gap.relative_decay =
            (ideal.total_return - realistic.total_return) / ideal.total_return;
    }

    return gap;
}

std::vector<EquityPoint> PerformanceAnalytics::build_equity_curve(
    const std::vector<Fill>& fills, double initial_cash) {
    std::vector<Fill> sorted_fills = fills;
    std::sort(sorted_fills.begin(), sorted_fills.end(),
              [](const Fill& lhs, const Fill& rhs) {
                  return normalize_datetime(lhs.timestamp) <
                         normalize_datetime(rhs.timestamp);
              });

    double cash = initial_cash;
    std::unordered_map<std::string, double> positions;
    std::unordered_map<std::string, double> mark_prices;

    std::vector<EquityPoint> curve;
    curve.reserve(sorted_fills.size());

    for (const Fill& fill : sorted_fills) {
        if (fill.quantity <= 0.0) {
            continue;
        }

        const double trade_value = fill.quantity * fill.price;
        if (fill.side == OrderSide::BUY) {
            cash -= trade_value;
            positions[fill.symbol] += fill.quantity;
        } else {
            cash += trade_value;
            positions[fill.symbol] -= fill.quantity;
        }

        mark_prices[fill.symbol] = fill.price;

        double equity = cash;
        for (const auto& entry : positions) {
            const auto price_it = mark_prices.find(entry.first);
            if (price_it != mark_prices.end()) {
                equity += entry.second * price_it->second;
            }
        }

        curve.push_back(EquityPoint{normalize_datetime(fill.timestamp), equity});
    }

    if (curve.empty()) {
        curve.push_back(EquityPoint{"", initial_cash});
    }

    return curve;
}

}  // namespace backtest::analytics
