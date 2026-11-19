import fs from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
const runtime='C:/Program Files/WindowsApps/OpenAI.CodexPrimaryRuntime.v26-1118-904-0_26.1118.904.0_x64__3k8sg7r9htsxt/dependencies';
process.env.RUNTIME_NODE_MODULES=runtime+'/node/node_modules';
process.env.RUNTIME_NODE=runtime+'/node/bin/node.exe';
const skill='C:/Users/Carlo/.codex/plugins/cache/openai-primary-runtime/presentations/26.1118.11504/skills/presentations';
const {PresentationFile,FileBlob}=await import(pathToFileURL(runtime+'/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs'));
const root='D:/SSW/Docs/Presentations/CombinedLecture';
const build=root+'/Build',out=root+'/Output';
await fs.mkdir(build+'/renders',{recursive:true});await fs.mkdir(out,{recursive:true});
const source='C:/Users/Carlo/OneDrive/Documents/Coventry/Agentic_AI_Reverse_Engineering_Dr_Carlos_Bott.pptx';
const presentation=await PresentationFile.importPptx(await FileBlob.load(source));
const originals=[...presentation.slides.items];
const data=JSON.parse(await fs.readFile(root+'/combined-content.json','utf8'));
presentation.slides.keep(data.slides.filter(s=>s.kind==='source').map(s=>s.source-1));
const color={bg:'#F1ECDF',blue:'#1F3DA8',ink:'#1A1A1A',muted:'#6B6B6B',footer:'#9A9A9A'};
function text(s,value,x,y,w,h,size=23,fill=color.ink,bold=false,font='Helvetica Neue'){
 const sh=s.shapes.add({geometry:'textbox',position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
 sh.text=value;sh.text.style={typeface:font,fontSize:size,color:fill,bold,autoFit:'shrinkText',insets:0};return sh;
}
function rectangle(s,x,y,w,h,fill){return s.shapes.add({geometry:'rect',position:{left:x,top:y,width:w,height:h},fill,line:{fill:'none',width:0}});}
function frame(s,d,index){
 s.background.fill=color.bg;rectangle(s,0,0,1280,7.68,color.blue);
 text(s,d.appendix?'APPENDIX  /  CODE EXHIBITS':d.part==='Theory'?'PART I  /  THEORY':d.part==='Discussion'?'DISCUSSION':'PART II  /  PRACTICAL',52.8,26.88,1000,27,14.67,color.muted);
 text(s,d.title,52.8,72,1171.2,56,37.33,color.ink,true);
 text(s,d.appendix?'BACKUP '+String(index-34).padStart(2,'0'):String(index+1).padStart(2,'0')+' / 35',52.8,676.8,300,27,13.33,color.footer);
 text(s,d.appendix?'NOT IN TIMED LECTURE':d.part.toUpperCase(),880,676.8,350,27,13.33,color.footer);
}
function find(slide,needle){return slide.shapes.items.find(s=>String(s.text).includes(needle));}
function replace(slide,needle,value,size,colorValue=color.ink,bold=false){
 const sh=find(slide,needle);if(!sh)throw new Error('Missing source shape '+needle);
 sh.text=value;sh.text.style={typeface:'Helvetica Neue',fontSize:size,color:colorValue,bold,autoFit:'shrinkText'};return sh;
}
const ordered=[],timing=[];
for (const [i,d] of data.slides.entries()){
 let s;
 if(d.kind==='source'){
  s=originals[d.source-1];
  if(d.source===1){
   replace(s,'Leveraging NSA','Part I: Theory   /   Part II: Practical REA',24,color.blue);
  } else if(d.source===2){
   replace(s,'Today','One hour: theory, practical and discussion',37.33,color.ink,true);
   const labels=['Foundations, evidence and permission','Tools: Ghidra, ILSpy and Python bytecode','Agentic workflows and verification','Practical: raw bytes and recovered source','SSW comparisons grounded in recovered rules','Limits, takeaways and reproducible evidence','Questions and discussion'];
   const needles=['Foundations of Reverse','Core Tools:','What is Agentic','Agentic Workflows','Practical Integration','Challenges, Ethics','Future Directions'];
   labels.forEach((v,k)=>replace(s,needles[k],v,23));
  } else if(d.source===3){
   const replacements=[['Understand the purpose','Explain what compiled artifacts can and cannot reveal.'],['Navigate NSA','Choose Ghidra, ILSpy or a bytecode decoder for the target.'],['Explain what Agentic','Describe a tool-using agent and its analysis loop.'],['Design practical','Trace raw instructions into readable recovered logic.'],['Identify limitations','Check claims against disassembly and state clear limits.'],['Apply these','Relate recovered rules to short native SSW examples.']];
   replacements.forEach(([a,b])=>replace(s,a,b,22));
  } else if(d.source===5){
   replace(s,'Legal & Ethical','Permission, scope and handling',37.33,color.ink,true);
   const replacements=[['Generally Permitted','Authorization'],['Security research, interoperability','Define the target, purpose and permitted actions before examining an artifact.'],['High Risk / Often Illegal','License and institutional scope'],['Circumventing DRM','Check applicable terms and requirements. Ownership alone does not settle every permission question.'],['Best Practice','Controlled handling'],['Always document scope','Keep originals intact. Minimize shared code and protect any recovered secrets.'],['University Context','Lecture scope'],['This lecture focuses','Static educational examples. We do not execute the recovered client or recover its remote server implementation.']];
   replacements.forEach(([a,b])=>replace(s,a,b,b.length>45?19:20,b.length>45?color.ink:color.blue,b.length<=45));
  } else if(d.source===7){
   replace(s,'Leading open-source','Managed assembly inspection and readable C# recovery',19);
   replace(s,'JADX','Python bytecode',20,color.blue,true);
   replace(s,'Popular for Android','xdis for instructions; uncompyle6 for source recovery in this case study',19);
  } else if(d.source===11){
   replace(s,'•  Decompiles CIL','•  Decompiles managed IL to readable C#',20);
   replace(s,'•  Debugger integration','•  Pair static inspection with a compatible debugger when needed',20);
   const bullets=s.shapes.items.filter(sh=>String(sh.text).startsWith('•')).sort((a,b)=>a.position.top-b.position.top);
   bullets.forEach((sh,k)=>{sh.position={...sh.position,top:244+k*55,height:46};});
  } else if(d.source===23){
   replace(s,'Use multiple models','Test the same claim with a second tool or a controlled trace.',21);
   replace(s,'Prefer agents that cite','Require function identities, hashes and specific operations for important claims.',21);
  } else if(d.source===28){
   replace(s,'What areas','Which conclusion is supported by the recovered function?\nWhat evidence would establish behavior across an RPC boundary?\nWhere would you place human review in the agent loop?',24);
  } else if(d.source===29){
   replace(s,'Ghidra  ·','Theory, practical REA and evidence-led implementation',22,color.blue);
  }
  // Preserve the source geometry, theme, shapes and fonts; update its running footer.
  const foot=s.shapes.items.find(sh=>/^\d\d \/ 29$/.test(String(sh.text)));
  if(foot){foot.text=String(i+1).padStart(2,'0')+' / 35';foot.text.style={typeface:'Helvetica Neue',fontSize:13.33,color:color.footer};}
 }else{
  s=presentation.slides.add({width:1280,height:720});
  if(d.kind==='divider'){
   s.background.fill=color.bg;rectangle(s,0,0,1280,230.4,color.blue);
   text(s,d.title,67.2,70,1152,90,48,'#FFFFFF',true);
   text(s,d.subtitle,67.2,275,1120,130,36,color.blue);
   text(s,d.title.includes('PART I  /')?'25 minutes':'28 minutes',67.2,465,1000,54,28,color.ink);
   text(s,String(i+1).padStart(2,'0')+' / 35',67.2,676.8,300,27,13.33,color.footer);
  }else{
   frame(s,d,i);
   if(d.code){
    let code=d.code;
    if(d.title==='Ore scan knowledge: recovered source')code='def is_quantity_known(self, asteroid_id):\n    return self.get_quantity(asteroid_id) not in (\n        None, QUANTITY_UNKNOWN\n    )';
    if(d.title==='Drone recovery command: recovered source')code='def ReturnToDroneBay(droneIDs):\n    entity = eveMoniker.GetEntityAccess()\n    if entity:\n        ret = entity.CmdReturnBay(droneIDs)\n        HandleMultipleCallError(\n            droneIDs, ret, \'MultiDroneCmdResult\'\n        )\n    return';
    text(s,d.body.join('\n'),52.8,145,1171.2,75,23);
    const lines=code.split('\n').length;
    const size=d.title.includes('bytecode')?(lines>17?21:23):24;
    text(s,code,60,235,1160,415,size,color.blue,false,'Consolas');
   }else{
    const body=d.title==='The investigation starts with an artifact' ? ['Target: the installed EVE client archive, code.ccp.','REA frames the question and artifact inspection.','Static fallback: xdis decodes PyPy 2.7; uncompyle6 recovers Python.','Record the member, hash and function. Never execute recovered code.'] : d.body;
    body.forEach((line,j)=>text(s,line,52.8,155+j*(body.length>5?70:100),1171.2,body.length>5?62:86,25,j===0?color.blue:color.ink,j===0));
   }
  }
 }
 if(d.appendix)s.hidden=true;
 const mm=seconds=>String(Math.floor(seconds/60)).padStart(2,'0')+':'+String(seconds%60).padStart(2,'0');
 const pacing=d.appendix?'BACKUP. Hidden from the one-hour slide show. No allocated speaking time.':`TIMING ${mm(d.start)}–${mm(d.end)}. Speak for ${d.seconds} seconds. Part: ${d.part}.`;
 let prompts='';
 if(d.kind==='practical'&&d.code)prompts=d.title.includes('bytecode')?'Explain the function identity, then one important instruction and its operand. Ask what conclusion the bytes support. Do not read every opcode.':'Walk through the recovered expression or branch. Explain the rule it proves, then identify the dependencies or authority it leaves unknown.';
 if(d.title?.startsWith('SSW example:'))prompts='Label this as native SSW implementation. Explain the link to the previous recovered rule. State which behavior SSW supplies independently.';
 if(d.source===28)prompts='Reserve this entire interval for questions. If the room is quiet, ask the three displayed questions. Optional hidden exhibits are available for follow-up. Finish at 59:45.';
 if(d.source===16)prompts='Walk through one scoped question. Explain why each request should return a function, artifact identity and uncertainty rather than a broad speculative summary.';
 s.speakerNotes.text=[pacing,prompts,d.note||'',...(d.refs||[]).map(r=>'Source: '+r)].filter(Boolean).join('\n\n');
 ordered.push('sl/'+s.id);timing.push({slide:i+1,title:d.title||String(s.shapes.items.find(sh=>sh.position.top>=70&&sh.position.top<100)?.text||''),part:d.part||'Appendix',seconds:d.seconds,start:d.start,end:d.end,hidden:!!d.appendix});
}
presentation.slides.reorder(ordered);
await fs.writeFile(build+'/timing.json',JSON.stringify(timing,null,2));
const candidate=build+'/candidate.pptx';
await (await PresentationFile.exportPptx(presentation)).save(candidate);
console.log('Exported '+ordered.length+' slides. Timed main deck: 35 slides, 3600 seconds.');
for(const [i,s] of presentation.slides.items.entries()){
 const preview=await presentation.export({slide:s,format:'png',scale:1});
 await fs.writeFile(build+'/renders/slide-'+String(i+1).padStart(2,'0')+'.png',new Uint8Array(await preview.arrayBuffer()));
 const layout=await s.export({format:'layout'});
 await fs.writeFile(build+'/renders/slide-'+String(i+1).padStart(2,'0')+'.layout.json',new Uint8Array(await layout.arrayBuffer()));
 if((i+1)%10===0)console.log('Rendered '+(i+1));
}
const {finalizePresentation}=await import(pathToFileURL(skill+'/container_tools/artifact_tool_utils.mjs'));
const result=await finalizePresentation({workspaceDir:root,candidatePath:candidate,finalPath:out+'/Agentic_AI_REA_Theory_and_Practical_60_Minutes_Final.pptx',explicitTotalSlideCount:49,pythonExecutable:runtime+'/python/python.exe',integrityValidatorPath:skill+'/container_tools/inspect_presentation_package_integrity.py',layoutValidatorPath:skill+'/container_tools/inspect_presentation_layout_geometry.py',layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit'],fontPolicy:{basis:'design',families:['Helvetica Neue','Consolas']},verifyArtifactToolImport:true,receiptPath:build+'/validation-final.json'});
console.log(JSON.stringify(result));
