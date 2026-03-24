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

#include "ShipDesign.h"
#include "Weapon.h"

#include "Game.h"
#include "Sprite.h"
#include "SimLight.h"
// Bitmap removed: render assets are Unreal UTexture2D*
#include "Solid.h"
#include "SimModel.h"
#include "Sound.h"
#include "DataLoader.h"

#include "ParseUtil.h"

// Static storage for WeaponDesign catalogs/state:
List<WeaponDesign> WeaponDesign::catalog;
List<WeaponDesign> WeaponDesign::mod_catalog;
bool WeaponDesign::degrees = false;
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
	delete sound_resource;
}

// +--------------------------------------------------------------------+

void
WeaponDesign::Initialize(const char* filename)
{
	UE_LOG(LogTemp, Log, TEXT("Loading Weapon Designs '%s'"), ANSI_TO_TCHAR(filename));
	LoadDesign("Weapons/", filename);
}

// +--------------------------------------------------------------------+

void
WeaponDesign::Close()
{
	catalog.destroy();
	mod_catalog.destroy();
}

// +--------------------------------------------------------------------+

void
WeaponDesign::LoadDesign(const char* path, const char* filename, bool mod)
{
	// Load Design File:
	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath(path);

	BYTE* block = nullptr;
	int blocklen = loader->LoadBuffer(filename, block, true);

	Parser parser(new BlockReader((const char*)block, blocklen));
	Term* term = parser.ParseTerm();

	if (!term) {
		UE_LOG(LogTemp, Error, TEXT("ERROR: could not parse '%s'"), ANSI_TO_TCHAR(filename));
		return;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "WEAPON") {
			UE_LOG(LogTemp, Error, TEXT("ERROR: invalid weapon design file '%s'"), ANSI_TO_TCHAR(filename));
			return;
		}
	}

	int type = 1;
	degrees = false;

	do {
		delete term;
		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				Text defname = def->name()->value();
				defname.setSensitive(false);

				if (defname == "primary" ||
					defname == "missile" ||
					defname == "drone" ||
					defname == "beam") {

					if (!def->term() || !def->term()->isStruct()) {
						UE_LOG(LogTemp, Warning, TEXT("WARNING: weapon structure missing in '%s'"), ANSI_TO_TCHAR(filename));
					}
					else {
						TermStruct* val = def->term()->isStruct();
						WeaponDesign* design = new WeaponDesign;

						design->type = type++;

						if (defname == "primary") {
							design->primary = true;
						}
						else if (defname == "beam") {
							design->primary = true;
							design->beam = true;
							design->guided = true;

							design->spread_az = 0.15f;
							design->spread_el = 0.15f;
						}
						else if (defname == "drone") {
							design->drone = true;
							design->penetration = 5.0f;
						}
						else { // missile
							design->penetration = 5.0f;
						}

						float sound_min_dist = 1.0f;
						float sound_max_dist = 100.0e3f;

						for (int i = 0; i < val->elements()->size(); i++) {
							TermDef* pdef = val->elements()->at(i)->isDef();
							if (pdef) {
								defname = pdef->name()->value();
								defname.setSensitive(false);

								// -----------------------------------------------------------------
								// Former GET_DEF_* macro chain: expanded and formatted
								// -----------------------------------------------------------------

								if (defname == "name")                 GetDefText(design->name, pdef, filename);
								else if (defname == "group")                GetDefText(design->group, pdef, filename);
								else if (defname == "description")          GetDefText(design->description, pdef, filename);

								else if (defname == "guided")               GetDefNumber(design->guided, pdef, filename);
								else if (defname == "self_aiming")          GetDefBool(design->self_aiming, pdef, filename);
								else if (defname == "flak")                 GetDefBool(design->flak, pdef, filename);
								else if (defname == "syncro")               GetDefBool(design->syncro, pdef, filename);
								else if (defname == "visible_stores")       GetDefBool(design->visible_stores, pdef, filename);

								else if (defname == "value")                GetDefNumber(design->value, pdef, filename);

								else if (defname == "capacity")             GetDefNumber(design->capacity, pdef, filename);
								else if (defname == "recharge_rate")        GetDefNumber(design->recharge_rate, pdef, filename);
								else if (defname == "refire_delay")         GetDefNumber(design->refire_delay, pdef, filename);
								else if (defname == "salvo_delay")          GetDefNumber(design->salvo_delay, pdef, filename);
								else if (defname == "ammo")                 GetDefNumber(design->ammo, pdef, filename);
								else if (defname == "ripple_count")         GetDefNumber(design->ripple_count, pdef, filename);
								else if (defname == "charge")               GetDefNumber(design->charge, pdef, filename);
								else if (defname == "min_charge")           GetDefNumber(design->min_charge, pdef, filename);
								else if (defname == "carry_mass")           GetDefNumber(design->carry_mass, pdef, filename);
								else if (defname == "carry_resist")         GetDefNumber(design->carry_resist, pdef, filename);
								else if (defname == "damage")               GetDefNumber(design->damage, pdef, filename);
								else if (defname == "penetration")          GetDefNumber(design->penetration, pdef, filename);
								else if (defname == "speed")                GetDefNumber(design->speed, pdef, filename);
								else if (defname == "life")                 GetDefNumber(design->life, pdef, filename);
								else if (defname == "mass")                 GetDefNumber(design->mass, pdef, filename);
								else if (defname == "drag")                 GetDefNumber(design->drag, pdef, filename);
								else if (defname == "thrust")               GetDefNumber(design->thrust, pdef, filename);
								else if (defname == "roll_rate")            GetDefNumber(design->roll_rate, pdef, filename);
								else if (defname == "pitch_rate")           GetDefNumber(design->pitch_rate, pdef, filename);
								else if (defname == "yaw_rate")             GetDefNumber(design->yaw_rate, pdef, filename);
								else if (defname == "roll_drag")            GetDefNumber(design->roll_drag, pdef, filename);
								else if (defname == "pitch_drag")           GetDefNumber(design->pitch_drag, pdef, filename);
								else if (defname == "yaw_drag")             GetDefNumber(design->yaw_drag, pdef, filename);
								else if (defname == "lethal_radius")        GetDefNumber(design->lethal_radius, pdef, filename);
								else if (defname == "integrity")            GetDefNumber(design->integrity, pdef, filename);

								else if (defname == "det_range")            GetDefNumber(design->det_range, pdef, filename);
								else if (defname == "det_count")            GetDefNumber(design->det_count, pdef, filename);
								else if (defname == "det_spread")           GetDefNumber(design->det_spread, pdef, filename);
								else if (defname == "det_child")            GetDefText(design->det_child, pdef, filename);

								else if (defname == "slew_rate")            GetDefNumber(design->slew_rate, pdef, filename);

								else if (defname == "min_range")            GetDefNumber(design->min_range, pdef, filename);
								else if (defname == "max_range")            GetDefNumber(design->max_range, pdef, filename);
								else if (defname == "max_track")            GetDefNumber(design->max_track, pdef, filename);

								else if (defname == "graphic_type")         GetDefNumber(design->graphic_type, pdef, filename);
								else if (defname == "width")                GetDefNumber(design->width, pdef, filename);
								else if (defname == "length")               GetDefNumber(design->length, pdef, filename);
								else if (defname == "scale")                GetDefNumber(design->scale, pdef, filename);
								else if (defname == "explosion_scale")      GetDefNumber(design->explosion_scale, pdef, filename);
								else if (defname == "light")                GetDefNumber(design->light, pdef, filename);
								else if (defname == "flash_scale")          GetDefNumber(design->flash_scale, pdef, filename);
								else if (defname == "flare_scale")          GetDefNumber(design->flare_scale, pdef, filename);

								else if (defname == "trail_length")         GetDefNumber(design->trail_length, pdef, filename);
								else if (defname == "trail_width")          GetDefNumber(design->trail_width, pdef, filename);
								else if (defname == "trail_dim")            GetDefNumber(design->trail_dim, pdef, filename);

								else if (defname == "beauty")               GetDefText(design->beauty, pdef, filename);
								else if (defname == "bitmap")               GetDefText(design->bitmap, pdef, filename);
								else if (defname == "turret")               GetDefText(design->turret, pdef, filename);
								else if (defname == "turret_base")          GetDefText(design->turret_base, pdef, filename);
								else if (defname == "model")                GetDefText(design->model, pdef, filename);
								else if (defname == "trail")                GetDefText(design->trail, pdef, filename);
								else if (defname == "flash")                GetDefText(design->flash, pdef, filename);
								else if (defname == "flare")                GetDefText(design->flare, pdef, filename);
								else if (defname == "sound")                GetDefText(design->sound, pdef, filename);

								else if (defname == "probe")                GetDefBool(design->probe, pdef, filename);

								else if (defname == "turret_axis")          GetDefNumber(design->turret_axis, pdef, filename);
								else if (defname == "target_type")          GetDefNumber(design->target_type, pdef, filename);

								// -----------------------------------------------------------------
								// Remaining special cases
								// -----------------------------------------------------------------

								else if (defname == "degrees") {
									GetDefBool(degrees, pdef, filename);
								}

								else if (defname == "secret") {
									GetDefBool(design->secret, pdef, filename);
								}

								else if (defname == "aim_az_max") {
									GetDefNumber(design->aim_az_max, pdef, filename);
									if (degrees) design->aim_az_max *= (float)DEGREES;
									design->aim_az_min = 0.0f - design->aim_az_max;
								}

								else if (defname == "aim_el_max") {
									GetDefNumber(design->aim_el_max, pdef, filename);
									if (degrees) design->aim_el_max *= (float)DEGREES;
									design->aim_el_min = 0.0f - design->aim_el_max;
								}

								else if (defname == "aim_az_min") {
									GetDefNumber(design->aim_az_min, pdef, filename);
									if (degrees) design->aim_az_min *= (float)DEGREES;
								}

								else if (defname == "aim_el_min") {
									GetDefNumber(design->aim_el_min, pdef, filename);
									if (degrees) design->aim_el_min *= (float)DEGREES;
								}

								else if (defname == "aim_az_rest") {
									GetDefNumber(design->aim_az_rest, pdef, filename);
									if (degrees) design->aim_az_rest *= (float)DEGREES;
								}

								else if (defname == "aim_el_rest") {
									GetDefNumber(design->aim_el_rest, pdef, filename);
									if (degrees) design->aim_el_rest *= (float)DEGREES;
								}

								else if (defname == "spread_az") {
									GetDefNumber(design->spread_az, pdef, filename);
									if (degrees) design->spread_az *= (float)DEGREES;
								}

								else if (defname == "spread_el") {
									GetDefNumber(design->spread_el, pdef, filename);
									if (degrees) design->spread_el *= (float)DEGREES;
								}

								else if (defname == "animation") {
									if (design->anim_length < 16) {
										GetDefText(design->anim_frames[design->anim_length++], pdef, filename);
									}
									else {
										UE_LOG(LogTemp, Warning,
											TEXT("WARNING: too many animation frames for weapon '%s' in '%s'"),
											ANSI_TO_TCHAR(design->name.data()),
											ANSI_TO_TCHAR(filename));
									}
								}

								else if (defname == "light_color") {
									GetDefFColor(design->light_color, pdef, filename);
								}

								else if (defname == "sound_min_dist") {
									GetDefNumber(sound_min_dist, pdef, filename);
								}

								else if (defname == "sound_max_dist") {
									GetDefNumber(sound_max_dist, pdef, filename);
								}

								else if (defname == "muzzle") {
									if (design->nbarrels < MAX_STORES) {
										FVector a;
										GetDefVec(a, pdef, filename);
										design->muzzle_pts[design->nbarrels++] = a;
									}
									else {
										UE_LOG(LogTemp, Warning,
											TEXT("WARNING: too many muzzles for weapon '%s' in '%s'"),
											ANSI_TO_TCHAR(design->name.data()),
											ANSI_TO_TCHAR(filename));
									}
								}

								else if (defname == "attachment") {
									if (design->nstores < MAX_STORES) {
										FVector a;
										GetDefVec(a, pdef, filename);
										design->attachments[design->nstores++] = a;
									}
									else {
										UE_LOG(LogTemp, Warning,
											TEXT("WARNING: too many attachments for weapon '%s' in '%s'"),
											ANSI_TO_TCHAR(design->name.data()),
											ANSI_TO_TCHAR(filename));
									}
								}

								else if (defname == "eject") {
									GetDefVec(design->eject, pdef, filename);
								}

								else if (defname == "decoy") {
									char typestr[32];
									GetDefText(typestr, pdef, filename);
									design->decoy_type = ShipDesign::ClassForName(typestr);
								}

								else if (defname == "damage_type") {
									char typestr[32];
									GetDefText(typestr, pdef, filename);

									if (!_stricmp(typestr, "normal"))
										design->damage_type = DMG_NORMAL;
									else if (!_stricmp(typestr, "emp"))
										design->damage_type = DMG_EMP;
									else if (!_stricmp(typestr, "power"))
										design->damage_type = DMG_POWER;
									else
										UE_LOG(LogTemp, Warning,
											TEXT("WARNING: unknown weapon damage type '%s' in '%s'"),
											ANSI_TO_TCHAR(typestr),
											ANSI_TO_TCHAR(filename));
								}

								else {
									UE_LOG(LogTemp, Warning,
										TEXT("WARNING: parameter '%s' ignored in '%s'"),
										ANSI_TO_TCHAR(defname.data()),
										ANSI_TO_TCHAR(filename));
								}
							}
							else {
								UE_LOG(LogTemp, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
								val->elements()->at(i)->print();
							}
						}

						if (design->description.length()) {
							design->description = Game::GetText(design->description);
						}

						// Texture loading:
						if (design->anim_length > 0) {
							loader->LoadTexture(design->anim_frames[0], design->animation, 0);

							if (design->anim_length > 1) {
								UE_LOG(LogTemp, Warning,
									TEXT("WARNING: weapon '%s' defines %d animation frames; only the first is loaded in Unreal."),
									ANSI_TO_TCHAR(design->name.data()),
									design->anim_length);
							}
						}
						else if (design->bitmap.length()) {
							loader->LoadTexture(design->bitmap, design->shot_img, 0);
						}

						if (design->beauty.length()) {
							loader->LoadTexture(design->beauty, design->beauty_img, 0);
						}

						if (design->turret.length()) {
							Text        p;
							Text        t = design->turret;
							const char* s = strrchr(t.data(), '/');

							if (s) {
								p = Text(path) + t.substring(0, s - t.data() + 1);
								t = t.substring(s - t.data() + 1, t.length());
							}
							else {
								s = strrchr(t.data(), '\\');
								if (s) {
									p = Text(path) + t.substring(0, s - t.data() + 1);
									t = t.substring(s - t.data() + 1, t.length());
								}
							}

							if (p.length() && p.data())
								loader->SetDataPath(p.data());

							design->turret_model = new SimModel;
							design->turret_model->Load(t, design->scale);

							if (design->turret_base.length()) {
								t = design->turret_base;
								s = strrchr(t.data(), '/');

								if (s) {
									p = Text(path) + t.substring(0, s - t.data() + 1);
									t = t.substring(s - t.data() + 1, t.length());
								}
								else {
									s = strrchr(t.data(), '\\');
									if (s) {
										p = Text(path) + t.substring(0, s - t.data() + 1);
										t = t.substring(s - t.data() + 1, t.length());
									}
								}

								if (p.length() && p.data())
									loader->SetDataPath(p.data());

								design->turret_base_model = new SimModel;
								design->turret_base_model->Load(t, design->scale);
							}

							loader->SetDataPath(path);
						}

						if (design->model.length()) {
							Text        p;
							Text        t = design->model;
							const char* s = strrchr(t.data(), '/');

							if (s) {
								p = Text(path) + t.substring(0, s - t.data() + 1);
								t = t.substring(s - t.data() + 1, t.length());
							}
							else {
								s = strrchr(t.data(), '\\');
								if (s) {
									p = Text(path) + t.substring(0, s - t.data() + 1);
									t = t.substring(s - t.data() + 1, t.length());
								}
							}

							if (p.length() && p.data())
								loader->SetDataPath(p.data());

							design->shot_model = new SimModel;
							design->shot_model->Load(t, design->scale);

							// Return to the original weapon data path after loading the model:
							loader->SetDataPath(path);
						}

						if (design->trail.length()) {
							loader->LoadTexture(design->trail, design->trail_img, 0);
						}

						if (design->flash.length()) {
							loader->LoadTexture(design->flash, design->flash_img, 0);
						}

						if (design->flare.length()) {
							loader->LoadTexture(design->flare, design->flare_img, 0);
						}

						if (design->sound.length()) {
							int SOUND_FLAGS = USound::LOCALIZED | USound::LOC_3D;

							if (design->beam)
								SOUND_FLAGS = USound::LOCALIZED | USound::LOC_3D | USound::LOCKED;

							if (strstr(path, "Mods") == 0)
								loader->SetDataPath("Sounds/");

							loader->LoadSound(design->sound, design->sound_resource, SOUND_FLAGS);
							loader->SetDataPath(path);

							if (design->sound_resource) {
								design->sound_resource->SetMinDistance(sound_min_dist);
								design->sound_resource->SetMaxDistance(sound_max_dist);
							}
						}

						if (design->max_range == 0.0f)
							design->max_range = design->speed * design->life;

						if (design->max_track == 0.0f)
							design->max_track = 3.0f * design->max_range;

						if (design->probe && design->lethal_radius < 1e3)
							design->lethal_radius = 50e3f;

						if (design->beam)
							design->flak = false;

						if (design->self_aiming) {
							if (fabs(design->aim_az_max) > design->firing_cone)
								design->firing_cone = (float)fabs(design->aim_az_max);

							if (fabs(design->aim_az_min) > design->firing_cone)
								design->firing_cone = (float)fabs(design->aim_az_min);

							if (fabs(design->aim_el_max) > design->firing_cone)
								design->firing_cone = (float)fabs(design->aim_el_max);

							if (fabs(design->aim_el_min) > design->firing_cone)
								design->firing_cone = (float)fabs(design->aim_el_min);
						}

						if (mod)
							mod_catalog.append(design);
						else
							catalog.append(design);
					}
				}
				else {
					UE_LOG(LogTemp, Warning,
						TEXT("WARNING: unknown definition '%s' in '%s'"),
						ANSI_TO_TCHAR(def->name()->value().data()),
						ANSI_TO_TCHAR(filename));
				}
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("WARNING: term ignored in '%s'"), ANSI_TO_TCHAR(filename));
				term->print();
			}
		}
	} while (term);

	loader->ReleaseBuffer(block);
	loader->SetDataPath("");
}

// +--------------------------------------------------------------------+

WeaponDesign*
WeaponDesign::Get(int type)
{
	WeaponDesign  test;
	test.type = type;

	WeaponDesign* result = catalog.find(&test);

	if (!result)
		result = mod_catalog.find(&test);

	return result;
}

// +--------------------------------------------------------------------+

WeaponDesign*
WeaponDesign::Find(const char* name)
{
	for (int i = 0; i < catalog.size(); i++) {
		WeaponDesign* d = catalog.at(i);

		if (d->name == name) {
			return d;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("WeaponDesign: no catalog entry for design '%s', checking mods..."),
		ANSI_TO_TCHAR(name));
	return WeaponDesign::Find(name);
}

// +--------------------------------------------------------------------+

int
WeaponDesign::GetDesignList(List<Text>& designs)
{
	designs.clear();

	for (int i = 0; i < catalog.size(); i++) {
		WeaponDesign* design = catalog[i];
		designs.append(&design->name);
	}

	return designs.size();
}
