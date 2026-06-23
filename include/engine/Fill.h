#pragma once

#include <string>

#include "Order.h"

namespace backtest {

struct Fill {
    std::string fill_id;
    std::string order_id;
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    double quantity{0.0};
    double price{0.0};
    std::string timestamp;

    Fill() = default;

    Fill(std::string fill_id, std::string order_id, std::string symbol, OrderSide side,
         double quantity, double price, std::string timestamp)
        : fill_id(std::move(fill_id))
        , order_id(std::move(order_id))
        , symbol(std::move(symbol))
        , side(side)
        , quantity(quantity)
        , price(price)
        , timestamp(std::move(timestamp)) {}
};

}  // namespace backtest
