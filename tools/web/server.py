#!/usr/bin/env python3
"""
Web 网关：浏览器 <-- WebSocket --> 本脚本 <-- TCP --> ESP-01S

用法：
  python3 tools/web/server.py --esp 192.168.1.123 --port 8080
然后浏览器打开 http://localhost:8000
"""
from __future__ import annotations

import argparse
import asyncio
import json
import pathlib
from typing import Set

from aiohttp import web, WSMsgType

ROOT = pathlib.Path(__file__).parent
INDEX = ROOT / "index.html"


class Bridge:
    def __init__(self, esp_host: str, esp_port: int):
        self.esp_host = esp_host
        self.esp_port = esp_port
        self.clients: Set[web.WebSocketResponse] = set()
        self.writer: asyncio.StreamWriter | None = None
        self.connected = False

    async def broadcast(self, msg: str):
        dead = []
        for ws in self.clients:
            try:
                await ws.send_str(msg)
            except ConnectionResetError:
                dead.append(ws)
        for ws in dead:
            self.clients.discard(ws)

    async def esp_loop(self):
        while True:
            try:
                print(f"[bridge] connecting ESP {self.esp_host}:{self.esp_port}")
                reader, writer = await asyncio.open_connection(self.esp_host, self.esp_port)
                self.writer = writer
                self.connected = True
                await self.broadcast(json.dumps({"type": "status", "esp": "online"}))
                print("[bridge] ESP connected")
                while True:
                    line = await reader.readline()
                    if not line:
                        break
                    text = line.decode(errors="replace").strip()
                    if not text:
                        continue
                    print(f"[esp] {text}")
                    await self.broadcast(text)
            except Exception as e:
                print(f"[bridge] ESP error: {e}")
            self.connected = False
            self.writer = None
            await self.broadcast(json.dumps({"type": "status", "esp": "offline"}))
            await asyncio.sleep(3)

    async def send_to_esp(self, text: str):
        if self.writer is None:
            return False
        try:
            self.writer.write(text.encode() + b"\n")
            await self.writer.drain()
            return True
        except Exception as e:
            print(f"[bridge] send err: {e}")
            return False


async def index(request):
    return web.FileResponse(INDEX)


async def ws_handler(request):
    bridge: Bridge = request.app["bridge"]
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    bridge.clients.add(ws)
    await ws.send_str(json.dumps({"type": "status", "esp": "online" if bridge.connected else "offline"}))
    try:
        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                await bridge.send_to_esp(msg.data)
            elif msg.type == WSMsgType.ERROR:
                break
    finally:
        bridge.clients.discard(ws)
    return ws


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--esp", required=True, help="ESP-01S IP address")
    p.add_argument("--port", type=int, default=8080, help="ESP TCP port")
    p.add_argument("--web-port", type=int, default=8000, help="Web UI port")
    args = p.parse_args()

    bridge = Bridge(args.esp, args.port)

    app = web.Application()
    app["bridge"] = bridge
    app.router.add_get("/", index)
    app.router.add_get("/ws", ws_handler)

    async def start_bg(app):
        app["esp_task"] = asyncio.create_task(bridge.esp_loop())

    async def stop_bg(app):
        app["esp_task"].cancel()

    app.on_startup.append(start_bg)
    app.on_cleanup.append(stop_bg)

    print(f"[web] http://0.0.0.0:{args.web_port}")
    web.run_app(app, host="0.0.0.0", port=args.web_port)


if __name__ == "__main__":
    main()
