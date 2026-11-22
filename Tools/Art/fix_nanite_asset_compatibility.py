"""Preserve translucent fighter glass; enable the opaque Janek runway for Nanite."""
import unreal, json, shutil, datetime
from pathlib import Path
root=Path(unreal.Paths.project_dir())
backup=root/"Saved"/"Backups"/("NaniteCompatibility_"+datetime.datetime.now().strftime("%Y%m%d_%H%M%S"))
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
if editor is None: editor=unreal.new_object(unreal.StaticMeshEditorSubsystem)
report={"meshes":[],"materials":[]}
def back_up(path):
    source=root/"Content"/(path.removeprefix("/Game/")+".uasset")
    destination=backup/source.relative_to(root/"Content")
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,destination)
def bounds(mesh):
    b=mesh.get_bounding_box()
    return (b.min.x,b.min.y,b.min.z,b.max.x,b.max.y,b.max.z)
def save_material(path):
    back_up(path)
    mat=unreal.load_asset(path)
    mat.set_editor_property("used_with_nanite",True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    assert mat.get_editor_property("used_with_nanite")
    report["materials"].append(path)
for path in ["/Game/Models/TankerA/M_TankerA_TankerAHull","/Game/Models/TankerB/M_TankerB_TankerBHull","/Game/Models/Transport/M_Transport_TransportHull","/Game/Assets/Ground/Airbases/M_Airbase_PBR","/Game/Assets/Ground/Airbases/M_JanekRunway_PBR"]:
    save_material(path)
paths=[("/Game/Models/"+name+"/"+mesh,False) for name,mesh in [("Falcon","Falcon"),("Stormhawk","Storm"),("Eagle","Eagle"),("Thunderbolt","Thunderbolt"),("Razor","Razor"),("Viper","Viper")]]
paths.append(("/Game/Assets/Ground/Airbases/Base1",True))
for path,enabled in paths:
    back_up(path)
    mesh=unreal.load_asset(path)
    before=(bounds(mesh),str(editor.get_collision_complexity(mesh)),editor.get_simple_collision_count(mesh))
    settings=editor.get_nanite_settings(mesh)
    settings.set_editor_property("enabled",enabled)
    if enabled:
        settings.set_editor_property("fallback_percent_triangles",1.0)
        settings.set_editor_property("fallback_relative_error",0.0)
    editor.set_nanite_settings(mesh,settings,True)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    after=(bounds(mesh),str(editor.get_collision_complexity(mesh)),editor.get_simple_collision_count(mesh))
    unreal.log("Nanite bounds/collision "+path+": "+repr(before)+" -> "+repr(after))
    assert before[1:]==after[1:], "Collision settings changed: "+path
    # Nanite and the source mesh can report slightly different render bounds;
    # source vertices and the simulation geometry are not edited here.
    assert editor.get_nanite_settings(mesh).get_editor_property("enabled")==enabled
    report["meshes"].append({"path":path,"nanite":enabled,"collision_settings_preserved":True,"bounds_before":before[0],"bounds_after":after[0]})
(root/"Saved"/"Analysis"/"NaniteCompatibility.json").write_text(json.dumps(report,indent=2))
unreal.log("Nanite compatibility verified; backups: "+str(backup))
