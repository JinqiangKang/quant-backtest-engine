#include "Bar.h"
#include "CoreEngine.h"
#include "Fill.h"
#include "Order.h"
#include "Position.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

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
        .def_readonly("order_id", &backtest::Order::order_id)
        .def_readonly("symbol", &backtest::Order::symbol)
        .def_readonly("side", &backtest::Order::side)
        .def_readonly("quantity", &backtest::Order::quantity)
        .def_readonly("price", &backtest::Order::price)
        .def_readonly("status", &backtest::Order::status)
        .def_readonly("timestamp", &backtest::Order::timestamp);

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

    py::class_<backtest::CoreEngine>(m, "CoreEngine")
        .def(py::init<>())
        .def("run", &backtest::CoreEngine::run)
        .def("submit_order", &backtest::CoreEngine::submit_order)
        .def("set_strategy", &backtest::CoreEngine::set_strategy)
        .def("get_cash", &backtest::CoreEngine::get_cash)
        .def("get_position", &backtest::CoreEngine::get_position)
        .def("get_fills", &backtest::CoreEngine::get_fills);
}
