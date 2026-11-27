# FractalEconomy native service

Standalone C++20 process; no Unreal Engine, renderer or level required. Build with `pwsh -File EconomyServer/Build-Server.ps1`. CMake requires Visual Studio C++ tools and Python for isolated lifecycle tests. The script builds/tests before staging a uniquely named package under Build/EconomyServerStaging; it does not publish or replace a live game.

Run `FractalEconomy.exe --data <GameData folder> --state <private persistent folder> --port 17881`. HTTP binds loopback only. A Windows session-wide mutex rejects another authority even on a different port or state directory. Commands/snapshots require the process-generated authority.key through X-SSW-Authority or Authorization: Bearer. Never log or publish this key. The process is independent of launcher lifetime; authenticated POST /shutdown checkpoints before exit. Native scheduled work runs on one thread; HTTP work is serialized against it.

Catalogs: SSW cargo, multi-output recipes, facilities, finite background mines, authored galaxy links and market/fee data. Scheduled manufacturing reserves output space; freight reserves stock/buyer funds before pickup and credits the destination only after unloading. Base prices and tax constants come from SSW; included GST uses SSW rounding. Full live price-policy parity is not established yet. No CCP server code or EVE balance values are included. Vendored MIT dependencies are pinned with hashes in third_party/manifest.json.

Persistence uses checksummed, flushed atomic replacement with a previous-generation backup. Corrupt current data recovers the previous complete generation; both corrupt fail closed without resetting inventory. Request receipts survive restart. Checksum detects accidental corruption; it is not an authenticity signature. Persistence failure suspends mutations. The service advances only while running; wall-clock offline catch-up is not implemented.

## Live integration boundary

Player-owned ships, equipment, cargo, skins, skills and wallets remain in the player record. They do not need migration into this NPC economy service. The native external-trade API prepares NPC stock/budget/capacity, settles once against a durable player escrow reference, returns persistent receipts and cancels or expires abandoned reservations. It never creates a native wallet for the external player. Exact original requests are retained with receipts; another command cannot reuse an ID.

These trade endpoints currently trust a local player adapter. The supplied escrow ID is a recovery reference, not cryptographic proof of funds. Public multiplayer needs authentication/authorization from a trusted player/gameplay service; do not expose the loopback key to untrusted clients or call this an online security boundary.

The service supports `--await-bootstrap`: a new instance stays idle until a valid NPC-only native snapshot is checkpointed by `bootstrap_npc`. Activation pins its world ID and original handoff payload; another handoff cannot overwrite the world. An activated instance resumes the same checkpoint after restart. Windows also fences the NPC writer separately from the service process, so standby can coexist with the old game without advancing another economy.

The game has persistent player escrow and reconnect reconciliation, but UE-to-native NPC snapshot conversion and the ongoing read model remain unconnected. Live cutover stays disabled. The launcher may autostart a hash-verified standby package; that is not evidence that the live game has migrated. Neither standalone test success nor protocol activation tests establish full live-client parity.

## Verified tests

Native catalog/inventory/job/freight checks include six simulated hours, midpoint save/restart parity, idempotent purchases, included GST rounding, rejected mutation rollback and two-generation recovery. Loopback lifecycle coverage includes authentication, duplicate instances, alternate-port duplicate rejection, retained NPC budgets and prepared trade receipts, key rotation and graceful restart. Uses temporary test-owned directories and no real pilot saves.
