"""仪表盘使用的延迟模型（模块级定义，便于后台线程中 pybind 派生）。"""

from __future__ import annotations

import random

import _backtest_core as bc


class CustomExponentialLatencyModel(bc.LatencyModel):
    """指数分布延迟模型（毫秒）。"""

    def __init__(self, scale: float = 5.0):
        super().__init__()
        self.scale = scale

    def compute(self, order) -> float:
        del order
        latency = random.expovariate(1.0 / self.scale)
        return max(0.5, latency)
