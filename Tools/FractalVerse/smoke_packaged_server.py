"""Verify the actual archived server boots and restores its isolated checkpoint."""
from pathlib import Path
import subprocess,time,json,re,uuid
repo=Path('D:/SSW')
exe=repo/'Build/FractalVerse/WindowsServer/StarshatterWars/Binaries/Win64/FractalVerse.exe'
run=repo/'Saved/Automation'/('FractalVersePackageSmoke-'+time.strftime('%Y%m%d-%H%M%S'))
run.mkdir(parents=True)
world=run/'World'
snapshots=[];identities=[]
for attempt in range(2):
 instance=uuid.uuid4().hex
 log=run/('server-'+str(attempt)+'.log')
 output=(run/('console-'+str(attempt)+'.log')).open('wb')
 args=[str(exe),'/Game/Maps/Transition?game=/Script/StarshatterWars.FractalVerseGameMode','-server','-SSWFractalVerseServer','-MULTIHOME=127.0.0.1','-port=17884','-FractalVerseInstance='+instance,'-FractalVerseSaveDir='+str(world),'-FractalVerseTestSeconds=8','-abslog='+str(log),'-unattended','-nosound','-NullRHI','-SSWLogging']
 process=subprocess.Popen(args,cwd=exe.parent,stdout=output,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 latest=None;deadline=time.monotonic()+40
 while process.poll() is None and time.monotonic()<deadline:
  try:
   state=json.loads((world/'status.json').read_text())
   if state.get('instance')==instance and state.get('processId')==process.pid:latest=state
  except (FileNotFoundError,json.JSONDecodeError):pass
  time.sleep(.2)
 if process.poll() is None:
  process.terminate();process.wait();raise RuntimeError('Owned test server exceeded its time limit')
 output.close()
 text=log.read_text(errors='replace')
 errors=[line for line in text.splitlines() if ': Error:' in line or 'Fatal error' in line]
 if process.returncode!=0 or errors or not latest or not latest.get('online') or not latest.get('checkpointReady') or latest.get('maximumClients')!=8:
  raise RuntimeError('Packaged boot failed: '+str({'exit':process.returncode,'errors':errors[:5],'status':latest,'log':str(log)}))
 identity=re.search(r'Server world ready: (\w+) revision (\d+)',text)
 if not identity:raise RuntimeError('No authoritative universe boot evidence: '+str(log))
 snapshots.append(latest);identities.append(identity.groups())
if identities[0][0]!=identities[1][0] or snapshots[1]['revision']<snapshots[0]['revision']:
 raise RuntimeError('Archived restart did not preserve the universe/checkpoint revision')
report={'status':'passed','executable':str(exe),'run':str(run),'maximumClients':8,'firstRevision':snapshots[0]['revision'],'restartRevision':snapshots[1]['revision'],'universePreserved':True,'cleanExit':True,'bootErrors':0,'checkpointReady':True}
(repo/'Saved/Analysis/Phase5PackagedServerSmoke.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
