#include "CoreEngine.h"

#include "DataLoader.h"
#include "EventScheduler.h"
#include "OrderManager.h"
#include "PortfolioManager.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace backtest {

namespace {

std::string current_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_local{};
#if defined(_WIN32)
    localtime_s(&tm_local, &time);
#else
    localtime_r(&time, &tm_local);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_local, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

CoreEngine::CoreEngine(std::unique_ptr<MatchingEngine> matcher)
    : scheduler_(std::make_unique<EventScheduler>())
    , order_mgr_(std::make_unique<OrderManager>())
    , portfolio_(std::make_unique<PortfolioManager>())
    , matcher_(std::move(matcher)) {}

CoreEngine::~CoreEngine() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void CoreEngine::run(const std::string& symbol, const std::string& start_date,
                     const std::string& end_date, const std::string& csv_path) {
    run_params_.clear();
    run_params_["symbol"] = symbol;
    run_params_["start_date"] = start_date;
    run_params_["end_date"] = end_date;
    run_params_["csv_path"] = csv_path;
    run_params_["initial_cash"] = std::to_string(portfolio_->get_cash());

    if (log_file_.is_open()) {
        log_file_.close();
    }

    log_file_.open("run.log", std::ios::out | std::ios::trunc);
    log_to_file_ = log_file_.is_open();
    if (!log_to_file_) {
        std::cout << "[" << current_timestamp()
                  << "] Warning: failed to open run.log, logging to stdout\n";
    }

    log("=== Backtest Run ===");
    log("timestamp=" + current_timestamp());
    log("symbol=" + symbol);
    log("start_date=" + start_date);
    log("end_date=" + end_date);
    log("csv_path=" + csv_path);
    log("initial_cash=" + run_params_["initial_cash"]);
    log("====================");

    const auto all_bars = DataLoader::load_csv(csv_path);

    std::vector<Bar> filtered_bars;
    filtered_bars.reserve(all_bars.size());
    for (const Bar& bar : all_bars) {
        if (bar.datetime >= start_date && bar.datetime <= end_date) {
            filtered_bars.push_back(bar);
        }
    }

    log("Loaded " + std::to_string(all_bars.size()) + " bars, filtered to " +
        std::to_string(filtered_bars.size()) + " bars");

    scheduler_->load_bars(filtered_bars);
    order_mgr_->clear();
    fills_.clear();

    while (scheduler_->has_next()) {
        const Bar current_bar = scheduler_->next();
        log("[" + current_bar.datetime + "] Bar: " + current_bar.datetime);

        // TODO: call strategy on_bar

        // No matching engine yet; pending orders are not processed here.
    }

    log("=== Run Complete ===");

    if (log_file_.is_open()) {
        log_file_.close();
    }
}

bool CoreEngine::submit_order(const Order& order) {
    if (order.symbol.empty() || order.quantity <= 0.0) {
        log("[REJECT] Invalid symbol or quantity");
        return false;
    }

    if (!order_mgr_->validate(order, portfolio_->get_cash(),
                              portfolio_->positions())) {
        if (order.side == OrderSide::BUY) {
            log("[REJECT] Insufficient cash for BUY order: symbol=" + order.symbol);
        } else {
            log("[REJECT] Insufficient position for SELL order: symbol=" +
                order.symbol);
        }
        return false;
    }

    if (!order_mgr_->route(order)) {
        log("[REJECT] Failed to route order (duplicate order_id): symbol=" +
            order.symbol);
        return false;
    }

    log("[ORDER] Order routed: symbol=" + order.symbol + ", side=" +
        (order.side == OrderSide::BUY ? "BUY" : "SELL") +
        ", quantity=" + std::to_string(order.quantity));
    return true;
}

double CoreEngine::get_cash() const { return portfolio_->get_cash(); }

Position CoreEngine::get_position(const std::string& symbol) const {
    return portfolio_->get_position(symbol);
}

const std::vector<Fill>& CoreEngine::get_fills() const noexcept { return fills_; }

void CoreEngine::log(const std::string& msg) {
    const std::string line = "[" + current_timestamp() + "] " + msg;

    if (log_to_file_ && log_file_.is_open()) {
        log_file_ << line << '\n';
        log_file_.flush();
    } else {
        std::cout << line << '\n';
    }
}

}  // namespace backtest
