# Starshatter Wars Alpha 2 legacy parity audit

**2026-11-08 · branch abc**

The core single-player simulation is represented in SSW. The largest ingestion migration risk is the difference between the checked-in tables and the DEF-backed rows rebuilt at startup. Several integrated scenarios still need live verification before declaring full behavioral parity.

## Scope and evidence

Compared the local Stars45 source at `D:/GameDev/starshatter-open-master/Stars45` with the active SSW code. Screen redesign, Unreal asset and engine substitutions, mods and multiplayer are outside this audit. DEF ingestion is intentionally Unreal-specific. Its acceptance test is whether the data required by the simulation survives into the tables and runtime, regardless of the import implementation.

The source inventory matched 132 legacy implementation filenames, including the renamed `System`, `Component` and `Shot` equivalents. The 63 unmatched filenames include known exclusions and renamed replacements such as `FlightComputer`, `SimContact`, `SimElement` and the Unreal player subsystem. Filename and method counts guided inspection. They do not establish behavioral equivalence.

The current build passed 46 targeted editor automation checks and the separate 17-check fleet and presentation suite. A D3D12 standalone automation run passed with fresh DEF rows. A diagnostic run against the saved ship table exposed stale cached rows. Normal startup rebuilds the in-memory registry from DEFs. The corrected D3D12 standalone test used the boot-populated registry and passed for all six Zolons, including the four shield hits and the boot version label. These tests exercise runtime C++ in fixtures. They do not constitute a manual mission flight or a visual inspection of every muzzle and shield.

## Confirmed findings

| Priority | Finding | Evidence and impact |
|---|---|---|
| P1 | Saved ship data can lag its DEF | `StarshatterShipDesignSubsystem::LoadShipDesignTable` registers active table rows. All six Zolons instantiated with six thruster effects and three lights from current DEFs. With the disk `DT_ShipDesign` read directly, bypassing startup ingestion, all six failed the running-light and runtime-thruster assertions. The startup log confirmed six thruster ports per Zolon in the runtime registry. The four capital shield meshes resolved through the shield model aliases in the shield fixture. The two fighters have no DEF shield. |
| P1 | System components were disconnected from the Unreal ingestion path | Stars45 populates the SystemDesign catalog from sys.def and copies components onto each named ship system. SSW had the parser and legacy maintenance machinery, but it did not publish a populated catalog or bind most systems in the registry conversion. This build publishes parsed or table-backed component designs and binds the explicit design key for power, drives, thrusters, shields, sensors, computers, travel systems, nav systems, lights, weapons, gear and flight decks. Per-ship component copies survive catalog refresh. Orion reactor damage, repair time, replacement time and spare consumption passed. |
| P2 | Two importer limits exceed legacy/runtime capacity | The Farcaster parser accepts eight approach points and the landing-gear parser accepts eight items. Both legacy and current runtime classes have capacity four. Flight-deck approach and weapon muzzle capacity are eight in both. Ingestion should report or reject overflow consistently before saving. This audit did not identify a current mission failing because of these limits. |
| P2 | Exact system-design key joins can silently leave systems without component rows | Legacy looks up an explicit design key. Some current DEF labels differ from available component-template names, such as Orion shield design `Grav` versus sys.def `Grav Shield`. A functioning shield can therefore have no named maintenance components. Report unresolved keys and distinguish deliberate systems without templates from aliases that should map to legacy designs. |
| P2 | Terrain parity is only partly verified | Atmospheric handling, terrain height loading, ground-region scheduling and native terrain presentation are present. The automated checks cover representative height data and flight-model cases. The full Janek surface combat sequence and space-to-surface transition are still open. WorldScape is deferred. |
| P2 | Content and end-to-end acceptance remain incomplete | Campaign 1 catalog checks are green. The suite does not fly every dynamic mission or play all Campaign 1–5 cutscenes through their startup, skip and return paths. Campaign 2–5 playable story conversions are an expansion item. |

## System-by-system assessment

| Area | What is in SSW | Evidence or remaining acceptance |
|---|---|---|
| Simulation time and regions | Simulation and mission clocks, time compression, contact decay, region ownership and handoff | Clock/contact and region tests passed. Mixed long-duration runs with transitions and time compression still need an integrated replay. |
| Flight model and FLCS | Space, atmospheric and arcade flight, control laws, propulsion, translation and augmenter | Fixed-step, atmospheric and arcade replay fixtures passed. Live mixed flight, fuel and speed checks remain useful after design-table promotion. |
| Tactical AI and navigation | FighterAI, ShipAI, TacticalAI, steering, FlightPlanner, formations, objective and waypoint following, RTB and radio orders | Steering, tactics, recovery action translation, authored route and arrival tests passed. Verify T2 target engagement and T3 combat/debrief on the final active tables. |
| Carrier operations | FlightDeck, Hangar, queueing, launch, clearance, ILS approach, docking and recovery lifecycle | Queue boundaries, deck coordinates, marker guidance, retained player, recovery camera and completion fixtures passed. A full multi-fighter launch and recovery flight is still the integrated gate. |
| Sensors and IFF | Contact lifetime, sensor detection, combat target selection, probe coverage, track and identification data | Contact decay and probe-coverage fixtures passed. Verify friendly Archon, hostile drones, and targeting across successive kills during a live run. Target tracking and assisted bolts are deliberate SSW changes. |
| Weapons and stores | Primary guns, energy and ammunition, missiles, beams, bolts, loadouts, turret mounts, barrels, seeker logic, carried stores | Import, registry and swept-hit tests passed. The Zolon fixture checks mounted weapons and barrel forward alignment. A human visual pass for mesh-level muzzle placement and a live fire sequence remain open. |
| Collision, shields and destruction | Ship, obstacle and projectile collision, shield deflection, hull damage, death spiral, explosion and debris routing | Swept projectile damage, shield routing and debris fixtures passed. Verify visible shields and breakup in live combat. UE visual components use no collision for shield flashes. |
| Probes, drones and countermeasures | Separate probe and decoy launchers, recon drones, life/expiry, seeker delay and decoy recovery | All five drone/countermeasure fixture groups passed. Integrated flight with the stock loadouts remains separate. |
| Engineering and maintenance | Reactor power, routed systems, availability, efficiency/safety/stability masks, repair timers, spare consumption and auto-maintenance | The active engineering popup reads runtime components. Its older compatibility widget has unused placeholder list handlers. Newly connected component templates passed a representative reactor test. Complete the cross-family design-key audit before claiming every subsystem template resolves. |
| Quantum and Farcaster | Destination validation, power-off, transfer, region ownership and approach/arrival | Four travel checks passed. The approach parser limit issue above remains. Live interrupted and repeated trips with the final presentation remain open. |
| Mission lifecycle and authoring | Mission and template loading, elements and events, briefings, editor/nav editing, completion/debrief and save | Campaign 1 mission catalog, editor action, live-update, recovery and persistence fixtures passed. Full mission round trips and campaign handoffs are still the release test. |
| Dynamic campaign and pilot progression | Campaign planning, combat groups/units, situation reports, awards, ranks and UE pilot persistence | Core paths are present. Fresh-pilot Highland generation, several successive dynamic outcomes and reload of campaign/award state need a paired campaign run on the final data. |
| Fleet and asset resolution | Canonical Blueprint lookup, native actors, hulls, hardpoints, engine/RCS effects, nav lights and shield asset lookup | Fleet suite checked 76 hulls and 179 turret mounts. The Zolon saved-table mismatch is a migration acceptance case. Normal startup rebuilds the runtime registry. |

## Next build gate — editor ingestion

The starter `SSWDesignIngestion` commandlet reuses the existing parsers. It can stage three UE design tables under `/Game/Editor/GeneratedDesigns` using `-WriteStaged`. The active table file hashes were unchanged after staging.

| Table | Staged | Active | Raw comparison |
|---|---:|---:|---|
| Systems | 15 | 15 | 15 changed |
| Weapons | 47 | 47 | 0 changed |
| Ships | 103 | 103 | 1 staged-only, 1 active-only, 95 common rows changed |

Raw reflected comparisons include source metadata. These counts are review signals, not 110 confirmed gameplay defects. Separate provenance-only changes from changed gameplay values and resolve the unmatched ship keys before promotion.

Next extend the tool to validate design keys, aliases and limits, include the other DEF-backed tables, and compare the staged output in a standalone run before replacing any active tables. Acceptance starts with the six Zolons loading the same engines, thrusters, lights, weapons and shield aliases from the promoted tables that the fresh-DEF fixtures verified. The existing startup path stays active until that comparison passes.

