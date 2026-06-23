#include "PortfolioManager.h"

#include <iostream>

namespace backtest {

PortfolioManager::PortfolioManager(double initial_cash)
    : cash_(initial_cash), initial_cash_(initial_cash) {}

void PortfolioManager::update(const Fill& fill) {
    const double trade_value = fill.quantity * fill.price;

    if (fill.side == OrderSide::BUY) {
        cash_ -= trade_value;

        Position& position = positions_[fill.symbol];
        position.symbol = fill.symbol;

        const double new_quantity = position.quantity + fill.quantity;
        position.average_price =
            (position.quantity * position.average_price + trade_value) / new_quantity;
        position.quantity = new_quantity;
        return;
    }

    cash_ += trade_value;

    const auto it = positions_.find(fill.symbol);
    if (it == positions_.end()) {
        positions_[fill.symbol] = Position{fill.symbol, -fill.quantity, 0.0};
        std::cerr << "Warning: negative position for " << fill.symbol << '\n';
        return;
    }

    Position& position = it->second;
    position.quantity -= fill.quantity;

    if (position.quantity < 0.0) {
        std::cerr << "Warning: negative position for " << fill.symbol << '\n';
    } else if (position.quantity == 0.0) {
        position.average_price = 0.0;
        positions_.erase(it);
    }
}

double PortfolioManager::get_cash() const noexcept { return cash_; }

Position PortfolioManager::get_position(const std::string& symbol) const {
    const auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }
    return Position{symbol, 0.0, 0.0};
}

double PortfolioManager::get_nav(double current_price) const {
    double nav = cash_;
    for (const auto& entry : positions_) {
        nav += entry.second.market_value(current_price);
    }
    return nav;
}

const std::unordered_map<std::string, Position>& PortfolioManager::positions() const noexcept {
    return positions_;
}

void PortfolioManager::reset() noexcept {
    cash_ = initial_cash_;
    positions_.clear();
}

}  // namespace backtest
