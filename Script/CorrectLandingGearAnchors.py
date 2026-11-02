"""Align existing gear mesh wheel bottoms with legacy deployed endpoints."""
import json
import os
import shutil
import unreal

backup_dir = os.path.join(unreal.Paths.project_saved_dir(), "GearAnchorCorrection")
os.makedirs(backup_dir, exist_ok=True)
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
results = []
for name in ("NoseGear", "LeftGear", "RightGear"):
    blueprint = unreal.load_asset("/Game/Models/LandingGear/BP_" + name)
    if not blueprint:
        raise RuntimeError("Missing gear Blueprint: " + name)
    source = os.path.join(unreal.Paths.project_content_dir(), "Models", "LandingGear", "BP_" + name + ".uasset")
    backup = os.path.join(backup_dir, "BP_" + name + ".uasset.before")
    if not os.path.exists(backup):
        shutil.copy2(source, backup)
    seen = set()
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if not isinstance(component, unreal.StaticMeshComponent) or component.get_path_name() in seen:
            continue
        seen.add(component.get_path_name())
        mesh = component.get_editor_property("static_mesh")
        if not mesh:
            continue
        rotation = component.get_editor_property("relative_rotation")
        if abs(rotation.pitch) > 0.001 or abs(rotation.roll) > 0.001:
            raise RuntimeError("Gear pitch/roll needs explicit pivot conversion: " + name)
        bounds = mesh.get_bounds()
        location = component.get_editor_property("relative_location")
        scale = component.get_editor_property("relative_scale3d")
        old_z = location.z
        new_z = -(bounds.origin.z - bounds.box_extent.z) * scale.z
        component.set_editor_property("relative_location",unreal.Vector(location.x,location.y,new_z))
        results.append({"blueprint":name,"mesh":mesh.get_path_name(),"old_z":old_z,"new_z":new_z})
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint,only_if_is_dirty=False):
        raise RuntimeError("Failed to save gear: " + name)
with open(os.path.join(backup_dir,"report.json"),"w") as output:
    json.dump(results,output,indent=2)
unreal.log("GEAR_ANCHORS_CORRECTED")
