import {pathToFileURL} from 'node:url';
const runtime='C:/Program Files/WindowsApps/OpenAI.CodexPrimaryRuntime.v26-1118-904-0_26.1118.904.0_x64__3k8sg7r9htsxt/dependencies';
process.env.RUNTIME_NODE_MODULES=runtime+'/node/node_modules';
process.env.RUNTIME_NODE=runtime+'/node/bin/node.exe';
process.env.RUNTIME_PYTHON=runtime+'/python/python.exe';
const skill='C:/Users/Carlo/.codex/plugins/cache/openai-primary-runtime/presentations/26.1118.11504/skills/presentations';
const root='D:/SSW/Docs/Presentations/EVE_REA_Lecture';
const {finalizePresentation}=await import(pathToFileURL(skill+'/container_tools/artifact_tool_utils.mjs'));
const result=await finalizePresentation({
 workspaceDir:root,candidatePath:root+'/Build/candidate.pptx',finalPath:root+'/Output/EVE_REA_Starshatter_Lecture.pptx',
 explicitTotalSlideCount:50,pythonExecutable:runtime+'/python/python.exe',
 integrityValidatorPath:skill+'/container_tools/inspect_presentation_package_integrity.py',
 layoutValidatorPath:skill+'/container_tools/inspect_presentation_layout_geometry.py',
 layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit'],
 fontPolicy:{basis:'design',families:['Arial','Consolas']},verifyArtifactToolImport:true,receiptPath:root+'/Build/validation.json'
});console.log(JSON.stringify(result));
