"""Refresh cached Falcon systems/gear after the legacy scale correction."""
import os
import shutil
import unreal

backup_dir = os.path.join(unreal.Paths.project_saved_dir(), "FalconLandingGearMigration")
os.makedirs(backup_dir, exist_ok=True)
asset_file = os.path.join(unreal.Paths.project_content_dir(), "Game", "DT_ShipDesign.uasset")
backup_file = os.path.join(backup_dir, "DT_ShipDesign.uasset.before")
if not os.path.exists(backup_file):
    shutil.copy2(asset_file, backup_file)
game_instance = unreal.new_object(unreal.GameInstance)
importer = unreal.new_object(unreal.StarshatterShipDesignSubsystem, outer=game_instance)
if not importer.reimport_falcon_flight_systems():
    raise RuntimeError("Falcon definition refresh failed")
table = unreal.load_asset("/Game/Game/DT_ShipDesign")
if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
    raise RuntimeError("Falcon data table save failed")
unreal.log("FALCON_GEAR_REFRESH_COMPLETE")
