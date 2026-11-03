# Alpha completion baseline — 2026-11-04

## Evidence

- Editor source build: passed.
- 103 ingested ship-design rows inspected, including ground objects, hulks, stations and drones.
- 30 conventional ship Blueprint assets found. 23 have a HullMesh on their class default object; 7 require spawned-actor inspection because Blueprint construction/overrides can supply meshes.
- TargetDrone has a known runtime fallback, so absence at its legacy model path is not treated as a missing visual.
- 26 named turret Blueprint assets have gun meshes. The generic BP_Turret parent has no specific mesh, as expected.
- Initial automated baseline: 13 passed, 1 failed on duplicate Mods page count. Duplicate page corrected; focused RHI rerun passed both options tests, with rendered Mods page inspected. All 14 distinct baseline checks now have passing results across those runs.
- Turret attachment/aim and cutscene presentation/loading tests passed in that baseline. Full cinematics and T2/T3 flight/combat playthroughs remain pending.

## Open work

See AlphaCompletionRoadmap.md for the requested Alpha Phase 1 and Phase 2 scope. Ship inventory is evidence for scheduling imports, not a completion claim. Existing space art has not been replaced or rescaled. No gameplay replication has been added in this starter stage.
