"""Author eight cosmetic finish families using cooked UE material instances."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
PATH = ROOT / "Content/GameData/Ships/ships.json"
FOLDER = "/Game/Materials/Ships/Skins/Starter"
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
PALETTE = [
    ("Naval Blue", (0.38, 0.58, 1.0), 0.9, 1.0),
    ("Arctic White", (1.0, 1.0, 1.0), 0.9, 0.8),
    ("Gunmetal", (0.60, 0.67, 0.74), 0.65, 1.4),
    ("Desert Sand", (1.0, 0.78, 0.45), 1.1, 0.7),
    ("Forest Green", (0.48, 0.80, 0.50), 1.05, 0.8),
    ("Rescue Orange", (1.0, 0.48, 0.18), 0.9, 0.9),
    ("Crimson", (1.0, 0.30, 0.28), 0.8, 1.0),
    ("Midnight", (0.42, 0.48, 0.62), 0.75, 1.1),
]
MODELS = {
    "Falcon": ("FalconShipActor", "/Game/Models/Falcon/Falcon_D_4K", "/Game/Models/Falcon/Falcon_D_SSW_ORM"),
    "Stormhawk": ("StormhawkShipActor", "/Game/Models/Stormhawk/TEX_STORM_D", "/Game/Models/Stormhawk/Hull_SSW_ORM"),
    "Zodiac": ("ZodiacShipActor", "/Game/Models/Zodiac/TEX_Zodiac_D", "/Game/Models/Zodiac/Main_SSW_ORM"),
}
ASSETS.make_directory(FOLDER)
master = unreal.load_asset(FOLDER + "/M_SSW_StarterSkin") if ASSETS.does_asset_exist(FOLDER + "/M_SSW_StarterSkin") else None
if master:
    LIB.delete_all_material_expressions(master)
else:
    master = TOOLS.create_asset("M_SSW_StarterSkin", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
if master:
    def node(cls, x, y):
        return LIB.create_material_expression(master, cls, x, y)
    base = node(unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    base.set_editor_property("parameter_name", "BaseColorTexture")
    base.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    tint = node(unreal.MaterialExpressionVectorParameter, -700, 200)
    tint.set_editor_property("parameter_name", "HullTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    multiply = node(unreal.MaterialExpressionMultiply, -350, 0)
    LIB.connect_material_expressions(base, "RGB", multiply, "A")
    LIB.connect_material_expressions(tint, "RGB", multiply, "B")
    LIB.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)
    orm = node(unreal.MaterialExpressionTextureSampleParameter2D, -700, 400)
    orm.set_editor_property("parameter_name", "ORMTexture")
    orm.set_editor_property("texture", unreal.load_asset(MODELS["Falcon"][2]))
    orm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    LIB.connect_material_property(orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    for name, channel, prop, y in [("RoughnessScale", "G", unreal.MaterialProperty.MP_ROUGHNESS, 400),
                                    ("MetallicScale", "B", unreal.MaterialProperty.MP_METALLIC, 650)]:
        scale = node(unreal.MaterialExpressionScalarParameter, -700, y + 170)
        scale.set_editor_property("parameter_name", name)
        scale.set_editor_property("default_value", 1.0)
        blend = node(unreal.MaterialExpressionMultiply, -350, y)
        LIB.connect_material_expressions(orm, channel, blend, "A")
        LIB.connect_material_expressions(scale, "", blend, "B")
        LIB.connect_material_property(blend, "", prop)
    LIB.recompile_material(master)
    if not ASSETS.save_loaded_asset(master, False):
        raise RuntimeError("Could not save starter skin master")

bindings = {}
report = []
for model, (actor_name, diffuse_path, orm_path) in MODELS.items():
    cls = unreal.load_class(None, "/Script/StarshatterWars." + actor_name)
    if not cls:
        raise RuntimeError("Missing native hull class " + actor_name)
    actor = unreal.get_default_object(cls)
    mesh = actor.get_editor_property("hull_mesh").get_editor_property("static_mesh")
    if not mesh:
        fallback={"Falcon":"/Game/Models/Falcon/Falcon", "Stormhawk":"/Game/Models/Stormhawk/Storm", "Zodiac":"/Game/Models/Zodiac/SM_Zodiac"}
        mesh=unreal.load_asset(fallback[model])
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Missing native hull mesh " + model)
    slots = mesh.get_editor_property("static_materials")
    eligible = []
    for index, slot in enumerate(slots):
        material = slot.material_interface
        names = (str(slot.material_slot_name) + " " + (material.get_name() if material else "")).lower()
        if any(word in names for word in ("glass", "canopy", "cockpit", "exhaust", "light")):
            continue
        if any(word in names for word in ("hull", "main", "falcon_d", "body")):
            eligible.append((index, str(slot.material_slot_name)))
    if not eligible:
        raise RuntimeError("No opaque body slots identified for " + model)
    diffuse, orm = unreal.load_asset(diffuse_path), unreal.load_asset(orm_path)
    if not diffuse or not orm:
        raise RuntimeError("Missing authored albedo/ORM for " + model)
    bindings[model] = {}
    for name, color, roughness, metallic in PALETTE:
        asset_name = "MI_" + model + "_" + name.replace(" ", "_")
        material = unreal.load_asset(FOLDER + "/" + asset_name) if ASSETS.does_asset_exist(FOLDER + "/" + asset_name) else None
        if not material:
            material = TOOLS.create_asset(asset_name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        LIB.set_material_instance_parent(material, master)
        LIB.set_material_instance_texture_parameter_value(material, "BaseColorTexture", diffuse)
        LIB.set_material_instance_texture_parameter_value(material, "ORMTexture", orm)
        LIB.set_material_instance_vector_parameter_value(material, "HullTint", unreal.LinearColor(*color, 1))
        LIB.set_material_instance_scalar_parameter_value(material, "RoughnessScale", roughness)
        LIB.set_material_instance_scalar_parameter_value(material, "MetallicScale", metallic)
        LIB.update_material_instance(material)
        if not ASSETS.save_loaded_asset(material, False):
            raise RuntimeError("Could not save " + asset_name)
        cell_defaults = {'TextureName': '', 'AuxTextureName': '', 'SpecularTextureName': '', 'BumpTextureName': '', 'EmissiveTextureName': '', 'AmbientColor': {'R': 1, 'G': 1, 'B': 1, 'A': 1}, 'DiffuseColor': {'R': 1, 'G': 1, 'B': 1, 'A': 1}, 'SpecularColor': {'R': 1, 'G': 1, 'B': 1, 'A': 1}, 'EmissiveColor': {'R': 1, 'G': 1, 'B': 1, 'A': 1}, 'SpecularPower': 1, 'BumpScale': 1, 'bHasAmbientColor': False, 'bHasDiffuseColor': False, 'bHasSpecularColor': False, 'bHasEmissiveColor': False, 'bHasSpecularPower': False, 'bHasBumpScale': False}
        bindings[model][name] = [{**cell_defaults, "MaterialName": slot, "CellIndex": -1, "UnrealMaterial": material.get_path_name()}
                                for index, slot in eligible]
        report.append({"model": model, "finish": name, "material": material.get_path_name(), "slots": eligible})

rows = json.loads(PATH.read_text(encoding="utf-8-sig"))
backup = ROOT / "Saved/Backups/StarterShipSkins"
backup.mkdir(parents=True, exist_ok=True)
if not (backup / "ships.json").exists():
    (backup / "ships.json").write_bytes(PATH.read_bytes())
for row in rows:
    model = row["Row"] if row["Row"] in ("Falcon", "Stormhawk") else "Zodiac" if row["Row"] in ("Zodiac", "Prospector", "CrewShuttle") else None
    if not model:
        continue
    skins = row.setdefault("Skin", [])
    for name, *_ in PALETTE:
        skins[:] = [skin for skin in skins if skin["Name"] != name]
        skins.append({"Name": name, "SourceFile": "skins.json", "Cells": bindings[model][name]})
catalog = []
for row in rows:
    for skin in row.get("Skin", []):
        catalog.append({"id": row["Row"].replace(" ", "_") + "." + skin["Name"].replace(" ", "_"),
                        "name": skin["Name"].replace("_", " ").title(), "hull": row["Row"], "skinName": skin["Name"],
                        "included": skin["Name"] not in {name for name, *_ in PALETTE},
                        "category": "Event" if skin["Name"] == "Midnight" else "Special" if skin["Name"] == "Crimson" else "Normal", "description": "Included cosmetic finish for " + row.get("DisplayName", row["Row"]) + ". Original surface details are retained; performance and equipment are unchanged."})
PATH.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
(PATH.parent / "skins.json").write_text(json.dumps({"schemaVersion": 1, "prices": {"Normal": 5000, "Special": 15000, "Event": 25000}, "skins": catalog}, indent=2) + "\n", encoding="utf-8")
(ROOT / "Saved/Analysis/StarterShipSkinAssets.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("SSW starter skin collection: 8 finishes, " + str(len(report)) + " material instances, " + str(len(catalog)) + " catalog entries")
