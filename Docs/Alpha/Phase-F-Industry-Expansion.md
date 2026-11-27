# Phase F industry extension — 27 November 2026

## Implemented

- Recipe output arrays with legacy single-output compatibility; atomic co-product settlement and combined output-space reservation.
- Authored facility lanes, allowed activities and automated recipe choices. Queue length is independent of running capacity; queued jobs reserve inputs only when starting.
- Independent lane deadlines, fair automated recipe selection, paused offline facilities and persistent cancellation/results.
- Blueprint material/time research and limited-run copying, exposed through the existing Blueprint panel. Ownership, skills, docking, activity, fees and original locks are validated before admission and commit. JSON defines SSW service terms.
- Source-aware price, material, yield and processing-time facts using SSW prices and modifiers.
- Owner/inventory/item/job change payloads with conservative global revision and profile fences retained. Cargo, wallet and industry lists filter unrelated updates.
- Finite bauxite, spodumene, fluorite and Khyber ore sources; crystal separation and Frasco bottling co-products. Existing exclusive Garrison supply is retained.
- Corrected Sintar facility endpoint to Tarsus in facility, galaxy and OOB data. No player saves were reset or deleted.

## Verification boundary

Build 20261127-024521Z: Editor and Windows Development builds passed with zero C++ warning/error diagnostics. All 432 native regressions passed, with zero test warnings, failures or skips (Saved/Automation/PhaseF-Final-Verified). Include-order checks pass for all 32 changed native files. Build-Cook-Deploy -PlanOnly prerequisite validation passed; no package was produced.
The read-only graph audit passes: 21 systems, 223 recipes and 281 enabled assignments, no unreachable products or unseeded cycles. Five audit regression tests pass.
Run `python Tools/Economy/audit_reachability.py` from the repository root to reproduce it.

The graph establishes feasible source/production/buyer routes; it is not a long-running throughput, depletion or flight soak. Targeted callback profiling measures refresh eligibility, not FPS. Global revision guards still reject conflicting snapshots conservatively. No cook, package or live pilot test is claimed by these automated checks.

## Execution domains

Economy calculations run on the native economy worker. UE objects, UI and saves remain on the game thread. The existing simulation worker handles copied projectile collision geometry; full flight/combat/AI is not yet migrated. Subsequent simulation changes require numerical parity and measured critical-path timing, including snapshot, commit and barrier costs.
