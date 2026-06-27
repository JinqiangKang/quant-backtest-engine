#include "EventScheduler.h"

#include <stdexcept>

namespace backtest {

void EventScheduler::load_bars(const std::vector<Bar>& bars) {
    bars_ = bars;
    current_index_ = 0;
}

bool EventScheduler::has_next() const noexcept {
    return current_index_ < bars_.size();
}

Bar EventScheduler::next() {
    if (!has_next()) {
        throw std::out_of_range("EventScheduler::next: no more bars");
    }
    return bars_[current_index_++];
}

const Bar& EventScheduler::current() const {
    if (!has_next()) {
        throw std::out_of_range("EventScheduler::current: no current bar");
    }
    return bars_[current_index_];
}

void EventScheduler::reset() noexcept {
    current_index_ = 0;
}

}  // namespace backtest
