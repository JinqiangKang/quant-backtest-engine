#pragma once

#include <string>

namespace backtest {

enum class OrderSide { BUY, SELL };

enum class OrderStatus { PENDING, FILLED, CANCELLED, REJECTED };

struct Order {
    std::string order_id;
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    double quantity{0.0};
    double price{0.0};
    OrderStatus status{OrderStatus::PENDING};
    std::string timestamp;

    Order() = default;

    Order(std::string order_id, std::string symbol, OrderSide side, double quantity,
          double price, OrderStatus status, std::string timestamp)
        : order_id(std::move(order_id))
        , symbol(std::move(symbol))
        , side(side)
        , quantity(quantity)
        , price(price)
        , status(status)
        , timestamp(std::move(timestamp)) {}
};

}  // namespace backtest
