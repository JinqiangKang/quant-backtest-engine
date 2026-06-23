#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Order.h"
#include "Position.h"

namespace backtest {

class OrderManager {
public:
    OrderManager();

    [[nodiscard]] bool validate(
        const Order& order, double cash,
        const std::unordered_map<std::string, Position>& positions) const;

    bool route(const Order& order);
    bool cancel(const std::string& order_id);
    [[nodiscard]] const std::unordered_map<std::string, Order>& pending_orders() const noexcept;
    void clear() noexcept;

private:
    std::unordered_map<std::string, Order> pending_orders_;
    std::int64_t next_id_{1};

    std::string generate_id();
};

}  // namespace backtest
