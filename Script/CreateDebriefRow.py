"""Create the thin UMG row asset required by UE ListView in Editor builds."""
import unreal
path='/Game/Screens/Mission/Rows/WBP_DebriefRow'
asset=unreal.load_asset(path)
if not asset:
    factory=unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',unreal.load_class(None,'/Script/StarshatterWars.DebriefRowWidget'))
    asset=unreal.AssetToolsHelpers.get_asset_tools().create_asset('WBP_DebriefRow','/Game/Screens/Mission/Rows',unreal.WidgetBlueprint,factory)
if not asset: raise RuntimeError('Could not create debrief row')
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
if not unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False): raise RuntimeError('Could not save debrief row')
unreal.log('DEBRIEF_ROW_READY')
