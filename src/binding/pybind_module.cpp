#include "Bar.h"
#include "CoreEngine.h"
#include "Fill.h"
#include "FixedLatencyModel.h"
#include "FixedSlippageModel.h"
#include "GaussianLatencyModel.h"
#include "LatencyModel.h"
#include "MatchingEngine.h"
#include "Order.h"
#include "Position.h"
#include "IdealMatchingEngine.h"
#include "PerformanceAnalytics.h"
#include "RealisticMatchingEngine.h"
#include "SlippageModel.h"
#include "VolumeProportionalSlippageModel.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace sim = backtest::simulation;
namespace analytics = backtest::analytics;

PYBIND11_MODULE(_backtest_core, m) {
    m.doc() = "Quantitative backtesting core engine (C++ bindings)";

    py::enum_<backtest::OrderSide>(m, "OrderSide")
        .value("BUY", backtest::OrderSide::BUY)
        .value("SELL", backtest::OrderSide::SELL)
        .export_values();

    py::enum_<backtest::OrderStatus>(m, "OrderStatus")
        .value("PENDING", backtest::OrderStatus::PENDING)
        .value("FILLED", backtest::OrderStatus::FILLED)
        .value("CANCELLED", backtest::OrderStatus::CANCELLED)
        .value("REJECTED", backtest::OrderStatus::REJECTED)
        .export_values();

    py::class_<backtest::Bar>(m, "Bar")
        .def_readonly("datetime", &backtest::Bar::datetime)
        .def_readonly("open", &backtest::Bar::open)
        .def_readonly("high", &backtest::Bar::high)
        .def_readonly("low", &backtest::Bar::low)
        .def_readonly("close", &backtest::Bar::close)
        .def_readonly("volume", &backtest::Bar::volume);

    py::class_<backtest::Order>(m, "Order")
        .def(py::init<>())
        .def_readwrite("order_id", &backtest::Order::order_id)
        .def_readwrite("symbol", &backtest::Order::symbol)
        .def_readwrite("side", &backtest::Order::side)
        .def_readwrite("quantity", &backtest::Order::quantity)
        .def_readwrite("price", &backtest::Order::price)
        .def_readwrite("status", &backtest::Order::status)
        .def_readwrite("timestamp", &backtest::Order::timestamp);

    py::class_<backtest::Fill>(m, "Fill")
        .def_readonly("fill_id", &backtest::Fill::fill_id)
        .def_readonly("order_id", &backtest::Fill::order_id)
        .def_readonly("symbol", &backtest::Fill::symbol)
        .def_readonly("side", &backtest::Fill::side)
        .def_readonly("quantity", &backtest::Fill::quantity)
        .def_readonly("price", &backtest::Fill::price)
        .def_readonly("timestamp", &backtest::Fill::timestamp);

    py::class_<backtest::Position>(m, "Position")
        .def_readonly("symbol", &backtest::Position::symbol)
        .def_readonly("quantity", &backtest::Position::quantity)
        .def_readonly("average_price", &backtest::Position::average_price)
        .def("market_value", &backtest::Position::market_value)
        .def("unrealized_pnl", &backtest::Position::unrealized_pnl);

    py::class_<sim::LatencyModel, std::shared_ptr<sim::LatencyModel>>(m, "LatencyModel");

    py::class_<sim::FixedLatencyModel, sim::LatencyModel,
               std::shared_ptr<sim::FixedLatencyModel>>(m, "FixedLatencyModel")
        .def(py::init<double>());

    py::class_<sim::GaussianLatencyModel, sim::LatencyModel,
               std::shared_ptr<sim::GaussianLatencyModel>>(m, "GaussianLatencyModel")
        .def(py::init<double, double>());

    py::class_<sim::SlippageModel, std::shared_ptr<sim::SlippageModel>>(m, "SlippageModel");

    py::class_<sim::FixedSlippageModel, sim::SlippageModel,
               std::shared_ptr<sim::FixedSlippageModel>>(m, "FixedSlippageModel")
        .def(py::init<double>());

    py::class_<sim::VolumeProportionalSlippageModel, sim::SlippageModel,
               std::shared_ptr<sim::VolumeProportionalSlippageModel>>(
        m, "VolumeProportionalSlippageModel")
        .def(py::init<double>());

    py::class_<sim::MatchingEngine, std::shared_ptr<sim::MatchingEngine>>(m,
                                                                         "MatchingEngine");

    py::class_<sim::RealisticMatchingEngine, sim::MatchingEngine,
               std::shared_ptr<sim::RealisticMatchingEngine>>(m,
                                                              "RealisticMatchingEngine")
        .def(py::init<std::shared_ptr<sim::LatencyModel>,
                      std::shared_ptr<sim::SlippageModel>>());

    py::class_<sim::IdealMatchingEngine, sim::MatchingEngine,
               std::shared_ptr<sim::IdealMatchingEngine>>(m, "IdealMatchingEngine")
        .def(py::init<>())
        .def("process", &sim::IdealMatchingEngine::process);

    py::class_<backtest::CoreEngine>(m, "CoreEngine")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<sim::MatchingEngine>>())
        .def("run", &backtest::CoreEngine::run)
        .def("submit_order", &backtest::CoreEngine::submit_order)
        .def("set_strategy", &backtest::CoreEngine::set_strategy)
        .def("set_matching_engine",
             static_cast<void (backtest::CoreEngine::*)(
                 std::shared_ptr<sim::MatchingEngine>)>(
                 &backtest::CoreEngine::set_matching_engine))
        .def("get_cash", &backtest::CoreEngine::get_cash)
        .def("get_position", &backtest::CoreEngine::get_position)
        .def("get_fills", &backtest::CoreEngine::get_fills);

    py::class_<analytics::EquityPoint>(m, "EquityPoint")
        .def_readonly("datetime", &analytics::EquityPoint::datetime)
        .def_readonly("equity", &analytics::EquityPoint::equity);

    py::class_<analytics::PerformanceMetrics>(m, "PerformanceMetrics")
        .def_readonly("total_return", &analytics::PerformanceMetrics::total_return)
        .def_readonly("sharpe_ratio", &analytics::PerformanceMetrics::sharpe_ratio)
        .def_readonly("max_drawdown", &analytics::PerformanceMetrics::max_drawdown)
        .def_readonly("win_rate", &analytics::PerformanceMetrics::win_rate);

    py::class_<analytics::GapMetrics>(m, "GapMetrics")
        .def_readonly("total_return_diff", &analytics::GapMetrics::total_return_diff)
        .def_readonly("sharpe_reduction", &analytics::GapMetrics::sharpe_reduction)
        .def_readonly("relative_decay", &analytics::GapMetrics::relative_decay);

    py::class_<analytics::PerformanceAnalytics>(m, "PerformanceAnalytics")
        .def_static("compute_metrics", &analytics::PerformanceAnalytics::compute_metrics)
        .def_static("compute_gap", &analytics::PerformanceAnalytics::compute_gap)
        .def_static("build_equity_curve",
                     &analytics::PerformanceAnalytics::build_equity_curve);
}
