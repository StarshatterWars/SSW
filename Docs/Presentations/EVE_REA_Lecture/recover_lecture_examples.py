"""Static recovery for lecture exhibits. Recovered bytecode is never executed."""
import sys,io,json,hashlib,contextlib
from pathlib import Path
sys.path.insert(0,'D:/SSW/Saved/Tools/REA_PythonRecovery')
from xdis.load import load_module
from xdis.bytecode import Bytecode
from xdis.op_imports import get_opcode_module
from uncompyle6.main import decompile
root=Path('D:/SSW');out=root/'Docs/Presentations/EVE_REA_Lecture/Build/rea-recovery'
examples=[
 ('Ore scan knowledge','EveSystemsAudit/4.pyc','AsteroidScanningService.is_quantity_known'),
 ('Drone classification','EveMiningDrones_20261119/drone_checks.pyc','DroneChecker.IsMiningDrone'),
 ('Drone ownership','EveMiningDrones_20261119/drone_checks.pyc','DroneChecker.IsControlledByActiveShip'),
 ('Drone recovery command','EveMiningDrones_20261119/drone_commands.pyc','ReturnToDroneBay'),
 ('Manufacturing cost','EveSystemsAudit/2.pyc','Manufacturing.job_cost'),
 ('Inventory location','EveSystemsAudit/7.pyc','BaseInvContainer.GetInventoryLocationID'),
 ('Wallet filtering','EveWallet_20261118/1.pyc','BaseTransactionsList.SetFilter'),
 ('Contract service boundary','EveFinalGapAudit_20261118/0.pyc','ContractsSvc.GetContractProxySvc'),
 ('Navigation distance','EveGateApproach_20261118/0.pyc','_GetMovementDistanceOrDefault'),
 ('Mission arc validation','EveEpicArcs/1.pyc','EpicArcController.IsValidArc'),
 ('Mission expiry','EveFinalGapAudit_20261118/2.pyc','Mission.is_expired'),
 ('Fitting slot count','EveSystemsAudit/9.pyc','FittingController.GetTotalNumSlots')]
results=[]
for title,file,target in examples:
 p=root/'Saved/Analysis'/file
 version,stamp,magic,module,*_=load_module(str(p))
 def find(c,name=''):
  here=(name+'.' if name else '')+c.co_name
  if here=='<module>.'+target:return c
  for child in c.co_consts:
   if hasattr(child,'co_code'):
    found=find(child,here)
    if found:return found
 code=find(module)
 if code is None:raise RuntimeError(target)
 stream=io.StringIO();noise=io.StringIO()
 try:
  with contextlib.redirect_stdout(noise),contextlib.redirect_stderr(noise):
   decompile(code,bytecode_version=version,out=stream,is_pypy=True)
  body='\n'.join(l for l in stream.getvalue().splitlines() if not l.startswith('#')).strip()
  args=list(code.co_varnames[:code.co_argcount]); extra=code.co_argcount
  if code.co_flags & 4: args.append('*'+code.co_varnames[extra]); extra+=1
  if code.co_flags & 8: args.append('**'+code.co_varnames[extra])
  signature='def '+code.co_name+'('+', '.join(args)+'):'
  source=signature+'\n'+'\n'.join('    '+l for l in body.splitlines())
  status='decompiled by uncompyle6 3.9.3'
 except Exception as e:
  source='RECOVERY FAILED: '+str(e);status='unavailable'
 raw=code.co_code
 if isinstance(raw,str):raw=raw.encode('latin1')
 record=dict(title=title,input=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),function=target,
  embedded_filename=code.co_filename,first_line=code.co_firstlineno,version=list(version),magic=magic,
  hex=raw.hex(' '),byte_count=len(raw),disassembly=Bytecode(code,get_opcode_module(version,variant='pypy')).dis(),
  source=source,status=status,executed=False)
 results.append(record)
 (out/(str(len(results)).zfill(2)+'-recovered.py')).write_text(source+'\n',encoding='utf-8')
 print(title,status,len(raw),'bytes',source[:160].replace('\n',' / '))
(out/'examples.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
