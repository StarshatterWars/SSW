"""Actual dedicated executable with a separate editor client; isolated development saves."""
from pathlib import Path
import subprocess,time,json,re,sys
root=Path('D:/SSW'); engine=Path('C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
run=root/'Saved/Automation'/('FractalVerseDedicatedSmoke-'+time.strftime('%Y%m%d-%H%M%S'));run.mkdir(parents=True)
serverlog=run/'server.log';clientlog=run/'client.log';handles=[];processes=[]
def launch(extra,name):
 stream=(run/(name+'-console.log')).open('wb');handles.append(stream)
 executable = root/'Binaries/Win64/FractalVerse.exe' if name.startswith('server') else engine
 p=subprocess.Popen([str(executable),str(root/'StarshatterWars.uproject'),*extra,'-unattended','-nop4','-nosplash','-nosound','-NullRHI','-SSWLogging','-stdout'],cwd=root,stdout=stream,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 processes.append(p);return p
def read(p):return p.read_text(errors='replace') if p.exists() else ''
try:
 sandbox=root/'Saved/Cooked/WindowsServer/StarshatterWars'/run.relative_to(root)/'World'
 server=launch(['/Game/Maps/Transition?game=/Script/StarshatterWars.FractalVerseGameMode','-server','-networkversionoverride=47537391','-SSWFractalVerseServer','-MULTIHOME=127.0.0.1','-port=17880','-FractalVerseSaveDir='+str(run/'World'),'-FractalVerseTestSeconds=75','-abslog='+str(serverlog)],'server')
 print('Started isolated FractalVerse smoke server',flush=True)
 deadline=time.monotonic()+100
 while time.monotonic()<deadline:
  log=read(sandbox.parent/'server.log') or read(serverlog) or read(run/'server-console.log')
  if (sandbox/'status.json').exists():
   listener=subprocess.run(['powershell','-NoProfile','-Command',f'(Get-NetUDPEndpoint -OwningProcess {server.pid} -ErrorAction SilentlyContinue | Where-Object LocalPort -eq 17880).LocalPort'],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
   if '17880' in listener.stdout:break
  if server.poll() is not None:raise RuntimeError('Server exited before listening; see '+str(serverlog))
  time.sleep(1)
 else:raise RuntimeError('Server never reported its listener; see '+str(serverlog))
 print('Server listener ready; connecting separate client',flush=True)
 client=launch(['127.0.0.1:17880?FractalVerseProtocol=1','-game','-SSWFractalVerseClient','-FractalVerseProbeTransfer','-abslog='+str(clientlog)],'client')
 deadline=time.monotonic()+55
 while time.monotonic()<deadline:
  log=read(clientlog)
  statuses=re.findall(r'\[FractalVerseClient\] Confirmed revision (\d+) clock ([\d.]+)',log)
  if len(statuses)>=2 and float(statuses[-1][1])>float(statuses[0][1]):break
  if client.poll() is not None:raise RuntimeError('Client exited before snapshots; see '+str(clientlog))
  time.sleep(1)
 else:raise RuntimeError('No increasing confirmed server clock received; see '+str(clientlog))
 if 'accepted=0' not in read(clientlog):raise RuntimeError('Unauthorized cargo probe was not rejected')
 if 'Pilot binding confirmed=0' not in read(clientlog):raise RuntimeError('Malformed identity proof was not rejected')
 print('Separate client received increasing authoritative revisions: '+str(statuses[-2:]),flush=True)
 discovery=json.loads((sandbox/'status.json').read_text(encoding='utf-8-sig'))
 if discovery['maximumClients']!=8 or discovery['connectedClients']!=1 or not discovery.get('players'):raise RuntimeError('Server discovery client count/cap mismatch')
 (run/'server-discovery.json').write_text(json.dumps(discovery,indent=2))
 client.terminate();client.wait(timeout=20)
 (sandbox/'shutdown.request').write_text(discovery['instance'])
 server.wait(timeout=30)
 if server.returncode!=0:raise RuntimeError('Server exit code '+str(server.returncode))
 if not (sandbox/'head').exists():raise RuntimeError('Server checkpoint missing')
 first=re.search(r'Server world ready: ([A-Fa-f0-9-]+) revision (\d+)',(read(sandbox.parent/'server.log') or read(serverlog) or read(run/'server-console.log')))
 restartlog=run/'server-restart.log'
 restarted=launch(['/Game/Maps/Transition?game=/Script/StarshatterWars.FractalVerseGameMode','-server','-networkversionoverride=47537391','-SSWFractalVerseServer','-MULTIHOME=127.0.0.1','-port=17880','-FractalVerseSaveDir='+str(run/'World'),'-FractalVerseTestSeconds=10','-abslog='+str(restartlog)],'server-restart')
 restarted.wait(timeout=100)
 restored=re.search(r'Server world ready: ([A-Fa-f0-9-]+) revision (\d+)',(read(sandbox.parent/'server-restart.log') or read(restartlog) or read(run/'server-restart-console.log')))
 if restarted.returncode!=0 or not first or not restored or first[1]!=restored[1] or int(restored[2])<int(statuses[-1][0]):raise RuntimeError('Restart did not preserve universe/revision; see '+str(restartlog))
 print('Restart recovered same universe at revision '+restored[2],flush=True)
 report={'restartRevision':restored[2],'universe':restored[1],'maximumClients':discovery['maximumClients'],'connectedClients':discovery['connectedClients'],'unauthorizedTransferRejected':True,'invalidIdentityRejected':True,'status':'passed' ,'run':str(run),'firstSnapshot':statuses[0],'lastSnapshot':statuses[-1],'serverExit':server.returncode,'worldCheckpoint':str(sandbox/'head'),'note':'Compiled FractalVerse executable and receive-only editor client; authentication and gameplay authority remain separate gates.'}
 (root/'Saved/Analysis/Phase5DedicatedTransportSmoke.json').write_text(json.dumps(report,indent=2));print(json.dumps(report),flush=True)
except Exception as exc:
 print(str(exc),flush=True);sys.exit(1)
finally:
 for p in processes:
  if p.poll() is None:p.terminate()
 for h in handles:h.close()
