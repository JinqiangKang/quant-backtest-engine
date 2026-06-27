#include "Bar.h"
#include "DataLoader.h"

#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        std::string csv_path = "tests/sample_data.csv";
        if (argc > 1) {
            csv_path = argv[1];
        }

        std::cout << "CSV path: " << csv_path << "\n\n";

        const auto bars = backtest::DataLoader::load_csv(csv_path);
        std::cout << "Loaded " << bars.size() << " bars\n";

        const bool valid = backtest::DataLoader::validate_ohlcv(bars);
        std::cout << "Validation passed: " << (valid ? "true" : "false") << "\n";

        std::cout << "\n--- First 5 bars ---\n";
        for (std::size_t i = 0; i < bars.size() && i < 5; ++i) {
            std::cout << "  [" << i << "] " << bars[i] << "\n";
        }

        const auto weekly = backtest::DataLoader::resample(bars, "W");
        std::cout << "\n--- Weekly resample (W): " << weekly.size() << " bars ---\n";
        for (const auto& bar : weekly) {
            std::cout << "  " << bar << "\n";
        }

        const auto monthly = backtest::DataLoader::resample(bars, "M");
        std::cout << "\n--- Monthly resample (M): " << monthly.size() << " bars ---\n";
        for (const auto& bar : monthly) {
            std::cout << "  " << bar << "\n";
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }
}
