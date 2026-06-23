#pragma once

#include <string>

namespace backtest {

struct Position {
    std::string symbol;
    double quantity{0.0};
    double average_price{0.0};

    Position() = default;

    Position(std::string symbol, double quantity, double average_price)
        : symbol(std::move(symbol))
        , quantity(quantity)
        , average_price(average_price) {}

    [[nodiscard]] double market_value(double current_price) const noexcept {
        return current_price * quantity;
    }

    [[nodiscard]] double unrealized_pnl(double current_price) const noexcept {
        return (current_price - average_price) * quantity;
    }
};

}  // namespace backtest
