#pragma once

#include <cstddef>
#include <vector>

#include "Bar.h"

namespace backtest {

class EventScheduler {
public:
    void load_bars(const std::vector<Bar>& bars);
    [[nodiscard]] bool has_next() const noexcept;
    Bar next();
    [[nodiscard]] const Bar& current() const;
    void reset() noexcept;

private:
    std::vector<Bar> bars_;
    std::size_t current_index_{0};
};

}  // namespace backtest
