# Flight physics worker — 27 November 2026

## Boundary

Inertial, atmospheric and arcade equations, orientation, gravity, drag, lift, flight-path angles and airborne LCA lateral damping have native copied-state implementations. The LCA damping remains after the original thrust/gravity/drag operations, with the same axis order, thresholds and float casts. It is part of the same job, never a separate dispatch.

The adapter retains controller callbacks, CRT random draws, object lifetime and live commits on the game thread. Whole-frame integration is used where there are no subframe callbacks. A subframe controller receives the committed result before its next callback; no input map, control law, timestep or extra frame of latency is introduced. The mathematical Camera is an owned SSW matrix/position value, not a UE camera. No actor, UObject, director, live Physical pointer, reflection, RNG or file IO reaches the worker.

The simulation worker has typed collision and flight mailboxes, one bounded outstanding request, a same-call barrier and joined teardown. A collision receiver cannot consume a flight response. A busy worker retains the serial native fallback. UE actors/effects/cameras/widgets remain on the game thread.

## Verification

Four focused tests cover all three modes, normal/subframe controllers, straight flight, actual airborne LCA damping, powered/unpowered axes, per-callback observed position/timestep, final numerical state, life, caller-thread RNG order, typed mailbox isolation and shutdown. Build 20261127-041734Z: Editor and Windows Development builds passed with zero C++ diagnostics. All 438 regressions passed with zero test warnings/failures/skips in both configurations (Saved/Automation/Flight-Physics-Full-Default and Flight-Physics-Full-Worker-Verified). Include-order checks pass for all nine changed native files. The initial worker run had a slow external DDC warning during concurrent compilation; the sequential verification run above is clean. No real pilot save or packaged game was run.

## Measurement and default

The production bridge benchmark uses 21 interleaved samples after three warm-ups, on the same i9-13900H host as the collision benchmark. No compilation ran alongside the isolated timing pass. Bodies have no director; controlled substep behavior is tested separately. All three models were measured at 1/64/256 bodies and nominal 1/8 substeps.

| Model | Bodies | Substeps | Serial median ms | Worker bridge median ms |
|---|---:|---:|---:|---:|
| Inertial | 1 | 1 | 0.0006 | 0.0117 |
| Inertial | 1 | 8 | 0.0016 | 0.0128 |
| Inertial | 64 | 1 | 0.0181 | 0.8119 |
| Inertial | 64 | 8 | 0.0841 | 1.2716 |
| Inertial | 256 | 1 | 0.0714 | 5.3631 |
| Inertial | 256 | 8 | 0.3313 | 5.6081 |
| Atmospheric | 1 | 1 | 0.0008 | 0.0204 |
| Atmospheric | 1 | 8 | 0.0027 | 0.0226 |
| Atmospheric | 64 | 1 | 0.0174 | 1.2916 |
| Atmospheric | 64 | 8 | 0.1336 | 1.5433 |
| Atmospheric | 256 | 1 | 0.0670 | 5.3611 |
| Atmospheric | 256 | 8 | 0.5318 | 5.6181 |
| Arcade | 1 | 1 | 0.0007 | 0.0204 |
| Arcade | 1 | 8 | 0.0016 | 0.0130 |
| Arcade | 64 | 1 | 0.0111 | 0.7881 |
| Arcade | 64 | 8 | 0.0830 | 1.4232 |
| Arcade | 256 | 1 | 0.0427 | 5.5183 |
| Arcade | 256 | 8 | 0.3304 | 5.5117 |

Evidence: Saved/Analysis/FlightPhysicsTiming-Isolated.json and Saved/Logs/Flight-Physics-Parity-Verified.log. These are actual Physical adapter CPU timings, including per-object handoff and wait, **not rendered FPS**. Input copying, scheduling and per-object waits outweigh these inexpensive kernels. The larger simulation split is not a demonstrated speedup yet.

`ssw.Simulation.ThreadedFlight` therefore defaults to 0. Use `-SSWThreadedFlight` at launch, or the CVar set to 1, for the parity-verified worker bridge. The command-line option sets the initial CVar value, allowing tests/runtime toggles to select both paths. Existing flight remains the safe default until useful batching/overlap is proven.

## Remaining migration

To remove the per-object and subframe barriers, controllers need owned world-query inputs and explicit output commands. Their observation order, target/neighbor state and random sequence must be preserved before batching ships. AI decisions, systems/weapons, narrow-phase damage, docking and UE projections are not moved by this change. The original Physical equations remain available as the independent parity reference.
