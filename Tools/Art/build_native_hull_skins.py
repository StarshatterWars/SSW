"""Editor authoring: persist UE skin materials; runtime only selects the cooked assets."""
import json, unreal
from pathlib import Path
root=Path('D:/SSW')
path=root/'Content/GameData/Ships/ships.json'
rows=json.loads(path.read_text(encoding='utf-8-sig'))
lib=unreal.MaterialEditingLibrary
ea=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.JPG 0')
folder='/Game/Materials/Ships/Skins'
ea.make_directory(folder)
report=[]
def imported(name):
    asset=folder+'/T_'+name
    if ea.does_asset_exist(asset):return unreal.load_asset(asset)
    candidates=list((root/'ArtSource/Models/Civil').glob(name+'.*'))
    if not candidates:return None
    task=unreal.AssetImportTask();task.filename=str(candidates[0]);task.destination_path=folder;task.destination_name='T_'+name
    task.automated=True;task.save=True;task.factory=unreal.TextureFactory();tools.import_asset_tasks([task])
    return unreal.load_asset(asset)
white=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
master_path=folder+'/M_SSW_HullSkin'
master=unreal.load_asset(master_path)
if not master:
    master=tools.create_asset('M_SSW_HullSkin',folder,unreal.Material,unreal.MaterialFactoryNew())
    def node(cls,x,y):return lib.create_material_expression(master,cls,x,y)
    texture=node(unreal.MaterialExpressionTextureSampleParameter2D,-700,0)
    texture.set_editor_property('parameter_name','BaseColorTexture');texture.set_editor_property('texture',white)
    tint=node(unreal.MaterialExpressionVectorParameter,-700,220)
    tint.set_editor_property('parameter_name','BaseColor');tint.set_editor_property('default_value',unreal.LinearColor(1,1,1,1))
    mult=node(unreal.MaterialExpressionMultiply,-380,0)
    lib.connect_material_expressions(texture,'RGB',mult,'A');lib.connect_material_expressions(tint,'RGB',mult,'B')
    lib.connect_material_property(mult,'',unreal.MaterialProperty.MP_BASE_COLOR)
    glow=node(unreal.MaterialExpressionTextureSampleParameter2D,-700,460)
    glow.set_editor_property('parameter_name','EmissiveTexture');glow.set_editor_property('texture',white)
    strength=node(unreal.MaterialExpressionScalarParameter,-700,680)
    strength.set_editor_property('parameter_name','EmissiveStrength');strength.set_editor_property('default_value',0.0)
    emit=node(unreal.MaterialExpressionMultiply,-380,460)
    lib.connect_material_expressions(glow,'RGB',emit,'A');lib.connect_material_expressions(strength,'',emit,'B')
    lib.connect_material_property(emit,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough=node(unreal.MaterialExpressionScalarParameter,-380,750)
    rough.set_editor_property('parameter_name','Roughness');rough.set_editor_property('default_value',0.62)
    lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    metallic=node(unreal.MaterialExpressionScalarParameter,-380,900)
    metallic.set_editor_property('parameter_name','Metallic');metallic.set_editor_property('default_value',0.35)
    lib.connect_material_property(metallic,'',unreal.MaterialProperty.MP_METALLIC)
    lib.recompile_material(master)
    assert ea.save_loaded_asset(master,False)
for row in rows:
    for skin in row.get('Skin',[]):
        for cell in skin.get('Cells',[]):
            diffuse=imported(cell.get('TextureName',''))
            fallback=False
            if not diffuse and row['Model']=='Zodiac':
                diffuse=unreal.load_asset('/Game/Models/Zodiac/TEX_Zodiac_D');fallback=True
            if not diffuse:raise RuntimeError('Missing native skin diffuse: '+row['Row']+' '+skin['Name'])
            name='MI_'+row['Row']+'_'+skin['Name']+'_'+cell['MaterialName']
            mi=unreal.load_asset(folder+'/'+name)
            if not mi:
                factory=unreal.MaterialInstanceConstantFactoryNew()
                mi=tools.create_asset(name,folder,unreal.MaterialInstanceConstant,factory)
            lib.set_material_instance_parent(mi,master)
            lib.set_material_instance_texture_parameter_value(mi,'BaseColorTexture',diffuse)
            tint=unreal.LinearColor(.75,1,.75,1) if fallback and skin['Name']=='Green' else unreal.LinearColor(1,1,1,1)
            lib.set_material_instance_vector_parameter_value(mi,'BaseColor',tint)
            glow=imported(cell.get('EmissiveTextureName',''))
            if glow:
                lib.set_material_instance_texture_parameter_value(mi,'EmissiveTexture',glow)
                lib.set_material_instance_scalar_parameter_value(mi,'EmissiveStrength',.8)
            else:lib.set_material_instance_scalar_parameter_value(mi,'EmissiveStrength',0)
            lib.update_material_instance(mi)
            assert ea.save_loaded_asset(mi,False)
            cell['UnrealMaterial']=mi.get_path_name()
            report.append({'design':row['Row'],'skin':skin['Name'],'slot':cell['MaterialName'],'material':mi.get_path_name(),'diffuse':diffuse.get_path_name(),'usesBaseZodiacTexture':fallback})
backup=root/'Saved/Backups/NativeHullSkins'
backup.mkdir(parents=True,exist_ok=True)
if not (backup/'ships-before-materials.json').exists():(backup/'ships-before-materials.json').write_bytes(path.read_bytes())
path.write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
(root/'Saved/NativeHullSkinAssets.json').write_text(json.dumps(report,indent=2))
unreal.log('SSW native hull skin materials saved: '+str(len(report)))
