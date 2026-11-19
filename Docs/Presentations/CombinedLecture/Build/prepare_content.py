from pathlib import Path
import json

here = Path('D:/SSW/Docs/Presentations/CombinedLecture')
practical = json.loads(Path('D:/SSW/Docs/Presentations/EVE_REA_Lecture/lecture-rea-content.json').read_text(encoding='utf-8'))['slides']
source_slots = [(1,45),(2,60),(None,15),(3,60),(4,150),(5,90),(7,120),(8,150),(11,90),(13,120),(14,90),(16,210),(22,120),(23,180)]
core = []
for source, seconds in source_slots:
    if source:
        core.append(dict(kind='source', source=source, seconds=seconds))
    else:
        core.append(dict(kind='divider', title='PART I  /  THEORY', subtitle='Evidence, tools and agentic analysis', seconds=seconds))
assert sum(s['seconds'] for s in core) == 1500
core.append(dict(kind='divider', title='PART II  /  PRACTICAL', subtitle='Raw bytecode, recovered Python and SSW comparisons', seconds=30))

def add_example(title, seconds):
    item = next(s for s in practical if s['title'] == title).copy()
    item.update(kind='practical', seconds=seconds)
    core.append(item)

add_example('The investigation starts with an artifact',90)
add_example('Three views of the same function',90)
for stem, native in [('Ore scan knowledge','SSW example: scan uncertainty'),('Drone recovery command','SSW example: drone authority'),('Manufacturing cost','SSW example: manufacturing locations'),('Contract service boundary',None),('Navigation distance',None),('Mission expiry',None)]:
    add_example(stem + ': bytecode',90)
    add_example(stem + ': recovered source',90)
    if native:
        add_example(native,90)
core.append(dict(kind='summary',title='What the investigation established',body=['Raw bytes anchor the claim. Disassembly exposes the operation.','Recovered source makes a rule readable, but still needs checking.','An RPC call reveals a boundary. It does not reveal the server.','SSW comparisons show deliberate adaptations with separate tests.'],seconds=120,refs=['D:/SSW/Docs/Presentations/EVE_REA_Lecture/Build/rea-recovery/examples.json']))
assert len(core) == 33
assert sum(s['seconds'] for s in core[14:]) == 1680
core.extend([dict(kind='source',source=28,seconds=405),dict(kind='source',source=29,seconds=15)])
assert len(core) == 35 and sum(s['seconds'] for s in core) == 3600
appendix = []
for stem in ['Drone classification','Drone ownership','Inventory location','Wallet filtering','Mission arc validation','Fitting slot count']:
    for suffix in [': bytecode',': recovered source']:
        item = next(s for s in practical if s['title'] == stem + suffix).copy()
        item.update(kind='practical',seconds=0,appendix=True)
        appendix.append(item)
item = next(s for s in practical if s['title'] == 'SSW example: wallet query').copy()
item.update(kind='practical',seconds=0,appendix=True)
appendix.append(item)
resources = ['https://github.com/NationalSecurityAgency/ghidra','https://github.com/icsharpcode/ILSpy','https://github.com/morluto/rea','https://github.com/rocky/python-xdis','https://github.com/rocky/python-uncompyle6']
appendix.append(dict(kind='resources',title='Sources and further reading',body=[u.replace('https://','') for u in resources]+['The lecture evidence includes twelve saved code exhibits, hashes and full disassemblies.'],seconds=0,appendix=True,refs=resources))
elapsed = 0
for index, item in enumerate(core):
    item['start'] = elapsed
    elapsed += item['seconds']
    item['end'] = elapsed
    item['part'] = 'Theory' if index < 14 else 'Practical' if index < 33 else 'Discussion'
    item['note'] = item.get('note','')
    source = item.get('source')
    if source:
        item['refs'] = ['C:/Users/Carlo/OneDrive/Documents/Coventry/Agentic_AI_Reverse_Engineering_Dr_Carlos_Bott.pptx, source slide '+str(source)]
    if source in [7,8]:
        item['refs'].append(resources[0])
    if source in [7,11]:
        item['refs'].append(resources[1])
(here/'combined-content.json').write_text(json.dumps(dict(core_count=35,appendix_count=len(appendix),duration_seconds=3600,slides=core+appendix),indent=2),encoding='utf-8')
print('35 timed slides,',len(appendix),'hidden backup slides; 60 minutes including Q&A')
