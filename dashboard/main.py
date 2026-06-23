from fastapi import FastAPI
from fastapi.responses import HTMLResponse, PlainTextResponse
from fastapi.staticfiles import StaticFiles
from pathlib import Path

app = FastAPI(title="回测仪表盘")

STATIC_DIR = Path(__file__).resolve().parent / "static"
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


@app.get("/", response_class=PlainTextResponse)
def root():
    return "Dashboard is running"


@app.get("/dashboard", response_class=HTMLResponse)
def dashboard_page():
    return (STATIC_DIR / "index.html").read_text(encoding="utf-8")
