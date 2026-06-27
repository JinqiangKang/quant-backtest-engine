import csv
import os
import shutil
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
os.chdir(PROJECT_ROOT)

import _backtest_core as bc

CSV_PATH = os.path.join(PROJECT_ROOT, "tests", "sample_data.csv")
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "test_output")
INITIAL_CASH = 100000.0


def test_dual_execution_and_export():
    ideal = bc.IdealMatchingEngine()
    realistic = bc.RealisticMatchingEngine(
        bc.FixedLatencyModel(10), bc.FixedSlippageModel(50)
    )
    assert ideal is not None
    assert realistic is not None

    engine = bc.CoreEngine(realistic)

    class DualTestStrategy:
        def __init__(self, eng):
            self.engine = eng

        def on_bar(self, bar):
            order = bc.Order()
            order.symbol = "TEST"
            order.side = bc.OrderSide.BUY
            order.quantity = 100
            order.price = bar.close
            order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(order)

        def on_fill(self, fill):
            pass

    engine.set_strategy(DualTestStrategy(engine))
    engine.run_dual("TEST", "2023-01-01", "2023-01-15", CSV_PATH)

    ideal_fills = engine.get_ideal_fills()
    realistic_fills = engine.get_realistic_fills()
    assert len(ideal_fills) > 0
    assert len(realistic_fills) == len(ideal_fills)

    for ideal_fill, realistic_fill in zip(ideal_fills, realistic_fills):
        assert ideal_fill.quantity == realistic_fill.quantity
        assert ideal_fill.price <= realistic_fill.price

    ideal_curve = engine.get_ideal_equity_curve()
    realistic_curve = engine.get_realistic_equity_curve()
    assert len(ideal_curve) == 15
    assert len(realistic_curve) == 15

    ideal_final = ideal_curve[-1].equity
    realistic_final = realistic_curve[-1].equity
    assert realistic_final < ideal_final

    ideal_return = (ideal_final - INITIAL_CASH) / INITIAL_CASH
    realistic_return = (realistic_final - INITIAL_CASH) / INITIAL_CASH
    assert realistic_return < ideal_return

    assert engine.get_equity_curve()[-1].equity == realistic_final

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
    engine.export_results(OUTPUT_DIR)

    trades_path = os.path.join(OUTPUT_DIR, "trades.csv")
    metrics_path = os.path.join(OUTPUT_DIR, "metrics.csv")
    gap_path = os.path.join(OUTPUT_DIR, "gap_metrics.csv")

    assert os.path.isfile(trades_path)
    assert os.path.isfile(metrics_path)
    assert os.path.isfile(gap_path)

    with open(trades_path, newline="", encoding="utf-8") as f:
        trades_rows = list(csv.reader(f))
    assert len(trades_rows) == len(realistic_fills) + 1
    assert trades_rows[0] == [
        "fill_id",
        "order_id",
        "symbol",
        "side",
        "quantity",
        "price",
        "timestamp",
    ]

    with open(metrics_path, newline="", encoding="utf-8") as f:
        metrics_rows = list(csv.reader(f))
    assert metrics_rows[0] == [
        "total_return",
        "sharpe_ratio",
        "max_drawdown",
        "win_rate",
    ]
    metrics_values = [float(x) for x in metrics_rows[1]]
    assert len(metrics_values) == 4
    assert abs(metrics_values[0] - realistic_return) < 1e-6

    with open(gap_path, newline="", encoding="utf-8") as f:
        gap_rows = list(csv.reader(f))
    assert gap_rows[0] == [
        "total_return_diff",
        "sharpe_reduction",
        "relative_decay",
    ]
    gap_values = [float(x) for x in gap_rows[1]]
    assert gap_values[0] > 0
    assert abs(gap_values[0] - (ideal_return - realistic_return)) < 1e-6

    shutil.rmtree(OUTPUT_DIR)


if __name__ == "__main__":
    test_dual_execution_and_export()
    print("Dual execution and export test passed.")
