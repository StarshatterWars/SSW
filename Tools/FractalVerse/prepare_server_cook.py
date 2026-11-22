"""Editor-only cook label: native server data, no UI or rendered world assets."""
import unreal
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/Game'],force_rescan=True)
assets=[a for a in registry.get_assets_by_path('/Game/Game',recursive=True) if str(a.asset_class_path.asset_name)=='DataTable']
assert assets,'No native data tables were found'
path='/Game/Server/PAL_FractalVerseCook'
label=unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not label:
 factory=unreal.DataAssetFactory()
 factory.set_editor_property('data_asset_class',unreal.PrimaryAssetLabel)
 label=unreal.AssetToolsHelpers.get_asset_tools().create_asset('PAL_FractalVerseCook','/Game/Server',unreal.PrimaryAssetLabel,factory)
assert label
rules=unreal.PrimaryAssetRules()
rules.set_editor_property('cook_rule',unreal.PrimaryAssetCookRule.ALWAYS_COOK)
label.set_editor_property('rules',rules)
label.set_editor_property('label_assets_in_my_directory',False)
label.set_editor_property('is_runtime_label',False)
label.set_editor_property('explicit_assets',[a.get_asset() for a in assets])
assert unreal.EditorAssetLibrary.save_loaded_asset(label)
unreal.log('[FractalVerseCook] Labeled '+str(len(assets))+' native data tables for the headless server.')
