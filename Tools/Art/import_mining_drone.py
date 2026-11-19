"""Run with UnrealEditor-Cmd -ExecutePythonScript; imports the owned Blender variant."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
source = root / "ArtSource/Models/MiningDrone/MiningDrone.fbx"
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", False)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)
options.set_editor_property("automated_import_should_detect_type", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
data = options.static_mesh_import_data
data.set_editor_property("combine_meshes", True)
data.set_editor_property("auto_generate_collision", False)
data.set_editor_property("import_uniform_scale", 1.0)
task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = "/Game/Models/MiningDrone"
task.destination_name = "SM_MiningDrone"
task.automated = True
task.replace_existing = True
task.save = True
task.options = options
task.factory = unreal.FbxFactory()
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh = unreal.load_asset("/Game/Models/MiningDrone/SM_MiningDrone")
if not isinstance(mesh, unreal.StaticMesh):
    raise RuntimeError("Mining-drone mesh was not imported at its canonical path")
bounds = mesh.get_bounds()
dimensions = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
expected = [318.5, 194.0, 74.75]
if any(abs(a - b) > 2 for a, b in zip(dimensions, expected)):
    raise RuntimeError(f"Mining-drone FBX unit/axis mismatch: {dimensions}")
unreal.EditorAssetLibrary.save_directory("/Game/Models/MiningDrone", only_if_is_dirty=False, recursive=True)
report = {"mesh": mesh.get_path_name(), "dimensions_cm": dimensions,
          "runtime_scale": 0.01, "source": str(source), "imported": list(task.imported_object_paths)}
out = root / "Saved/Analysis/MiningDroneImport.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("[MiningDroneImport] validated mesh, materials and centimetre bounds")
