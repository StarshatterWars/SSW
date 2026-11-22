# Starshatter Wars — Alpha completion roadmap

Updated 2026-11-20. Branch abc. This records the requested scope; unchecked work is not declared complete. Preserve legacy simulation coordinates, AI, damage, energy, mission progression and dialogue unless an explicit requirement changes them. Do not modify Starshatter.cpp/.h. Deliver complete replacement files and assets in stage ZIPs.

## Alpha Phase 1 — completion stages

| Stage | Scope | Acceptance gate |
|---|---|---|
| A1.1 Fleet coverage and turrets | Inventory all remaining space ships/fighters and their source models; import missing visuals; native/BP class, hull, cockpit, gear, propulsion, weapons/hardpoints, turret bases and guns. Ground structures tracked for Phase 2. | Every space design resolves to the correct visible actor at legacy scale/orientation; all authored mounts present; every barrel fires from its muzzle; aim/range/timing/damage verified. No placeholders counted as completed models. |
| A1.2 Campaign cinematics and moving briefings | Verify every Campaign 1–5 cutscene; legacy entry flow and loading gate; dialogue/VO, camera tracks, captions, environments, model scale; animated 3D mission briefing staging. | Every scene parses, required assets resolve, playback starts after readiness and ends/skips into the correct mission/hub; 3D briefings depict the selected mission and clean up actors/captures. |
| A1.3 System/sector map and travel | Improve sector view with a spatial system-map presentation inspired by Star Citizen: hierarchy, readable orbit/route geometry, pan/zoom/focus and selection. EVE-inspired Farcaster activation/transit effects; integrate installed Procedural Vortex Generator for quantum tunnel. | Correct legacy positions, active campaign selection rules and route destinations; effects begin/end at simulation transitions, survive interruption and do not change travel timing or coordinates; scalable quality settings. |
| A1.4 Multiplayer foundation | Native session and lobby work carried forward into Alpha 6. | See Alpha 6 for multiplayer acceptance. |
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

## Alpha 4 — gameplay, economy, optimization and legacy completeness

**Closed after owner verification on 2026-11-20. Renumbered by the project owner on 2026-11-20.** This is the current additions and verification phase, including the documented EVE comparison work. Earlier Phase 5 build logs retain their historical names; they are evidence for this current phase, not proof of server completion.

Scope includes native mining, production, transport, market and shipyard behavior; inventory and wallet consistency; contracts, skills and player progression; agents and campaign handoffs; faction missions and training; JSON migrations; launcher installation and recovery; UI, propulsion, carrier travel and planet regressions. Use recovered client evidence and public data to identify gaps, while retaining SSW rules unless a change has been authorized. Distinguish client behavior from inferred server behavior.

Current closeout includes direct JSON OOB loading and parity, faction training and campaign-specific mission checks, Marakan Campaign 5 against the non-playable Zolon threat, Orion lights and surface transit, native skins and full Janek WorldScape material checks. These remain subject to compilation and automated verification; passing data tests does not replace a live visual flight check.

Acceptance requires successful compilation, focused tests for each change and the broad offline regression suite. The owner performs cooking and packaging, then live installation and flight checks. Do not mark this phase complete while required failures remain. Multiplayer, Mod Editor and cinematic MP4 production are deferred.

### Background mineral deliveries

Survey each system using the same deterministic asteroid deposits the player can scan. A system requests a mineral when its usable local supply falls below the amount it needs. Match the request to a reachable system with the mineral in its asteroid fields and enough available supply to ship. Track each outstanding request and reserve the shipment once so repeated updates cannot dispatch duplicate freighters.

Spawn the freighter at the source system transfer station with the requested mineral, a quantity within the hull capacity, and a route through the actual linked farcaster pairs. Record the destination transfer station in its manifest. Credit the delivery only after the freighter docks, then remove the ship. The same station can originate a new outbound shipment from minerals supplied by its own fields. Campaign traffic runs in the background. Free play can expose the same supply and delivery records in a later alpha.

Verify a full trip with at least two farcaster hops, docking, a single delivery credit, and despawn. Include an unreachable supplier case to confirm the request remains pending.

### Next addition — standalone Windows launcher

The launcher signs the player in through Epic Account Portal, checks the installed game version, applies required patches, and launches SSW automatically when sign-in and patching are complete. The game restores the authenticated session and selects the existing account’s pilot save without prompting for a second login.

Distribute patches from a signed release manifest over HTTPS. Download only changed files, verify them before replacing the installed build, and preserve the prior working build and pilot saves if an update is interrupted or fails. The release host and signing key still need to be selected. Use Epic’s supported authentication handoff and keep player credentials out of command-line arguments and logs. [Epic launcher integration](https://dev.epicgames.com/docs/epic-online-services/accounts-and-social/eos-epic-account-services/auth-interface/integrate-a-third-party-launcher-with-egs)

## Alpha 5 — local dedicated FractalVerse server

**Authorized to begin after the current pre-Phase 5 UI and direct JSON ingestion changes pass compilation and automated tests. Renumbered 2026-11-20. Not yet completed as a server integration phase.** StarServer remains the multiplayer match server. FractalVerse is a separate persistent universe server, initially local, with a future dedicated host using the same authority rules.

- Native system records exist independently of UE levels: systems, stars, planets, moons, fixed stations, deposits, NPC craft and routes. Nearby presentation uses those records; unloading presentation must not reset state or destroy cargo.
- Mining, manufacturing, transport, NPC market budgets, supply/demand, price history and shipyard output continue in unloaded systems. T1 is a temporary test fixture and must not be required by the universe.
- FractalVerse is to own authoritative ships, inventories, wallet transactions, contracts, production, skills and universe time. Clients send validated commands and render confirmed snapshots. A local client must never run a competing economy.
- Persistent IDs, transaction receipts, coherent sequential checkpoints and normal-exit saves must prevent duplicate deliveries or payouts and recover unfinished journeys.
- Keplerian native orbits use authored masses, eccentricities and inclinations. Radial clearance includes body sizes and moon envelopes. Maps must sample the same orbits.
- Shipyards exist only at selected capital/strategic worlds, accept material deliveries and build stocked hulls. Purchase and hangar fulfillment must consume actual stock and preserve rank, empire, skill and docking gates.
- Multiplayer and Mod Editor access are temporarily disabled while the authoritative universe work is verified.

Acceptance requires local FractalVerse/client operation, rejection of client-side state changes, unloaded-system mining-to-manufacturing-to-delivery, ship purchase fulfillment, failure/retry and restart recovery, and passing compilation and automated regression checks. Initial native model tests alone do not complete this phase. Arena multiplayer follows in Phase 6; full scale remains Phase 7. Future unnumbered Free Verse work remains deferred.

## Alpha 6 — arena multiplayer

**Deferred until after the local dedicated server phase. Renumbered 2026-11-20.** Existing multiplayer work is retained; its outstanding acceptance checks are not waived. StarServer remains the arena match server, separate from FractalVerse.

Integrate Unreal Engine multiplayer across gameplay, building on the native session and lobby work. Use the legacy simulation on the authoritative host or server, with UE handling player ownership, input, and replication. Cover the galaxy and orbital state, flight controls, ships, weapons, damage, AI, objectives, mission flow, HUD, debrief, chat, and results. Verify standalone play as each system moves to the multiplayer architecture.

Hawk Match has launched in the Borova sector with two local LAN processes. Both players received fighters, steered, applied throttle, and saw live speed on their mini-HUDs. Matching hit counts and hull damage after a landed shot are still unverified. The propulsion and cockpit presentation fixes passed build and attachment checks and await a live rerun.

StarServer built as a Windows dedicated server on the UE 5.7.4 source engine. SSW launched it, registered a four-slot Hawk Match session in EOS Dev, and connected one pilot to the Borova lobby. The dedicated two-pilot run is deferred until a second Epic account is available.

Remaining arena multiplayer verification includes a supported match with matching combat state and results on both clients, EOS and Steam create/join, reconnects, mission travel, and match-end server and EOS session cleanup.

## Alpha 7 — full-scale conversion

Start the asset and presentation conversion with Falcon and Orion. Preserve legacy simulation distances and timing. Verify camera framing, launch and recovery, terrain contact, attachment points, and collision alignment before extending full scale to the fleet and mission environments.


## Alpha 8 — Free Verse ownership and corporations

**Planned; added at the owner’s request on 2026-11-20.** Player-owned stations and production facilities need acquisition, persistent ownership, upkeep, inventories, docking and service permissions, and logistics endpoints. Player corporations need membership, roles, shared wallets, hangars and station assets, transaction history and access controls. Corporations are separate from career guilds. All mutations must be server-authoritative. Expand docked production and market management here.

Acceptance includes permission rejection, ownership and inventory persistence, shared-account audit trails, production and freight at owned facilities, and restart recovery without duplicated stock or credits.

## Alpha 9 — walkable hangars and station spaces

**Planned after Alpha 7 full-scale resizing.** Begin with one hangar and fighter. Add walking, boarding/disembarking and station service access; verify human/ship scale, collision, cameras, spawn and exit safety before extending to carriers and stations. Walking must not bypass docking, ownership or service permissions.

## Faction content and cinematic scheduling

Faction training and the documented Marakan and Dantari mission additions are included in the active Phase 4 closeout, subject to progression and regression checks. Zolons remain non-playable. Do not declare faction content complete based only on file presence.

MP4 production and cinematic polish are a separate deferred workstream, with no rendering required for this build. Mod Editor remains deferred. No additional phase number is assigned to these workstreams here.

## Phase 4 release status — 2026-11-20

The owner completed cooking and packaging. Build 20261120-053210Z is archived as Build/Windows.zip with the existing StarServer, custom icon and anti-cheat bootstrapper. All 145 archived files were verified against the signed manifest, and the verified release was published to the launcher feed. The owner confirmed the update and verified Phase 4 on 2026-11-20; Phase 4 is closed.

Pre-Phase 5 work is direct JSON ingestion into native records with parity and regression testing. Retain the agreed campaign portions and galaxy/environment conversion for later. This migration is separate from Phase 5 local dedicated FractalVerse authority.

## Phase 4 build checkpoint — 2026-11-20

Work included in this build:

- Carrier fighter sales use eligible OOB squadrons, finite stock and the training Stormhawk price; T1 is Carrier Qualifications.
- Live Orion-to-Janek surface training transit, recovered craft restaging, hull-occluded Orion navigation lights, full Janek WorldScape terrain material and 23 native UE skin materials.
- Direct native JSON OOB loading; Marakan and Dantari seven-lesson training catalogs; expanded Marakan Campaigns 3/4 missions and chapter jobs; Campaign 5's non-playable Zolon opponents.
- Independent-job state, current rank and campaign-system checks; refresh/start commands cannot duplicate or reset running work.
- C++ beam classes have native Niagara defaults. Gun muzzle flashes are 25% smaller. Seven stock missile classes use nozzle-fitted native Niagara exhaust driven by simulation thrust, release, pause and lifetime.
- Flight Ops permits player commands only while piloting that carrier; other pilots retain read-only roster inspection. Autonomous simulation operations are unchanged.
- Production recipe eligibility follows existing regional production and exclusive supply. Previously 206 recipes had no supported facility; all 221 now have an installation route. Primary background recipes and SSW balance are retained.
- Launcher 1.0.15 Store prototype, separate News view and supported Epic authorization. Purchasing remains inactive.

The cook gate passed: editor and Windows Development game compilation succeeded, and the final rendering-enabled regression passed all 314 tests (191 successes, 123 successes with warnings, zero failed or skipped). Launcher checks passed 18/18. The final menu placement passed a focused render rerun after the full suite. The content audit covers 266 cargo entries, 221 recipes, 52 facilities, 221 blueprints and 153 skills, with no broken links or missing recipe installation sites. Ready for the owner to cook and package to Build/Windows. Packaged visual checks and full faction playthroughs remain owner verification; automated data/loading checks do not declare them complete.

Ships, weapons and systems remain JSON-sourced but use runtime DataTable bridges. Campaign collections, galaxy/environment/regions and separate force-summary OOB tables remain. Direct faction combat-group JSON loading does not remove that separate summary table. See Saved/RuntimeIngestionAudit-20261120.json.

The launcher and game share Alpha 4 release metadata, warning text and game build labels. Phase 4 was subsequently verified and closed by the owner on 2026-11-20. Alpha 5 now uses shared release metadata.

The simulation remains the source of gameplay state. Unreal weapon/engine effects are visualization and do not own hits, damage, inventories, thrust or timing.

## Current evidence and limitations

- 2026-11-08: T3 Flight School opened its briefing and launched in the isolated Ensign build. The runtime could not resolve Janek Station and failed to spawn Alpha, leaving the HUD at No Player Ship. At the time of that run, terrain-region registration and a passing live T3 retest were still pending. Alpha Phase 2 was subsequently closed by the project owner. The same build displayed 10 Highland story missions.

- 2026-11-04: Fleet authoring scaffolds completed for 70 space-entity DEFs: 47 canonical Blueprint assets and 20 native classes added. 42 new HullMesh slots await user mesh assignment; visual fleet completion remains open. Build and 15 automated checks passed. See FleetScaffolds.md.

- 2026-11-04: Turrets verified by the user. Turret verification is accepted as complete; remaining ship/fighter visual coverage remains open. Recheck affected mounts when adding new ships or changing turret code/assets.

- Empty native Mods page implemented in the existing options shell; editor build, options persistence/navigation checks and rendered capture passed. Remaining stages below are open.

- Initial static inventory is in ShipCoverage.md/json. File presence is not runtime validity, and the ingested design table may override source definitions.
- Existing model, turret and cutscene tests must be rerun on the current build. Passing parser/structure checks will not be described as complete cinematic or flight playback.
- Imported content and user-authored assets must be preserved. Missing source art is recorded as a dependency, not replaced with an unrelated ship.

## Alpha 5 preparation checkpoint — 2026-11-20

Phase 4 is verified and closed. Weapons and system/component designs now load JSON directly into native records in the normal boot path. Editor authoring and parity helpers retain their table interfaces. Ships still use their existing bridge; campaign and galaxy remain unchanged. The native comparison preserves all reflected fields, five component grades, names containing spaces and Recon Pod’s authored damage sentinel. Failed catalog reads preserve prior records.

The in-flight Player Logbook has separate Logbook and Chat Macros tabs. Tactical Reference groups stations/platforms and transports/shuttles with filters and omits their redundant type prefixes. Editor compilation and 315 offline tests passed (192 success, 123 with warnings, zero failed or skipped). Windows client compilation is tracked separately in Saved/Logs/PrePhase5NativeCatalogClientBuild.log; owner cooking and packaging remain required.

Launcher 1.0.17 adds an offline Roadmap tab from Launcher/roadmap.json, with 24 passing launcher checks and a rendered layout review. Alpha 8 covers player-owned stations and corporations; Alpha 9 covers walkable hangars after Alpha 7 resizing. These are planned features, not completed functionality. The current launcher/game stage label is Alpha 5; the available signed installation remains the last verified Alpha 4 package until the next owner package is published.


### Future docked presentation

Replace empty carrier and orbital-station bay entry views with a shared ready-room. Preserve the live docked session, agent assignments, station services and owned-craft locations. Ground bases can use the same room with site-specific scenery. Deferred until after the current JSON and effects regression build.


### AI-operated crew shuttle boarding

Use the quantum-equipped Zodiac Crew Shuttle as NPC transport between carrier/station docks and planetary bases. The pilot boards through a docked Board action, AI handles departure, quantum travel and surface approach, and the pilot disembarks at the required mission destination. Scheduled transfers are AI-operated. A purchased crew shuttle may also be player-flown for transport missions, subject to ownership, rank and mission gates. Retain the tested T3 surface-transfer fallback until the boarding, route, arrival and persistence flow is implemented and verified. Return transport after T4 follows the same service.


### Cosmetic hull appearances

EVE reference: https://support.eveonline.com/hc/en-us/articles/203280701-Ship-SKINs and https://developers.eveonline.com/docs/guides/skinr/ . EVE uses character-owned compatible-hull skin licenses and material/pattern definitions. SSW already supports native per-hull material-slot replacement through authored UE material instances and JSON ship skins.

Next pass: a JSON-only skin catalog (stable ID, readable name, compatible hulls, material-instance slot mappings and preview); pilot-owned unlock IDs; selected skin ID per owned hull; a Hangar Appearance selector with preview/apply/reset; persistence and later server validation of ownership/hull compatibility. NPC and authored mission skins continue using their existing skin names. Cosmetics never alter ship stats, fitted hardware or simulation damage. Preserve original materials for reset and do not modify shared master assets at runtime. Marketplace licensing can follow after the selection and save flow is verified.
