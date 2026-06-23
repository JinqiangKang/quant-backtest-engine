#pragma once

#include <string>
#include <unordered_map>

#include "Fill.h"
#include "Position.h"

namespace backtest {

class PortfolioManager {
public:
    explicit PortfolioManager(double initial_cash = 100000.0);

    void update(const Fill& fill);
    [[nodiscard]] double get_cash() const noexcept;
    [[nodiscard]] Position get_position(const std::string& symbol) const;
    [[nodiscard]] double get_nav(double current_price) const;
    [[nodiscard]] const std::unordered_map<std::string, Position>& positions() const noexcept;
    void reset() noexcept;

private:
    double cash_;
    double initial_cash_;
    std::unordered_map<std::string, Position> positions_;
};

}  // namespace backtest
