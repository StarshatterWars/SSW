# Phase 5 economy handoff: open release gate

G gameplay checkpoint: 278b17cbe. H native vortex: 779206759. I graphics/profiling: ad67fefde. Editor and Windows Development build 20261127-073023Z passed, as did 449 regressions with no test warnings/failures/skips. The current game still uses its existing economy worker. No live handoff has occurred.

## Authoritative state that migration must preserve

FEconomyState contains more than inventory quantities: wallets/player account identities; inventory IDs, owners, custodians, prices, restrictions and reservations; running/queued industry jobs, blueprint instances and research/copying results; serialized and used hull stock; committed transaction receipts; contracts and escrow; market orders and history; mining equipment/drones/leases; ground factories and offline facilities; fleet repair orders; the economy clock and scheduling deadlines. Traffic journeys additionally live in FGalaxyTrafficState, and owned equipment/skins/skill queues have profile state outside FEconomyState.

FractalEconomy's initial standalone schema covers background mines, primary facility jobs, freight, stock, budgets, receipts and limited commands. It cannot replace FEconomyState as a whole. Importing only shared fields would silently lose supported gameplay. Its instance mutex prevents two native servers, but does NOT fence the current UE worker. Launcher autostart is therefore not enabled for live play yet.

## Required handoff sequence

1. Version the complete migration schema and verify field-by-field round trips with existing pilot fixtures. Preserve identifiers, reservations, receipts and absolute deadlines. Never reseed an existing world or pilot balance.
2. Port or adapt every mutable economic operation before redirecting its UI, including stores, refitting, blueprints/research, insurance/repair, skills and contracts. Server-side validation and idempotent receipts must precede durable acknowledgement.
3. Drain the old request queue, checkpoint, acquire a fenced single-writer ownership token and deactivate local background advancement and abstract freight settlement. Visualized ships report arrival; they must not credit stocks independently.
4. Connect read projections and granular change payloads to existing panels. The server clock advances scheduled work; UE level loads only realize the view. Lost service suspends mutations rather than enabling local fallback.
5. Add launcher start/status/stop-for-update only once the above gate passes. Graceful shutdown must save successfully before files are replaced. Closing the launcher should not discard server state or force game exit.
6. Test two launches, concurrent/retried commands, client loss, server loss, restart, update, checkpoint recovery and the Janus mining-production-freight loop through the actual client. Verify conservation and no double progression, then publish.

The native six-hour test and loopback lifecycle tests provide service coverage, not proof of this handoff. Build-Cook-Deploy is not yet marked ready for live economy cutover.
