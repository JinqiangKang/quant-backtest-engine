"""验证仪表盘后台线程可使用指数延迟模型。"""

import os
import sys
import threading
import time
import uuid

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DASHBOARD_DIR = os.path.join(PROJECT_ROOT, "dashboard")
sys.path.insert(0, os.path.join(PROJECT_ROOT, "python"))
sys.path.insert(0, DASHBOARD_DIR)
sys.path.insert(0, PROJECT_ROOT)
os.chdir(PROJECT_ROOT)

from fastapi.testclient import TestClient

import main
from main import RunRequest, build_engine_and_run, jobs, jobs_lock


def test_exponential_latency_in_background_thread():
    req = RunRequest(
        symbol="TEST",
        start_date="2023-01-01",
        end_date="2023-01-15",
        csv_path="tests/sample_data.csv",
        mode="realistic",
        latency_model="exponential",
        latency_ms=5.0,
        slippage_bps=50,
        strategy="periodic",
    )
    job_id = str(uuid.uuid4())
    with jobs_lock:
        jobs[job_id] = {"status": "running", "progress": 0}

    thread = threading.Thread(target=build_engine_and_run, args=(job_id, req))
    thread.start()
    thread.join()

    with jobs_lock:
        job = jobs[job_id]

    assert job["status"] == "completed", job.get("error")
    assert len(job["fills"]) > 0


def test_exponential_latency_via_api():
    client = TestClient(main.app)
    payload = {
        "symbol": "TEST",
        "start_date": "2023-01-01",
        "end_date": "2023-01-15",
        "csv_path": "tests/sample_data.csv",
        "mode": "realistic",
        "latency_model": "exponential",
        "latency_ms": 5.0,
        "slippage_bps": 50,
        "strategy": "periodic",
    }
    job_id = client.post("/run", json=payload).json()["job_id"]

    for _ in range(60):
        status = client.get(f"/status/{job_id}").json()
        if status["status"] in ("completed", "failed"):
            break
        time.sleep(0.05)

    assert status["status"] == "completed"
    results = client.get(f"/results/{job_id}").json()
    assert len(results["fills"]) > 0


if __name__ == "__main__":
    test_exponential_latency_in_background_thread()
    test_exponential_latency_via_api()
    print("Custom latency dashboard tests passed.")
