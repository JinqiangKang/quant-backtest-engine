#include "DataLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

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

bool DataLoader::validate_ohlcv(const std::vector<Bar>& /*bars*/) { return false; }

std::vector<Bar> DataLoader::resample(const std::vector<Bar>& /*bars*/,
                                      const std::string& /*freq*/) {
    return {};
}

}  // namespace backtest
