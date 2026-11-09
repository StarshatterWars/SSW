# Starshatter Wars — Alpha completion roadmap

Updated 2026-11-04. Branch abc. This records the requested scope; unchecked work is not declared complete. Preserve legacy simulation coordinates, AI, damage, energy, mission progression and dialogue unless an explicit requirement changes them. Do not modify Starshatter.cpp/.h. Deliver complete replacement files and assets in stage ZIPs.

## Alpha Phase 1 — completion stages

| Stage | Scope | Acceptance gate |
|---|---|---|
| A1.1 Fleet coverage and turrets | Inventory all remaining space ships/fighters and their source models; import missing visuals; native/BP class, hull, cockpit, gear, propulsion, weapons/hardpoints, turret bases and guns. Ground structures tracked for Phase 2. | Every space design resolves to the correct visible actor at legacy scale/orientation; all authored mounts present; every barrel fires from its muzzle; aim/range/timing/damage verified. No placeholders counted as completed models. |
| A1.2 Campaign cinematics and moving briefings | Verify every Campaign 1–5 cutscene; legacy entry flow and loading gate; dialogue/VO, camera tracks, captions, environments, model scale; animated 3D mission briefing staging. | Every scene parses, required assets resolve, playback starts after readiness and ends/skips into the correct mission/hub; 3D briefings depict the selected mission and clean up actors/captures. |
| A1.3 System/sector map and travel | Improve sector view with a spatial system-map presentation inspired by Star Citizen: hierarchy, readable orbit/route geometry, pan/zoom/focus and selection. EVE-inspired Farcaster activation/transit effects; integrate installed Procedural Vortex Generator for quantum tunnel. | Correct legacy positions, active campaign selection rules and route destinations; effects begin/end at simulation transitions, survive interruption and do not change travel timing or coordinates; scalable quality settings. |
| A1.4 Full multiplayer missions | Extend existing native create/join/leave staging to mission selection, readiness, server-authoritative simulation, player ownership/input, replicated ships/weapons/damage/objectives, travel/reconnect/disconnect and results. | Two separate game processes complete a supported multiplayer mission with matching state, damage and results; test join/leave, late join policy, failure recovery and latency. Session discovery alone does not pass. |
| A1.5 UI and integrated acceptance | Empty Mods screen using modern options shell. Campaign 1 missions 2 and 3 full playthroughs. Regress T1 launch/ILS/docking, IFF, weapons and debrief persistence. | T2/T3 objectives, drones, combat, success, debrief, save/reload and T3 Ensign promotion verified. No blocking regressions or unverified critical assets. |

The earlier unresolved physical-waypoint render capture remains an open regression item. Green geometry and concise NAV labels exist; live-flight visibility is not yet verified.

## Alpha Phase 2 — performance, space and surface

1. Measure CPU/game/render/GPU time, memory, streaming, simulation costs and scene captures with repeatable flights before optimization. Establish target hardware/resolution and budgets from those measurements. Reduce observed bottlenecks while preserving simulation results; add scalable visual quality and regression captures.
2. Space beauty pass: stars, nebulae, planets/moons, lighting, atmosphere, exposure and consistent ship materials; correct celestial textures and scale.
3. Planet-to-ground transitions for ground/group missions moved to the Alpha 3 terrestrial closeout.
4. Surface mission areas moved to the Alpha 3 terrestrial closeout.
5. New splash screen and startup/loading presentation using game assets.
6. Convert selected Campaign 2–5 cutscene-only missions into playable story missions. Keep existing dialogue/story continuity and campaign ordering; each needs objectives, fail/success conditions, actors, scripting and save progression. The first batch contains 28 story missions across Campaigns 2–5. Live playthroughs remain to be verified.

7. Full in-game asset coverage. Finish faction hulls and relay station, fighter cockpits and landing gear, and DEF-referenced debris and hulks. The tactical reference must show each assembled ship with its authored weapon mounts, turret bases, running lights, engine and thruster ports, at the current legacy scale. Inspect orientation and validate combat effects in a live mission before marking an asset complete.

## Alpha 2 closeout — 2026-11-08

The Alpha 2 closeout includes the following checks:

1. Deep optimization. Profile repeatable in-game runs, prioritize Unreal rendering, Niagara, scene captures and frame pacing, and verify improvements against gameplay regression checks.
2. Playable story missions. Select mission-capable cutscenes from Campaigns 2–5, preserve their dialogue and story, and build objectives, success and failure conditions, debriefs and saved story progress. Keep dynamic missions under Campaigns.
3. Run T3 (Flight School) at Janek Station as the ground-flight smoke test before closing Alpha 2. Verify the terrain and airbase, flight, and debrief.

The editor ingestion comparison is a migration checkpoint. Keep startup ingestion active until the staged tables have been reviewed. Deferred scope above remains tracked separately from these two closeout workstreams.

## Alpha 3 — terrestrial missions, 3D briefings and account sign-in

**Closed on 2026-11-09 at the project owner's direction.**

The live T3 rerun confirmed a successful Janek landing with the wheels sitting on the runway. T4 reached the cockpit in live testing and passed its mission-load audit. A full live T4 run through debrief and save reload, and a representative terrestrial story run, remain follow-up verification. Login caching is deferred.

3D briefing and replay functional work is closed after 20 automated Alpha 3 closeout checks passed. This includes briefing mission selection, standalone isolation, campaign save round-trip, and mission clock lifecycle. Live Story replay and audiovisual polish were outside this closeout run.

Account sign-in and saves are closed at the project owner's direction. Look and feel changes are deferred to the final alpha beauty and usability passes.

Keep the current 1/100 presentation scale and legacy simulation coordinates for this milestone.

## Alpha 4 — optimization, cleanup, bug fixes and scale conversion

**Open as of 2026-11-09.** Begin with a major optimization, cleanup and bug-fixing pass. Profile representative missions, prioritize measured bottlenecks and gameplay defects, and rerun the affected workflows before starting scale conversion.

After stabilization, start asset and presentation scale conversion with Falcon and Orion. Preserve legacy simulation coordinates, distances and timing. Verify camera framing, launch and recovery, terrain contact, attachment points and collision alignment before extending the conversion to the fleet.

## Alpha 5 — multiplayer

Phase 5 is a project-wide refactor to integrate Unreal Engine multiplayer across nearly every gameplay system. Extend the existing A1.4 session and lobby work to create and join sessions through EOS and Steam. Establish server authority, player ownership, input routing and replication across mission flow, world and actor lifecycles, flight controls, ships, weapons, damage, AI, objectives, HUD state, debrief and results. Cover readiness, mission travel, disconnects and reconnects. Verify standalone play as each system moves to the multiplayer architecture.

Verify create and join with two separate game processes on each provider. Complete a supported multiplayer mission and compare state and results on both clients. Exercise join failures, disconnect recovery and latency.


## Current evidence and limitations

- 2026-11-08: T3 Flight School opened its briefing and launched in the isolated Ensign build. The runtime could not resolve Janek Station and failed to spawn Alpha, leaving the HUD at No Player Ship. At the time of that run, terrain-region registration and a passing live T3 retest were still pending. Alpha Phase 2 was subsequently closed by the project owner. The same build displayed 10 Highland story missions.

- 2026-11-04: Fleet authoring scaffolds completed for 70 space-entity DEFs: 47 canonical Blueprint assets and 20 native classes added. 42 new HullMesh slots await user mesh assignment; visual fleet completion remains open. Build and 15 automated checks passed. See FleetScaffolds.md.

- 2026-11-04: Turrets verified by the user. Turret verification is accepted as complete; remaining ship/fighter visual coverage remains open. Recheck affected mounts when adding new ships or changing turret code/assets.

- Empty native Mods page implemented in the existing options shell; editor build, options persistence/navigation checks and rendered capture passed. Remaining stages below are open.

- Native session/lobby support exists; combat replication is not yet implemented or verified.
- Initial static inventory is in ShipCoverage.md/json. File presence is not runtime validity, and the ingested design table may override source definitions.
- Existing model, turret and cutscene tests must be rerun on the current build. Passing parser/structure checks will not be described as complete cinematic or flight playback.
- Imported content and user-authored assets must be preserved. Missing source art is recorded as a dependency, not replaced with an unrelated ship.
