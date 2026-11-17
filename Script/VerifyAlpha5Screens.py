import unreal, json, os
assets=unreal.AssetToolsHelpers.get_asset_tools()
path='/Game/Screens/inGame/WBP_Skills'
parent=unreal.load_class(None,'/Script/StarshatterWars.SkillsDlg')
if not parent: raise RuntimeError('SkillsDlg native class missing')
bp=unreal.load_asset(path)
if not bp:
 factory=unreal.WidgetBlueprintFactory()
 factory.set_editor_property('parent_class',parent)
 bp=assets.create_asset('WBP_Skills','/Game/Screens/inGame',unreal.WidgetBlueprint,factory)
else:unreal.BlueprintEditorLibrary.reparent_blueprint(bp,parent)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
generated=unreal.BlueprintEditorLibrary.generated_class(bp)
if not generated or not isinstance(unreal.get_default_object(generated),unreal.SkillsDlg):raise RuntimeError('Skills parent verification failed')
if not unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False):raise RuntimeError('Skills save failed')
for name,native in [('WBP_Hangar','HangarDlg'),('WBP_Insurance','InsuranceDlg'),('WBP_ShipFit','ShipFitDlg')]:
 asset=unreal.load_asset('/Game/Screens/inGame/'+name)
 cls=unreal.BlueprintEditorLibrary.generated_class(asset)
 if not isinstance(unreal.get_default_object(cls),getattr(unreal,native)):raise RuntimeError('Wrong parent '+name)
unreal.log('ALPHA5_SCREEN_PARENTS_VERIFIED')
