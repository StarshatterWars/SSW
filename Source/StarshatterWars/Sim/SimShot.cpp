/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimShot.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Laser and Missile class
*/

#include "SimShot.h"

// Unreal logging:
#include "Logging/LogMacros.h"

// Minimal Unreal include for FVector math:
#include "Math/UnrealMathUtility.h"

// Starshatter / nGen includes:
#include "Weapon.h"
#include "DriveSprite.h"
#include "SeekerAI.h"
#include "Sim.h"
#include "Ship.h"
#include "Trail.h"
#include "AudioConfig.h"
#include "TerrainRegion.h"
#include "Terrain.h"
#include "Game.h"
#include "Bolt.h"
#include "Sprite.h"
#include "Solid.h"
#include "SimLight.h"
#include "DataLoader.h"
#include "Sound.h"

// NOTE: Bitmap removed (render assets are Unreal-side in SSW_UE55).

// +--------------------------------------------------------------------+

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimShot, Log, All);

// Helper: Starshatter-style random in [Min, Max], inclusive-ish.
// (Replaces Random.h / Random(...) from legacy code.)
static double SSRandRange(const double Min, const double Max)
{
	return Min + (Max - Min) * (double)FMath::FRand();
}

// +--------------------------------------------------------------------+

SimShot::SimShot(const FVector& pos, const Camera& shot_cam, WeaponDesign* dsn, const Ship* ship)
	: first_frame(true),
	owner(ship),
	flash(0),
	flare(0),
	trail(0),
	sound(0),
	eta(0),
	charge(1.0f),
	design(dsn),
	offset(1.0e5f),
	altitude_agl(-1.0e6f),
	hit_target(false)
{
	obj_type = SimObject::SIM_SHOT;
	type = design->type;
	primary = design->primary;
	beam = design->beam;
	base_damage = design->damage;
	armed = false;

	radius = 10.0f;

	if (primary || design->decoy_type || !design->guided) {
		straight = true;
		armed = true;
	}

	cam.Clone(shot_cam);

	life = design->life;
	velocity = cam.vpn() * (double)design->speed;

	MoveTo(pos);

	if (beam)
		origin = pos + (shot_cam.vpn() * -design->length);

	switch (design->graphic_type) {
	case Graphic::BOLT: {
		Bolt* s = new Bolt(design->length, design->width, design->shot_img, 1);
		s->SetDirection(cam.vpn());
		rep = s;
	}
					  break;

	case Graphic::SPRITE: {
		Sprite* s = 0;

		if (design->animation)
			s = new DriveSprite(design->animation, design->anim_length);
		else
			s = new DriveSprite(design->shot_img);

		s->Scale((double)design->scale);
		rep = s;
	}
						break;

	case Graphic::SOLID: {
		Solid* s = new Solid;
		s->UseModel(design->shot_model);
		rep = s;

		radius = rep->Radius();
	}
					   break;
	}

	if (rep)
		rep->MoveTo(pos);

	light = 0;

	if (design->light > 0) {
		light = new SimLight(design->light);
		light->SetColor(design->light_color);
	}

	mass = design->mass;
	drag = design->drag;
	thrust = 0.0f;

	dr_drg = design->roll_drag;
	dp_drg = design->pitch_drag;
	dy_drg = design->yaw_drag;

	SetAngularRates((float)design->roll_rate, (float)design->pitch_rate, (float)design->yaw_rate);

	if (design->flash_img != 0) {
		flash = new Sprite(design->flash_img);
		flash->Scale((double)design->flash_scale);
		flash->MoveTo(pos - cam.vpn() * design->length);
		flash->SetLuminous(true);
	}

	if (design->flare_img != 0) {
		flare = new DriveSprite(design->flare_img);
		flare->Scale((double)design->flare_scale);
		flare->MoveTo(pos);
	}

	if (owner) {
		iff_code = (BYTE)owner->GetIFF();
		Observe((SimObject*)owner);
	}

	sprintf_s(name, "SimShot(%s)", design->name.data());
}

// +--------------------------------------------------------------------+

SimShot::~SimShot()
{
	GRAPHIC_DESTROY(flash);
	GRAPHIC_DESTROY(flare);
	GRAPHIC_DESTROY(trail);

	if (sound) {
		sound->Stop();
		sound->Release();
	}
}

// +--------------------------------------------------------------------+

const char*
SimShot::DesignName() const
{
	return design->name;
}

// +--------------------------------------------------------------------+

void
SimShot::SetCharge(float c)
{
	charge = c;

	// trim beam life to amount of energy available:
	if (beam)
		life = design->life * charge / design->charge;
}

void
SimShot::SetFuse(double seconds)
{
	if (seconds > 0 && !beam)
		life = seconds;
}

// +--------------------------------------------------------------------+

void
SimShot::SeekTarget(SimObject* target, SimSystem* sub)
{
	if (dir && !primary) {
		SeekerAI* seeker = (SeekerAI*)dir;
		SimObject* old_target = seeker->GetTarget();

		if (old_target && old_target->Type() == SimObject::SIM_SHIP) {
			Ship* tgt_ship = (Ship*)old_target;
			tgt_ship->DropThreat(this);
		}
	}

	delete dir;
	dir = 0;

	if (target) {
		SeekerAI* seeker = new SeekerAI(this);
		seeker->SetTarget(target, sub);
		seeker->SetPursuit(design->guided);
		seeker->SetDelay(1);

		dir = seeker;

		if (!primary && target->Type() == SimObject::SIM_SHIP) {
			Ship* tgt_ship = (Ship*)target;
			tgt_ship->AddThreat(this);
		}
	}
}

bool
SimShot::IsTracking(Ship* tgt) const
{
	return tgt && (GetTarget() == tgt);
}

SimObject*
SimShot::GetTarget() const
{
	if (dir) {
		SeekerAI* seeker = (SeekerAI*)dir;

		if (seeker->GetDelay() <= 0)
			return seeker->GetTarget();
	}

	return 0;
}

bool
SimShot::IsFlak() const
{
	return design && design->flak;
}

// +--------------------------------------------------------------------+

bool
SimShot::IsHostileTo(const SimObject* o) const
{
	if (o) {
		if (o->Type() == SIM_SHIP) {
			Ship* s = (Ship*)o;

			if (s->IsRogue())
				return true;

			if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
				return true;
		}

		else if (o->Type() == SIM_SHOT || o->Type() == SIM_DRONE) {
			SimShot* s = (SimShot*)o;

			if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
				return true;
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

void
SimShot::ExecFrame(double seconds)
{
	altitude_agl = -1.0e6f;

	// add random flickering effect:
	double flicker = 0.75 + (double)FMath::Rand() / 8e4;
	if (flicker > 1) flicker = 1;

	if (flare) {
		flare->SetShade(flicker);
	}
	else if (beam) {
		Bolt* blob = (Bolt*)rep;
		blob->SetShade(flicker);
		offset -= (float)(seconds * 10);
	}

	if (Game::Paused())
		return;

	if (beam) {
		if (!first_frame) {
			if (life > 0) {
				life -= seconds;

				if (life < 0)
					life = 0;
			}
		}
	}
	else {
		origin = Location();

		if (!first_frame)
			Physical::ExecFrame(seconds);
		else
			Physical::ExecFrame(0);

		double len = design->length;
		if (len < 50) len = 50;

		if (!trail && life > 0 && design->life - life > 0.2) {
			if (design->trail.length()) {
				trail = new Trail(design->trail_img, design->trail_length);

				if (design->trail_width > 0)
					trail->SetWidth(design->trail_width);

				if (design->trail_dim > 0)
					trail->SetDim(design->trail_dim);

				trail->AddPoint(Location() + Heading() * -100);

				SimScene* scene = 0;

				if (rep)
					scene = rep->GetScene();

				if (scene)
					scene->AddGraphic(trail);
			}
		}

		if (trail)
			trail->AddPoint(Location());

		if (!armed) {
			SeekerAI* seeker = (SeekerAI*)dir;

			if (seeker && seeker->GetDelay() <= 0)
				armed = true;
		}

		// handle submunitions:
		else if (design->det_range > 0 && design->det_count > 0) {
			if (dir && !primary) {
				SeekerAI* seeker = (SeekerAI*)dir;
				SimObject* target = seeker->GetTarget();

				if (target) {
					double range = (FVector(Location() - target->Location())).Size();

					if (range < design->det_range) {
						life = 0;

						Sim* sim = Sim::GetSim();
						WeaponDesign* child_design = WeaponDesign::Find(design->det_child);

						if (sim && child_design) {
							double spread = design->det_spread;

							Camera aim_cam;
							aim_cam.Clone(Cam());
							aim_cam.LookAt(target->Location());

							for (int i = 0; i < design->det_count; i++) {
								SimShot* child = (SimShot*)sim->CreateShot(Location(), aim_cam, child_design,
									owner, owner->GetRegion());

								child->SetCharge(child_design->charge);

								if (child_design->guided)
									child->SeekTarget(target, seeker->GetSubTarget());

								if (child_design->beam)
									child->SetBeamPoints(Location(), target->Location());

								if (i) aim_cam.LookAt(target->Location());
								aim_cam.Pitch(SSRandRange(-spread, spread));
								aim_cam.Yaw(SSRandRange(-spread, spread));
							}
						}
					}
				}
			}
		}

		if (flash && !first_frame)
			GRAPHIC_DESTROY(flash);

		if (thrust < design->thrust)
			thrust += (float)(seconds * 5.0e3);
		else
			thrust = design->thrust;
	}

	first_frame = 0;

	if (flare)
		flare->MoveTo(Location());
}

// +--------------------------------------------------------------------+

void
SimShot::Disarm()
{
	if (armed && !primary) {
		armed = false;
		delete dir;
		dir = 0;
	}
}

void
SimShot::Destroy()
{
	life = 0;
}

// +--------------------------------------------------------------------+

void
SimShot::SetBeamPoints(const FVector& from, const FVector& to)
{
	if (beam) {
		MoveTo(to);
		origin = from;

		if (sound) {
			sound->SetLocation(from);
		}

		if (rep) {
			Bolt* s = (Bolt*)rep;
			s->SetEndPoints(from, to);

			double len = (FVector(to - from)).Size() / 500;
			s->SetTextureOffset(offset, offset + len);
		}
	}

	if (flash) {
		flash->MoveTo(origin);
	}
}

// +--------------------------------------------------------------------+

double
SimShot::AltitudeMSL() const
{
	// Starshatter convention used Y as vertical:
	return Location().Y;
}

double
SimShot::AltitudeAGL() const
{
	if (altitude_agl < -1000) {
		SimShot* pThis = (SimShot*)this; // cast-away const
		FVector  loc = Location();

		Terrain* terrain = region ? region->GetTerrain() : 0;

		if (terrain)
			pThis->altitude_agl = (float)(loc.Y - terrain->Height(loc.X, loc.Z));
		else
			pThis->altitude_agl = (float)loc.Y;

		if (!FMath::IsFinite(pThis->altitude_agl)) {
			pThis->altitude_agl = 0.0f;
		}
	}

	return altitude_agl;
}

// +--------------------------------------------------------------------+

void
SimShot::Initialize()
{
}

void
SimShot::Close()
{
}

// +--------------------------------------------------------------------+

double
SimShot::Damage() const
{
	double damage = 0;

	// beam damage based on length:
	if (beam) {
		double fade = 1;

		if (design) {
			// linear fade with distance:
			double len = (FVector(origin - Location())).Size();

			if (len > design->min_range)
				fade = (design->length - len) / (design->length - design->min_range);
		}

		damage = base_damage * charge * fade * Game::FrameTime();
	}

	// energy wep damage based on time:
	else if (primary) {
		damage = base_damage * charge * life;
	}

	// missile damage is constant:
	else {
		damage = base_damage * charge;
	}

	return damage;
}

double
SimShot::Length() const
{
	if (design)
		return design->length;

	return 500;
}

// +--------------------------------------------------------------------+

void
SimShot::Activate(SimScene& scene)
{
	SimObject::Activate(scene);

	if (trail)
		scene.AddGraphic(trail);

	if (flash)
		scene.AddGraphic(flash);

	if (flare)
		scene.AddGraphic(flare);

	if (first_frame) {
		if (design->sound_resource) {
			sound = design->sound_resource->Duplicate();

			if (sound) {
				long max_vol = AudioConfig::EfxVolume();
				long volume = -1000;

				if (volume > max_vol)
					volume = max_vol;

				if (beam) {
					sound->SetLocation(origin);
					sound->SetVolume(volume);
					sound->Play();
				}
				else {
					sound->SetLocation(Location());
					sound->SetVolume(volume);
					sound->Play();
					sound = 0; // fire and forget
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
SimShot::Deactivate(SimScene& scene)
{
	SimObject::Deactivate(scene);

	if (trail)
		scene.DelGraphic(trail);

	if (flash)
		scene.DelGraphic(flash);

	if (flare)
		scene.DelGraphic(flare);
}

// +--------------------------------------------------------------------+

int
SimShot::GetIFF() const
{
	return iff_code;
}

// +--------------------------------------------------------------------+

FColor
SimShot::MarkerColor() const
{
	return Ship::IFFColor(GetIFF());
}

// +--------------------------------------------------------------------+

const char*
SimShot::GetObserverName() const
{
	return name;
}

// +--------------------------------------------------------------------+

bool
SimShot::Update(SimObject* obj)
{
	if (obj == (SimObject*)owner)
		owner = 0;

	return SimObserver::Update(obj);
}
