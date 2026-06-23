import os
import sys
import threading
import uuid
from pathlib import Path
from typing import Any, Literal, Optional

import pandas as pd
from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse, PlainTextResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT / "python"))
os.chdir(PROJECT_ROOT)

import _backtest_core as bc  # noqa: E402

app = FastAPI(title="回测仪表盘")

STATIC_DIR = Path(__file__).resolve().parent / "static"
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")

INITIAL_CASH = 100_000.0
jobs: dict[str, dict[str, Any]] = {}
jobs_lock = threading.Lock()


class RunRequest(BaseModel):
    symbol: str
    start_date: str
    end_date: str
    csv_path: str
    mode: Literal["ideal", "realistic", "dual"] = "realistic"
    latency_ms: float = 0.0
    slippage_bps: float = 0.0


class JobStatusResponse(BaseModel):
    status: str
    progress: int = Field(ge=0, le=100)


class EquityPointResponse(BaseModel):
    datetime: str
    equity: float


class FillResponse(BaseModel):
    fill_id: str
    order_id: str
    symbol: str
    side: str
    quantity: float
    price: float
    timestamp: str


class MetricsResponse(BaseModel):
    total_return: float
    sharpe_ratio: float
    max_drawdown: float
    win_rate: float


class ResultsResponse(BaseModel):
    equity_curve: list[EquityPointResponse]
    fills: list[FillResponse]
    metrics: MetricsResponse
    mode: str
    ideal_equity_curve: Optional[list[EquityPointResponse]] = None
    realistic_equity_curve: Optional[list[EquityPointResponse]] = None


class GapResponse(BaseModel):
    total_return_diff: float
    sharpe_reduction: float
    relative_decay: float


def resolve_csv_path(csv_path: str) -> str:
    path = Path(csv_path)
    if not path.is_absolute():
        path = PROJECT_ROOT / path
    return str(path.resolve())


def count_bars(csv_path: str, start_date: str, end_date: str) -> int:
    df = pd.read_csv(csv_path)
    date_col = "datetime" if "datetime" in df.columns else df.columns[0]
    mask = (df[date_col] >= start_date) & (df[date_col] <= end_date)
    return int(mask.sum())


def side_to_str(side) -> str:
    return "BUY" if side == bc.OrderSide.BUY else "SELL"


def fill_to_dict(fill) -> dict[str, Any]:
    return {
        "fill_id": fill.fill_id,
        "order_id": fill.order_id,
        "symbol": fill.symbol,
        "side": side_to_str(fill.side),
        "quantity": fill.quantity,
        "price": fill.price,
        "timestamp": fill.timestamp,
    }


def equity_to_dict(point) -> dict[str, Any]:
    return {"datetime": point.datetime, "equity": point.equity}


def metrics_to_dict(metrics) -> dict[str, float]:
    return {
        "total_return": metrics.total_return,
        "sharpe_ratio": metrics.sharpe_ratio,
        "max_drawdown": metrics.max_drawdown,
        "win_rate": metrics.win_rate,
    }


def gap_to_dict(gap) -> dict[str, float]:
    return {
        "total_return_diff": gap.total_return_diff,
        "sharpe_reduction": gap.sharpe_reduction,
        "relative_decay": gap.relative_decay,
    }


def create_matcher(request: RunRequest):
    if request.mode == "ideal":
        return bc.IdealMatchingEngine()
    return bc.RealisticMatchingEngine(
        bc.FixedLatencyModel(request.latency_ms),
        bc.FixedSlippageModel(request.slippage_bps),
    )


class PeriodicBuyStrategy:
    """每 5 根 Bar 买入 100 股，用于产生测试成交。"""

    def __init__(self, engine, job_id: str, total_bars: int):
        self.engine = engine
        self.job_id = job_id
        self.total_bars = max(total_bars, 1)
        self.bar_count = 0
        self.symbol = ""

    def set_symbol(self, symbol: str):
        self.symbol = symbol

    def on_bar(self, bar):
        self.bar_count += 1
        progress = min(99, int(self.bar_count / self.total_bars * 100))
        with jobs_lock:
            if self.job_id in jobs:
                jobs[self.job_id]["progress"] = progress

        if self.bar_count % 5 == 0:
            order = bc.Order()
            order.symbol = self.symbol
            order.side = bc.OrderSide.BUY
            order.quantity = 100
            order.price = bar.close
            order.status = bc.OrderStatus.PENDING
            self.engine.submit_order(order)

    def on_fill(self, fill):
        pass


def build_engine_and_run(job_id: str, request: RunRequest) -> None:
    csv_path = resolve_csv_path(request.csv_path)
    total_bars = count_bars(csv_path, request.start_date, request.end_date)

    try:
        matcher = create_matcher(request)
        if request.mode == "ideal":
            engine = bc.CoreEngine(matcher)
        else:
            engine = bc.CoreEngine(matcher)

        strategy = PeriodicBuyStrategy(engine, job_id, total_bars)
        strategy.set_symbol(request.symbol)
        engine.set_strategy(strategy)

        if request.mode == "dual":
            engine.run_dual(
                request.symbol,
                request.start_date,
                request.end_date,
                csv_path,
            )
            ideal_fills = [fill_to_dict(f) for f in engine.get_ideal_fills()]
            realistic_fills = [fill_to_dict(f) for f in engine.get_realistic_fills()]
            ideal_curve = [
                equity_to_dict(p) for p in engine.get_ideal_equity_curve()
            ]
            realistic_curve = [
                equity_to_dict(p) for p in engine.get_realistic_equity_curve()
            ]
            ideal_metrics = bc.PerformanceAnalytics.compute_metrics(
                engine.get_ideal_fills(), engine.get_ideal_equity_curve(), INITIAL_CASH
            )
            realistic_metrics = bc.PerformanceAnalytics.compute_metrics(
                engine.get_realistic_fills(),
                engine.get_realistic_equity_curve(),
                INITIAL_CASH,
            )
            gap = bc.PerformanceAnalytics.compute_gap(ideal_metrics, realistic_metrics)

            result = {
                "fills": realistic_fills,
                "equity_curve": realistic_curve,
                "metrics": metrics_to_dict(realistic_metrics),
                "ideal_fills": ideal_fills,
                "ideal_equity_curve": ideal_curve,
                "realistic_fills": realistic_fills,
                "realistic_equity_curve": realistic_curve,
                "gap_metrics": gap_to_dict(gap),
            }
        else:
            engine.run(
                request.symbol,
                request.start_date,
                request.end_date,
                csv_path,
            )
            fills = [fill_to_dict(f) for f in engine.get_fills()]
            equity_curve = [equity_to_dict(p) for p in engine.get_equity_curve()]
            metrics = bc.PerformanceAnalytics.compute_metrics(
                engine.get_fills(), engine.get_equity_curve(), INITIAL_CASH
            )
            result = {
                "fills": fills,
                "equity_curve": equity_curve,
                "metrics": metrics_to_dict(metrics),
            }

        with jobs_lock:
            jobs[job_id].update(
                {
                    "status": "completed",
                    "progress": 100,
                    **result,
                }
            )
    except Exception as exc:
        with jobs_lock:
            jobs[job_id].update(
                {
                    "status": "failed",
                    "progress": jobs[job_id].get("progress", 0),
                    "error": str(exc),
                }
            )


def run_backtest_in_background(job_id: str, request: RunRequest) -> None:
    thread = threading.Thread(
        target=build_engine_and_run,
        args=(job_id, request),
        daemon=True,
    )
    thread.start()


@app.get("/", response_class=PlainTextResponse)
def root():
    return "Dashboard is running"


@app.get("/dashboard", response_class=HTMLResponse)
def dashboard_page():
    return (STATIC_DIR / "index.html").read_text(encoding="utf-8")


@app.post("/run")
def run_backtest(request: RunRequest) -> dict[str, str]:
    job_id = str(uuid.uuid4())
    with jobs_lock:
        jobs[job_id] = {
            "status": "running",
            "progress": 0,
            "mode": request.mode,
            "request": request.model_dump(),
        }

    run_backtest_in_background(job_id, request)
    return {"job_id": job_id}


@app.get("/status/{job_id}", response_model=JobStatusResponse)
def get_status(job_id: str) -> JobStatusResponse:
    with jobs_lock:
        job = jobs.get(job_id)
    if job is None:
        raise HTTPException(status_code=404, detail="Job not found")
    return JobStatusResponse(status=job["status"], progress=job.get("progress", 0))


@app.get("/results/{job_id}", response_model=ResultsResponse)
def get_results(job_id: str) -> ResultsResponse:
    with jobs_lock:
        job = jobs.get(job_id)
    if job is None:
        raise HTTPException(status_code=404, detail="Job not found")
    if job["status"] != "completed":
        raise HTTPException(status_code=400, detail=f"Job status is {job['status']}")

    response = ResultsResponse(
        equity_curve=job.get("equity_curve", []),
        fills=job.get("fills", []),
        metrics=job.get("metrics", {}),
        mode=job.get("mode", "realistic"),
    )
    if job.get("mode") == "dual":
        response.ideal_equity_curve = job.get("ideal_equity_curve")
        response.realistic_equity_curve = job.get("realistic_equity_curve")
    return response


@app.get("/gap/{job_id}")
def get_gap(job_id: str) -> GapResponse | dict[str, str]:
    with jobs_lock:
        job = jobs.get(job_id)
    if job is None:
        raise HTTPException(status_code=404, detail="Job not found")
    if job["status"] != "completed":
        raise HTTPException(status_code=400, detail=f"Job status is {job['status']}")
    if job.get("mode") != "dual":
        return {"error": "Gap metrics are only available for dual mode runs"}
    return GapResponse(**job["gap_metrics"])
