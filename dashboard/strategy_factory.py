"""仪表盘内置策略工厂与进度包装。"""

from __future__ import annotations

from typing import Any, Callable, Optional

from examples.strategies import (
    EmptyStrategy,
    MeanReversionStrategy,
    MovingAverageCrossoverStrategy,
)
from strategies import PeriodicTradeStrategy


class ProgressStrategyWrapper:
    """在 on_bar 中更新作业进度，并委托给内部策略。"""

    def __init__(self, inner, on_progress: Optional[Callable[[int], None]] = None):
        self.inner = inner
        self.on_progress = on_progress
        self.bar_count = 0

    def on_bar(self, bar) -> None:
        self.bar_count += 1
        if self.on_progress:
            self.on_progress(self.bar_count)
        self.inner.on_bar(bar)

    def on_fill(self, fill) -> None:
        if hasattr(self.inner, "on_fill"):
            self.inner.on_fill(fill)


def create_strategy(
    strategy_name: str,
    engine,
    symbol: str,
    params: dict[str, Any],
    on_progress: Optional[Callable[[int], None]] = None,
):
    name = strategy_name.lower()
    p = params or {}

    if name == "ma_cross":
        inner = MovingAverageCrossoverStrategy(
            engine,
            symbol=symbol,
            fast_window=int(p.get("fast_window", 5)),
            slow_window=int(p.get("slow_window", 20)),
        )
    elif name == "mean_reversion":
        inner = MeanReversionStrategy(
            engine,
            symbol=symbol,
            lookback=int(p.get("lookback", 20)),
            entry_z=float(p.get("entry_z", 1.5)),
            exit_z=float(p.get("exit_z", 0.5)),
        )
    elif name == "periodic":
        inner = PeriodicTradeStrategy(
            engine,
            symbol=symbol,
            on_progress=None,
        )
        return inner
    elif name == "empty":
        inner = EmptyStrategy(engine, symbol=symbol)
    else:
        raise ValueError(f"Unknown strategy: {strategy_name}")

    return ProgressStrategyWrapper(inner, on_progress=on_progress)
