import unreal, os, shutil, json

path = '/Game/Screens/inGame/WBP_Transaction'
parent = unreal.load_class(None, '/Script/StarshatterWars.TransactionDlg')
bp = unreal.load_asset(path)
if not parent or not bp:
    raise RuntimeError('Wallet Blueprint or native TransactionDlg is missing')
folder = os.path.join(unreal.Paths.project_saved_dir(), 'TransactionBlueprintMigration')
os.makedirs(folder, exist_ok=True)
source = os.path.join(unreal.Paths.project_content_dir(), 'Screens', 'inGame', 'WBP_Transaction.uasset')
backup = os.path.join(folder, 'WBP_Transaction.uasset.before')
if not os.path.exists(backup):
    shutil.copy2(source, backup)
unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
generated = unreal.BlueprintEditorLibrary.generated_class(bp)
if not generated or not isinstance(unreal.get_default_object(generated), unreal.TransactionDlg):
    raise RuntimeError('Wallet generated class has the wrong parent')
if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
    raise RuntimeError('Wallet Blueprint save failed')
with open(os.path.join(folder, 'report.json'), 'w') as report:
    json.dump({'asset': path, 'parent': parent.get_path_name()}, report, indent=2)
unreal.log('TRANSACTION_SCREEN_REPARENT_VERIFIED')
