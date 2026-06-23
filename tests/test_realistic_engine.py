import os
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
os.chdir(PROJECT_ROOT)

import _backtest_core as bc

CSV_PATH = os.path.join(PROJECT_ROOT, "tests", "sample_data.csv")
INITIAL_CASH = 100000.0


def test_slippage_buy():
    lat = bc.FixedLatencyModel(0)
    slp = bc.FixedSlippageModel(100)  # 100 bps = 1%
    matcher = bc.RealisticMatchingEngine(lat, slp)
    engine = bc.CoreEngine(matcher)

    class TestStrategy:
        def __init__(self, engine):
            self.engine = engine

        def on_bar(self, bar):
            order = bc.Order()
            order.symbol = "TEST"
            order.side = bc.OrderSide.BUY
            order.quantity = 100
            order.price = bar.close
            order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(order)

        def on_fill(self, fill):
            print("Fill price:", fill.price)

    engine.set_strategy(TestStrategy(engine))
    engine.run("TEST", "2023-01-01", "2023-01-05", CSV_PATH)

    fills = engine.get_fills()
    assert len(fills) > 0, "Should have fills"

    fill = fills[0]
    assert fill.price > 0

    cash_after = engine.get_cash()
    assert cash_after < INITIAL_CASH


def test_no_matcher_ideal():
    engine = bc.CoreEngine()

    class TestStrategy:
        def __init__(self, engine):
            self.engine = engine

        def on_bar(self, bar):
            order = bc.Order()
            order.symbol = "TEST"
            order.side = bc.OrderSide.BUY
            order.quantity = 100
            order.price = bar.close
            order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(order)

    engine.set_strategy(TestStrategy(engine))
    engine.run("TEST", "2023-01-01", "2023-01-05", CSV_PATH)

    fills = engine.get_fills()
    assert len(fills) == 0, "Without matcher, no fills should occur"
    assert engine.get_cash() == INITIAL_CASH


if __name__ == "__main__":
    test_slippage_buy()
    test_no_matcher_ideal()
    print("All realistic engine tests passed.")
