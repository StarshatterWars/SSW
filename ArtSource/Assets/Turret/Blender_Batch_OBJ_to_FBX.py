"""Batch OBJ to FBX: scale 0.01, apply transforms, export smooth faces.

Run in Blender's Scripting workspace.
Run in a fresh scene: all scene objects are deleted before each import.
Requires Blender with the wm.obj_import operator and FBX exporter available.
"""

import bpy
from pathlib import Path

INPUT_FOLDER = Path("D:/SSW/Art Source/Ships/Turret")
OUTPUT_FOLDER = INPUT_FOLDER / "FBX"
SCALE = 0.01


def main():
    if not INPUT_FOLDER.is_dir():
        raise RuntimeError(f"Input folder does not exist: {INPUT_FOLDER}")

    OUTPUT_FOLDER.mkdir(parents=True, exist_ok=True)
    obj_files = sorted(
        p for p in INPUT_FOLDER.iterdir()
        if p.is_file() and p.suffix.lower() == ".obj"
    )
    if not obj_files:
        print(f"No OBJ files found in {INPUT_FOLDER}")
        return

    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")

    converted = 0
    for obj_path in obj_files:
        # Remove the default cube, camera, light and any previous model,
        # including hidden objects that selection-based deletion can miss.
        for scene_object in list(bpy.data.objects):
            bpy.data.objects.remove(scene_object, do_unlink=True)

        bpy.ops.wm.obj_import(filepath=str(obj_path))
        objects = list(bpy.context.selected_objects)
        if not objects:
            print(f"SKIPPED: {obj_path.name} - no objects imported")
            continue

        # Scale positions as well as geometry to preserve multi-object layouts.
        for obj in objects:
            obj.location *= SCALE
            obj.scale *= SCALE

        bpy.context.view_layer.objects.active = objects[0]
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

        # Smooth shading; do not recalculate or flip face normals.
        for obj in objects:
            if obj.type == "MESH":
                for polygon in obj.data.polygons:
                    polygon.use_smooth = True
                obj.data.update()

        output_path = OUTPUT_FOLDER / f"{obj_path.stem}.fbx"
        bpy.ops.export_scene.fbx(
            filepath=str(output_path),
            use_selection=True,
            object_types={"MESH", "EMPTY"},
            global_scale=1.0,
            apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_NONE",
            axis_forward="-Z",
            axis_up="Y",
            mesh_smooth_type="FACE",
            bake_anim=False,
            add_leaf_bones=False,
            path_mode="AUTO",
        )
        converted += 1
        print(f"EXPORTED: {output_path}")

    print(f"Finished: {converted} of {len(obj_files)} files converted.")


if __name__ == "__main__":
    main()
