"""仪表盘内置示例策略。"""

from __future__ import annotations

from typing import Callable, Optional

import _backtest_core as bc


class PeriodicTradeStrategy:
    """
    示例策略：每 5 根 Bar 买入 100 股，每 10 根 Bar 卖出全部持仓。

    - counter % 5 == 0  → 买入
    - counter % 10 == 0 → 先卖出（若有持仓），再视情况买入
    """

    def __init__(
        self,
        engine,
        symbol: str,
        on_progress: Optional[Callable[[int], None]] = None,
    ):
        self.engine = engine
        self.symbol = symbol
        self.counter = 0
        self.on_progress = on_progress

    def on_bar(self, bar) -> None:
        self.counter += 1

        if self.on_progress:
            self.on_progress(self.counter)

        # 每 10 根 Bar 平仓（优先于买入，避免同 Bar 先买后卖）
        if self.counter % 10 == 0:
            position = self.engine.get_position(self.symbol)
            if position.quantity > 0:
                sell_order = bc.Order()
                sell_order.symbol = self.symbol
                sell_order.side = bc.OrderSide.SELL
                sell_order.quantity = position.quantity
                sell_order.price = bar.close
                sell_order.status = bc.OrderStatus.PENDING
                self.engine.submit_order(sell_order)

        # 每 5 根 Bar 买入
        if self.counter % 5 == 0:
            buy_order = bc.Order()
            buy_order.symbol = self.symbol
            buy_order.side = bc.OrderSide.BUY
            buy_order.quantity = 100
            buy_order.price = bar.close
            buy_order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(buy_order)

    def on_fill(self, fill) -> None:
        pass
