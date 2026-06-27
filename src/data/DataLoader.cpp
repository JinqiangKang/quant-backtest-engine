#include "DataLoader.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::tm parse_date(const std::string& date_str) {
    std::tm tm{};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    return tm;
}

std::string get_week_start(const std::string& date_str) {
    std::tm tm = parse_date(date_str);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    std::mktime(&tm);

    const int days_since_monday = (tm.tm_wday + 6) % 7;
    tm.tm_mday -= days_since_monday;
    std::mktime(&tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string get_month_start(const std::string& date_str) {
    std::tm tm = parse_date(date_str);
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    std::mktime(&tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

}  // namespace

namespace backtest {

std::vector<Bar> DataLoader::load_csv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    std::vector<Bar> bars;
    std::string line;

    if (!std::getline(file, line)) {
        return bars;
    }

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string datetime;
        std::string open_str;
        std::string high_str;
        std::string low_str;
        std::string close_str;
        std::string volume_str;

        if (!std::getline(ss, datetime, ',') || !std::getline(ss, open_str, ',') ||
            !std::getline(ss, high_str, ',') || !std::getline(ss, low_str, ',') ||
            !std::getline(ss, close_str, ',') || !std::getline(ss, volume_str, ',')) {
            std::cerr << "Warning: skipping line with insufficient fields: " << line
                      << '\n';
            continue;
        }

        bars.emplace_back(datetime, std::stod(open_str), std::stod(high_str),
                          std::stod(low_str), std::stod(close_str),
                          std::stod(volume_str));
    }

    return bars;
}

bool DataLoader::validate_ohlcv(const std::vector<Bar>& bars) {
    if (bars.empty()) {
        return true;
    }

    for (std::size_t i = 0; i < bars.size(); ++i) {
        const Bar& bar = bars[i];

        if (bar.high < bar.low) {
            std::cerr << "Warning: high < low for bar " << bar.datetime << '\n';
            return false;
        }

        if (bar.high < bar.open || bar.high < bar.close) {
            std::cerr << "Warning: high must be >= open and close for bar "
                      << bar.datetime << '\n';
            return false;
        }

        if (bar.low > bar.open || bar.low > bar.close) {
            std::cerr << "Warning: low must be <= open and close for bar "
                      << bar.datetime << '\n';
            return false;
        }

        if (bar.volume < 0.0) {
            std::cerr << "Warning: negative volume for bar " << bar.datetime << '\n';
            return false;
        }

        if (i > 0 && bar.datetime <= bars[i - 1].datetime) {
            std::cerr << "Warning: dates must be strictly increasing at bar "
                      << bar.datetime << " (previous: " << bars[i - 1].datetime
                      << ")\n";
            return false;
        }
    }

    return true;
}

std::vector<Bar> DataLoader::resample(const std::vector<Bar>& bars,
                                      const std::string& freq) {
    if (freq == "D") {
        return bars;
    }

    using GroupKeyFn = std::string (*)(const std::string&);
    GroupKeyFn get_group_key = nullptr;
    if (freq == "W") {
        get_group_key = get_week_start;
    } else if (freq == "M") {
        get_group_key = get_month_start;
    } else {
        throw std::invalid_argument("Unsupported frequency: " + freq);
    }

    struct Aggregate {
        double open{};
        double high{};
        double low{};
        double close{};
        double volume{};
        bool has_data{false};
    };

    std::unordered_map<std::string, Aggregate> groups;

    for (const Bar& bar : bars) {
        const std::string key = get_group_key(bar.datetime);
        Aggregate& agg = groups[key];

        if (!agg.has_data) {
            agg.open = bar.open;
            agg.high = bar.high;
            agg.low = bar.low;
            agg.close = bar.close;
            agg.volume = bar.volume;
            agg.has_data = true;
        } else {
            agg.high = std::max(agg.high, bar.high);
            agg.low = std::min(agg.low, bar.low);
            agg.close = bar.close;
            agg.volume += bar.volume;
        }
    }

    std::vector<std::string> keys;
    keys.reserve(groups.size());
    for (const auto& entry : groups) {
        keys.push_back(entry.first);
    }
    std::sort(keys.begin(), keys.end());

    std::vector<Bar> result;
    result.reserve(keys.size());
    for (const std::string& key : keys) {
        const Aggregate& agg = groups.at(key);
        result.emplace_back(key, agg.open, agg.high, agg.low, agg.close, agg.volume);
    }

    return result;
}

}  // namespace backtest
