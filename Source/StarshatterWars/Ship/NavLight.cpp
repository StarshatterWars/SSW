/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         NavLight.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Navigation Light System class
*/

#include "NavLight.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include <cstring>

#include "Game.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "SimSystem.h"

// +----------------------------------------------------------------------+

static class Bitmap* images[4];

// +----------------------------------------------------------------------+

NavLight::NavLight(double p, double s)
	: SimSystem(SYSTEM_CATEGORY::COMPUTER, 32, "Navigation Lights", 1, 0)
	, period(p)
	, scale(s)
	, enable(true)
	, nlights(0)
	, offset(0)
{
	name = "Navigation Lights";
	abrv = "NavLights";

	std::memset(beacon, 0, sizeof(beacon));
	std::memset(beacon_type, 0, sizeof(beacon_type));
	std::memset(pattern, 0, sizeof(pattern));
}

// +----------------------------------------------------------------------+

NavLight::NavLight(const NavLight& c)
	: SimSystem(c)
	, period(c.period)
	, scale(c.scale)
	, enable(true)
	, nlights(0)
	, offset(0)
{
	Mount(c);
	SetAbbreviation(c.Abbreviation());

	std::memset(beacon, 0, sizeof(beacon));
	std::memset(beacon_type, 0, sizeof(beacon_type));

	nlights = c.nlights;

	for (int i = 0; i < nlights; i++) {
		loc[i] = c.loc[i];
		pattern[i] = c.pattern[i];
		beacon_type[i] = c.beacon_type[i];

		DriveSprite* Rep = new DriveSprite(images[beacon_type[i]]);
		Rep->Scale(c.scale);

		beacon[i] = Rep;
	}

	offset = (DWORD)FMath::Rand();
}

// +--------------------------------------------------------------------+

NavLight::~NavLight()
{
	for (int i = 0; i < nlights; i++) {
		GRAPHIC_DESTROY(beacon[i]);
	}
}

// +--------------------------------------------------------------------+

void
NavLight::Initialize()
{
	static int initialized = 0;
	if (initialized) return;

	DataLoader* loader = DataLoader::GetLoader();

	loader->LoadTexture("beacon1.pcx", images[0], /*Bitmap::BMP_TRANSLUCENT*/ 0);
	loader->LoadTexture("beacon2.pcx", images[1], /*Bitmap::BMP_TRANSLUCENT*/ 0);
	loader->LoadTexture("beacon3.pcx", images[2], /*Bitmap::BMP_TRANSLUCENT*/ 0);
	loader->LoadTexture("beacon4.pcx", images[3], /*Bitmap::BMP_TRANSLUCENT*/ 0);

	initialized = 1;
}

void
NavLight::Close()
{
}

// +--------------------------------------------------------------------+

void
NavLight::ExecFrame(double seconds)
{
	if (enable && power_on) {
		const double t = (Game::GameTime() + offset) / 1000.0;
		const DWORD  n = (DWORD)((int)(fmod(t, period) * 32 / period));
		const DWORD  code = 1u << n;

		for (int i = 0; i < nlights; i++) {
			if (beacon[i]) {
				if (pattern[i] & code)
					beacon[i]->SetShade(1);
				else
					beacon[i]->SetShade(0);
			}
		}
	}
	else {
		for (int i = 0; i < nlights; i++) {
			if (beacon[i]) {
				beacon[i]->SetShade(0);
			}
		}
	}
}

void
NavLight::Enable()
{
	enable = true;
}

void
NavLight::Disable()
{
	enable = false;
}

void
NavLight::AddBeacon(FVector l, DWORD ptn, int t)
{
	if (nlights < MAX_LIGHTS) {
		loc[nlights] = l;
		pattern[nlights] = ptn;
		beacon_type[nlights] = t;

		DriveSprite* Rep = new DriveSprite(images[t]);
		Rep->Scale(scale);

		beacon[nlights] = Rep;
		nlights++;
	}
}

void
NavLight::SetPeriod(double p)
{
	period = p;
}

void
NavLight::SetPattern(int index, DWORD ptn)
{
	if (index >= 0 && index < nlights)
		pattern[index] = ptn;
}

void
NavLight::SetOffset(DWORD o)
{
	offset = o;
}

// +--------------------------------------------------------------------+

void
NavLight::Orient(const Physical* Rep)
{
	SimSystem::Orient(Rep);

	if (!Rep)
		return;

	const Camera& RepCam = Rep->Cam();
	const FVector ShipLoc = Rep->Location();

	// Build a UE matrix from the Starshatter camera basis vectors.
	// Assumes Cam.vrt/vup/vpn are FVector basis vectors in world space.
	const FMatrix Basis(
		FPlane(RepCam.vrt(), 0.0f),
		FPlane(RepCam.vup(), 0.0f),
		FPlane(RepCam.vpn(), 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f)
	);

	for (int32 LightIndex = 0; LightIndex < nlights; ++LightIndex) {
		// Transform local light offset into world space, then add ship location:
		const FVector Projector = Basis.TransformVector(loc[LightIndex]) + ShipLoc;

		if (beacon[LightIndex]) {
			beacon[LightIndex]->MoveTo(Projector);
		}
	}
}
