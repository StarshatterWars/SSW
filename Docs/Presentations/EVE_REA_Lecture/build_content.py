"""Build a source-linked lecture outline without executing recovered client code."""
import json, hashlib, csv
from pathlib import Path
ROOT=Path('D:/SSW')
HERE=Path(__file__).parent
HERE.mkdir(parents=True,exist_ok=True)
slides=[]
def add(title, body, refs=(), code=None, image=None, note=''):
    slides.append(dict(title=title,body=body,refs=list(refs),code=code,image=image,note=note))
def cpp(path, first, count):
    p=ROOT/path; lines=p.read_text(encoding='utf-8-sig',errors='replace').splitlines()
    at=next(i for i,l in enumerate(lines) if first in l)
    return '\n'.join(lines[at:at+count]),f'{p.as_posix()}:{at+1}'
def code(title,body,path,first,count,refs=()):
    text,ref=cpp(path,first,count);add(title,body,[ref,*refs],text)

add('Reverse engineering EVE Online',
    ['Evidence, native C++ adaptation, and Starshatter Wars',
     'Fractal Dev Games • 19 November 2026'],note='A self-contained lecture. Every code slide is a static example; no live demonstration is required. Native SSW excerpts are our implementation, not claimed CCP server source.')
add('The research question',[
    'Which observable EVE behaviors can improve a native space simulation?',
    'Recover contracts and constraints before reproducing a screen.',
    'Implement SSW rules in C++; test that money, cargo and state remain consistent.'
],['D:/SSW/Saved/Analysis/EveSystemsAudit/evidence.json'])
add('Four kinds of evidence',[
    'Client bytecode: function names, constants, control flow and remote calls.',
    'Published SDE: static types, recipes, requirements and declared modifiers.',
    'SSW source: the implementation we can inspect and change.',
    'Tests and recordings: evidence that a particular behavior actually ran.'
],['D:/SSW/Saved/Research/EVE/SDE/download.json','D:/SSW/Saved/Analysis/EveWallet_20261118/report.json'],note='Do not substitute one evidence class for another. A client RPC name does not reveal the server transaction or accounting algorithm.')
add('What the client does not prove',[
    'Remote service calls do not expose authoritative EVE server code.',
    'A UI preview is not proof that a transaction can commit.',
    'Static base prices are not live market prices.',
    'NPC scheduling and server settlement remain partly unknown; SSW supplies its own rules.'
],['D:/SSW/Saved/Analysis/EveNPCMovement_20261118/report.json','D:/SSW/Saved/Analysis/SDE_SkillsAgentsContracts_20261119/report.txt'])
add('The shipped artifact',[
    'Installed client: C:/CCP/EVE Online/tq/code.ccp',
    'Selected .pyj members contain compressed legacy Python bytecode.',
    'Record archive identity, member path and SHA-256 before analysis.',
    'Treat bytecode as data; do not import or execute recovered modules.'
],['D:/SSW/Tools/ReverseEngineering/read_legacy_bytecode.py','D:/SSW/Saved/Analysis/EveGateApproach_20261118/report.json'])
add('REA and the Python fallback',[
    'REA provides the artifact investigation workflow and retained evidence.',
    'The built-in decoder did not support this Python 2.7 bytecode variant.',
    'A static xdis reader supplied function indexes and disassembly.',
    'Ghidra is useful for native binaries; it is not the decoder used for these Python modules.'
],['D:/SSW/Tools/ReverseEngineering/read_legacy_bytecode.py','D:/SSW/Saved/Analysis/EveWallet_20261118/report.json'],note='Several audit reports explicitly say xdis fallback, not fresh REA semantic decoding. Preserve that distinction in the lecture. Extended expression comments can be unreliable; inspect opcodes and metadata.')
code('Bound decompression before decoding',[
    'Our reader limits expanded input and rejects truncated streams or trailing data.'
],'Tools/ReverseEngineering/read_legacy_bytecode.py','    decoder = zlib.decompressobj()',8)
code('Read metadata without running the module',[
    'The reader walks code objects and records symbols; it does not call recovered functions.'
],'Tools/ReverseEngineering/read_legacy_bytecode.py','    loaded = load_module(str(path))',13)
add('A repeatable analysis method',[
    'Inventory the artifact and select a narrow behavioral question.',
    'Inspect symbols, call sites and control flow; record uncertainty.',
    'Compare the result with existing SSW behavior before adding code.',
    'Write native rules and regression tests; retain source references.'
],['D:/SSW/Saved/Analysis/EveImplementedSystemsAudit_20261119_agent','D:/SSW/Saved/Analysis/SDE_Integration_20261119/progress.json'])
add('Asteroid scanning',[
    'Recovered client data distinguishes asteroid identity, distance and ore knowledge.',
    'SSW scan tiers separate known ore from unknown remaining quantity.',
    'Range and scan freshness constrain selection; full coverage is required to declare depletion.'
],['D:/SSW/Saved/Analysis/EveSystemsAudit/evidence.json','D:/SSW/Source/StarshatterWars/AIAgent/MiningAI.cpp'],note='The requested asteroid_scanning_service.pyj was a behavioral reference. SSW does not read every live asteroid as fully surveyed anymore.')
code('A scan result can contain an unknown quantity',[
    'SSW excerpt: an optional quantity prevents an unscanned deposit from becoming free information.'
],'Source/StarshatterWars/AIAgent/MiningAI.cpp','        const double Distance = FVector::Dist',13)
add('Mining is a state machine',[
    'Select a valid scanned deposit, approach and station-keep within beam range.',
    'Extract at the authored rate while checking remaining ore and free hold space.',
    'Deliver to the mining station; freighters move stock onward.',
    'The asteroid base is a base, rather than a mandatory extra ore handoff.'
],['D:/SSW/Source/StarshatterWars/AIAgent/MiningAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/ProductionAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/TransportAI.cpp'])
add('Mining lasers and crystals',[
    'Client effect dispatch and crystal attributes informed the presentation and checks.',
    'SSW uses its own beam actor, deposits, cargo and balance.',
    'A visual beam alone must not create ore; successful extraction changes inventories.',
    'Mining equipment is separate from combat damage.'
],['D:/SSW/Saved/Analysis/EveMiningFx_20261118/report.json','D:/SSW/Source/StarshatterWars/System/MiningCrystalCatalog.cpp','D:/SSW/Source/StarshatterWars/Sim/SimRegion.cpp'])
add('Mining drone commands',[
    'Recovered drone functions include mining, repeat mining and return actions.',
    'SSW separates outbound flight, extraction, return and unload.',
    'Ore reaches the hold only on return; recall, loss and full holds stop the cycle.',
    'Prospector capacity: 3 drones. Rockhopper capacity: 10 drones.'
],['D:/SSW/Saved/Analysis/EveMiningDrones_20261119/report.json','D:/SSW/Source/StarshatterWars/AIAgent/MiningDroneAI.cpp','D:/SSW/Content/GameData/Cargo/mining_drones.json'],note='The native capacities are SSW decisions. Check the live JSON path in the source inventory if the content folder differs; the system is not an import of EVE ship capacities.')
add('A spare drone is real inventory',[
    'Docked replacement consumes an actual unreserved drone from cargo.',
    'Replace a lost drone first; otherwise respect bay capacity.',
    'Save failure restores both the cargo and the bay.',
    'The model and beam are newly authored SSW assets.'
],['D:/SSW/Source/StarshatterWars/AIAgent/MiningDroneBayService.h','D:/SSW/Source/StarshatterWars/Tests/MiningDroneBayServiceTests.cpp'],image='D:/SSW/ArtSource/Models/MiningDrone/MiningDrone_Multiview.png')
add('Transport and contract responsibilities',[
    'TransportAI flies and transfers ore or finished goods.',
    'ContractAI tracks the agreement and cargo reservation.',
    'Pickup reserves cargo so another hauler cannot take the same stock.',
    'Jump routes connect systems; destination stock changes only after unloading.'
],['D:/SSW/Source/StarshatterWars/AIAgent/TransportAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/ContractAI.cpp','D:/SSW/Saved/Analysis/EveRoutes'])
code('Unload through the inventory ledger',[
    'SSW excerpt: validate the destination and capacity before transferring ownership.'
],'Source/StarshatterWars/AIAgent/TransportAI.cpp','double TransportAI::Unload(FEconomyState&',12)
add('Manufacturing locations are explicit',[
    'industry/job.pyj distinguishes facilities, inputs and outputs.',
    'SSW jobs bind separate input and output inventories.',
    'Reserve capacity and consume inputs once; finish the batch before crediting output.',
    'ProductionAI supervises facilities; ManufacturingAI handles job rules.'
],['D:/SSW/Saved/Analysis/EveSystemsAudit/evidence.json','D:/SSW/Source/StarshatterWars/AIAgent/ManufacturingAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/ProductionAI.cpp'])
code('Reserve output while a batch runs',[
    'SSW excerpt: consuming input and reserving output are distinct from completing the job.'
],'Source/StarshatterWars/AIAgent/ManufacturingAI.cpp','        Stock.GetInventory(WorkOrder.InputLocation) -= Inputs;',12)
add('Recipes and blueprint ownership',[
    'Recipes list input materials, quantities, outputs and batch time.',
    'Player manufacturing requires the appropriate owned blueprint and skills.',
    'Facility capabilities and available storage are checked before charging.',
    'NPC automated production is separate from player blueprint ownership.'
],['D:/SSW/Source/StarshatterWars/System/BlueprintCatalogSubsystem.cpp','D:/SSW/Source/StarshatterWars/Util/GameStructs_products.h','D:/SSW/Content/GameData/Cargo/recipes.json'],note='Recipes and blueprint paths are resolved and included in the evidence index. Blueprint research, copying and invention should not be claimed complete merely because EVE provides those activities.')
add('Market availability and scarcity',[
    'SSW production is distributed by system to create reasons to transport goods.',
    'The marketplace searches a catalog and shows availability and route distance.',
    'Prices are SSW credits; the official SDE is not a feed of live player orders.',
    'Lawfulness and exotic goods are authored SSW lore, not recovered EVE server policy.'
],['D:/SSW/Source/StarshatterWars/Display/MarketplaceDlg.cpp','D:/SSW/Source/StarshatterWars/System/EconomyRuntime.cpp','D:/SSW/Content/GameData/Cargo/economy.json'])
add('Contracts: UI state is not settlement',[
    'contractStruct.pyj helped identify contract fields and validation needs.',
    'SSW separates AcceptanceAI, BiddingAI and SettlementAI.',
    'Courier packages consume hold capacity and are removed on delivery.',
    'Collateral, rewards, outbids, claims and abandonment need explicit accounting.'
],['D:/SSW/Saved/Analysis/EveSystemsAudit/evidence.json','D:/SSW/Source/StarshatterWars/AIAgent/AcceptanceAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/BiddingAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/SettlementAI.cpp'])
add('Wallet receipts need identity',[
    'A correct final balance can conceal missing operation history.',
    'A bid and a refund can have a net delta of zero but are still two transactions.',
    'SSW records operation IDs and contract references, then rejects duplicate commits.',
    'Save failure restores the prior balance and inventory state.'
],['D:/SSW/Source/StarshatterWars/AIAgent/ContractTransactions.h','D:/SSW/Source/StarshatterWars/AIAgent/ContractLedger.cpp','D:/SSW/Source/StarshatterWars/AIAgent/TransactionAI.cpp','D:/SSW/Saved/Analysis/EvePlayerWalletRecheck_20261118/report.txt'])
add('Player wallet and recovery',[
    'Personal transactions include typed entries, filters, sorting and receipt details.',
    'The player panel exposes the wallet, assignment, awards and history.',
    'Destruction keeps the pilot’s progression but loses the ship and cargo.',
    'Insurance and the starter Falcon recovery path use SSW rules.'
],['D:/SSW/Source/StarshatterWars/Display/TransactionDlg.cpp','D:/SSW/Source/StarshatterWars/AIAgent/TransactionAI.cpp','D:/SSW/Saved/Analysis/EveWallet_20261118/report.json'],note='50,000 cr starting funds and Janek recovery are authored SSW choices. They are not EVE amounts or a reconstruction of EVE server payout code.')
add('Insurance and hangar services',[
    'The client shows policy offers, coverage state and purchase calls.',
    'SSW policies bind to owned hulls and distinguish mission loaners.',
    'Cash-purchased ships receive lifetime insurance by SSW design.',
    'A quote is revalidated before payment; claims must not be paid twice.'
],['D:/SSW/Source/StarshatterWars/System/ShipInsuranceSubsystem.cpp','D:/SSW/Source/StarshatterWars/Display/InsuranceDlg.cpp','D:/SSW/Source/StarshatterWars/Display/HangarDlg.cpp','D:/SSW/Saved/Analysis/EveStationServices/evidence.json'],note='Do not present SSW premium percentages or full component coverage as recovered EVE formulas. Authoritative EVE insurance settlement was not available in this client evidence.')
add('Fitting: quality is not damage',[
    'A component grade changes performance and power demand; damage is a separate state.',
    'SSW reactor output and native power drains remain authoritative.',
    'Rank, empire, skills, slots and available power gate a refit.',
    'Full refits require a shipyard; flight views remain read-only.'
],['D:/SSW/Saved/Analysis/SDE_FittingServices_20261119','D:/SSW/Source/StarshatterWars/System/ShipInsuranceSubsystem.cpp','D:/SSW/Source/StarshatterWars/Display/ShipFitDlg.cpp'])
add('Skill books and progression',[
    'Client requirement checks and the SDE informed prerequisite and rank gates.',
    'SSW uses JSON skills grouped by category with long prerequisite trees.',
    'Purchasing a book currently grants the complete skill; future levels are provisioned.',
    'A certification gate is not automatically a passive DPS or sensor modifier.'
],['D:/SSW/Saved/Analysis/EveSkills/evidence.json','D:/SSW/Saved/Analysis/SDE_SkillsAgentsContracts_20261119/report.txt','D:/SSW/Source/StarshatterWars/System/SkillsSubsystem.cpp'],note='Distinguish eligibility requirements from runtime effects. The SDE comparison identified skill descriptions without a direct combat consumer; do not promise a combat bonus solely because a skill exists.')
add('Agents and mission arcs',[
    'Agent identity, location, division and level map naturally to a native catalog.',
    'SSW agents offer training, story and dynamic missions with rank and skill gates.',
    'Campaign acceptance and active progression belong to the pilot’s journal.',
    'Short arcs can be sequenced; branching and failure handoff need authored content.'
],['D:/SSW/Saved/Analysis/EveEpicArcs/evidence.json','D:/SSW/Saved/Analysis/EveMissionFlow','D:/SSW/Source/StarshatterWars/AIAgent/AgentAI.cpp','D:/SSW/Source/StarshatterWars/AIAgent/MissionArcRules.h'])
add('Mission offers and live panels',[
    'Show the assigned hull, destination and why an offer is unavailable.',
    'Keep mission briefing, objectives and package information accessible in flight.',
    'Intel and replay are distinct from accepting the mission.',
    'Training replays for testing do not imply normal missions can award points repeatedly.'
],['D:/SSW/Saved/Analysis/EveMissionUIFollowup','D:/SSW/Source/StarshatterWars/Display/AgentsDlg.cpp','D:/SSW/Source/StarshatterWars/Display/PlayerDlg.cpp'],note='The floating SSW interface is an adaptation to this game. Do not describe every SSW panel restyle as recovered EVE application logic.')
add('Gate alignment and travel',[
    'Movement functions, autopilot and jump checks reveal the client travel contract.',
    'SSW aligns with the farcaster corridor before entering the gate.',
    'Routes and next-jump information must agree with the selected destination.',
    'Quantum and gate screen effects are SSW implementations; proprietary EVE effects were not imported.'
],['D:/SSW/Saved/Analysis/EveGateApproach_20261118/report.json','D:/SSW/Saved/Analysis/EveQuantumTravel_20261118','D:/SSW/Source/StarshatterWars/AIAgent/TransportAI.cpp'])
add('NPC traffic: observed calls and native scheduling',[
    'Recovered behavior modules include hauling, logistics, combat navigation and fleet actions.',
    'They do not reveal the complete live server population scheduler.',
    'TrafficAI supplies SSW routing, endpoints and lifecycle rules.',
    'Mining, transport and production remain separate handlers with explicit handoffs.'
],['D:/SSW/Saved/Analysis/EveNPCMovement_20261118/report.json','D:/SSW/Source/StarshatterWars/AIAgent/TrafficAI.cpp'],note='One michelle module failed unmarshalling in the retained investigation. Record that coverage gap rather than treating every EVE movement path as recovered.')
add('Station services are an authority boundary',[
    'Viewing a station must not grant remote repairs or purchases.',
    'SSW checks the actual dock against the viewed facility on every action.',
    'Carriers provide basic support; station roles restrict other services.',
    'The UI explains unavailable services and the backend checks again.'
],['D:/SSW/Source/StarshatterWars/AIAgent/StationServicesAI.cpp','D:/SSW/Source/StarshatterWars/System/StationServiceCatalog.cpp','D:/SSW/Source/StarshatterWars/Tests/StationServicesAITests.cpp'])
add('Career guilds and empire boundaries',[
    'SSW guilds currently represent career groups and starter-craft choices.',
    'Listings follow the pilot’s empire; an unloaded profile does not invent an empire.',
    'EVE corporation roles and shared finances are separate multiplayer concerns.',
    'Do not claim server-backed organizations from an offline catalog.'
],['D:/SSW/Saved/Analysis/EveGuilds_20261119','D:/SSW/Source/StarshatterWars/Display/GuildsDlg.cpp','D:/SSW/Source/StarshatterWars/System/GuildCatalogSubsystem.cpp'])
add('The official static data export',[
    'Downloaded build 3714173, published 18 November 2026.',
    '102 JSONL tables; 678,974 records parsed and inventoried.',
    'Archive size: 99,763,163 bytes. SHA-256 is retained with the download.',
    'This is a research input, not a copy of the live EVE economy or server database.'
],['https://developers.eveonline.com/docs/services/static-data/','https://developers.eveonline.com/static-data/tranquility/eve-online-static-data-3714173-jsonl.zip','D:/SSW/Saved/Research/EVE/SDE/inventory.json'],note='Archive SHA-256: 4eabd0c60c4c92e1b82b0302b981c7010191cac3aa68ae2c374f471c16109c20. The record count refers to the inspected archive, not every historical EVE export.')
add('SDE families used in the comparison',[
    'types and groups: item identity, category and catalog relationships.',
    'blueprints, materials and industry activities: recipes and requirements.',
    'typeDogma, attributes and effects: declared equipment relationships.',
    'agents, epic arcs, skill plans and accounting types: schema and workflow gaps.'
],['D:/SSW/Saved/Research/EVE/SDE/table-index.txt','D:/SSW/Saved/Analysis/SDE_IndustryMining_20261119/report.json','D:/SSW/Saved/Analysis/SDE_SkillsAgentsContracts_20261119/report.json'])
add('Price relationships without importing ISK',[
    'Map an SSW hull role to a reviewed EVE role group.',
    'Normalize the group median around an SSW anchor, then damp and bound the ratio.',
    '46 hull prices change; unmapped hulls keep their prior values.',
    'Physical mass, thrust, reactor output and geometry are not copied.'
],['D:/SSW/Tools/Research/rebalance_ship_systems_from_sde.py','D:/SSW/Saved/Research/EVE/SDE/ship-system-calibration-current.json'],code='role_ratio = eve_role_median / eve_frigate_median\nwithin_role = old_price / old_ssw_class_median\ncandidate = 574000 * role_ratio**0.75 * role_factor * within_role\nnew_price = round1000(clamp(candidate, 0.5*old_price, 2*old_price))\n\n# Formula summary; Falcon stays fixed at 574,000 SSW credits')
add('A worked mining-drone ratio',[
    'EVE Miner I: 10 units per 15 s → 0.6667 units/s.',
    'EVE Mining Drone I: 25 per 60 s → 0.4167 units/s.',
    'Relative rate: 0.4167 / 0.6667 = 0.625.',
    'SSW laser anchor 0.25 t/s × 0.625 = 0.15625 t/s per drone.'
],['D:/SSW/Tools/Economy/rebalance_sde_mining.py','D:/SSW/Saved/Analysis/SDE_MiningYieldRebalance_20261119'],note='A ratio is dimensionless, so it can be applied to an authored SSW tonnes/second anchor. This is an explicit balance choice, not a physical equivalence between EVE ore units and metric tonnes.')
add('Component quality with a fixed midpoint',[
    'Grade III retains the normal physical baseline.',
    'Relative performance and power ratios become smooth I–V curves.',
    'Weapon-specific profiles can override the generic component curve.',
    'A refit still has to pass the SSW reactor budget.'
],['D:/SSW/Tools/Research/rebalance_ship_systems_from_sde.py','D:/SSW/Tools/Research/apply_weapon_component_grade_profiles.py','D:/SSW/Source/StarshatterWars/System/StarshatterSystemDesignSubsystem.cpp'],code='exponent = (grade - 3) / 4.0\nperformance = reference_performance_ratio ** exponent\npower_demand = reference_power_ratio ** exponent\n\n# At grade III, exponent = 0 → both multipliers = 1\n# Formula summary; actual profiles remain in SSW JSON.')
add('JSON ingestion and parity checks',[
    'Editable source data enters through native ingestors and existing tables.',
    'Awards, ranks, mission catalogs, actions and zones are checked against legacy loaders.',
    'Use full-field comparisons and preserve order when repeated entries are meaningful.',
    'Campaign headers and galaxy/terrain definitions are intentionally left for later.'
],['D:/SSW/Source/StarshatterWars/System/SSWDesignIngestionCommandlet.cpp','D:/SSW/Source/StarshatterWars/Tests/CampaignJsonCatalogTests.cpp','D:/SSW/Source/StarshatterWars/Tests/AwardsJsonCatalogTests.cpp'])
add('A real failure: two owners of one beam',[
    'The battle recording crashed in projectile cleanup.',
    'A weapon deleted its beam, but the region still held the pointer.',
    'A null check cannot make a freed object valid.',
    'The repair is ownership: expire the beam, then let its region delete it once.'
],['D:/SSW/Source/StarshatterWars/Ship/Weapon.cpp','D:/SSW/Source/StarshatterWars/Tests/BeamOwnershipTests.cpp','D:/SSW/Saved/Logs/BeamOwnershipFixTests2.log'])
code('Retire a beam without freeing its owner’s pointer',[
    'SSW excerpt: clear the weapon’s borrowed pointer before deferred region cleanup.'
],'Source/StarshatterWars/Ship/Weapon.cpp','        SimShot* Beam = beams[i];',10)
add('Tests convert assumptions into contracts',[
    'Inventory: no ore credit before unload; no reservation can be spent twice.',
    'Manufacturing: consume once, reserve capacity, complete into the right inventory.',
    'Services: reject actions while flying or docked at another facility.',
    'Lifetime: disable a weapon, destroy it, then advance the region without double deletion.'
],['D:/SSW/Source/StarshatterWars/Tests/MiningDroneTests.cpp','D:/SSW/Source/StarshatterWars/Tests/IndustrySkillGateTests.cpp','D:/SSW/Source/StarshatterWars/Tests/StationServicesAITests.cpp','D:/SSW/Source/StarshatterWars/Tests/BeamOwnershipTests.cpp'],note='The beam ownership and shield material tests passed in the focused run. Broader build verification is separate; do not present source inspection alone as a successful gameplay test.')
add('Features that remain separate work',[
    'Server-authoritative persistence and the complete EVE NPC scheduler were not recovered.',
    'Blueprint copying, research and invention are not automatically implemented.',
    'Branching story arcs and career skill plans need authored SSW content.',
    'Corporation roles, shared wallets and online guild governance need their own authority model.'
],['D:/SSW/Saved/Analysis/SDE_SkillsAgentsContracts_20261119/report.txt','D:/SSW/Saved/Analysis/SDE_IndustryMining_20261119/report.txt','D:/SSW/Saved/Analysis/EveNPCMovement_20261118/report.json'])
add('Static teaching exercise',[
    'Given a client call named DeliverContract, list what it proves and what it does not.',
    'Write preconditions for cargo ownership, capacity, collateral and destination.',
    'Walk through success, duplicate delivery and save failure on paper.',
    'Use a receipt ID and an inventory invariant to explain why a second payout is rejected.'
],['D:/SSW/Source/StarshatterWars/AIAgent/ContractSaveTransaction.h','D:/SSW/Source/StarshatterWars/AIAgent/ContractLedger.cpp'],note='No live demonstration required. Suggested expected invariant: total cargo across pickup, reservation, in-transit hold and delivered inventory changes only through explicit extraction/consumption/loss rules. Settlement must not infer a second payment from UI retries.')

add('Observed bytecode: a repeat-mining command',[
    'The client obtains a target, then calls an entity service. Server execution remains outside this evidence.'
],['D:/SSW/Saved/Analysis/EveMiningDrones_20261119/Decoded/000_drone_commands.dis.txt'],code='66  LOAD_FAST      (entity)\n69  LOAD_ATTR      (CmdMineRepeatedly)\n72  LOAD_FAST      (droneIDs)\n75  LOAD_FAST      (targetID)\n78  CALL_FUNCTION  (2 positional, 0 named)\n81  STORE_FAST     (ret)',note='Short disassembly excerpt from MineRepeatedly, original first line 169. Formatting is reduced to bytecode offset, mnemonic and operand. This proves an observable call shape; it does not recover authoritative server mining or unload implementation.')

observed=slides.pop();slides.insert(9,observed)

# Topic-level traceability is useful during a lecture, rather than a huge raw dump.
topics=[
 ('Mining, drones and effects','EveSystemsAudit; EveMiningFx_20261118; EveMiningDrones_20261119','MiningAI; MiningDroneAI; MiningDroneSubsystem; MiningDroneActor'),
 ('Transport and production','EveRoutes; EveSystemsAudit; SDE_IndustryMining_20261119','TransportAI; ContractAI; ProductionAI; ManufacturingAI; ReprocessingAI'),
 ('Contracts and wallet','EveWallet_20261118; EvePlayerWalletRecheck_20261118; ContractReceiptReview_20261119','AcceptanceAI; BiddingAI; SettlementAI; TransactionAI; ContractLedger'),
 ('Fitting and station services','EveStationServices; SDE_FittingServices_20261119','ShipInsuranceSubsystem; StationServicesAI; StationServiceCatalog; ShipFitDlg'),
 ('Skills and player progression','EveSkills; EveGuilds_20261119; SDE_SkillsAgentsContracts_20261119','SkillsSubsystem; PlayerDlg; GuildCatalogSubsystem; HangarDlg'),
 ('Agents, missions and arcs','EveMissionFlow; EveMissionUIFollowup; EveEpicArcs','AgentAI; AgentCatalogSubsystem; MissionArcRules; AgentsDlg'),
 ('Travel and traffic','EveGateApproach_20261118; EveQuantumTravel_20261118; EveNPCMovement_20261118','TransportAI; TrafficAI; Farcaster; QuantumEffectsActor'),
 ('Native rendering and data','EvePlumes_20261119; ShipSystemSDECalibration_20261119; SDE_Integration_20261119','ShipActor; EnginePlumeTuning; JSON ingestors; native regression tests')
]
for i in range(0,len(topics),2):
    a,b=topics[i:i+2]
    add('Research index '+str(i//2+1),[a[0]+'\nEvidence: '+a[1]+'\nSSW: '+a[2],b[0]+'\nEvidence: '+b[1]+'\nSSW: '+b[2]],['D:/SSW/Saved/Analysis/'+name for row in (a,b) for name in row[1].split('; ')])
add('Source and reproduction references',[
    'Official export: developers.eveonline.com/docs/services/static-data/',
    'Local reader: Tools/ReverseEngineering/read_legacy_bytecode.py',
    'Investigation records: Saved/Analysis/Eve* and Saved/Analysis/SDE*',
    'Native implementations and tests: Source/StarshatterWars/AIAgent, System and Tests'
],['https://developers.eveonline.com/docs/services/static-data/','D:/SSW/Saved/Research/EVE/SDE/table-index.txt'],note='The accompanying evidence index lists retained module paths and report locations. External proprietary code is not presented as SSW source. Short C++ excerpts are from the local SSW implementation; formula slides are explicitly explanatory.')

# Enumerate every retained EVE module reference without redistributing modules.
records=set()
def walk(x,report):
    if isinstance(x,dict):
        member=x.get('member') or x.get('path') or x.get('source_filename')
        if isinstance(member,str) and ('.pyj' in member or '.pyc' in member):
            records.add((member,str(x.get('sha256','')),report))
        for v in x.values():walk(v,report)
    elif isinstance(x,list):
        for v in x:walk(v,report)
for folder in (ROOT/'Saved/Analysis').iterdir():
    if folder.is_dir() and folder.name.lower().startswith(('eve','sde_')):
        for p in folder.glob('*.json'):
            try:walk(json.loads(p.read_text(encoding='utf-8-sig')),p.as_posix())
            except (ValueError,OSError):pass
with (HERE/'evidence-index.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['Module reference','Retained SHA-256 if present','Evidence report']);w.writerows(sorted(records))
slides[-1]['note'] += '\n\nRetained module references (module | hash | report):\n' + '\n'.join(' | '.join(r) for r in sorted(records))
slides[-1]['note'] += '\n\nOfficial SDE table index:\n' + (ROOT/'Saved/Research/EVE/SDE/table-index.txt').read_text(encoding='utf-8-sig')
for s in slides:
    checked=[]
    for ref in s['refs']:
        if ref.startswith('D:/'):
            base=ref.rsplit(':',1)[0] if ref.rsplit(':',1)[-1].isdigit() else ref
            if not Path(base).exists():checked.append(ref)
    s['unresolvedRefs']=checked
(HERE/'lecture-content.json').write_text(json.dumps({'slides':slides,'moduleReferenceRows':len(records)},indent=2,ensure_ascii=False),encoding='utf-8')
print(f'{len(slides)} slides; {len(records)} evidence-reference rows')
for s in slides:
    if s['unresolvedRefs']:print('Resolve citations:',s['title'],s['unresolvedRefs'])
