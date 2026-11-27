# Phase F: EVE economy evidence and SSW follow-up

27 November 2026 (pilot local date). Read-only investigation requested after the
base economy migration. No CCP code or data has been imported into SSW gameplay;
no economic balance changes are made by this comparison.

## Evidence and coverage

REA inspected the supplied C:/CCP/EVE Online/tq/code.ccp ZIP, SHA-256
328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581.
Inventory evidence is ev_f4bd2cc03774b73b67448de842f5d3a958f9089f2f7966fae34e68f00aea2bfb;
summary ev_9dcddc637c2f0b0687a0e8924adc437ba0bc88fec4a773213a787051d0ef22b6.
The complete retained bundle is Saved/Analysis/PhaseF-Economy-Audit/rea-evidence.json.
REA inventories this archive but does not decode its Python 2.7 modules. The
existing xdis reader statically decoded six selected modules without executing
them. Disassembly and function indexes are under Saved/Analysis/PhaseF-Economy-Audit
and PhaseF-Economy-Inventory. These local outputs are not distributed with the game.

| Selected module | SHA-256 of decoded bytecode |
| --- | --- |
| industry/job.pyj | 69ceb1328d2e516fb9cfd811d79b028391ec01ecbf8c5c7b8e3f998cf0311278 |
| industry/activities/manufacturing.pyj | cb53dfea3d1f0b3a79639f0f992cf2e5baa59faeda2f4e92c5c77defd1952a74 |
| eve/client/script/industry/industrySvc.pyj | 5163ac16341fabef925aa36d2959403d6e208fbaf2525f212f6d015e311693e6 |
| eve/client/script/ui/shared/market/quote.pyj | d9e32105bfe8e943eab18ccd77985ba53814a1b7c578e294f89bfffd8217f250 |
| eve/client/script/environment/invCache.pyj | 55fa0d45b02fb50a029bee023c986b4b2cb5f34876a1e601b10694de8e9e624c |
| eve/client/script/ui/services/inventorysvc.pyj | 1225de2f9e4f6485035a9c786380777a88c431e5aeef01ec0cb6e1ef2ed8b3c3 |

For static data, this pass used the already downloaded official CCP SDE build
3714173, resolved URL https://developers.eveonline.com/static-data/tranquility/eve-online-static-data-3714173-jsonl.zip.
Archive SHA-256 is 4eabd0c60c4c92e1b82b0302b981c7010191cac3aa68ae2c374f471c16109c20.
It was downloaded on 19 November; it is a pinned snapshot, not a claim of latest
live data. Fifteen tables were streamed and hashed; see Phase-F-Economy-REA-Manifest.json.
The shipped client source paths identify V24.02. The client and SDE are not proven
to be matching releases. No live server behavior was observed.

Public references checked: [CCP Static Data](https://developers.eveonline.com/static-data),
[static-data service documentation](https://developers.eveonline.com/docs/services/static-data/)
and [using static data](https://developers.eveonline.com/docs/guides/staticdata/).
The guides are hosted community documentation; the archive itself is CCP's export.

## Direct observations and inferred SSW value

| Evidence | Observed fields/client path | SSW comparison and inference |
| --- | --- | --- |
| 5,136 blueprints; 4,926 manufacturing activities | Materials, products, skill requirements, time, maximum production limit. Also copying, research and invention activities. | SSW has licensed runs, skill requirements, material/time efficiency, arrays in the job ledger and atomic settlement. Research/copying are additional activity types, not currently implemented UI actions. |
| 146 assembly-line definitions; 102 installation types | Activity IDs, cost/material/time multipliers and category/group/type-list restrictions. | SSW has supported recipes, facility time multipliers and one running job per facility. Configurable lanes, activity restrictions and parallel capacity would be a useful extension. The data does not establish live lane occupancy. |
| 220 modifier sources; typeDogma and dogmaAttributes | Per-activity modifier attributes and typed skill requirements. | SSW already separates grades from damage and has recipe skill gates. A common modifier source/provenance view could explain prices, yield and time without adopting EVE values. |
| 70 station operations; 27 services | Operation-to-service IDs and manufacturing/research factors. | SSW service/faction/docking checks and facility roles are present. Continue testing service capability and economic permission as separate checks; a listed service alone must not authorize a transaction. |
| 68 planetary schematics; typeMaterials and compressibleTypes | Typed input/output cycles, material recovery records and explicit compressed item mappings. | SSW has ground factory recipes and recovery quotes. A multi-output JSON recipe schema and optional compression recipes are useful additions. Do not confuse reprocessing records with manufacturing recipes. |
| Job._get_errors; IndustryService.InstallJob/CompleteJob/CancelJob | Client checks locations, ownership/access, funds, skills, distance, slots and blueprint runs. Installation/completion/cancellation call remote industryManager services. | SSW's native commit/revalidation boundaries fit this split. EVE server settlement code is not recovered here. SSW auto-delivery is an intentional policy, not an omitted manual EVE claim step. |
| IndustryService.OnIndustryJob/OnIndustryMaterials | Cache updates and notifications; the client also polls end dates and marks a job ready. | SSW already uses completion deadlines and change events. EVE is not entirely event-driven. SSW can keep its existing scheduled model rather than copy the polling loop. |
| invCache.OnItemChange and InventorySvc.OnItemChange | Item/location/owner/flag/quantity changes update registered views; weak registrations and session cache invalidation. | SSW has weak UI callbacks, profile epochs and shared data-change notifications. More granular owner/inventory/item events could reduce unnecessary refreshes and unrelated stale-result rejections. |
| invCacheContainer.Add/MultiAdd/StackAll | Client item locks around remote calls, with unlock paths. | SSW's serialized commands, reservations, receipts and rollback already cover the local concurrency need. Do not infer EVE server locking from these client locks. |
| types, marketGroups, npcCorporations | Type/category hierarchy, static base prices, corporation trade metadata and taxes. | Useful inputs for authored market categories and NPC roles. These records are not live orders, stock, wallets or a recovered autonomous NPC decision engine. Keep SSW credits, prices and budgets. |

## Confirmed SSW coverage

The current JSON catalogs contain 266 cargo rows and 221 recipes. Every one of the
219 FinishedGoods cargo keys has at least one exact recipe output key. This is a
catalog coverage check only: it does not prove that all inputs have a reachable
source, a factory is assigned, an NPC demands the output or a delivery can finish.
Hull/component/missile stores also have separate ownership structures that this
key-only check does not unify.

Native worker regressions cover asynchronous calculation, authoritative commit,
profile/revision rejection, read sets, failed-save rollback and receipt replay.
The broader suite covers freight custody, blueprint runs, output reservations,
market escrow and carrier stores. These are SSW observations from source/tests,
not assertions about EVE server internals.

## Follow-up priorities after this verified Phase F build

1. Audit economic reachability: each produced good needs a source chain, an active
   permitted facility, a buyer and a viable route. Validate dependencies and cycles
   rather than adding more unassigned rows. Queued SSW jobs currently wait for
   unreserved inputs at start; explicit queued-input reservation is a separate policy.
2. Add output arrays to authored recipe JSON and importer, retaining the existing
   single-output fields for compatibility. The ledger already supports multiple
   products, but the current recipe schema exposes one finished output.
3. Make facility lanes and activity eligibility authored data, defaulting to the
   existing one-job behavior. Research/copying can then be added deliberately.
4. Add targeted inventory/job change payloads after profiling refresh contention.
   Retain the current global epoch/revision safety checks during migration.

Public-data relationships support these design inferences, not particular SSW
prices, yields, tax percentages or AI algorithms. The snapshot also contains 21
blueprint activity type references absent from its types table; they are retained
as unresolved public-data references, not silently invented or imported. Neither
source inspection nor full unit-suite success is a live, long-duration economy
soak or a packaged gameplay test.

Nexus research remains deferred until after the existing phases.

## Implemented follow-up, 27 November 2026

The subsequent native SSW implementation addresses reachability, output arrays, authored activity/lane policy, targeted owner/inventory/job payloads and blueprint research/copying. See Phase-F-Industry-Expansion.md for implementation, tests and limits. This is SSW code and balance; the evidence above remains a client/public-data comparison, not recovered server policy.
