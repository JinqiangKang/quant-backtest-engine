#include "CoreEngine.h"

#include "DataLoader.h"
#include "EventScheduler.h"
#include "IdealMatchingEngine.h"
#include "OrderManager.h"
#include "PortfolioManager.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
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

std::string csv_quote(const std::string& value) { return "\"" + value + "\""; }

std::string side_to_string(OrderSide side) {
    return side == OrderSide::BUY ? "BUY" : "SELL";
}

}  // namespace

CoreEngine::CoreEngine(std::unique_ptr<simulation::MatchingEngine> matcher)
    : scheduler_(std::make_unique<EventScheduler>())
    , order_mgr_(std::make_unique<OrderManager>())
    , portfolio_(std::make_unique<PortfolioManager>())
    , matcher_(std::move(matcher))
    , initial_cash_(portfolio_->get_cash()) {}

CoreEngine::CoreEngine(std::shared_ptr<simulation::MatchingEngine> matcher)
    : scheduler_(std::make_unique<EventScheduler>())
    , order_mgr_(std::make_unique<OrderManager>())
    , portfolio_(std::make_unique<PortfolioManager>())
    , matcher_(std::move(matcher))
    , initial_cash_(portfolio_->get_cash()) {}

CoreEngine::~CoreEngine() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void CoreEngine::reset() {
    portfolio_->reset();
    order_mgr_->clear();
    fills_.clear();
    equity_curve_.clear();
    current_bar_ = Bar{};
    active_matcher_ = nullptr;
    scheduler_->reset();
}

void CoreEngine::run(const std::string& symbol, const std::string& start_date,
                     const std::string& end_date, const std::string& csv_path) {
    has_dual_results_ = false;
    run_internal(symbol, start_date, end_date, csv_path, matcher_.get());
    realistic_fills_ = fills_;
    realistic_equity_curve_ = equity_curve_;
    realistic_metrics_ = analytics::PerformanceAnalytics::compute_metrics(
        realistic_fills_, realistic_equity_curve_, initial_cash_);
}

void CoreEngine::run_dual(const std::string& symbol, const std::string& start_date,
                          const std::string& end_date, const std::string& csv_path) {
    has_dual_results_ = true;

    simulation::IdealMatchingEngine ideal_engine;
    run_internal(symbol, start_date, end_date, csv_path, &ideal_engine, "Ideal Backtest Run");
    ideal_fills_ = fills_;
    ideal_equity_curve_ = equity_curve_;
    ideal_metrics_ = analytics::PerformanceAnalytics::compute_metrics(
        ideal_fills_, ideal_equity_curve_, initial_cash_);

    run_internal(symbol, start_date, end_date, csv_path, matcher_.get(),
                 "Realistic Backtest Run");
    realistic_fills_ = fills_;
    realistic_equity_curve_ = equity_curve_;
    realistic_metrics_ = analytics::PerformanceAnalytics::compute_metrics(
        realistic_fills_, realistic_equity_curve_, initial_cash_);

    gap_metrics_ = analytics::PerformanceAnalytics::compute_gap(ideal_metrics_,
                                                                realistic_metrics_);
}

void CoreEngine::run_internal(const std::string& symbol, const std::string& start_date,
                              const std::string& end_date, const std::string& csv_path,
                              simulation::MatchingEngine* engine_override,
                              const std::string& run_label) {
    reset();

    run_params_.clear();
    run_params_["symbol"] = symbol;
    run_params_["start_date"] = start_date;
    run_params_["end_date"] = end_date;
    run_params_["csv_path"] = csv_path;
    run_params_["initial_cash"] = std::to_string(initial_cash_);

    if (log_file_.is_open()) {
        log_file_.close();
    }

    log_file_.open("run.log", std::ios::out | std::ios::trunc);
    log_to_file_ = log_file_.is_open();
    if (!log_to_file_) {
        std::cout << "[" << current_timestamp()
                  << "] Warning: failed to open run.log, logging to stdout\n";
    }

    log("=== " + run_label + " ===");
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
    active_matcher_ = engine_override;

    while (scheduler_->has_next()) {
        current_bar_ = scheduler_->next();
        log("[" + current_bar_.datetime + "] Bar: " + current_bar_.datetime);

        if (strategy_ && !strategy_.is_none()) {
            strategy_.attr("on_bar")(current_bar_);
        }

        equity_curve_.push_back(
            analytics::EquityPoint{current_bar_.datetime,
                                   portfolio_->get_nav(current_bar_.close)});
    }

    log("=== Run Complete ===");

    if (log_file_.is_open()) {
        log_file_.close();
    }

    active_matcher_ = nullptr;
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

    simulation::MatchingEngine* matcher = active_matcher_;
    if (matcher == nullptr) {
        matcher = matcher_.get();
    }

    if (matcher != nullptr && !current_bar_.datetime.empty()) {
        Order match_order = order;
        if (match_order.status != OrderStatus::PENDING) {
            match_order.status = OrderStatus::PENDING;
        }

        const Fill fill = matcher->process(match_order, current_bar_);
        if (fill.quantity > 0.0) {
            fills_.push_back(fill);
            portfolio_->update(fill);
            log("Filled: " + fill.fill_id + " @" + std::to_string(fill.price));
            return true;
        }

        log("[NO FILL] Order not filled: symbol=" + order.symbol);
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

void CoreEngine::export_results(const std::string& path) {
    std::filesystem::create_directories(path);

    const std::filesystem::path trades_path = std::filesystem::path(path) / "trades.csv";
    std::ofstream trades_file(trades_path);
    if (!trades_file.is_open()) {
        throw std::runtime_error("Failed to open trades export file: " +
                                 trades_path.string());
    }

    trades_file << "fill_id,order_id,symbol,side,quantity,price,timestamp\n";
    for (const Fill& fill : fills_) {
        trades_file << csv_quote(fill.fill_id) << ','
                    << csv_quote(fill.order_id) << ','
                    << csv_quote(fill.symbol) << ','
                    << csv_quote(side_to_string(fill.side)) << ','
                    << fill.quantity << ',' << fill.price << ','
                    << csv_quote(fill.timestamp) << '\n';
    }
    trades_file.close();

    const analytics::PerformanceMetrics& metrics =
        has_dual_results_ ? realistic_metrics_
                          : analytics::PerformanceAnalytics::compute_metrics(
                                fills_, equity_curve_, initial_cash_);

    const std::filesystem::path metrics_path = std::filesystem::path(path) / "metrics.csv";
    std::ofstream metrics_file(metrics_path);
    if (!metrics_file.is_open()) {
        throw std::runtime_error("Failed to open metrics export file: " +
                                 metrics_path.string());
    }

    metrics_file << "total_return,sharpe_ratio,max_drawdown,win_rate\n";
    metrics_file << metrics.total_return << ',' << metrics.sharpe_ratio << ','
                 << metrics.max_drawdown << ',' << metrics.win_rate << '\n';
    metrics_file.close();

    if (has_dual_results_) {
        const std::filesystem::path gap_path =
            std::filesystem::path(path) / "gap_metrics.csv";
        std::ofstream gap_file(gap_path);
        if (!gap_file.is_open()) {
            throw std::runtime_error("Failed to open gap metrics export file: " +
                                     gap_path.string());
        }

        gap_file << "total_return_diff,sharpe_reduction,relative_decay\n";
        gap_file << gap_metrics_.total_return_diff << ','
                 << gap_metrics_.sharpe_reduction << ','
                 << gap_metrics_.relative_decay << '\n';
        gap_file.close();
    }
}

void CoreEngine::set_matching_engine(std::unique_ptr<simulation::MatchingEngine> matcher) {
    matcher_ = std::move(matcher);
}

void CoreEngine::set_matching_engine(std::shared_ptr<simulation::MatchingEngine> matcher) {
    matcher_ = std::move(matcher);
}

void CoreEngine::set_strategy(pybind11::object strategy) {
    strategy_ = std::move(strategy);
}

double CoreEngine::get_cash() const { return portfolio_->get_cash(); }

Position CoreEngine::get_position(const std::string& symbol) const {
    return portfolio_->get_position(symbol);
}

const std::vector<Fill>& CoreEngine::get_fills() const noexcept { return fills_; }

const std::vector<analytics::EquityPoint>& CoreEngine::get_equity_curve() const noexcept {
    return equity_curve_;
}

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
