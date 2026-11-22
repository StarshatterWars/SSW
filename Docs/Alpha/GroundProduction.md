# Ground industry and storage

Ground building behavior is authored in `Content/GameData/Production/factories.json`. Its Name is the existing ship/building design key. Exterior replacement meshes do not change industry identities or recipes.

All 30 current Building designs have an explicit role. Five Fact00–Fact04 models perform assembly, alloy processing, avionics fabrication, fleet-spares production and heavy engine manufacture. Domes, selected civilian blocks, administration and support buildings have appropriate existing recipes. No new combat balance or mission objectives are authored here.

Tank00 is a 12,000 t cryogenic tank for liquid hydrogen. Bldg02 is a 20,000 t general warehouse; Bldg03 is 8,000 t cold storage for food, rations and medical supplies; Bldg05 is a 10,000 t fuel/propellant depot. Accepted cargo and capacities are JSON-editable. Cooling, power, bridge/platform infrastructure and abandoned structures do not fabricate goods.

Existing ground building instances are registered when their mission sector is discovered. Identities use system, region and authored unit name; discovered sites, inventories and destruction state persist in the economy save. This does not populate every unexplored planet with new buildings or add automatic atmospheric freight routes. Those require authored placements and surface logistics.

Production starts with real delivered inputs. Output is credited only when a batch completes. A destroyed building is marked offline through the native retirement event, and loading the same mission again does not repair the economic record. Storage deliveries enforce accepted items and finite capacity. Destroyed storage currently retains its inventory record; loot/destruction of its contents is a separate feature.

The runtime production controller uses saved completion deadlines and inventory/facility-change wakeups rather than decrementing every job every frame. Start, stop, completion, blocked-output and cancellation notifications are native events. Background mining cycles and regional price samples use saved deadlines; freight phases use saved transition times. Local management is dispatched by one-shot timers and inventory/facility-change wakeups. Realized ship movement, collision avoidance and active mining beams remain continuous gameplay work. Dedicated world clock advancement pumps the same native deadline gates without requiring scene actors.

The production detail card opts into simulation change notifications. Agent, blueprint, cargo, market, contract, skill and wallet detail cards also opt into change notifications and selection revision invalidation. Contact range presentation remains continuous gameplay data.


## First economy-worker boundary

The local career dispatches copied native economy state and pre-resolved recipe/catalog values to a dedicated single-thread queue named SSW Economy. Background mining, production jobs and regional economic calculations execute there. The worker has no access to actors, UObjects, widgets or player saves. Mineral enum labels are resolved before dispatch. Simulation/UI notifications are suppressed while calculating, and production lifecycle events are buffered with the result.

Results return to the game thread. A mutation revision and pilot/checkpoint token must still match before commit. A stale or wrong-pilot result is discarded and recalculated; a result can commit only once. The live universe clock is never rewound to the snapshot time. Commit publishes UI invalidation and marks the existing save state dirty. Shutdown joins owned work; uncommitted output cannot modify the live save.

This is the first threading step. Live ship/drone movement, physical interactions, freight routing/arrival adapters, transaction commands, UE presentation and SaveGame operations remain on their current authoritative/game-thread paths. The full native simulation has not moved to a separate thread. Headless authority tests use the same pure calculation synchronously for deterministic stepping; this change does not rebuild or deploy a new server binary.


## Verified cook checkpoint — 21 November 2026

Editor Development and Windows client Development builds passed without compiler warnings. The final Starshatter automation report contains 359 successes, zero failures and zero test warnings. Coverage includes GST conservation, store rollback/storage, finite missile stocks, native ground production/storage, saved freight deadlines, and the dedicated economy worker. The worker test also delivers actual mined ore to the authored Janek refinery recipe and verifies exact concentrate output, buffered lifecycle events, stale/wrong-pilot rejection, preservation of live scanning claims, duplicate-commit rejection and safe shutdown.

Report: Saved/Automation/ShipStoreRegression/index.json. Build logs: Saved/Logs/ShipStoreEditorFinal.log and Saved/Logs/ShipStoreClientFinal.log. This checkpoint does not include a client cook/package, packaged T1 play-through, ZIP publication, a rebuilt dedicated-server binary or the full simulation-thread split.
