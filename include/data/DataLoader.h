#pragma once

#include <string>
#include <vector>

#include "Bar.h"

namespace backtest {

/**
 * @brief Loads, validates, and resamples OHLCV bar data.
 */
class DataLoader {
public:
    /**
     * @brief Loads bar data from a CSV file.
     *
     * @param file_path Path to the CSV file containing OHLCV records.
     * @return A vector of Bar objects parsed from the file.
     */
    static std::vector<Bar> load_csv(const std::string& file_path);

    /**
     * @brief Validates OHLCV consistency for a series of bars.
     *
     * Checks that each bar has valid price relationships (e.g. high >= low)
     * and non-negative volume where applicable.
     *
     * @param bars Input bar series to validate.
     * @return true if all bars pass validation; false otherwise.
     */
    static bool validate_ohlcv(const std::vector<Bar>& bars);

    /**
     * @brief Resamples bar data to a target frequency.
     *
     * Aggregates input bars into larger time buckets according to @p freq
     * (e.g. "D", "W", "M").
     *
     * @param bars Input bar series in ascending time order.
     * @param freq Target resampling frequency identifier.
     * @return Resampled bar series at the requested frequency.
     */
    static std::vector<Bar> resample(const std::vector<Bar>& bars,
                                     const std::string& freq);
};

}  // namespace backtest
