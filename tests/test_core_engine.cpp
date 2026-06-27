#include "Bar.h"
#include "CoreEngine.h"
#include "DataLoader.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    try {
        backtest::CoreEngine engine;

        engine.run("TEST", "2023-01-01", "2023-01-10", "tests/sample_data.csv");

        constexpr double kInitialCash = 100000.0;
        if (std::abs(engine.get_cash() - kInitialCash) > 1e-9) {
            std::cerr << "Expected cash " << kInitialCash << ", got " << engine.get_cash()
                      << '\n';
            return -1;
        }

        if (engine.get_position("TEST").quantity != 0.0) {
            std::cerr << "Expected zero position for TEST, got quantity "
                      << engine.get_position("TEST").quantity << '\n';
            return -1;
        }

        std::ifstream log_file("run.log");
        if (!log_file.is_open()) {
            std::cerr << "Failed to open run.log\n";
            return -1;
        }

        std::stringstream log_content;
        log_content << log_file.rdbuf();
        const std::string log_text = log_content.str();

        if (log_text.find("symbol=TEST") == std::string::npos ||
            log_text.find("start_date=2023-01-01") == std::string::npos ||
            log_text.find("end_date=2023-01-10") == std::string::npos ||
            log_text.find("csv_path=tests/sample_data.csv") == std::string::npos) {
            std::cerr << "run.log missing expected run parameters\n";
            return -1;
        }

        if (log_text.find("[2023-01-01] Bar: 2023-01-01") == std::string::npos ||
            log_text.find("[2023-01-10] Bar: 2023-01-10") == std::string::npos) {
            std::cerr << "run.log missing expected bar date entries\n";
            return -1;
        }

        std::cout << "CoreEngine skeleton test passed.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return -1;
    }
}
