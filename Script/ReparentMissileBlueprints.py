"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.

Reparent existing weapon Blueprints without replacing their mesh components.
"""
import json
import os
import shutil
import unreal

names = ("Archer", "Hammer", "Harpoon", "Javelin", "Torpedo", "Vector")
report_dir = os.path.join(unreal.Paths.project_saved_dir(), "MissileBlueprintMigration")
os.makedirs(report_dir, exist_ok=True)
records = []
for name in names:
    parent = unreal.load_class(None, "/Script/StarshatterWars." + name + "MissileActor")
    if not parent:
        raise RuntimeError("Build the native " + name + "MissileActor class before reparenting.")
    path = "/Game/Models/Weapons/{0}/BP_{0}".format(name)
    blueprint = unreal.load_asset(path)
    if not blueprint:
        raise RuntimeError("Missing missile Blueprint: " + path)
    old_parent = unreal.EditorAssetLibrary.find_asset_data(path).get_tag_value("NativeParentClass")
    asset_file = os.path.join(unreal.Paths.project_content_dir(), "Models", "Weapons", name, "BP_" + name + ".uasset")
    backup = os.path.join(report_dir, "BP_" + name + ".uasset.before")
    if not os.path.exists(backup):
        shutil.copy2(asset_file, backup)
    unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent)
    # A folder moved outside Unreal can leave a stale mesh reference. Reuse the
    # existing component and current model asset, preserving its transform.
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    mesh_components = []
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if isinstance(component, unreal.StaticMeshComponent):
            if component.get_editor_property("static_mesh") is None:
                mesh_path = "/Game/Models/Weapons/{0}/{1}".format(name, name.lower())
                mesh = unreal.load_asset(mesh_path)
                if not mesh:
                    raise RuntimeError("Missing model asset: " + mesh_path)
                component.set_static_mesh(mesh)
            mesh_components.append(component.get_editor_property("static_mesh").get_path_name())
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    if not isinstance(unreal.get_default_object(generated), unreal.MissileActor):
        raise RuntimeError("Reparent failed: " + path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError("Save failed: " + path)
    records.append({"asset": path, "old_parent": old_parent, "parent": parent.get_path_name(), "meshes": mesh_components, "backup": backup})

with open(os.path.join(report_dir, "report.json"), "w") as output:
    json.dump(records, output, indent=2)
unreal.log("MISSILE_REPARENT_COMPLETE: " + str(len(records)))
