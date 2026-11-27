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
            facility = json.loads((pathlib.Path(data) / "Production" / "facilities.json").read_text(encoding="utf-8-sig"))[0]["Name"]
            cmd = {"id": "service-facility", "action": "set_facility_operational", "facility": facility, "operational": False}
            first = request(port, "/v1/commands", key, cmd)
            retry = request(port, "/v1/commands", key, cmd)
            check(first == retry, "HTTP command retry returns same receipt")
            check(not any(owner.startswith("Pilot/") for owner in request(port, "/v1/snapshot", key)["wallets"]), "Service creates no player wallet or starter balance")
            try:
                request(port, "/v1/commands", key, {"id": "forbidden-wallet", "action": "create_account", "actor": "Pilot/Fixture", "system": "Janus"})
                raise AssertionError("NPC service accepted a player account")
            except urllib.error.HTTPError as error:
                check(error.code == 400, "Legacy player-account command rejected at the service boundary")
            snapshot = request(port, "/v1/snapshot", key)
            market = "Janus/NPC Market/Stock"
            goods = json.loads((pathlib.Path(data) / "Cargo" / "cargo.json").read_text(encoding="utf-8-sig"))
            good = next(row for row in goods if "".join(row["Name"].lower().split()) == "industrialconcentrate")
            item = good["Name"]
            stock = snapshot["inventories"][market]["stock"][item]
            native_quote = request(port, "/v1/commands", key, {"action": "quote_external_trade", "market": market, "item": item, "side": "Buy", "quantity": 1})
            price = native_quote["totalCredits"]
            check(native_quote["baseCredits"] + native_quote["taxCredits"] == price, "Server quote includes the exact GST breakdown")
            check(request(port, "/v1/snapshot", key)["revision"] == snapshot["revision"], "Quoting is read-only and does not reserve or save stock")
            prepare = {"id": "http-prepare", "action": "prepare_external_trade", "tradeId": "http-trade", "actor": "Pilot/ExternalLifecycle", "market": market, "item": item, "quantity": 1, "side": "Buy", "expectedTotal": price}
            check(request(port, "/v1/commands", key, prepare)["state"] == "Prepared", "HTTP trade preparation checkpoints its reservation")
            check("Pilot/ExternalLifecycle" not in request(port, "/v1/snapshot", key)["wallets"], "External player wallet is not duplicated on the NPC server")
            check(request(port, "/shutdown", key, {})["ok"], "Authenticated graceful shutdown")
            check(process.wait(timeout=20) == 0, "Shutdown waits for checkpoint and exits cleanly")
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            ready(process, port)
            new_key = (root / "world Unicode \u00e9\u0394" / "authority.key").read_text()
            check(new_key != key, "Restart rotates process authority key")
            check(not any(owner.startswith("Pilot/") for owner in request(port, "/v1/snapshot", new_key)["wallets"]), "Restart does not create player accounts")
            status_command = {"id": "http-status", "action": "external_trade_status", "tradeId": "http-trade", "actor": "Pilot/ExternalLifecycle"}
            check(request(port, "/v1/commands", new_key, status_command)["state"] == "Prepared", "Trade remains recoverable across service restart")
            settle = {"id": "http-settle", "action": "settle_external_trade", "tradeId": "http-trade", "actor": "Pilot/ExternalLifecycle", "playerEscrowId": "http-trade"}
            result = request(port, "/v1/commands", new_key, settle)
            check(result["state"] == "Committed", "Restarted service settles prepared trade")
            check(request(port, "/v1/commands", new_key, settle) == result, "Lost settlement replies can be retried safely")
            check(request(port, "/v1/snapshot", new_key)["inventories"][market]["stock"][item] == stock - 1, "HTTP retries remove NPC stock exactly once")
            request(port, "/shutdown", new_key, {})
            check(process.wait(timeout=20) == 0, "Restarted service checkpoints cleanly")
            standby_command = command + ["--await-bootstrap"]
            standby_command[standby_command.index("--state") + 1] = str(root / "standby")
            process = subprocess.Popen(standby_command, stdout=log, stderr=subprocess.STDOUT)
            pending = ready(process, port)
            pending_key = (root / "standby" / "authority.key").read_text()
            check(not pending["npcAuthorityActive"], "Launcher standby does not claim live NPC authority")
            time.sleep(.2)
            check(request(port, "/status")["economySeconds"] == 0, "Standby cannot advance a second universe")
            seed = request(port, "/v1/snapshot", pending_key)
            activate = {"action": "bootstrap_npc", "format": "SSW-NPC-1", "sourceId": "isolated-test-world", "handoffId": "durable-handoff", "snapshot": seed}
            bad = dict(activate, snapshot=dict(seed, wallets={"Pilot/Forbidden": 5000}))
            try:
                request(port, "/v1/commands", pending_key, bad)
                raise AssertionError("Player wallet entered NPC authority")
            except urllib.error.HTTPError as error:
                check(error.code == 400 and not request(port, "/status")["npcAuthorityActive"], "Failed handoff remains standby")
            activated = request(port, "/v1/commands", pending_key, activate)
            check(activated["worldId"] == pending["worldId"], "Handoff pins the advertised persistent world")
            check(request(port, "/v1/commands", pending_key, activate) == activated, "Activation response can be recovered after a lost reply")
            check(request(port, "/status")["npcAuthorityActive"], "Checkpointed activation is advertised")
            request(port, "/shutdown", pending_key, {})
            check(process.wait(timeout=20) == 0, "Activated service stops with a complete checkpoint")
            process = subprocess.Popen(standby_command, stdout=log, stderr=subprocess.STDOUT)
            recovered = ready(process, port)
            check(recovered["npcAuthorityActive"] and recovered["worldId"] == activated["worldId"], "Launcher restart recovers the same authority instead of seeding another")
            recovered_key = (root / "standby" / "authority.key").read_text()
            check(request(port, "/v1/commands", recovered_key, activate) == activated, "Reconnect recovers activation receipt")
            replacement = dict(activate, handoffId="other")
            try:
                request(port, "/v1/commands", recovered_key, replacement)
                raise AssertionError("Existing world was overwritten")
            except urllib.error.HTTPError as error:
                check(error.code == 400, "Another handoff cannot overwrite a running economy")
            request(port, "/shutdown", recovered_key, {})
            check(process.wait(timeout=20) == 0, "Recovered authority exits cleanly")
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=20)
print(f"{checks} native service checks passed")
