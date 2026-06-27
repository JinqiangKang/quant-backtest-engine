import os
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
os.chdir(PROJECT_ROOT)

import _backtest_core as bc

CSV_PATH = os.path.join(PROJECT_ROOT, "tests", "sample_data.csv")
RUN_LOG_PATH = os.path.join(PROJECT_ROOT, "run.log")


def test_import():
    assert hasattr(bc, "Bar")
    assert hasattr(bc, "CoreEngine")
    assert bc.OrderSide.BUY == bc.OrderSide.BUY


def test_engine_run():
    engine = bc.CoreEngine()
    assert engine.get_cash() == 100000.0
    engine.run("TEST", "2023-01-01", "2023-01-10", CSV_PATH)
    assert engine.get_cash() == 100000.0
    assert engine.get_position("TEST").quantity == 0.0

    with open(RUN_LOG_PATH, "r", encoding="utf-8") as f:
        content = f.read()
        assert "symbol=TEST" in content
        assert "Bar:" in content


def test_empty_strategy():
    class SimpleStrategy:
        def __init__(self, engine):
            self.engine = engine

        def on_bar(self, bar):
            print(f"Strategy sees {bar.datetime}")

        def on_fill(self, fill):
            pass

    engine = bc.CoreEngine()
    engine.set_strategy(SimpleStrategy(engine))
    engine.run("TEST", "2023-01-01", "2023-01-05", CSV_PATH)
    assert True


if __name__ == "__main__":
    test_import()
    test_engine_run()
    test_empty_strategy()
    print("All Python tests passed.")
