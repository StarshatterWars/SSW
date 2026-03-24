/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimContact.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Integrated (Passive and Active) Sensor Package class implementation
*/

#include "SimContact.h"

#include "Drone.h"
#include "Sensor.h"
#include "Ship.h"
#include "Sim.h"
#include "WeaponDesign.h"
#include "Game.h"

#include "Logging/LogMacros.h"
#include "Math/Vector.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterWarsSimContact, Log, All);

// +----------------------------------------------------------------------+

SimContact::SimContact()
	: ship(nullptr),
	shot(nullptr),
	loc(FVector::ZeroVector),
	acquire_time(0),
	time(0),
	track(nullptr),
	ntrack(0),
	track_time(0),
	d_pas(0.0f),
	d_act(0.0f),
	probe(false)
{
	acquire_time = Game::GameTime();
}

SimContact::SimContact(Ship* s, float p, float a)
	: ship(s),
	shot(nullptr),
	loc(FVector::ZeroVector),
	acquire_time(0),
	time(0),
	track(nullptr),
	ntrack(0),
	track_time(0),
	d_pas(p),
	d_act(a),
	probe(false)
{
	acquire_time = Game::GameTime();
	Observe(ship);
}

SimContact::SimContact(SimShot* s, float p, float a)
	: ship(nullptr),
	shot(s),
	loc(FVector::ZeroVector),
	acquire_time(0),
	time(0),
	track(nullptr),
	ntrack(0),
	track_time(0),
	d_pas(p),
	d_act(a),
	probe(false)
{
	acquire_time = Game::GameTime();
	Observe(shot);
}

// +----------------------------------------------------------------------+

SimContact::~SimContact()
{
	delete[] track;
	track = nullptr;
}

// +----------------------------------------------------------------------+

int
SimContact::operator == (const SimContact& c) const
{
	if (ship)
		return ship == c.ship;

	else if (shot)
		return shot == c.shot;

	return 0;
}

// +----------------------------------------------------------------------+

bool
SimContact::Update(SimObject* obj)
{
	if (obj == ship || obj == shot) {
		ship = nullptr;
		shot = nullptr;
		d_act = 0;
		d_pas = 0;

		ClearTrack();
	}

	return SimObserver::Update(obj);
}

const char*
SimContact::GetObserverName() const
{
	static char name[128];

	if (ship) {
		sprintf_s(name, "SimContact Ship='%s'", ship->Name());
	}
	else if (shot) {
		sprintf_s(name, "SimContact Shot='%s' %s", shot->Name(), shot->DesignName());
	}
	else {
		sprintf_s(name, "SimContact (unknown)");
	}

	return name;
}

// +----------------------------------------------------------------------+

double
SimContact::Age() const
{
	double age = 0;

	if (!ship && !shot)
		return age;

	const double seconds = (Game::GameTime() - time) / 1000.0;
	age = 1.0 - seconds / Game::DefaultTrackAge;

	if (age < 0)
		age = 0;

	return age;
}

int
SimContact::GetIFF(const Ship* observer) const
{
	int i = 0;

	if (ship) {
		i = ship->GetIFF();

		// if the contact is on our side or has locked us up,
		// we know whose side he's on.
		// Otherwise:
		if (i != observer->GetIFF() && !Threat(observer)) {
			if (d_pas < 2 * Game::SensorThreshold && d_act < Game::SensorThreshold && !Visible(observer))
				i = -1000;   // indeterminate iff reading
		}
	}

	else if (shot && shot->Owner()) {
		i = shot->Owner()->GetIFF();
	}

	return i;
}

bool
SimContact::ActLock() const
{
	return d_act >= Game::SensorThreshold;
}

bool
SimContact::PasLock() const
{
	return d_pas >= Game::SensorThreshold;
}

// +----------------------------------------------------------------------+

void
SimContact::GetBearing(const Ship* observer, double& az, double& el, double& rng) const
{
	// translate:
	const FVector TargPt = loc - observer->Location();

	// rotate:
	const Camera* cam = &observer->Cam();

	const double tx = FVector::DotProduct(TargPt, cam->vrt());
	const double ty = FVector::DotProduct(TargPt, cam->vup());
	const double tz = FVector::DotProduct(TargPt, cam->vpn());
	// convert to spherical coords:
	rng = TargPt.Length();
	az = asin(fabs(tx) / rng);
	el = asin(fabs(ty) / rng);

	if (tx < 0) az = -az;
	if (ty < 0) el = -el;

	// correct az/el for back hemisphere:
	if (tz < 0) {
		if (az < 0) az = -PI - az;
		else        az = PI - az;
	}
}

double
SimContact::Range(const Ship* observer, double limit) const
{
	double r = FVector(loc - observer->Location()).Length();

	// if passive only, return approximate range:
	if (!ActLock()) {
		const int chunk = 25000;

		if (!PasLock()) {
			r = (int)limit;
		}
		else if (r <= chunk) {
			r = chunk;
		}
		else {
			const int r1 = (int)(r + chunk / 2) / chunk;
			r = r1 * chunk;
		}
	}

	return r;
}

// +----------------------------------------------------------------------+

bool SimContact::InFront(const Ship* observer) const
{
	// translate:
	const FVector targ_pt = loc - observer->Location();

	// rotate:
	const Camera* cam = &observer->Cam();
	const double tz = FVector::DotProduct(targ_pt, cam->vpn());

	return tz > 1.0;
}

bool
SimContact::Threat(const Ship* observer) const
{
	bool threat = false;

	if (observer && observer->Life() != 0) {
		if (ship && ship->Life() != 0) {
			threat = (ship->GetIFF() &&
				ship->GetIFF() != observer->GetIFF() &&
				ship->GetEMCON() > 2 &&
				ship->IsTracking((Ship*)observer) &&
				ship->GetWeapons().size() > 0);

			if (threat && observer->GetIFF() == 0)
				threat = ship->GetIFF() > 1;
		}

		else if (shot) {
			threat = shot->IsTracking((Ship*)observer);

			if (!threat && shot->Design()->probe && shot->GetIFF() != observer->GetIFF()) {
				const FVector probe_pt = shot->Location() - observer->Location();
				const double  prng = probe_pt.Length();

				threat = (prng < shot->Design()->lethal_radius);
			}
		}
	}

	return threat;
}

bool
SimContact::Visible(const Ship* observer) const
{
	// translate:
	const FVector targ_pt = loc - observer->Location();
	double radius = 0;

	if (ship)
		radius = ship->Radius();
	else if (shot)
		radius = shot->Radius();

	// rotate:
	const double rng = targ_pt.Length();

	return (rng > 0.0) ? (radius / rng > 0.002) : false;
}

// +----------------------------------------------------------------------+

void
SimContact::Reset()
{
	if (Game::Paused()) return;

	const float step_down = (float)(Game::FrameTime() / 10);

	if (d_pas > 0)
		d_pas -= step_down;

	if (d_act > 0)
		d_act -= step_down;
}

void
SimContact::Merge(SimContact* c)
{
	if (!c) return;

	if (c->GetShip() == ship && c->GetShot() == shot) {
		if (c->d_pas > d_pas)
			d_pas = c->d_pas;

		if (c->d_act > d_act)
			d_act = c->d_act;
	}
}

void
SimContact::ClearTrack()
{
	delete[] track;
	track = nullptr;
	ntrack = 0;
}

void
SimContact::UpdateTrack()
{
	time = Game::GameTime();

	if (shot || (ship && ship->IsGroundUnit()))
		return;

	if (!track) {
		track = new  FVector[Game::DefaultTrackLength];
		track[0] = loc;
		ntrack = 1;
		track_time = time;
	}

	else if (time - track_time > (DWORD)Game::DefaultTrackUpdate) {
		if (loc != track[0]) {
			for (int i = Game::DefaultTrackLength - 2; i >= 0; i--)
				track[i + 1] = track[i];

			track[0] = loc;
			if (ntrack < Game::DefaultTrackLength) ntrack++;
		}

		track_time = time;
	}
}

// +----------------------------------------------------------------------+

FVector
SimContact::TrackPoint(int i) const
{
	if (track && i < ntrack)
		return track[i];

	return FVector::ZeroVector;
}
