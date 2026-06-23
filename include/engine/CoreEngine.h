#pragma once

#include <pybind11/pybind11.h>

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Fill.h"
#include "MatchingEngine.h"
#include "Order.h"
#include "Position.h"

namespace backtest {

class EventScheduler;
class OrderManager;
class PortfolioManager;

class CoreEngine {
public:
    explicit CoreEngine(std::unique_ptr<simulation::MatchingEngine> matcher = nullptr);
    ~CoreEngine();

    CoreEngine(const CoreEngine&) = delete;
    CoreEngine& operator=(const CoreEngine&) = delete;
    CoreEngine(CoreEngine&&) = delete;
    CoreEngine& operator=(CoreEngine&&) = delete;

    void run(const std::string& symbol, const std::string& start_date,
             const std::string& end_date, const std::string& csv_path);

    bool submit_order(const Order& order);
    void set_strategy(pybind11::object strategy);

    [[nodiscard]] double get_cash() const;
    [[nodiscard]] Position get_position(const std::string& symbol) const;
    [[nodiscard]] const std::vector<Fill>& get_fills() const noexcept;

private:
    std::unique_ptr<EventScheduler> scheduler_;
    std::unique_ptr<OrderManager> order_mgr_;
    std::unique_ptr<PortfolioManager> portfolio_;
    std::unique_ptr<simulation::MatchingEngine> matcher_;
    std::vector<Fill> fills_;
    std::unordered_map<std::string, std::string> run_params_;
    std::ofstream log_file_;
    bool log_to_file_{true};
    pybind11::object strategy_{};

    void log(const std::string& msg);
};

}  // namespace backtest
