"""Python 自定义模拟模型示例。"""

from __future__ import annotations

import random

import _backtest_core as bc


class CustomExponentialLatencyModel(bc.LatencyModel):
    """
    指数分布延迟模型（毫秒）。

    scale 为指数分布的尺度参数（均值约为 scale）。
    """

    def __init__(self, scale: float = 5.0):
        super().__init__()
        self.scale = scale

    def compute(self, order) -> float:
        del order
        latency = random.expovariate(1.0 / self.scale)
        return max(0.5, latency)
