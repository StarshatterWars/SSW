# Alpha 2 editor ingestion review

Run the editor commandlet with the project file and `-run=SSWDesignIngestion -unattended -nop4` for a dry run. Add `-WriteStaged` to save the tables under `/Game/Editor/GeneratedDesigns`. Runtime still uses its existing startup ingestion. Saved production table hashes were unchanged by staging.

## Staged coverage

| Table family | Rows |
|---|---:|
| System, weapon and ship designs | 15, 47, 103 |
| Campaigns, campaign actions and zones | 5, 294, 83 |
| Cutscenes | 73 |
| Single and multiplayer mission catalogs | 10, 4 |
| Combat groups and order of battle | 543, 8 |
| Ranks and medals | 11, 16 |
| Galaxy map and regions | 21, 75 |

Planet, moon and star content is carried in the nested galaxy rows produced by the active parser. Separate legacy or authored assets such as CampaignOOB, GameText, FormDef and CustomMissions are outside this DEF regeneration. Campaign templates and scripted missions are carried inside the campaign rows.

The tool validates catalog ship aliases and the retained farcaster, landing gear, flight-deck and weapon limits. Unresolved sys.def template keys are reported for review without assigning a guessed component design. Staging is separate from promotion.

## Order-of-battle differences

The saved order-of-battle overview was manually corrected. Its complete eight-force hierarchy now lives in `Content/GameData/Systems/OrderOfBattleOverview.def`. The commandlet stages that source directly. A reflected row comparison against the saved table found eight matching keys and zero changed rows, including nested groups and units.

The faction DEFs still supply the combat roster. The separate overview source preserves the authored distinction between the two datasets. Runtime ingestion and the saved overview asset remain in place.

## Awards and cutscenes

Korian Crisis Service and Grand Order were found only in the saved medals table. Both now have DEF records with their existing IDs, text, image stems and campaign flags. Their existing automatic-award eligibility remains disabled. The parser now resolves rank and medal image stems into UE soft references. Existing DEF-authored medal eligibility conditions are retained.

Reviewed 70 Campaign 2–5 scene files against Campaign 2 News Start. Changed 39 files to correct misspelled Message events, replace legacy nested event keys, and normalize quoted media references to asset stems. Retained dialogue, camera staging, timing, and existing begin/end boundaries. Standalone validates that staged scenes have parsed events and compares campaign counts against normal startup. Full playback and media availability still need visual checks.

A future cutscene viewer fits under Campaign Intel as a campaign-grouped archive. Normal mode should show scenes viewed by the current pilot. An explicit development flag can show all staged scenes without changing unlock or viewing state.
