"""示例交易策略，供回测引擎与仪表盘使用。"""

from __future__ import annotations

import math
from typing import Optional

import _backtest_core as bc


class EmptyStrategy(bc.Strategy):
    """不交易，仅观察行情。"""

    def __init__(self, engine, symbol: str = "TEST"):
        super().__init__()
        self.engine = engine
        self.symbol = symbol
        self.init()

    def init(self) -> None:
        pass

    def on_bar(self, bar) -> None:
        pass

    def on_fill(self, fill) -> None:
        pass


class MovingAverageCrossoverStrategy(bc.Strategy):
    """双均线交叉策略：快线上穿慢线买入，下穿卖出。"""

    def __init__(
        self,
        engine,
        symbol: str = "TEST",
        fast_window: int = 5,
        slow_window: int = 20,
    ):
        super().__init__()
        self.engine = engine
        self.symbol = symbol
        self.fast_window = fast_window
        self.slow_window = slow_window
        self.prices: list[float] = []
        self.prev_fast: Optional[float] = None
        self.prev_slow: Optional[float] = None
        self.init()

    def init(self) -> None:
        self.prices.clear()
        self.prev_fast = None
        self.prev_slow = None

    def _moving_average(self, window: int) -> float:
        return sum(self.prices[-window:]) / window

    def on_bar(self, bar) -> None:
        self.prices.append(bar.close)
        if len(self.prices) < self.slow_window:
            return

        fast_ma = self._moving_average(self.fast_window)
        slow_ma = self._moving_average(self.slow_window)

        if self.prev_fast is not None and self.prev_slow is not None:
            if self.prev_fast <= self.prev_slow and fast_ma > slow_ma:
                self._submit_buy(bar.close)
            elif self.prev_fast >= self.prev_slow and fast_ma < slow_ma:
                self._submit_sell(bar.close)

        self.prev_fast = fast_ma
        self.prev_slow = slow_ma

    def _submit_buy(self, price: float) -> None:
        order = bc.Order()
        order.symbol = self.symbol
        order.side = bc.OrderSide.BUY
        order.quantity = 100
        order.price = price
        order.status = bc.OrderStatus.PENDING
        self.engine.submit_order(order)

    def _submit_sell(self, price: float) -> None:
        position = self.engine.get_position(self.symbol)
        if position.quantity <= 0:
            return
        order = bc.Order()
        order.symbol = self.symbol
        order.side = bc.OrderSide.SELL
        order.quantity = position.quantity
        order.price = price
        order.status = bc.OrderStatus.PENDING
        self.engine.submit_order(order)

    def on_fill(self, fill) -> None:
        pass


class MeanReversionStrategy(bc.Strategy):
    """均值回归策略：价格显著低于均值时买入，回归后卖出。"""

    def __init__(
        self,
        engine,
        symbol: str = "TEST",
        lookback: int = 20,
        entry_z: float = 1.5,
        exit_z: float = 0.5,
    ):
        super().__init__()
        self.engine = engine
        self.symbol = symbol
        self.lookback = lookback
        self.entry_z = entry_z
        self.exit_z = exit_z
        self.prices: list[float] = []
        self.init()

    def init(self) -> None:
        self.prices.clear()

    def on_bar(self, bar) -> None:
        self.prices.append(bar.close)
        if len(self.prices) < self.lookback:
            return

        window = self.prices[-self.lookback :]
        mean = sum(window) / self.lookback
        variance = sum((p - mean) ** 2 for p in window) / self.lookback
        std = math.sqrt(variance)
        if std < 1e-12:
            return

        z_score = (bar.close - mean) / std
        position = self.engine.get_position(self.symbol)

        if z_score < -self.entry_z and position.quantity <= 0:
            self._submit_buy(bar.close)
        elif z_score > self.exit_z and position.quantity > 0:
            self._submit_sell(bar.close, position.quantity)

    def _submit_buy(self, price: float) -> None:
        order = bc.Order()
        order.symbol = self.symbol
        order.side = bc.OrderSide.BUY
        order.quantity = 100
        order.price = price
        order.status = bc.OrderStatus.PENDING
        self.engine.submit_order(order)

    def _submit_sell(self, price: float, quantity: float) -> None:
        order = bc.Order()
        order.symbol = self.symbol
        order.side = bc.OrderSide.SELL
        order.quantity = quantity
        order.price = price
        order.status = bc.OrderStatus.PENDING
        self.engine.submit_order(order)

    def on_fill(self, fill) -> None:
        pass
