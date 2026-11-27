# Simulation worker extension and timing — 27 November 2026

## Scope and default behavior

The worker can now calculate ship-sphere candidate masks alongside projectile masks from immutable copied geometry. The game thread owns all ships, damage, docking, collision response, actors and widgets. No live object pointers cross the queue.

The extension is opt-in: `ssw.Simulation.ThreadedShipPairs 1`. Default is 0 because the measured immediate-wait workload is slower than serial. The existing projectile worker remains enabled. `ssw.Simulation.ThreadedCollision 0` disables dedicated collision batches for broader A/B profiling; it does not disable all existing task-graph work.

Masks cover 256–2048 hulls (at most 4 MiB for ship pairs), with the existing separate projectile-mask budget. Each frame validates epoch, layout, object IDs, positions and radii before using ship-pair rejections. Any collision response, including debris or asteroids, invalidates the ship mask for the rest of the pass. Changed/unknown geometry falls back to the established path. Queue teardown joins native work.

## Measurements

Deterministic Development-build CPU microbenchmark on 13th Gen Intel(R) Core(TM) i9-13900H. 21 interleaved samples after three warm-ups per workload; 32 projectile queries. No client compilation was running during the focused measurement. Tests assert candidate parity, not hardware-dependent speed.

| Hulls | Serial median ms | Worker + immediate barrier + consume ms | Worker calculation ms | Consume ms |
|---:|---:|---:|---:|---:|
| 256 | 0.199 | 0.350 | 0.228 | 0.107 |
| 512 | 0.688 | 1.236 | 0.794 | 0.380 |
| 1024 | 2.433 | 4.414 | 2.942 | 1.401 |

The first implementation validated geometry per pair and measured 3.779 ms at 512 hulls versus 0.646 ms serial. Validation was moved to one pass, reducing the measured worker path to the values above, but it still does not establish a net frame-time benefit. It is intentionally not enabled by default.

These are kernel/queue timings, **not FPS or a live T1 benchmark**. They include copied input, queue submission, immediate wait and candidate consumption; they exclude actual region object projection, collision eligibility/narrow phase, GPU work and real overlap with explosions/debris. Real overlap could hide worker cost; this benchmark does not invent or assume it. Full-suite reruns also exercise the benchmark, but focused isolated results above are the comparison evidence.

## Next migration boundary

Flight integration currently calls directors within substeps, uses shared random vibration and updates live camera/physical state. Moving Ship::ExecFrame wholesale would race those dependencies. The next useful step is a plain flight-state snapshot and deterministic arithmetic kernel, retaining control sampling, ordered AI decisions and live commit on the game thread until parity tests prove them. The complete simulation is not yet on its own thread.

For a live comparison, use the same busy scene/save, hardware and graphics settings, warm shaders, disable VSync/frame caps, and capture equal-duration CPU/frame/GPU traces with the new ship-pair switch off/on. Compare GameThread critical-path, barrier time, average/p95/p99 frame time and GPU time. Do not extrapolate these microseconds into an FPS percentage.

## Verification

Build 20261127-035603Z: Editor and Windows Development builds passed without C++ diagnostics. All 434 native regressions passed with zero test warnings, failures or skips (Saved/Automation/Simulation-Checkpoint-Final). All 29 focused domain tests passed. Include-order checks and Build-Cook-Deploy -PlanOnly prerequisite validation passed. The industry checkpoint remains f3d9398f2 and is independently verified. No cook/package/deployment or user save modification was performed.
