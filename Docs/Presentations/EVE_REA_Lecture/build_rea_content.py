import json,re,hashlib
from pathlib import Path
root=Path('D:/SSW');here=Path(__file__).parent
examples=json.loads((here/'Build/rea-recovery/examples.json').read_text(encoding='utf-8'))
slides=[]
def add(title,body,refs=(),code=None,note=''):
 slides.append(dict(title=title,body=body,refs=list(refs),code=code,note=note,image=None))
add('Reverse engineering EVE Online',['REA investigations with raw bytecode and recovered Python','Fractal Dev Games | 19 November 2026'],note='A static lecture with code exhibits. No live demonstration. SSW appears only in brief implementation comparisons tied to recovered evidence.')
add('The investigation starts with an artifact',[
 'Target: the installed EVE client archive, code.ccp.',
 'The selected .pyj members contain compressed Python bytecode.',
 'A member path, SHA-256 and function name identify every exhibit.'
],['D:/SSW/Saved/Analysis/EveGateApproach_20261118/report.json'])
add('REA coordinates the investigation',[
 'Frame one behavioral question, then inspect the matching artifact.',
 'Retain the artifact identity, observations and unresolved questions.',
 'For this client variant, use xdis for decoding and uncompyle6 for source recovery.'
],['D:/SSW/Tools/ReverseEngineering/read_legacy_bytecode.py'],note='REA built-in inspect_python_pyc supports newer CPython variants, not this PyPy 2.7 artifact. These exhibits are real static fallback outputs, not claimed REA-native decompilation. All twelve functions decompiled with uncompyle6 3.9.3 and xdis 6.1.7 in an isolated tool directory.')
add('Three views of the same function',[
 'Raw bytes show the actual co_code payload.',
 'Disassembly resolves offsets, operations and operands.',
 'Recovered Python reconstructs readable control flow from those operations.'
],note='The next exhibits show actual function bodies. A def line is reconstructed from the code object metadata. Original decorators, default argument expressions, comments and formatting are not recovered from an isolated code object.')
add('A safe static recovery path',[
 'Read one archive member and limit its decompressed size.',
 'Load the cross-version code object as data.',
 'Decode instructions, then decompile a selected function.',
 'Inspect the result without importing or calling the recovered module.'
],['D:/SSW/Tools/ReverseEngineering/read_legacy_bytecode.py','D:/SSW/Docs/Presentations/EVE_REA_Lecture/recover_lecture_examples.py'])
explanations=[
 ('A scan can exist without a known quantity.','The predicate rejects both None and QUANTITY_UNKNOWN. It says nothing about the authoritative ore amount.'),
 ('A group ID identifies the mining-drone class.','Two attribute chains feed a single equality comparison. The type constant supplies meaning that byte values alone do not convey.'),
 ('Ownership and current control are separate ideas.','The client checks the current controller against the active ship. This is a client-side availability rule, not proof of server authorization.'),
 ('The recovery command crosses a service boundary.','The client obtains entity access, sends CmdReturnBay and handles per-drone errors. The server movement implementation is absent.'),
 ('The recovered helper prices a manufacturing job.','This function looks up a blueprint price and multiplies by runs. Other fees and final settlement may live elsewhere.'),
 ('An inventory identity contains a location.','Tuple unpacking reveals that the first component is the location identifier. The ignored component remains part of the data model.'),
 ('Changing a filter refreshes the visible list.','The client stores a filter function and rebuilds its sorted, filtered view. No money changes here.'),
 ('A cached proxy provides contract access.','The client lazily creates contractProxy. A service name identifies a boundary, not its server settlement algorithm.'),
 ('An absent range selects an action default.','The branch falls back to GetDefaultDist and forwards keyword arguments. A supplied range returns unchanged.'),
 ('Arc eligibility comes from an interval helper.','This predicate compares an externally supplied restart interval with 1. The helper implementation is needed to interpret its full policy.'),
 ('An offer without an expiry is still valid.','A missing expiration returns False. Otherwise the client compares its expiration with wall-clock time.'),
 ('The fitting view counts slots across groups.','The comprehension sums each slot group length. It describes a UI-facing count, not power, CPU or fitting legality.')]
for i,(ex,meaning) in enumerate(zip(examples,explanations),1):
 refs=[ex['input'],'D:/SSW/Docs/Presentations/EVE_REA_Lecture/Build/rea-recovery/examples.json']
 instructions=[]
 for line in ex['disassembly'].splitlines():
  # Remove source-line prefixes while retaining instruction byte offsets.
  line=re.sub(r'^\s*\d+:\s*','',line).strip()
  if line:instructions.append(line)
 hexlines=[' '.join(ex['hex'].split()[j:j+16]) for j in range(0,ex['byte_count'],16)]
 excerpt=instructions[:11]
 code='co_code ('+str(ex['byte_count'])+' bytes)\n'+'\n'.join(hexlines)+'\n\n'+'\n'.join(excerpt)
 if len(instructions)>11:code+='\n... full disassembly in speaker notes'
 note='Function: '+ex['function']+'\nEmbedded file: '+ex['embedded_filename']+'\nSHA-256: '+ex['sha256']+'\nPython version: '+str(ex['version'])+'; magic: '+str(ex['magic'])+'\n\nFull disassembly:\n'+ex['disassembly']+'\n\nRecovered source:\n'+ex['source']+'\n\n'+meaning[1]
 add(ex['title']+': bytecode',[meaning[0]],refs,code,note)
 add(ex['title']+': recovered source',[meaning[1]],refs,ex['source'],note+'\nRecovery: '+ex['status']+'. Function body decompiled; def line rebuilt from argument metadata. No target code executed.')
 if i in (1,3,5,7):
  specs={1:('SSW example: scan uncertainty','Source/StarshatterWars/AIAgent/MiningAI.cpp','        const TOptional<double> Quantity',7,'The recovered distinction informs an explicit optional quantity. This is SSW C++, not decompiled EVE server code.'),
   3:('SSW example: drone authority','Source/StarshatterWars/AIAgent/MiningDroneAI.cpp','bool MiningDroneAI::Return(',3,'SSW adds its own authority check and return state. The recovered client predicate alone would not be sufficient.'),
   5:('SSW example: manufacturing locations','Source/StarshatterWars/AIAgent/ManufacturingAI.cpp','        if (WorkOrder.Facility.IsNone()',6,'SSW validates separate input and output locations before consuming material. This is an adaptation comparison, not an EVE formula.'),
   7:('SSW example: wallet query','Source/StarshatterWars/AIAgent/TransactionAI.cpp','TArray<FTransactionEntry> TransactionAI::Query',8,'The recovered filter-refresh behavior motivates a separate query path. SSW supplies its own ledger and accounting rules.')}
  title,file,needle,count,body=specs[i];p=root/file;lines=p.read_text(encoding='utf-8-sig',errors='replace').splitlines();at=next(j for j,l in enumerate(lines) if needle in l)
  add(title,[body],[p.as_posix()+':'+str(at+1),*refs],'\n'.join(lines[at:at+count]),note='Native SSW source excerpt. Included only to demonstrate interpretation of the preceding recovered evidence. No claim of equivalent EVE server implementation.')
add('A decompiler can produce plausible errors',[
 'Check branch targets and return values against the disassembly.',
 'Inspect argument flags so *args and **kwargs survive reconstruction.',
 'Treat expression comments as hints, then verify the actual operations.'
],['D:/SSW/Docs/Presentations/EVE_REA_Lecture/recover_lecture_examples.py'],note='The navigation function uses **kwargs. The recovered signature now includes the CO_VARKEYWORDS metadata. Defaults and decorators require inspection of the enclosing code object.')
add('What this client reveals',[
 'UI predicates, cached state, function arguments and service boundaries.',
 'Local calculations such as filtering, expiration checks and slot counts.',
 'The structure of requests and the errors their callers expect.'
],note='These are conclusions from the twelve demonstrated functions, not blanket claims about all EVE behavior.')
add('What remains outside the evidence',[
 'Authoritative settlement and inventory transactions on the remote server.',
 'Server NPC scheduling, conflict resolution and anti-abuse policy.',
 'Original comments, exact source formatting and missing dependencies.'
],note='The client may contain shared game logic, but an RPC call by itself does not establish server behavior. Keep unsupported implementation guesses separate.')
add('A reproducible REA handoff',[
 'Ship the selected artifact hashes and member identities.',
 'Include raw bytes, full disassembly and recovered source together.',
 'Document the decoding tools and the limits of each conclusion.'
],['D:/SSW/Docs/Presentations/EVE_REA_Lecture/Build/rea-recovery/examples.json'],note='This lecture was generated from twelve saved PYC files. All twelve recovery statuses are successful. The supporting evidence bundle contains exact function bytes and complete disassemblies. No live demo is required.')
(here/'lecture-rea-content.json').write_text(json.dumps({'slides':slides},indent=2),encoding='utf-8')
print(len(slides),'slides',len(examples),'bytecode/source pairs; 4 SSW comparisons')
