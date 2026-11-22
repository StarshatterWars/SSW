"""Launch an isolated Epic-authenticated flight probe. No account or proof bypass."""
from pathlib import Path
import subprocess,time,json,shutil,hashlib,argparse
parser=argparse.ArgumentParser()
parser.add_argument("--server-exe",default="D:/SSW/Binaries/Win64/FractalVerse.exe")
parser.add_argument("--packaged-server",action="store_true")
options=parser.parse_args()
repo=Path('D:/SSW')
previous=json.loads((repo/'Saved/Analysis/Phase5LivePilotRun.json').read_text())
source=Path(previous['root'])/'LegacyImports'
run=Path.home()/'AppData/Local/Fractal Dev Games/FractalVerse/Tests'/('OwnedFlight-'+time.strftime('%Y%m%d-%H%M%S'))
(run/'LegacyImports').mkdir(parents=True)
manifest=[]
for path in source.glob('*.sav'):
 data=path.read_bytes();copy=run/'LegacyImports'/path.name;copy.write_bytes(data)
 assert hashlib.sha256(copy.read_bytes()).digest()==hashlib.sha256(data).digest()
 manifest.append({'file':path.name,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)})
(run/'source-manifest.json').write_text(json.dumps({'source':str(source),'files':manifest},indent=2))
def argument(value):
 value=str(value)
 if any(c in value for c in ['"','\r','\n','\0']):raise ValueError('Unsafe argument')
 if not any(c.isspace() for c in value):return value
 if '=' in value:
  key,val=value.split('=',1);return key+'="'+val+'"'
 return '"'+value+'"'
def launch(exe,args,name,hidden):
 command='"'+str(exe)+'" '+ ' '.join(argument(a) for a in args)
 stream=(run/(name+'-console.log')).open('wb')
 process=subprocess.Popen(command,executable=str(exe),cwd=exe.parent if hidden and options.packaged_server else repo,stdout=stream,stderr=subprocess.STDOUT,
  creationflags=subprocess.CREATE_NO_WINDOW if hidden else 0)
 stream.close();return process
server_exe=Path(options.server_exe)
server=launch(server_exe,([] if options.packaged_server else [repo/'StarshatterWars.uproject'])+[
 '/Game/Maps/Transition?game=/Script/StarshatterWars.FractalVerseGameMode','-server','-SSWFractalVerseServer',
 '-networkversionoverride=47537391','-MULTIHOME=127.0.0.1','-port=17883','-FractalVerseFlightProbe',
 '-FractalVerseSaveDir='+str(run/'World'),'-FractalVerseLegacySaveDir='+str(run/'LegacyImports'),
 '-FractalVerseImportLegacyPilots','-FractalVerseTestSeconds=1800','-abslog='+str(run/'server.log'),
 '-unattended','-nosound','-NullRHI','-SSWLogging','-stdout'],'server',True)
metadata={'root':str(run),'serverPid':server.pid,'port':17883,'state':'Starting isolated flight probe','originalsUntouched':True,'serverExecutable':str(server_exe),'packagedServer':options.packaged_server}
meta=repo/'Saved/Analysis/Phase5ProvisioningLiveRun.json';meta.write_text(json.dumps(metadata,indent=2))
deadline=time.monotonic()+40
while time.monotonic()<deadline:
 if (run/'World/status.json').exists():break
 if server.poll() is not None:raise RuntimeError('Server exited: '+str(run))
 time.sleep(.5)
else:raise RuntimeError('No server status: '+str(run))
client=launch(Path('C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'),[
 repo/'StarshatterWars.uproject','127.0.0.1:17883?FractalVerseProtocol=1','-game','-SSWFractalVerseClient',
 '-FractalVerseSignIn','-FractalVerseFlightProbe','-SSWLauncherProvider=EOS','-windowed','-ResX=1280','-ResY=800',
 '-nop4','-nosplash','-nosound','-ExecCmds=t.MaxFPS 60','-SSWLogging','-abslog='+str(run/'client.log')],'client',False)
metadata.update(clientPid=client.pid,state='Epic client running; automatic controls begin after verified pilot assignment')
meta.write_text(json.dumps(metadata,indent=2));print(json.dumps(metadata),flush=True)
