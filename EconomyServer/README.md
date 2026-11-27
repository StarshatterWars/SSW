# FractalEconomy native service

Standalone C++20 process; no Unreal Engine, renderer or level required. Build with `pwsh -File EconomyServer/Build-Server.ps1`. CMake requires Visual Studio C++ tools and Python for isolated lifecycle tests. The script builds/tests before staging a uniquely named package under Build/EconomyServerStaging; it does not publish or replace a live game.

Run `FractalEconomy.exe --data <GameData folder> --state <private persistent folder> --port 17881`. HTTP binds loopback only. A Windows session-wide mutex rejects another authority even on a different port or state directory. Commands/snapshots require the process-generated authority.key through X-SSW-Authority. Never log or publish this key. The process is independent of launcher lifetime; authenticated POST /shutdown checkpoints before exit. Native scheduled work runs on one thread; HTTP work is serialized against it.

Catalogs: SSW cargo, multi-output recipes, facilities, finite background mines, authored galaxy links and market/fee data. Scheduled manufacturing reserves output space; freight reserves stock/buyer funds before pickup and credits the destination only after unloading. Current gameplay prices are retained; included GST uses SSW rounding. No CCP server code or EVE balance values are included. Vendored MIT dependencies are pinned with hashes in third_party/manifest.json.

Persistence uses checksummed, flushed atomic replacement with a previous-generation backup. Corrupt current data recovers the previous complete generation; both corrupt fail closed without resetting inventory. Request receipts survive restart. Checksum detects accidental corruption; it is not an authenticity signature. Persistence failure suspends mutations. The service advances only while running; wall-clock offline catch-up is not implemented.

## Migration boundary — not yet a live replacement

The service currently has an independent schema. It is NOT connected to the UE economy and must not be automatically started alongside the current game as its live authority. Launcher cutover remains disabled until an explicit, verified migration preserves pilot accounts, jobs, reservations, contracts, serialized hulls, stores and traffic. Three native command types are currently covered (account creation, direct market purchase, facility availability); existing UE commerce/skills/contracts/refitting handlers have not all been ported. Replacing their state with this prototype would lose functionality.

The next handoff must import an exact validated checkpoint, stop/drain the old writer, transfer a fencing token, start the new scheduler once, and expose read models plus command adapters. On disconnect the client must stop writes; it must never silently resume a local economy. Rollback must preserve the service checkpoint, not fork it. Standalone test success does not establish live-client parity.

## Verified tests

Native catalog/inventory/job/freight checks include six simulated hours, midpoint save/restart parity, idempotent purchases, included GST rounding, rejected mutation rollback and two-generation recovery. Loopback lifecycle coverage includes authentication, duplicate instances, alternate-port duplicate rejection, retained account balances, key rotation and graceful restart. Uses temporary test-owned directories and no real pilot saves.
