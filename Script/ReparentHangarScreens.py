import unreal, os, shutil, json
folder = os.path.join(unreal.Paths.project_saved_dir(), 'HangarBlueprintMigration')
os.makedirs(folder, exist_ok=True)
records = []
for name, native in [('WBP_Hangar','HangarDlg'),('WBP_Insurance','InsuranceDlg'),('WBP_ShipFit','ShipFitDlg')]:
    path = '/Game/Screens/inGame/' + name
    parent = unreal.load_class(None, '/Script/StarshatterWars.' + native)
    bp = unreal.load_asset(path)
    if not parent or not bp:
        raise RuntimeError('Missing asset or compiled parent: ' + path + ' -> ' + native)
    source = os.path.join(unreal.Paths.project_content_dir(), 'Screens', 'inGame', name + '.uasset')
    backup = os.path.join(folder, name + '.uasset.before')
    if not os.path.exists(backup): shutil.copy2(source, backup)
    before = str(unreal.EditorAssetLibrary.find_asset_data(path).get_tag_value('NativeParentClass'))
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    generated = unreal.BlueprintEditorLibrary.generated_class(bp)
    if not generated or not isinstance(unreal.get_default_object(generated), getattr(unreal, native)):
        raise RuntimeError('Wrong generated parent: ' + path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('Save failed: ' + path)
    records.append({'asset':path, 'before':before, 'parent':parent.get_path_name()})
    unreal.log('SCREEN_REPARENT_VERIFIED ' + path + ' -> ' + parent.get_path_name())
with open(os.path.join(folder, 'report.json'), 'w') as f: json.dump(records, f, indent=2)
unreal.log('HANGAR_REPARENT_COMPLETE')

