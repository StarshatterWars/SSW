import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const runtime='C:/Program Files/WindowsApps/OpenAI.CodexPrimaryRuntime.v26-1118-904-0_26.1118.904.0_x64__3k8sg7r9htsxt/dependencies';
process.env.RUNTIME_NODE_MODULES=runtime+'/node/node_modules';
process.env.RUNTIME_NODE=runtime+'/node/bin/node.exe';
const skill='C:/Users/Carlo/.codex/plugins/cache/openai-primary-runtime/presentations/26.1118.11504/skills/presentations';
const {Presentation,PresentationFile}=await import(pathToFileURL(runtime+'/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs'));
const root='D:/SSW/Docs/Presentations/EVE_REA_Lecture';
const build=root+'/Build',out=root+'/Output';
await fs.mkdir(build+'/renders',{recursive:true});await fs.mkdir(out,{recursive:true});
const data=JSON.parse(await fs.readFile(root+'/lecture-content.json','utf8'));
const presentation=Presentation.create({slideSize:{width:1280,height:720}});
const colors={bg:'#0B1422',title:'#EEF5FA',body:'#CDD9E5',accent:'#78D5E6',muted:'#8FA5BC'};
function text(s,value,x,y,w,h,size=25,color=colors.body,font='Arial',bold=false){
 const shape=s.shapes.add({geometry:'textbox',position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
 shape.text=value;shape.text.style={typeface:font,fontSize:size,color,bold,autoFit:'shrinkText'};return shape;
}
function wrapCode(code){
 return code.split('\n').flatMap(line=>{
  const out=[];let current=line;
  while(current.length>86){let at=current.lastIndexOf(' ',86);if(at<24)at=current.lastIndexOf(',',86)+1;if(at<24)break;out.push(current.slice(0,at));current='    '+current.slice(at).trimStart();}
  return [...out,current];
 }).join('\n');
}
const layoutAudit=[];
for(const [i,d] of data.slides.entries()){
 const slide=presentation.slides.add();slide.background.fill=colors.bg;
 if(i===0){
  text(slide,'Reverse engineering\nEVE Online',76,136,1110,220,64,colors.title,'Arial',true);
  text(slide,d.body[0],80,420,1100,90,32,colors.accent);
  text(slide,d.body[1],80,610,1080,46,21,colors.muted);
 }else{
  const titleLines=d.title.length>54?2:1;
  text(slide,d.title,64,42,1152,titleLines*60,44,colors.title,'Arial',true);
  const bodyTop=titleLines===2?178:145;
  if(d.image){
   text(slide,d.body.join('\n\n'),64,bodyTop,506,472,25);
   slide.images.add({blob:new Uint8Array(await fs.readFile(d.image)),contentType:'image/png',fit:'contain',alt:'New SSW mining drone: source model multi-view',position:{left:608,top:174,width:608,height:410}});
  }else if(d.code){
   const body=d.body.join('\n\n');
   const estimatedLines=d.body.reduce((n,v)=>n+Math.ceil(v.length/101),0)+(d.body.length-1);
   const bodyH=Math.max(55,estimatedLines*30+12);
   text(slide,body,64,bodyTop,1152,bodyH,24);
   const code=wrapCode(d.code),codeTop=bodyTop+bodyH+8,codeH=647-codeTop;
   const lineCount=code.split('\n').length;
   let font=23;if(lineCount*26>codeH)font=22;
   text(slide,code,70,codeTop,1146,codeH,font,colors.accent,'Consolas');
   layoutAudit.push({slide:i+1,title:d.title,codeLines:lineCount,font,availableHeight:codeH});
  }else{
   text(slide,d.body.join('\n\n'),64,bodyTop,1152,490-(bodyTop-145),27);
  }
  text(slide,`${i+1}   ·   Sources and qualifications in speaker notes`,64,668,1130,28,16,colors.muted);
 }
 slide.speakerNotes.text=[d.note||'Evidence-led SSW adaptation; native source presence is not automatically a runtime pass.',...d.refs.map(r=>'Source: '+r)].join('\n\n');
}
await fs.writeFile(build+'/layout-audit.json',JSON.stringify(layoutAudit,null,2));
const candidate=build+'/candidate.pptx';
await (await PresentationFile.exportPptx(presentation)).save(candidate);
console.log('Exported '+data.slides.length+' slides');
for(const [i,slide] of presentation.slides.items.entries()){
 const png=await presentation.export({slide,format:'png',scale:1});
 await fs.writeFile(build+'/renders/slide-'+String(i+1).padStart(2,'0')+'.png',new Uint8Array(await png.arrayBuffer()));
 const layout=await slide.export({format:'layout'});
 await fs.writeFile(build+'/renders/slide-'+String(i+1).padStart(2,'0')+'.layout.json',new Uint8Array(await layout.arrayBuffer()));
 if((i+1)%10===0)console.log('Rendered '+(i+1)+' slides');
}
const {finalizePresentation}=await import(pathToFileURL(skill+'/container_tools/artifact_tool_utils.mjs'));
const finalPath=out+'/EVE_REA_Starshatter_Lecture_v3.pptx';
const result=await finalizePresentation({
 workspaceDir:root,candidatePath:candidate,finalPath,
 explicitTotalSlideCount:data.slides.length,
 pythonExecutable:runtime+'/python/python.exe',
 integrityValidatorPath:skill+'/container_tools/inspect_presentation_package_integrity.py',
 layoutValidatorPath:skill+'/container_tools/inspect_presentation_layout_geometry.py',
 layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit'],
 fontPolicy:{basis:'design',families:['Arial','Consolas']},
 verifyArtifactToolImport:true,receiptPath:build+'/validation-v3.json'
});
await fs.copyFile(root+'/evidence-index.csv',out+'/evidence-index.csv');
console.log(JSON.stringify(result));
