#pragma once

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace backtest {

struct Bar {
    std::string datetime;
    double open;
    double high;
    double low;
    double close;
    double volume;

    Bar() noexcept = default;

    Bar(std::string datetime, double open, double high, double low, double close,
        double volume) noexcept
        : datetime(std::move(datetime))
        , open(open)
        , high(high)
        , low(low)
        , close(close)
        , volume(volume) {}

    static std::tm from_string(const std::string& date_str) {
        std::tm tm{};
        std::istringstream ss(date_str);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        return tm;
    }

    [[nodiscard]] std::tm to_tm() const { return from_string(datetime); }
};

inline std::ostream& operator<<(std::ostream& os, const Bar& bar) {
    return os << bar.datetime << ',' << bar.open << ',' << bar.high << ',' << bar.low
              << ',' << bar.close << ',' << bar.volume;
}

}  // namespace backtest
