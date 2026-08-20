#!/usr/bin/env python3
"""Serve a firmware image to a Yeelight's stock OTA downloader.

The downloader (User-Agent: MIoT) will not complete a transfer from an HTTP/1.0
server: it connects, starts reading, then resets the connection, and the device
returns to `idle` with nothing written. Python's built-in http.server answers
HTTP/1.0 and ignores Range, so it fails here.

This server speaks HTTP/1.1 with keep-alive, honours Range requests, and logs
exactly what the device asks for and how many bytes were delivered - which makes
a failed transfer obvious rather than silent.

Usage:
    ota_server.py <firmware.bin> [port]

The file is served at any path, so the URL passed to miIO.ota only has to point
at this host and port.
"""

from http.server import BaseHTTPRequestHandler, HTTPServer
import re
import socketserver
import sys
import threading

_lock = threading.Lock()


def build_handler(data: bytes):
    size = len(data)

    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"
        server_version = "nginx/1.24.0"
        sys_version = ""

        def log_message(self, fmt, *args):
            pass  # replaced by the explicit logging below

        def _log_request(self):
            with _lock:
                print(f"\n=== {self.client_address[0]}:{self.client_address[1]} "
                      f"{self.command} {self.path} {self.request_version} ===",
                      flush=True)
                for k, v in self.headers.items():
                    print(f"    {k}: {v}", flush=True)

        def do_HEAD(self):
            self._log_request()
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(size))
            self.send_header("Accept-Ranges", "bytes")
            self.end_headers()

        def do_GET(self):
            self._log_request()
            start, end, partial = 0, size - 1, False
            rng = self.headers.get("Range")
            if rng:
                m = re.match(r"bytes=(\d*)-(\d*)", rng.strip())
                if m:
                    if m.group(1):
                        start = int(m.group(1))
                    if m.group(2):
                        end = min(int(m.group(2)), size - 1)
                    partial = True

            body = data[start:end + 1]
            self.send_response(206 if partial else 200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Accept-Ranges", "bytes")
            if partial:
                self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
            self.end_headers()

            sent = 0
            try:
                while sent < len(body):
                    n = self.wfile.write(body[sent:sent + 4096])
                    sent += n or 4096
                with _lock:
                    print(f"    --> sent {sent}/{len(body)} bytes  COMPLETE",
                          flush=True)
            except Exception as e:  # noqa: BLE001 - report whatever went wrong
                with _lock:
                    print(f"    --> sent {sent}/{len(body)} bytes then "
                          f"DISCONNECT: {type(e).__name__}: {e}", flush=True)

    return Handler


class Server(socketserver.ThreadingMixIn, HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    path = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
    with open(path, "rb") as f:
        data = f.read()
    print(f"serving {path} ({len(data)} bytes) on port {port}", flush=True)
    print("verify another host on the LAN can fetch the whole file before "
          "sending the OTA command", flush=True)
    Server(("0.0.0.0", port), build_handler(data)).serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
