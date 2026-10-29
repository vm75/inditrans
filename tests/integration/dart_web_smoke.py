#!/usr/bin/env python3
"""Exercise the Dart web FFI wrapper against the current standalone WASM."""
import base64
import http.server
import os
from pathlib import Path
import shlex
import re
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
ASSET = ROOT / "flutter/assets/inditrans.wasm"


class WasmHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/inditrans.wasm":
            self.send_error(404)
            return
        data = ASSET.read_bytes()
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Type", "application/wasm")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format, *args):
        pass


def main():
    test_source = (ROOT / "flutter/test/urdu_web_test.dart").read_text(encoding="utf-8")
    fixture = re.search(r"const _pack = '([^']+)';", test_source)
    if not fixture or base64.b64decode(fixture.group(1), validate=True) != (ROOT / "test-files/urdu-test-lexicon.iul").read_bytes():
        raise SystemExit("Dart web fixture does not match the shared Urdu test pack")
    browser = os.environ.get("CHROME_EXECUTABLE") or shutil.which("chromium") or shutil.which("google-chrome")
    dart = shutil.which("dart")
    if not browser or not dart:
        raise SystemExit("Dart and Chromium are required")
    with http.server.ThreadingHTTPServer(("127.0.0.1", 18773), WasmHandler) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            with tempfile.TemporaryDirectory(prefix="inditrans-dart-web-") as directory:
                wrapper = Path(directory) / "chrome"
                wrapper.write_text(f"#!/bin/sh\nexec {shlex.quote(browser)} --no-sandbox --disable-dev-shm-usage \"$@\"\n")
                wrapper.chmod(0o755)
                env = dict(os.environ, CHROME_EXECUTABLE=str(wrapper))
                return subprocess.run([dart, "test", "-p", "chrome", "test/urdu_web_test.dart", "--reporter", "expanded"],
                                      cwd=ROOT / "flutter", env=env, check=False).returncode
        finally:
            server.shutdown()
            thread.join()


if __name__ == "__main__":
    raise SystemExit(main())
