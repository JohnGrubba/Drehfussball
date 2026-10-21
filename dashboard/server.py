#!/usr/bin/env python3
"""Local Tourney Master dashboard server.

Serves the same HTML that is embedded in the ESP32 firmware and provides a
WebSocket mock at /ws, so the dashboard can be developed and shown on a large
screen without the ESP32.

Usage:
    pip install -r requirements.txt
    python3 server.py
    python3 server.py --host 0.0.0.0 --port 8000   # for a TV on the LAN

Then open:
    http://localhost:8000/
    http://localhost:8000/tv                      # TV / tournament overview
    http://localhost:8000/?demo=1&tv=1            # demo tournament
"""

from __future__ import annotations

import argparse
import json
import socket
from pathlib import Path

from aiohttp import WSMsgType, web

from build_site import build

ROOT = Path(__file__).resolve().parent
DIST = ROOT / "dist"


class Hub:
    def __init__(self) -> None:
        self.clients: set[web.WebSocketResponse] = set()
        self.state = {"team1": 0, "team2": 0}

    async def broadcast(self) -> None:
        message = json.dumps(self.state)
        for ws in list(self.clients):
            try:
                await ws.send_str(message)
            except Exception:
                self.clients.discard(ws)

    async def ws_handler(self, request: web.Request) -> web.WebSocketResponse:
        ws = web.WebSocketResponse(heartbeat=30)
        await ws.prepare(request)
        self.clients.add(ws)
        await ws.send_json(self.state)

        async for msg in ws:
            if msg.type != WSMsgType.TEXT:
                continue
            cmd = msg.data.strip()
            if cmd == "t1_add":
                self.state["team1"] = min(255, self.state["team1"] + 1)
            elif cmd == "t1_sub":
                self.state["team1"] = max(0, self.state["team1"] - 1)
            elif cmd == "t2_add":
                self.state["team2"] = min(255, self.state["team2"] + 1)
            elif cmd == "t2_sub":
                self.state["team2"] = max(0, self.state["team2"] - 1)
            elif cmd == "reset":
                self.state = {"team1": 0, "team2": 0}
            else:
                continue
            await self.broadcast()

        self.clients.discard(ws)
        return ws


async def index(request: web.Request) -> web.Response:
    path = DIST / "index.html"
    if not path.exists():
        build()
    return web.Response(
        text=path.read_text(encoding="utf-8"),
        content_type="text/html",
        headers={"Cache-Control": "no-store"},
    )


async def favicon(request: web.Request) -> web.Response:
    return web.Response(status=204)


def lan_ip() -> str:
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.connect(("8.8.8.8", 80))
        ip = sock.getsockname()[0]
        sock.close()
        return ip
    except Exception:
        return "127.0.0.1"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1",
                        help="bind address (use 0.0.0.0 for a TV on the LAN)")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--no-build", action="store_true",
                        help="do not rebuild dist/index.html on startup")
    args = parser.parse_args()

    if not args.no_build:
        build()

    hub = Hub()
    app = web.Application()
    app.router.add_get("/", index)
    app.router.add_get("/tv", index)
    app.router.add_get("/ws", hub.ws_handler)
    app.router.add_get("/favicon.ico", favicon)

    host = args.host
    print("Tourney Master local dashboard")
    print(f"  Live: http://127.0.0.1:{args.port}/")
    print(f"  TV:   http://127.0.0.1:{args.port}/tv")
    if host == "0.0.0.0":
        print(f"  LAN:  http://{lan_ip()}:{args.port}/tv")
    print("  Stop with Ctrl+C")

    web.run_app(app, host=host, port=args.port, print=None)


if __name__ == "__main__":
    main()
