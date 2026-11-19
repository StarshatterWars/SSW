import fs from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
const runtime='C:/Program Files/WindowsApps/OpenAI.CodexPrimaryRuntime.v26-1118-904-0_26.1118.904.0_x64__3k8sg7r9htsxt/dependencies';
const {PresentationFile,FileBlob}=await import(pathToFileURL(runtime+'/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs'));
const p=await PresentationFile.importPptx(await FileBlob.load('C:/Users/Carlo/OneDrive/Documents/Coventry/Agentic_AI_Reverse_Engineering_Dr_Carlos_Bott.pptx'));
console.log('masters',p.masters.items.length,'layouts',p.layouts?.items?.length,'slides',p.slides.items.length);
for(const i of [0,1,3]){
 const s=p.slides.items[i];console.log('slide',i,s.id,s.size, 'shapes',s.shapes.items.length);
 console.log(JSON.stringify(s.shapes.items.map(sh=>({id:sh.id,text:String(sh.text),style:sh.text.style,position:sh.position}))).slice(0,5500));
}
