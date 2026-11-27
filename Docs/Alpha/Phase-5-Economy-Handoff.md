# Phase 5 live economy: revised ownership boundary

G gameplay checkpoint: 278b17cbe. H native vortex: 779206759. I graphics/profiling: ad67fefde. These passed Editor/Windows compilation and 449 regressions. The standalone native service foundation is committed separately. No live economy cutover is claimed yet.

## Agreed division

Player assets stay in the player record: ships and their locations, equipment, cargo, skins, skills, wallet, contracts and receipts. The NPC service owns shared NPC stock/budgets, mining, facilities, production, freight and market signals. Moving the entire pilot to that service is unnecessary and has been withdrawn. See EVE-Player-Economy-Boundary.md for evidence, limits and the revised design.

## Release gates

- Bootstrap/migrate only the NPC domain, preserving finite stocks, mine depletion, active jobs and freight. Never import player-owned assets or grant another starting balance.
- Project NPC inventories/jobs/freight into the game without treating the projection as a second writable economy.
- Wire prepared NPC trades to persistent player escrow/outbox recovery. Settle both sides once; reconnect must not repeat charges or duplicate cargo.
- Fence and drain the old NPC worker/scheduler before activating the new service. Stop local advancement even if the service disconnects; never silently fork back to local production.
- Launcher autostart/status and update shutdown follow the verified activation protocol. Keep gameplay-server movement/combat separate.
- Test restart, concurrent launches, lost replies, cancellation/settlement races, store/UI parity and the Janus loop through the real client before marking Build-Cook-Deploy ready.

The native six-hour and process-lifecycle tests cover the service foundation, not full live-client parity. Player storage can move to a separate server later without merging it into NPC simulation.

## Explicitly deferred

After Phase 5: expansion of the persistent NPC registry/possible NPC-service split. The launcher button cleanup (Repair, Install, Locate, Updates and speaker/mute) is bundled with the launcher rebuild. Endpoint configuration and server status in the launcher and Player Logbook remain in Phase 5.

## Current implementation checkpoint (27 November)

- Native standby supports `--await-bootstrap`: no autonomous advancement before an explicit, checkpointed handoff. Activation retains a persistent world ID, rejects player assets and rejects a second source overwriting active stock. Restart retains the identity and activation receipt.
- Windows writer fences prevent a game and service from claiming NPC advancement together. The game-side handoff marker is flushed to disk before releasing its writer. Late local background results are rejected after cutover. Commandlets remain isolated unless integration mode is explicitly requested.
- The player outbox persists escrow with wallet/cargo changes. Native trades carry world IDs; the game trade adapter and reconnect reconciler reject cross-world receipts and never refund blindly on a network timeout. This connection code is not yet verified through a real live handoff.
- Launcher startup validates the native package hashes, runs the service hidden in standby, serializes lifecycle operations and bounds crash-restart attempts. Remote failures do not start a local replacement. Utility icons and navigation occupy one row; gameplay and economy status have distinct cards.

**Still not ready for deployment:** the UE-to-native NPC snapshot conversion and ongoing read-model projection are not connected. Existing active freight, ground factory records, serialized hull output, authored automated recipe choices and market-consumption parity need to be preserved and checked at that boundary. Other NPC-mutating local transaction adapters still need remote implementations; local worker results that change the NPC ownership slice are rejected in external mode. The native package retains `liveClientCutoverReady=false`; do not flip it based on protocol/unit tests alone. The build pipeline does not yet bundle an activated live economy release.

Live authority checkpoint corruption fails closed instead of silently loading an older inventory generation. Prototype-only worlds retain the old backup-recovery tests. Receipt-aware live recovery is an explicit remaining administration task; rolling back settled stock while a player retains delivered cargo is unsafe.

## Verified source checkpoint: 20261127-115218Z

Editor and Win64 Development client builds passed. Full `Starshatter` automation: 452 succeeded, zero failures/warnings/skips (`Saved/Automation/Economy-Verified-Full`). Native CTest: 55 kernel assertions and 33 HTTP lifecycle assertions passed, including live-checkpoint rollback rejection. Launcher 1.0.20: 67 checks passed with the real native-manager fixture enabled, covering hidden standby startup, graceful update shutdown, same-world restart, package integrity, endpoint settings and single-row toolbar. Include-order and whitespace checks passed. These are source/protocol checkpoints; no client cook, public feed publication or completed live Janus handoff is claimed.
