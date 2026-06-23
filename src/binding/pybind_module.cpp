#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_backtest_core, m) {
    m.doc() = "Quantitative backtesting core engine (C++ bindings)";
}
