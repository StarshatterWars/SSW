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

## Alpha 3 — terrestrial missions and 3D briefings

Start with T3 (Flight School) at Janek Station for takeoff and landing. Follow with T4 (Ground Strike), whose mission DEF names the terrain region, Airbase1 and the strike targets. The legacy StarSystem parser can construct a TerrainRegion, and SimRegion uses it to enter airspace. Unreal already has an ANativeTerrainActor presentation path for an active SimRegion with terrain. Verify which region the live mission resolves and whether it retains its terrain before changing the mission or adding a second terrain system.

1. Launch T3 from the Training screen. Verify Janek Station resolves as airspace and the base and fighters spawn at their authored positions. In T4, also verify the ground target positions.
2. Use a separate temporary Unreal surface level for Janek presentation. Use /Game/Maps/Ground/JanekSurface for staging. It contains the Airbase1 preview and a legacy heightfield mesh at the current 1/100 presentation scale. Verify runway heading and ground contact when binding it into flight. Keep simulation terrain authoritative for altitude, contact, landing and combat. WorldScape can replace the presentation later through the same region binding.
3. Verify takeoff, navigation, recovery and debrief in T3. Verify targeting and strike in T4 with an Ensign pilot. Check ground impact, objective success and failure, and saved completion after reload. Where a mission crosses from orbit to the surface, verify that streaming and the region handoff preserve the active ship and objective state.
4. Use the same region path for Campaign 2–5 terrestrial story missions. Keep named locations available for HUD targeting, and leave objective completion to the mission script and simulation. Verify a representative story mission before closing Alpha 3.

5. Build a Wing Commander-style 3D briefing in the existing briefing flow. Stage the route, ships, locations and objectives in sequence with camera moves and existing briefing narration where available. Keep the briefing text accessible and allow the player to skip directly to mission setup. Verify one space sortie and one terrestrial sortie. This is the current milestone for moving 3D briefings originally listed in A1.2.

Keep the current 1/100 presentation scale and legacy simulation coordinates for this milestone. Alpha 3 remains open pending the live terrestrial checks.

## Alpha 4 — scale conversion

Move the asset and presentation scale conversion here. Start with Falcon and Orion. Preserve legacy simulation coordinates, distances and timing. Verify camera framing, launch and recovery, terrain contact, attachment points and collision alignment before extending the conversion to the fleet.

## Current evidence and limitations

- 2026-11-04: Fleet authoring scaffolds completed for 70 space-entity DEFs: 47 canonical Blueprint assets and 20 native classes added. 42 new HullMesh slots await user mesh assignment; visual fleet completion remains open. Build and 15 automated checks passed. See FleetScaffolds.md.

- 2026-11-04: Turrets verified by the user. Turret verification is accepted as complete; remaining ship/fighter visual coverage remains open. Recheck affected mounts when adding new ships or changing turret code/assets.

- Empty native Mods page implemented in the existing options shell; editor build, options persistence/navigation checks and rendered capture passed. Remaining stages below are open.

- Native session/lobby support exists; combat replication is not yet implemented or verified.
- Initial static inventory is in ShipCoverage.md/json. File presence is not runtime validity, and the ingested design table may override source definitions.
- Existing model, turret and cutscene tests must be rerun on the current build. Passing parser/structure checks will not be described as complete cinematic or flight playback.
- Imported content and user-authored assets must be preserved. Missing source art is recorded as a dependency, not replaced with an unrelated ship.
