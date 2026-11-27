# Alpha 5 - sprint Phase F execution domains

Current status: industry extension verified, build 20261127-024521Z: Editor/client builds and all 432 regressions passed with zero C++ diagnostics or test warnings/failures/skips. Five graph-audit tests pass. See Phase-F-Industry-Expansion.md. Earlier entries below retain their historical checkpoint boundaries. Remaining simulation threading follows as a separate measured change.

27 November 2026 (pilot local date). Phase F's planned incremental execution-domain
migration is complete at the automated verification boundary: build 20261127-014142Z,
both targets and all 422 regressions passed. User cook/package/T1 verification
follows. This does not claim that the entire legacy flight/combat/AI loop is threaded.
The sections below retain each verified checkpoint and its boundary.

## Domains implemented in this pass

- **Economy:** the existing dedicated `SSW Economy` worker continues background
  mining, production and regional economy calculations. It now also accepts typed,
  owned Ship Store equipment/skin/missile/drone purchase commands. Purchase
  calculation does not touch UObjects or invoke persistence/UI callbacks.
- **Simulation:** a dedicated `SSW Simulation` worker calculates batched ballistic
  projectile/hull broad-phase candidate masks from copied geometry. UE/legacy
  objects and live containers are never passed to that worker.
- **Visualization/adapters:** Unreal's existing game thread retains Actors,
  UObjects, Slate/UMG, cameras and effects. No rendering thread is created.

## Commands, state and persistence

Ship Store uses asynchronous requests and disables another purchase while one is
pending. Admission and completion both validate authority, current dock/craft,
quote, profile and inventory revision. Successful calculation is only confirmed
after the game-thread adapter saves the wallet, equipment and skin ownership
atomically. A failed save rolls everything back. Receipts prevent double charges.
Closing the panel does not duplicate or transfer ownership of an outstanding
request. Changing dock/craft/authority cancels it without a charge.

Profile identity includes a session epoch and save revision. Loading even the
same slot increments the epoch; mission unloading also invalidates outstanding
work. Worker state replacement never rewinds the live economy clock. Existing
live domains not calculated in a batch are retained by the commit adapter.
Shutdown joins native workers and cancels uncommitted requests. No file save or
UObject access occurs on either native worker.

## Simulation migration boundary

The first worker dispatch begins after native ship and projectile movement. It
can overlap explosion/debris updates, then reaches a same-frame barrier before
hit resolution. Existing shield, subsystem, friendly-fire, damage/death and
projectile ordering remain on their existing path. No extra frame of hit latency
is introduced by design.

Dispatch requires at least 256 hulls, two queries and 32,768 candidate comparisons.
A frame batch contains at most 512 queries and approximately 4 MiB of candidate
masks. Small scenes use the existing collision path. Changed live hull geometry
is conservatively restored to narrow-phase eligibility; changed projectile
segments, layouts or mission epochs use the existing calculation. The worker
never resolves damage or mutates a live Ship.

The migration is intentionally incomplete: full flight/combat/AI execution,
RepairAI adapters, hull sales/rentals/insurance, contracts and remaining market
mutators still need subsequent domain conversion. They remain serialized on the
game thread and invalidate outstanding economy calculations through the existing
revision boundary. Do not describe this build as a full simulation split.

## Mouse capture

Shared ship previews use high-precision relative drag with mouse confinement.
Mouse-up, reset and visual/panel teardown release capture and cursor confinement.
Loss of the held button also ends the drag. Rotation and zoom scales are retained.

## Validation

Build 20261126-193305Z passed the Editor and Windows Development builds with
zero C++ warning/error diagnostics. All 397 regressions passed with zero test
warnings, failures or skips (Saved/Automation/PhaseF-Verified). The two focused
domain tests also passed. Final compiler logs are PhaseF-Editor-Verified.log and
PhaseF-Client-Verified.log. Automated worker tests cover actual
off-game-thread execution, no early wallet/UI changes, stale revision/profile
rejection, save rollback, replayed receipts, deterministic collision parity,
epoch/frame matching, single-consumption mailboxes and teardown while native work
is outstanding. No FPS improvement, packaged manual mouse test or full server
migration is claimed. The user performs cooking/packaging and the T1 check.

The purchase commit additionally compares the original equipment and license
read set against live values, so a loadout/fitting change cannot be overwritten
even if it did not raise a financial revision. Worker unavailability fails the
purchase admission without a charge.

## Marketplace command migration and queue

Build 20261126-201018Z passed Editor and Windows Development compilation with zero C++ warning/error diagnostics. All 404 regressions passed with zero test warnings, failures or skips (Saved/Automation/PhaseF-Market-Verified); all seven focused market/queue tests passed. Logs: PhaseF-Market-Queue-Editor.log and PhaseF-Market-Client.log.

Immediate market trades and limit-order changes now calculate off the game thread. A bounded FIFO provides saved/rejected/cancelled results and pending cancellation. See Phase-F-Market-REA.md for evidence, rules, tests and remaining domain boundaries. Phase F remains open. The user performs the packaged visual/gameplay check.

## Fleet maintenance worker migration

Build 20261126-201707Z passed Editor and Windows Development compilation with zero C++ warning/error diagnostics. All 406 regressions passed with zero test warnings/failures/skips (Saved/Automation/PhaseF-Repair-Verified); both focused repair domain tests passed. Logs: PhaseF-Repair-Editor.log and PhaseF-Repair-Client.log.

Fleet repair assessment, supply debit, work milestones and replacement calculations now run on the economy worker from copied fleet/context/policy data. The shared request queue serializes this work with transactions. Campaign state and deployed hull eligibility are revalidated; stale results do not heal ships. A failed economy save restores supplies and work. Campaign projection remains on the game thread, using the existing durable-work reconciliation if its second save fails.

The adapter wakes on fleet/supply changes and milestones, coalesces changes while a command is pending, and shuts down before the economy coordinator. No live campaign, ship, hangar or UObject reaches the worker. Two new tests cover actual worker isolation, stale fleet/profile/revision rejection, atomic rollback, single-use results, completion, ammunition/grade retention and deployed hull protection.

Remaining Phase F adapter work: hull commerce/insurance, inventory/manufacturing services and contracts, plus scheduled order maintenance. Nexus REA fleet-management research is deferred until after existing phases; supplied installation: C:/Program Files (x86)/Steam/steamapps/common/Nexus The Jupiter Incident.

## Inventory, manufacturing, contract and order-maintenance adapters

Build 20261126-203246Z passed Editor and Windows Development compilation with zero C++ warning/error diagnostics. All 411 automated regressions passed with zero test warnings, failures or skips (Saved/Automation/PhaseF-Ledger-Verified); all five focused ledger tests passed. Logs: PhaseF-Ledger-Editor-Verified.log and PhaseF-Ledger-Client.log.

See Phase-F-Ledger-Adapters.md. The live cargo, blueprint and industry panels now submit immutable requests. Training contract publication/lifecycle, acceptance, bidding, delivery, abandonment and claims calculate on the worker; saved ledger state drives sealed cargo projection. Contract and market deadlines are serviced while their panels are closed. Unchanged maintenance results do not save or replace live state. A native headless fallback without player/UI adapters remains available. Hull commerce and insurance are next; Phase F remains open.

## Ownership and live-scene adapters

Build 20261126-205131Z passed Editor and Windows Development builds with zero C++ warning/error diagnostics and correct own-header/generated-header order. All 417 regressions passed with zero test warnings/failures/skips (Saved/Automation/PhaseF-Owned-Verified); all six focused ownership tests passed. Logs: PhaseF-Owned-Editor-Verified.log and PhaseF-Owned-Client.log.

See Phase-F-Owned-Adapters.md. Live hull commerce, policies/claims, local component fitting, missile-loadout exchange, mining-drone transfers and full station repairs calculate through native ownership commands. Game-thread adapters retain physical reserve transfer, live projection and saved confirmation/rollback. A critical-loss transition barrier runs before mission retirement and shutdown. Compatibility native APIs remain for callers/tests; live UI uses the worker. Skill financial actions are the final identified UI adapter migration.

## Skill adapter and Phase F closeout

See Phase-F-Skills-Adapters.md for the final build/test evidence and remaining
live-scene boundaries. Skill books, advanced books, paid instant training, queue
changes and offline completion now use the native economy queue. Typed commits
merge only the owned skill/economic domain, compare original progress and queue,
and confirm UI results after saving. The five new domain tests bring the full
suite to 422 passing tests, with no test warnings/failures/skips. Both final builds
passed with zero C++ warning/error diagnostics.

The requested EVE review used six shipped Python 2.7 modules and fifteen public
CCP SDE tables. See Phase-F-Economy-REA.md and its pinned hash/schema manifest.
All 219 finished goods have recipe output keys. Follow-up recommendations are
production-chain reachability, multi-output authored recipes, configurable facility
lanes and targeted inventory notifications; they are research findings, not newly
implemented gameplay or a claim of EVE server-code recovery. Nexus remains deferred
until after the existing phases.

## Measured ship-pair extension

Build 20261127-035603Z: both targets and all 434 regressions pass. See Simulation-Thread-Timing.md. Copied ship-pair geometry can run on the simulation worker; it remains opt-in because the measured immediate-wait path is slower than serial. Existing projectile workers remain enabled. Flight physics is the next migration; no FPS improvement is claimed.

## Flight kernel parity checkpoint

Build 20261127-041734Z: both targets and all 438 regressions pass in normal and worker-forced configurations, with zero C++ diagnostics or test warnings/failures/skips. See Flight-Physics-Worker.md. All three flight models and LCA damping have copied-state worker kernels; controllers/RNG/live commits remain serialized. The bridge remains opt-in because immediate per-object handoffs cost more than the original equations. No FPS gain or complete simulation-domain migration is claimed.
