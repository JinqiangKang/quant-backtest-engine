"""验证 Python 自定义延迟模型可接入 RealisticMatchingEngine。"""

import os
import re
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
sys.path.insert(0, PROJECT_ROOT)
os.chdir(PROJECT_ROOT)

import _backtest_core as bc
from examples.custom_models import CustomExponentialLatencyModel
from examples.strategies import MovingAverageCrossoverStrategy

# 使用周期策略以产生多笔成交，便于验证延迟分布
sys.path.insert(0, os.path.join(PROJECT_ROOT, "dashboard"))
from strategies import PeriodicTradeStrategy  # noqa: E402

CSV_PATH = os.path.join(PROJECT_ROOT, "tests", "sample_data.csv")
RUN_LOG_PATH = os.path.join(PROJECT_ROOT, "run.log")


def main() -> None:
    latency = CustomExponentialLatencyModel(scale=5.0)
    slippage = bc.FixedSlippageModel(50)
    matcher = bc.RealisticMatchingEngine(latency, slippage)
    engine = bc.CoreEngine(matcher)
    engine.set_strategy(PeriodicTradeStrategy(engine, symbol="TEST"))

    engine.run("TEST", "2023-01-01", "2023-01-31", CSV_PATH)

    fills = engine.get_fills()
    assert fills, "应有成交记录"

    latencies = []
    for fill in fills:
        match = re.search(r"\+([0-9.]+)ms$", fill.timestamp)
        assert match, f"成交时间戳应包含延迟: {fill.timestamp}"
        latencies.append(float(match.group(1)))

    unique_latencies = set(round(v, 4) for v in latencies)
    assert len(unique_latencies) > 1, "指数延迟应产生不同延迟值"

    with open(RUN_LOG_PATH, encoding="utf-8") as f:
        log = f.read()
    assert "Filled:" in log, "日志应记录成交"

    print("Custom exponential latency model verification passed.")
    print(f"Sample latencies (ms): {sorted(latencies)[:5]} ...")
    print(f"Unique latency count: {len(unique_latencies)}")


if __name__ == "__main__":
    main()
