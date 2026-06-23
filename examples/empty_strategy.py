import os
import sys

# Allow importing _backtest_core from project python/ directory
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))

import _backtest_core


class EmptyStrategy:
    def __init__(self, engine):
        self.engine = engine

    def init(self):
        pass

    def on_bar(self, bar):
        print(bar.datetime, bar.close)

    def on_fill(self, fill):
        print(f"Fill: {fill.fill_id} {fill.symbol} {fill.quantity} @ {fill.price}")


if __name__ == "__main__":
    engine = _backtest_core.CoreEngine()
    strategy = EmptyStrategy(engine)
    strategy.init()
    engine.set_strategy(strategy)
    engine.run("TEST", "2023-01-01", "2023-01-10", "../tests/sample_data.csv")
    print(f"Final cash: {engine.get_cash()}")
