/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         WeaponDesign.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Weapon Design parameters class
*/

#include "WeaponDesign.h"
#include "CoreMinimal.h" // UE_LOG, ANSI_TO_TCHAR


// Bitmap removed: render assets are Unreal UTexture2D*
#include "Sound.h"
#include "SimModel.h"
#include "DataLoader.h"
#include "WeaponDesignRegistry.h"


// Static storage for WeaponDesign catalogs/state:

// +--------------------------------------------------------------------+

WeaponDesign::WeaponDesign()
{
	type = 0;
	secret = 0;
	drone = 0;
	primary = 0;
	beam = 0;
	flak = 0;
	guided = 0;
	self_aiming = 0;
	syncro = 0;
	value = 0;
	decoy_type = 0;
	probe = 0;
	target_type = 0;

	visible_stores = 0;
	nstores = 0;
	nbarrels = 0;

	recharge_rate = 0.0f;
	refire_delay = 0.0f;
	salvo_delay = 0.0f;
	charge = 0.0f;
	min_charge = 0.0f;
	carry_mass = 0.0f;
	carry_resist = 0.0f;

	speed = 0.0f;
	life = 0.0f;
	mass = 0.0f;
	drag = 0.0f;
	thrust = 0.0f;
	roll_rate = 0.0f;
	pitch_rate = 0.0f;
	yaw_rate = 0.0f;
	roll_drag = 0.0f;
	pitch_drag = 0.0f;
	yaw_drag = 0.0f;

	min_range = 0.0f;
	max_range = 0.0f;
	max_track = 0.0f;

	projectile = "";
	graphic_type = 0;
	width = 0;
	length = 0;

	scale = 1.0f;
	explosion_scale = 0.0f;
	light = 0.0f;
	light_color = FColor::White;
	flash_scale = 0.0f;
	flare_scale = 0.0f;

	spread_az = 0.0f;
	spread_el = 0.0f;

	beauty_img = 0;
	turret_model = 0;
	turret_base_model = 0;
	animation = 0;
	anim_length = 0;
	shot_img = 0;
	shot_model = 0;
	trail_img = 0;
	flash_img = 0;
	flare_img = 0;
	sound_resource = 0;
    sound_min_distance = 1.0f;
    sound_max_distance = 100000.0f;

	ammo = -1;
	ripple_count = 0;
	capacity = 100.0f;
	damage = 1.0f;
	damage_type = 0;
	penetration = 1.0f;
	firing_cone = 0.0f;
	aim_az_max = 1.5f;
	aim_az_min = -1.5f;
	aim_az_rest = 0.0f;
	aim_el_max = 1.5f;
	aim_el_min = -1.5f;
	aim_el_rest = 0.0f;
	slew_rate = (float)(60 * DEGREES);
	turret_axis = 0;
	lethal_radius = 500.0f;
	integrity = 100.0f;

	eject = FVector(0.0f, -100.0f, 0.0f);

	det_range = 0.0f;
	det_count = 0;
	det_spread = (float)(PI / 8);

	trail_length = 0;
	trail_width = 0;
	trail_dim = 0;

	for (int i = 0; i < MAX_STORES; i++) {
		muzzle_pts[i] = FVector(0.0f, 0.0f, 0.0f);
		attachments[i] = FVector(0.0f, 0.0f, 0.0f);
	}
}

WeaponDesign::~WeaponDesign()
{
	delete turret_model;
	delete turret_base_model;
	delete shot_model;
	if (sound_resource) sound_resource->Release();
}

// +--------------------------------------------------------------------+

void
WeaponDesign::Close() {}

WeaponDesign* WeaponDesign::GetWeaponDesign(int Type)
{
    for(const auto& Pair:WeaponDesignRegistry::GetAll())
        if(auto* Design=WeaponDesignRegistry::FindLegacy(Pair.Key);Design && Design->type==Type) return Design;
    return nullptr;
}
WeaponDesign* WeaponDesign::Find(const char* Name)
{
    return Name && Name[0]?WeaponDesignRegistry::FindLegacy(Name):nullptr;
}
int WeaponDesign::GetDesignList(List<Text>& Designs)
{
    Designs.clear();
    for(const auto& Pair:WeaponDesignRegistry::GetAll())
        if(auto* Design=WeaponDesignRegistry::FindLegacy(Pair.Key)) Designs.append(&Design->name);
    return Designs.size();
}

// Lazy Unreal audio binding preserves the bare legacy ID and configured attenuation.
bool WeaponDesign::ResolveSoundResource()
{
 if(sound_resource)return true;
 if(!sound.length())return false;
 auto* Loader=DataLoader::GetLoader();if(!Loader)return false;
 const Text Previous=Loader->GetDataPath();Loader->SetDataPath("Sounds/");
 Loader->LoadSound(sound.data(),sound_resource,USound::LOCALIZED|USound::LOC_3D,true);
 Loader->SetDataPath(Previous.data());
 if(!sound_resource)return false;
 sound_resource->SetMinDistance(sound_min_distance);sound_resource->SetMaxDistance(sound_max_distance);
 return true;
}
