"""Check changed native files: own header first, generated header last, no embedded UTF-8 BOM."""
import argparse,json,re,subprocess
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('files',nargs='*');args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
def git(*args):return subprocess.check_output(['git',*args],cwd=root,text=True,stderr=subprocess.DEVNULL).splitlines()
names=args.files or sorted(set(git('diff','--name-only')+git('ls-files','--others','--exclude-standard')))
issues=[];checked=0
for name in names:
 p=root/name
 if p.suffix not in ('.cpp','.h') or not p.is_file():continue
 checked+=1;raw=p.read_bytes();text=raw.decode('latin1')
 includes=re.findall(r'^\s*#include\s*[<"]([^>"\r\n]+)',text,re.M)
 if p.suffix=='.cpp' and p.with_suffix('.h').exists() and includes and includes[0].split('/')[-1]!=p.stem+'.h':issues.append({'file':name,'issue':'Own header must be first','expected':p.stem+'.h'})
 if p.suffix=='.h' and any(x.endswith('.generated.h') for x in includes) and not includes[-1].endswith('.generated.h'):issues.append({'file':name,'issue':'Generated header must be last'})
 if raw.find(b'\xef\xbb\xbf',3)>=0:issues.append({'file':name,'issue':'Embedded byte-order mark'})
report={'checked':checked,'issues':issues};out=root/'Saved/Analysis/PrePhase5IncludeOrderAudit.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2));raise SystemExit(bool(issues))
