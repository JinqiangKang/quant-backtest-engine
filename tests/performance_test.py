"""
五年日线数据回测性能测试。

生成约 1250 根 Bar 的 CSV，使用理想撮合引擎与示例策略运行回测，
验证耗时在 10 秒以内。
"""

import os
import sys
import time
import warnings

import numpy as np
import pandas as pd

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
os.chdir(PROJECT_ROOT)

import _backtest_core as bc

CSV_PATH = os.path.join(PROJECT_ROOT, "tests", "five_year_data.csv")
SYMBOL = "PERF"
START_DATE = "2019-01-01"
END_DATE = "2023-12-31"
TIME_LIMIT_SEC = 10.0


def generate_five_year_csv(path: str) -> int:
    """生成五年交易日 OHLCV 数据，返回行数。"""
    dates = pd.bdate_range(start=START_DATE, end=END_DATE)
    n = len(dates)

    rng = np.random.default_rng(42)
    close = 100.0 + np.cumsum(rng.normal(0, 0.8, n))
    close = np.maximum(close, 10.0)

    open_ = close + rng.normal(0, 0.3, n)
    high = np.maximum(open_, close) + rng.uniform(0.1, 1.5, n)
    low = np.minimum(open_, close) - rng.uniform(0.1, 1.5, n)
    low = np.maximum(low, 1.0)
    volume = rng.integers(5_000, 50_000, n)

    df = pd.DataFrame(
        {
            "Date": dates.strftime("%Y-%m-%d"),
            "Open": np.round(open_, 2),
            "High": np.round(high, 2),
            "Low": np.round(low, 2),
            "Close": np.round(close, 2),
            "Volume": volume,
        }
    )
    df.to_csv(path, index=False)
    return n


class PeriodicTradeStrategy:
    """每 5 根 Bar 买入 100 股，每 10 根 Bar 卖出全部持仓。"""

    def __init__(self, engine, symbol: str):
        self.engine = engine
        self.symbol = symbol
        self.counter = 0

    def on_bar(self, bar) -> None:
        self.counter += 1

        if self.counter % 10 == 0:
            position = self.engine.get_position(self.symbol)
            if position.quantity > 0:
                order = bc.Order()
                order.symbol = self.symbol
                order.side = bc.OrderSide.SELL
                order.quantity = position.quantity
                order.price = bar.close
                order.status = bc.OrderStatus.PENDING
                self.engine.submit_order(order)

        if self.counter % 5 == 0:
            order = bc.Order()
            order.symbol = self.symbol
            order.side = bc.OrderSide.BUY
            order.quantity = 100
            order.price = bar.close
            order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(order)

    def on_fill(self, fill) -> None:
        pass


def run_performance_test() -> float:
    if not os.path.exists(CSV_PATH):
        bar_count = generate_five_year_csv(CSV_PATH)
        print(f"Generated {bar_count} bars -> {CSV_PATH}")
    else:
        df = pd.read_csv(CSV_PATH)
        bar_count = len(df)
        print(f"Using existing CSV with {bar_count} bars: {CSV_PATH}")

    matcher = bc.IdealMatchingEngine()
    engine = bc.CoreEngine(matcher)
    engine.set_strategy(PeriodicTradeStrategy(engine, SYMBOL))

    t0 = time.perf_counter()
    engine.run(SYMBOL, START_DATE, END_DATE, CSV_PATH)
    elapsed = time.perf_counter() - t0

    fills = engine.get_fills()
    curve = engine.get_equity_curve()
    print(f"Bars processed: {len(curve)}")
    print(f"Fills: {len(fills)}")
    print(f"Final NAV: {curve[-1].equity:.2f}")
    print(f"Elapsed: {elapsed:.3f}s")

    return elapsed


if __name__ == "__main__":
    elapsed = run_performance_test()

    if elapsed >= TIME_LIMIT_SEC:
        warnings.warn(
            f"Performance test exceeded {TIME_LIMIT_SEC}s limit ({elapsed:.3f}s). "
            "Consider optimizing C++ engine or reducing Python callback overhead.",
            UserWarning,
            stacklevel=1,
        )

    assert elapsed < TIME_LIMIT_SEC, (
        f"Backtest took {elapsed:.3f}s, expected < {TIME_LIMIT_SEC}s"
    )
    print(f"Performance test passed (< {TIME_LIMIT_SEC}s).")
