"""Isolated loopback service lifecycle tests; never uses a pilot save."""
import json
import pathlib
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

exe, data = map(str, map(pathlib.Path, sys.argv[1:3]))
checks = 0

def check(value, message):
    global checks
    if not value:
        raise AssertionError(message)
    checks += 1

def request(port, path, key=None, body=None):
    headers = {"Content-Type": "application/json"}
    if key:
        headers["X-SSW-Authority"] = key
    req = urllib.request.Request(f"http://127.0.0.1:{port}{path}",
                                 data=None if body is None else json.dumps(body).encode(), headers=headers)
    with urllib.request.urlopen(req, timeout=5) as response:
        return json.load(response)

def ready(process, port):
    until = time.monotonic() + 20
    while time.monotonic() < until:
        if process.poll() is not None:
            raise AssertionError(f"Service exited: {process.returncode}")
        try:
            return request(port, "/status")
        except (OSError, urllib.error.URLError):
            time.sleep(.05)
    raise AssertionError("Service did not become ready")

with tempfile.TemporaryDirectory(prefix="SSW native service ") as folder:
    root = pathlib.Path(folder)
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    command = [exe, "--data", data, "--state", str(root / "world Unicode \u00e9\u0394"), "--port", str(port)]
    with open(root / "service.log", "wb") as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
        try:
            status = ready(process, port)
            check(status["protocol"] == 1 and status["checkpointReady"], "Ready after checkpoint creation")
            key = (root / "world Unicode \u00e9\u0394" / "authority.key").read_text()
            try:
                request(port, "/v1/snapshot", "invalid")
                raise AssertionError("Unauthorized snapshot accepted")
            except urllib.error.HTTPError as error:
                check(error.code == 403, "Snapshot requires local authority key")
            duplicate = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
            check(duplicate.returncode != 0, "Duplicate authority rejected")
            check((root / "world Unicode \u00e9\u0394" / "authority.key").read_text() == key, "Duplicate cannot replace authority key")
            if sys.platform == "win32":
                other = command.copy()
                other[other.index("--state") + 1] = str(root / "other")
                other[other.index("--port") + 1] = str(port + 1 if port < 65535 else port - 1)
                duplicate = subprocess.run(other, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
                check(duplicate.returncode != 0, "Alternate port cannot start a second session authority")
                check(not (root / "other" / "economy.json").exists(), "Rejected authority cannot seed another world")
            cmd = {"id": "service-pilot", "action": "create_account", "actor": "Pilot/ServiceFixture", "system": "Janus", "capacityTonnes": 48}
            first = request(port, "/v1/commands", key, cmd)
            retry = request(port, "/v1/commands", key, cmd)
            check(first == retry, "HTTP command retry returns same receipt")
            check(request(port, "/v1/snapshot", key)["wallets"]["Pilot/ServiceFixture"] == 5000, "Only one starter balance granted")
            check(request(port, "/shutdown", key, {})["ok"], "Authenticated graceful shutdown")
            check(process.wait(timeout=20) == 0, "Shutdown waits for checkpoint and exits cleanly")
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            ready(process, port)
            new_key = (root / "world Unicode \u00e9\u0394" / "authority.key").read_text()
            check(new_key != key, "Restart rotates process authority key")
            check(request(port, "/v1/snapshot", new_key)["wallets"]["Pilot/ServiceFixture"] == 5000, "Restart retains authoritative account")
            request(port, "/shutdown", new_key, {})
            check(process.wait(timeout=20) == 0, "Restarted service checkpoints cleanly")
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=20)
print(f"{checks} native service checks passed")
