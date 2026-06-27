#include "OrderManager.h"

#include <iomanip>
#include <sstream>

namespace backtest {

OrderManager::OrderManager() = default;

bool OrderManager::validate(
    const Order& order, double cash,
    const std::unordered_map<std::string, Position>& positions) const {
    if (order.symbol.empty() || order.quantity <= 0.0) {
        return false;
    }

    if (order.side == OrderSide::BUY) {
        if (order.price > 0.0 && cash < order.quantity * order.price) {
            return false;
        }
        return true;
    }

    const auto it = positions.find(order.symbol);
    if (it == positions.end() || it->second.quantity < order.quantity) {
        return false;
    }

    return true;
}

bool OrderManager::route(const Order& order) {
    Order routed = order;

    if (routed.order_id.empty()) {
        routed.order_id = generate_id();
    }

    if (pending_orders_.count(routed.order_id) > 0) {
        return false;
    }

    routed.status = OrderStatus::PENDING;
    pending_orders_.emplace(routed.order_id, std::move(routed));
    return true;
}

bool OrderManager::cancel(const std::string& order_id) {
    const auto it = pending_orders_.find(order_id);
    if (it == pending_orders_.end()) {
        return false;
    }

    it->second.status = OrderStatus::CANCELLED;
    pending_orders_.erase(it);
    return true;
}

const std::unordered_map<std::string, Order>& OrderManager::pending_orders() const noexcept {
    return pending_orders_;
}

void OrderManager::clear() noexcept {
    pending_orders_.clear();
}

std::string OrderManager::generate_id() {
    std::ostringstream oss;
    oss << "ORD-" << std::setw(6) << std::setfill('0') << next_id_++;
    return oss.str();
}

}  // namespace backtest
