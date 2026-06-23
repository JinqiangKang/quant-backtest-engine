#pragma once

#include "Bar.h"
#include "Fill.h"

namespace backtest {

class Strategy {
public:
    virtual ~Strategy() = default;

    virtual void init() {}
    virtual void on_bar(const Bar& bar) {}
    virtual void on_fill(const Fill& fill) {}
};

}  // namespace backtest
