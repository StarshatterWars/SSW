"""Align warned constructor lists with declaration order; preserves C++ execution order."""
from pathlib import Path
import re,argparse,json
ap=argparse.ArgumentParser();ap.add_argument('log');ap.add_argument('--apply',action='store_true');a=ap.parse_args()
log=Path(a.log).read_text(errors='replace');groups=[]
for file,cls in re.findall(r'(D:\\SSW\\Source\\[^\n(]+\.cpp)\([^\n]*warning C5038: data member \'(\w+)::',log):
 key=(file,cls)
 if key not in groups:groups.append(key)
changes=[]
for file,cls in groups:
 p=Path(file);h=p.with_suffix('.h')
 if not h.exists():continue
 raw=p.read_bytes();enc='utf-8-sig' if raw.startswith(b'\xef\xbb\xbf') else 'utf-8'
 try:src=raw.decode(enc)
 except UnicodeDecodeError:enc='cp1252';src=raw.decode(enc)
 crlf='\r\n' in src;src=src.replace('\r\n','\n');header=h.read_text(encoding='utf-8-sig',errors='replace');header=re.sub(r'//[^\n]*|/\*.*?\*/','',header,flags=re.S);out=src
 pattern=re.compile(r'\b'+re.escape(cls)+r'::'+re.escape(cls)+r'\s*\([^)]*\)\s*:\s*')
 for match in reversed(list(pattern.finditer(src))):
  start=match.end();i=start;depth=0;quote=None;escape=False;parts=[];part=start
  while i<len(src):
   ch=src[i]
   if quote:
    if escape:escape=False
    elif ch=='\\':escape=True
    elif ch==quote:quote=None
   elif ch in ['"',"'"]:quote=ch
   elif ch=='(':depth+=1
   elif ch==')':depth-=1
   elif depth==0 and ch==',':parts.append(src[part:i].strip());part=i+1
   elif depth==0 and ch=='{':parts.append(src[part:i].strip());break
   i+=1
  if i==len(src):continue
  names=[re.match(r'^(\w+)\s*\(',part) for part in parts]
  if not all(names):continue
  positions=[]
  for n in names:
   field=n.group(1)
   declarations=list(re.finditer(r'^[^\n{}()]*\b'+re.escape(field)+r'\b[^\n{}();]*;',header,re.M))
   positions.append(declarations[-1].start() if declarations else -1)
  indexed=list(zip(positions,range(len(parts)),parts));ordered=[v[2] for v in sorted(indexed)]
  if ordered==parts:continue
  changes.append({'file':str(p),'class':cls,'before':[n.group(1) for n in names], 'after':[re.match(r'^(\w+)',v).group(1) for v in ordered]})
  out=out[:start]+('\n    '+',\n    '.join(ordered)+'\n')+out[i:]
 if a.apply and out!=src:p.write_bytes((out.replace('\n','\r\n') if crlf else out).encode(enc))
Path('Saved/Analysis/Phase5InitializerRepairPlan.json').write_text(json.dumps(changes,indent=2));print(json.dumps({'constructors':len(changes),'files':len(set(x['file'] for x in changes)),'applied':a.apply}));
for c in changes:print(c['class'],c['before'],'->',c['after'])
