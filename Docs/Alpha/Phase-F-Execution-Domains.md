# Alpha 5 - sprint Phase F execution domains

27 November 2026 (pilot local date). First incremental migration; not a claim
that the entire legacy simulation or every economic transaction is threaded.

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
