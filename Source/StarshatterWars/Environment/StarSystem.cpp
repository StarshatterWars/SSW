/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright � 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         StarSystem.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Various Heavenly Bodies
*/

#include "StarSystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "SSWSessionLobbyGameMode.h"
#include "CoreMinimal.h"
#include "Galaxy.h"
#include "Sky.h"
#include "Starshatter.h"
#include "TerrainRegion.h"
#include "TerrainHaze.h"
#include "Weather.h"

#include "Game.h"
#include "Sound.h"
#include "Solid.h"
#include "SimLight.h"
#include "DataLoader.h"
#include "SimScene.h"

#include "Misc/DateTime.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "Misc/Timespan.h"

static const double epoch = 0.5e9;
double StarSystem::stardate = 0;
static double sim_time = 0;
static double base_time = 0;

// +====================================================================+



static FORCEINLINE FColor ScaleColor(const FColor& In, float Scale)
{
	return FColor(
		(uint8)FMath::Clamp(int32(In.R * Scale), 0, 255),
		(uint8)FMath::Clamp(int32(In.G * Scale), 0, 255),
		(uint8)FMath::Clamp(int32(In.B * Scale), 0, 255),
		(uint8)FMath::Clamp(int32(In.A * Scale), 0, 255)
	);
}

void StarSystem::SetBaseTime(double t, bool absolute)
{
	if (absolute)
	{
		base_time = t;
	}
	else if (t > 0)
	{
		if (t > epoch)
		{
			t -= epoch;
		}

		base_time = t;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[StarSystem] SetBaseTime ignored because t <= 0"));
	}
}

double StarSystem::GetBaseTime()
{
	return base_time;
}

void StarSystem::CalcStardate()
{
	if (base_time < 1.0)
	{
		return;
	}

	const double gtime = sim_time;
	const double sdate = gtime + base_time + epoch;

	//UE_LOG(LogTemp, Warning,
	//	TEXT("[StarSystem] CalcStardate base_time=%f gtime=%f sdate=%f"),
	//	base_time, gtime, sdate);

	stardate = sdate;
}

 // +====================================================================+


// +====================================================================+

StarSystem::StarSystem(const char* sys_name, FVector l, int iff, int s)
	: name(sys_name)
	, affiliation(iff)
	, seq(s)
	, loc(l)
	, radius(0.0f)
	, instantiated(false)
	, sky_stars(0)
	, sky_dust(0)
	, ambient(0, 0, 0)
	, sun_color(255, 255, 255)
	, sun_brightness(0)
	, sun_scale(1)
	, point_stars(0)
	, poly_stars(0)
	, nebula(0)
	, haze(0)
	, center(0)
	, active_region(0)
	, tvpn(0, 0, 0)
	, tvup(0, 0, 0)
	, tvrt(0, 0, 0)
{
	center = new Orbital(this, "CG", Orbital::NOTHING, 1.0e35f, 0.0f, 0.0f, 0);
	radius = 0.0f;
}

// +--------------------------------------------------------------------+

StarSystem::~StarSystem()
{
	UE_LOG(LogTemp, Log,
		TEXT("   Destroying Star System %s"),
		ANSI_TO_TCHAR((const char*)name));

	if (instantiated) {
		Deactivate();
		Destroy();
	}

	// SAFE DELETE: bodies (sole owner)
	{
		ListIter<OrbitalBody> BodyIter = bodies;
		while (++BodyIter)
		{
			OrbitalBody* Body = BodyIter.value();
			if (Body)
			{
				delete Body;
			}
		}
		bodies.clear();
	}

	// SAFE DELETE: root regions only
	{
		ListIter<OrbitalRegion> RegionIter = regions;
		while (++RegionIter)
		{
			OrbitalRegion* Region = RegionIter.value();
			if (Region)
			{
				delete Region;
			}
		}
		regions.clear();
	}

	// NON-OWNING CACHE
	all_regions.clear();

	delete center;
	center = nullptr;
}

// +--------------------------------------------------------------------+

OrbitalRegion* StarSystem::LoadTerrainRegion(const FString& CatalogPath, const char* RegionName, Orbital* Parent)
{
    FString Source;TSharedPtr<FJsonObject> Root;double Version=0;
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!FFileHelper::LoadFileToString(Source,*CatalogPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Root) ||
        !Root || !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 || !Root->TryGetArrayField(TEXT("regions"),Rows))
    {UE_LOG(LogTemp,Error,TEXT("Invalid terrain JSON catalog: %s"),*CatalogPath);return nullptr;}
    DataLoader* Loader=DataLoader::GetLoader();
    const Text PreviousPath(Loader?Loader->GetDataPath():"");
    if(Loader)Loader->SetDataPath("Galaxy/");
    ON_SCOPE_EXIT{if(Loader)Loader->SetDataPath(PreviousPath.data());};
    FString SystemName=UTF8_TO_TCHAR(GetName());SystemName.RemoveFromEnd(TEXT(" System"));
    OrbitalRegion* Last=nullptr;
    for(const auto& Value:*Rows)
    {
        const auto Row=Value.IsValid() && Value->Type==EJson::Object?Value->AsObject():nullptr;
        FString System,Name,PrimaryName;
        if(!Row || !Row->TryGetStringField(TEXT("system"),System) || !Row->TryGetStringField(TEXT("name"),Name) || !Row->TryGetStringField(TEXT("parent"),PrimaryName))
        {UE_LOG(LogTemp,Error,TEXT("Invalid terrain identity in %s"),*CatalogPath);return nullptr;}
        if(!System.Equals(SystemName,ESearchCase::IgnoreCase) || (RegionName && Name!=UTF8_TO_TCHAR(RegionName)))continue;
        if(auto* Existing=FindRegion(TCHAR_TO_UTF8(*Name))){Last=Existing;continue;}
        Orbital* Primary=Parent?Parent:FindOrbital(TCHAR_TO_UTF8(*PrimaryName));
        double Radius=0;
        if(!Primary || !Row->TryGetNumberField(TEXT("radius"),Radius) || !FMath::IsFinite(Radius) || Radius<=0)
        {UE_LOG(LogTemp,Error,TEXT("Terrain %s has no valid primary/radius"),*Name);return nullptr;}
        auto* Region=new TerrainRegion(this,TCHAR_TO_UTF8(*Name),Radius,Primary);
        auto TextField=[&](const TCHAR* Key,Text& Target){FString V;if(Row->TryGetStringField(Key,V))Target=TCHAR_TO_UTF8(*V);};
        auto Number=[&](const TCHAR* Key,double Fallback){double V=Fallback;Row->TryGetNumberField(Key,V);return FMath::IsFinite(V)?V:Fallback;};
        Region->grid=Number(TEXT("grid"),25000);Region->inclination=Number(TEXT("inclination"),0);
        Region->scale=Number(TEXT("scale"),10000);Region->mtnscale=Number(TEXT("mtnscale"),1000);
        Region->fog_density=Number(TEXT("fog_density"),0);Region->fog_scale=Number(TEXT("fog_scale"),0);Region->haze_fade=Number(TEXT("haze_fade"),0);
        Region->clouds_alt_high=Number(TEXT("clouds_alt_high"),0);Region->clouds_alt_low=Number(TEXT("clouds_alt_low"),0);
        TextField(TEXT("patch"),Region->patch_name);TextField(TEXT("patch_texture"),Region->patch_texture);
        TextField(TEXT("detail_texture_0"),Region->noise_tex0);TextField(TEXT("detail_texture_1"),Region->noise_tex1);
        TextField(TEXT("apron"),Region->apron_name);TextField(TEXT("apron_texture"),Region->apron_texture);TextField(TEXT("water_texture"),Region->water_texture);
        TextField(TEXT("haze"),Region->haze_name);TextField(TEXT("clouds_high"),Region->clouds_high);TextField(TEXT("clouds_low"),Region->clouds_low);
        TextField(TEXT("shades_high"),Region->shades_high);TextField(TEXT("shades_low"),Region->shades_low);
        TextField(TEXT("env_texture_positive_x"),Region->env_texture_positive_x);TextField(TEXT("env_texture_negative_x"),Region->env_texture_negative_x);
        TextField(TEXT("env_texture_positive_y"),Region->env_texture_positive_y);TextField(TEXT("env_texture_negative_y"),Region->env_texture_negative_y);
        TextField(TEXT("env_texture_positive_z"),Region->env_texture_positive_z);TextField(TEXT("env_texture_negative_z"),Region->env_texture_negative_z);
        const TArray<TSharedPtr<FJsonValue>>* Layers=nullptr;
        if(Row->TryGetArrayField(TEXT("layers"),Layers))for(const auto& LayerValue:*Layers)
        {
            const auto Layer=LayerValue.IsValid() && LayerValue->Type==EJson::Object?LayerValue->AsObject():nullptr;FString Tile,Detail;double Height=0;
            if(!Layer || !Layer->TryGetStringField(TEXT("tile"),Tile))continue;
            Layer->TryGetStringField(TEXT("detail"),Detail);Layer->TryGetNumberField(TEXT("height"),Height);
            Region->AddLayer(Height,TCHAR_TO_UTF8(*Tile),TCHAR_TO_UTF8(*Detail));
        }
        Region->GetWeather().SetPeriod(Number(TEXT("weather_period"),0));
        const TCHAR* Chances[]={TEXT("weather_clear"),TEXT("weather_high_clouds"),TEXT("weather_moderate_clouds"),TEXT("weather_overcast"),TEXT("weather_fog"),TEXT("weather_storm")};
        for(int32 Index=0;Index<UE_ARRAY_COUNT(Chances);++Index)Region->GetWeather().SetChance(Index,Number(Chances[Index],0));
        const TSharedPtr<FJsonObject>* Presentation=nullptr;
        if(Row->TryGetObjectField(TEXT("presentation"),Presentation) && Presentation && Presentation->IsValid())
        {
            (*Presentation)->TryGetStringField(TEXT("surfaceMaterial"),Region->surface_material);
            (*Presentation)->TryGetNumberField(TEXT("sunIntensityLux"),Region->surface_sun_intensity);
            (*Presentation)->TryGetStringField(TEXT("biomeProfile"),Region->biome_profile);
            (*Presentation)->TryGetStringField(TEXT("weatherProfile"),Region->weather_profile);
        }
        FString Sky;if(Loader && Row->TryGetStringField(TEXT("sky_color"),Sky))Region->LoadSkyColors(TCHAR_TO_UTF8(*Sky));
        Primary->AddRegion(Region);all_regions.append(Region);Last=Region;
    }
    return Last;
}

void StarSystem::Create()
{
	if (Game::Server())
		return;

	if (!instantiated) {
		UE_LOG(LogTemp, Log, TEXT("Creating Star System %s"), ANSI_TO_TCHAR((const char*)name));

		DataLoader* loader = DataLoader::GetLoader();
		const FString DataPathString(datapath.data());
		loader->SetDataPath(TCHAR_TO_ANSI(*DataPathString));

		// poly star shell:
		if (sky_poly_stars.length()) {
			poly_stars = new Solid;
			poly_stars->Load(sky_poly_stars, 120);
			poly_stars->SetLuminous(true);
			poly_stars->SetInfinite(true);

			UE_LOG(LogTemp, Log, TEXT("Celestial Sphere '%s' loaded"), ANSI_TO_TCHAR((const char*)sky_poly_stars));
			UE_LOG(LogTemp, Log, TEXT("   radius: %f"), poly_stars->Radius());
		}

		if (sky_stars) {
			UE_LOG(LogTemp, Log, TEXT("Point Stars: %d"), sky_stars);
			point_stars = new Stars(sky_stars);
		}

		const FString SSDataPathString(datapath.data());
		loader->SetDataPath(TCHAR_TO_ANSI(*SSDataPathString));

		// nebula:
		if (sky_nebula.length()) {
			nebula = new Solid;
			nebula->Load(sky_nebula, 100);
			nebula->SetLuminous(true);
			nebula->SetInfinite(true);

			UE_LOG(LogTemp, Log, TEXT("Nebular Sky '%s' loaded"), ANSI_TO_TCHAR((const char*)sky_nebula));
			UE_LOG(LogTemp, Log, TEXT("   radius: %f"), nebula->Radius());
		}

		// atmospheric haze:
		if (sky_haze.length()) {
			const FString SHDataPathString(datapath.data());
			loader->SetDataPath(TCHAR_TO_ANSI(*SHDataPathString));
			haze = new TerrainHaze();
			haze->Load(sky_haze, 120);
			haze->SetInfinite(true);

			UE_LOG(LogTemp, Log, TEXT("Atmospheric Haze '%s' loaded"), ANSI_TO_TCHAR((const char*)sky_haze));
			UE_LOG(LogTemp, Log, TEXT("   radius: %f"), haze->Radius());

			haze->Hide();
		}

		loader->SetDataPath("");

		ListIter<OrbitalBody> star = bodies;
		while (++star) {
			CreateBody(*star);

			ListIter<OrbitalBody> planet = star->Satellites();
			while (++planet) {
				CreateBody(*planet);

				ListIter<OrbitalBody> moon = planet->Satellites();
				while (++moon) {
					CreateBody(*moon);
				}
			}
		}
	}

	instantiated = true;
}

// +--------------------------------------------------------------------+

void StarSystem::CreateBody(OrbitalBody& body)
{
	DataLoader* loader = DataLoader::GetLoader();
	const FString DataPathString(datapath.data());
	loader->SetDataPath(TCHAR_TO_ANSI(*DataPathString));

	// stars:
	if (body.type == Orbital::STAR) {
		FVector starloc = OtherHand(body.loc);

		PlanetRep* rep = new PlanetRep(body.tex_name,
			0,
			body.radius,
			starloc,
			body.tscale);

		rep->SetLuminous(true);
		rep->MoveTo(loc);
		body.rep = rep;

		sun_brightness = body.light;
		sun_color = body.color;

		SimLight* sun_light = new SimLight(1.1f);
		sun_light->SetColor(sun_color);
		sun_light->SetShadow(true);
		sun_light->MoveTo(body.loc);
		sun_light->SetType(LIGHTTYPE::DIRECTIONAL);

		sun_lights.append(sun_light);
		body.light_rep = sun_light;

		if (body.back != FColor::Black) {
			SimLight* back_light = new SimLight(0.6f);
			back_light->SetColor(body.back);
			back_light->SetShadow(false);
			back_light->MoveTo(body.loc * -1);
			back_light->SetType(LIGHTTYPE::DIRECTIONAL);

			back_lights.append(back_light);
			body.back_light = back_light;
		}
	}

	// planets and moons:
	else {
		FVector planetloc = OtherHand(body.loc);

		double rmax = 0;
		double rmin = 0;

		if (body.ring_max > 0) {
			rmax = body.ring_max * body.radius;
			rmin = body.ring_min * body.radius;
		}

		Text surface = body.tex_name;
		Text glow = body.tex_glow;

		if (Game::MaxTexSize() >= 512) {
			if (body.tex_high_res.length())
				surface = body.tex_high_res;

			if (body.tex_glow_high_res.length())
				glow = body.tex_glow_high_res;
		}

		PlanetRep* rep = new PlanetRep(surface,
			glow,
			body.radius,
			planetloc,
			body.tscale,
			body.tex_ring,
			rmin,
			rmax,
			body.atmosphere,
			body.tex_gloss);

		rep->SetStarSystem(this);

		// UE FIX (new Solid): Solid/Graphic orientation is now FMatrix.
		// Starshatter "tilt" is typically stored in radians; convert to degrees for FRotator.
		if (body.tilt != 0) {
			const float PitchDeg = FMath::RadiansToDegrees((float)body.tilt);
			const FMatrix TiltM = FRotationMatrix(FRotator(PitchDeg, 0.0f, 0.0f));
			rep->SetOrientation(TiltM);
		}

		body.rep = rep;
	}
}

// +--------------------------------------------------------------------+

void StarSystem::Destroy()
{
	if (instantiated) {
		ListIter<OrbitalBody> star_iter = bodies;
		while (++star_iter) {
			OrbitalBody* star = star_iter.value();

			GRAPHIC_DESTROY(star->rep);
			SIMLIGHT_DESTROY(star->light_rep);
			SIMLIGHT_DESTROY(star->back_light);

			ListIter<OrbitalBody> planet = star->Satellites();
			while (++planet) {
				GRAPHIC_DESTROY(planet->rep);

				ListIter<OrbitalBody> moon = planet->Satellites();
				while (++moon) {
					GRAPHIC_DESTROY(moon->rep);
				}
			}
		}

		GRAPHIC_DESTROY(point_stars);
		GRAPHIC_DESTROY(poly_stars);
		GRAPHIC_DESTROY(nebula);
		GRAPHIC_DESTROY(haze);

		sun_lights.clear();
		back_lights.clear();
	}

	instantiated = false;
	sun_scale = 1;
}

// +--------------------------------------------------------------------+

void StarSystem::Activate(SimScene& scene)
{
	if (!instantiated)
		Create();

	Starshatter* stars = Starshatter::GetInstance();
	if (!stars)
		return;

	if (point_stars) {
		scene.AddBackground(point_stars);
		point_stars->Hide();
	}

	if (poly_stars) {
		scene.AddBackground(poly_stars);
	}

	if (stars->Nebula() && nebula) {
		scene.AddBackground(nebula);
	}

	if (haze) {
		scene.AddBackground(haze);
		haze->Hide();
	}

	ListIter<OrbitalBody> star_iter = bodies;
	while (++star_iter) {
		OrbitalBody* star = star_iter.value();
		scene.AddGraphic(star->rep);
		scene.AddLight(star->light_rep);

		if (nebula && stars && stars->Nebula()) {
			star->back_light->SetColor(star->back);
		}
		else {
			FColor c = FColor(60, 60, 65);
			star->back_light->SetColor(c);
		}

		scene.AddLight(star->back_light);

		if (nebula && stars && stars->Nebula()) {
			scene.SetAmbient(ambient);
		}
		else {
			FColor c = ambient;
			int n = (c.R + c.G + c.B) / 3;

			c = FColor(n, n, n);
			scene.SetAmbient(c);
		}

		ListIter<OrbitalBody> planet = star->Satellites();
		while (++planet) {
			scene.AddGraphic(planet->rep);

			ListIter<OrbitalBody> moon = planet->Satellites();
			while (++moon) {
				scene.AddGraphic(moon->rep);
			}
		}
	}

	ExecFrame();
}

// +--------------------------------------------------------------------+

void StarSystem::Deactivate()
{
	if (!instantiated)
		return;

	active_region = 0;

	if (point_stars && point_stars->GetScene())
		point_stars->GetScene()->DelBackground(point_stars);

	if (poly_stars && poly_stars->GetScene())
		poly_stars->GetScene()->DelBackground(poly_stars);

	if (nebula && nebula->GetScene())
		nebula->GetScene()->DelBackground(nebula);

	if (haze && haze->GetScene()) {
		haze->GetScene()->DelBackground(haze);
		haze->Hide();
	}

	ListIter<OrbitalBody> star = bodies;
	while (++star) {
		if (star->rep && star->rep->GetScene())
			star->rep->GetScene()->DelGraphic(star->rep);

		if (star->light_rep && star->light_rep->GetScene())
			star->light_rep->GetScene()->DelLight(star->light_rep);

		if (star->back_light && star->back_light->GetScene())
			star->back_light->GetScene()->DelLight(star->back_light);

		ListIter<OrbitalBody> planet = star->Satellites();
		while (++planet) {
			if (planet->rep && planet->rep->GetScene())
				planet->rep->GetScene()->DelGraphic(planet->rep);

			ListIter<OrbitalBody> moon = planet->Satellites();
			while (++moon) {
				if (moon->rep && moon->rep->GetScene())
					moon->rep->GetScene()->DelGraphic(moon->rep);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void StarSystem::SetActiveRegion(OrbitalRegion* rgn)
{
	SimScene* scene = 0;

	if (active_region != rgn) {
		active_region = rgn;

		if (active_region) {
			if (active_region->GetType() != Orbital::TERRAIN) {
				if (point_stars) point_stars->Hide();
				if (poly_stars)  poly_stars->Show();
				if (nebula)      nebula->Show();
				if (haze)        haze->Hide();
			}
			else {
				if (point_stars) point_stars->Show();
				if (poly_stars)  poly_stars->Hide();
				if (nebula)      nebula->Hide();
				if (haze)        haze->Show();
			}

			if (poly_stars) {
				scene = poly_stars->GetScene();
				if (scene)
					scene->SetAmbient(ambient);
			}
			else if (nebula) {
				scene = nebula->GetScene();
				if (scene)
					scene->SetAmbient(ambient);
			}

			ListIter<OrbitalBody> star = bodies;
			while (++star) {
				if (star->rep)
					star->rep->Show();
			}

			ExecFrame();
		}
		else {
			if (point_stars) point_stars->Hide();
			if (poly_stars)  poly_stars->Hide();
			if (nebula)      nebula->Hide();
			if (haze)        haze->Hide();

			ListIter<OrbitalBody> star = bodies;
			while (++star) {
				if (star->rep)
					star->rep->Hide();

				if (star->light_rep) {
					scene = star->light_rep->GetScene();
					if (scene)
						scene->DelLight(star->light_rep);
				}

				if (star->back_light) {
					scene = star->back_light->GetScene();
					if (scene)
						scene->DelLight(star->back_light);
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

static BYTE max3(BYTE a, BYTE b, BYTE c)
{
	if (a > b)
		if (a > c) return a;
		else       return c;
	else
		if (b > c) return b;
		else       return c;
}

static BYTE min3(BYTE a, BYTE b, BYTE c)
{
	if (a < b)
		if (a < c) return a;
		else       return c;
	else
		if (b < c) return b;
		else       return c;
}

void StarSystem::ExecFrame()
{
	CalcStardate();

	ListIter<OrbitalBody> star = bodies;
	while (++star)
		star->Update();

	ListIter<OrbitalRegion> region = regions;
	while (++region)
		region->Update();

	// update the graphic reps, relative to the active region:
	if (instantiated && active_region) {
		FVector active_loc = OtherHand(active_region->Location());

		SimScene* scene = 0;
		TerrainRegion* trgn = 0;
		const bool      terrain = (active_region->GetType() == Orbital::TERRAIN);

		// UE FIX: Solid/Graphic orientation is now FMatrix (not legacy Matrix):
		FMatrix         terrain_orientation = FMatrix::Identity;
		FMatrix         terrain_transformation = FMatrix::Identity;

		if (terrain) {
			trgn = (TerrainRegion*)active_region;
			FColor tmp = trgn->SkyColor();
			Game::SetScreenColor(tmp);

			tvpn = (active_region->Location() - active_region->Primary()->Location());
			tvpn.Normalize();

			tvup = FVector(0, 0, -1);

			tvrt = FVector::CrossProduct(tvpn, tvup);
			tvrt.Normalize();

			// Original: terrain_orientation.Rotate(0, PI/2, 0);
			// Assuming Starshatter uses radians: PI/2 -> 90 degrees.
			terrain_orientation = FRotationMatrix(FRotator(0.0f, 90.0f, 0.0f));

			// Original: terrain_transformation = Matrix(tvrt, tvup, tvpn);
			// Build an Unreal FMatrix from basis vectors (rotation only):
			const FVector XAxis = tvrt.GetSafeNormal(); // right
			const FVector YAxis = tvup.GetSafeNormal(); // up
			const FVector ZAxis = tvpn.GetSafeNormal(); // forward

			terrain_transformation = FMatrix(
				FPlane(XAxis.X, XAxis.Y, XAxis.Z, 0.0f),
				FPlane(YAxis.X, YAxis.Y, YAxis.Z, 0.0f),
				FPlane(ZAxis.X, ZAxis.Y, ZAxis.Z, 0.0f),
				FPlane(0.0f, 0.0f, 0.0f, 1.0f)
			);

			if (point_stars) {
				point_stars->SetOrientation(terrain_transformation);

				FColor sky_color = trgn->SkyColor();
				BYTE  sky_red = (BYTE)sky_color.R;
				BYTE  sky_green = (BYTE)sky_color.G;
				BYTE  sky_blue = (BYTE)sky_color.B;

				BYTE Max = max3(sky_red, sky_green, sky_blue);
				BYTE Min = min3(sky_red, sky_green, sky_blue);

				BYTE lum = (BYTE)(240.0 * (Max + Min) / 510.0);

				if (lum > 50) {
					point_stars->Hide();
				}
				else {
					Stars* pstars = (Stars*)point_stars;
					pstars->Illuminate(1.0 - lum / 50.0);
					pstars->Show();
				}

				scene = point_stars->GetScene();
			}

			if (haze) {
				((TerrainHaze*)haze)->UseTerrainRegion(trgn);
				scene = haze->GetScene();
			}

			if (scene) {
				scene->SetAmbient(ambient);
			}
		}
		else {
			Game::SetScreenColor(FColor::Black);
		}

		double star_alt = 0;

		ListIter<OrbitalBody> star_iter = bodies;
		while (++star_iter) {
			OrbitalBody* star_body = star_iter.value();

			if (active_region->GetInclination() != 0) {
				const double distance = (active_region->Location() - star_body->Location()).Size();
				star_alt = FMath::Sin(active_region->GetInclination()) * distance;
			}

			if (terrain) {
				const FVector sloc = TerrainTransform(star_body->Location());

				if (star_body->rep) {
					star_body->rep->MoveTo(sloc);

					PlanetRep* pr = (PlanetRep*)star_body->rep;
					pr->SetDaytime(true);
				}

				if (star_body->light_rep) {
					star_body->light_rep->MoveTo(sloc);
					star_body->light_rep->SetActive(sloc.Y > -100);
				}

				if (star_body->back_light) {
					star_body->back_light->MoveTo(sloc * -1);
					star_body->back_light->SetActive(sloc.Y > -100);
				}

				if (trgn && star_body->rep) {
					if (trgn->IsEclipsed())
						star_body->rep->Hide();
					else
						star_body->rep->Show();
				}
			}
			else {
				FVector sloc = OtherHand(star_body->Location() - active_region->Location());
				sloc.Y = (float)star_alt;

				if (star_body->rep) {
					star_body->rep->MoveTo(sloc);

					PlanetRep* pr = (PlanetRep*)star_body->rep;
					pr->SetDaytime(false);
				}

				if (star_body->light_rep) {
					star_body->light_rep->MoveTo(sloc);
					star_body->light_rep->SetActive(true);
				}

				if (star_body->back_light) {
					star_body->back_light->MoveTo(sloc * -1);
					star_body->back_light->SetActive(true);
				}
			}

			ListIter<OrbitalBody> planet_iter = star_body->Satellites();
			while (++planet_iter) {
				OrbitalBody* planet = planet_iter.value();

				if (planet->rep) {
					PlanetRep* pr = (PlanetRep*)planet->rep;

					if (terrain) {
						pr->MoveTo(TerrainTransform(planet->Location()));
						pr->SetOrientation(terrain_orientation);

						if (planet == active_region->Primary()) {
							pr->Hide();
						}
						else {
							pr->Show();
							pr->SetDaytime(true);
						}
					}
					else {
						pr->Show();
						pr->TranslateBy(active_loc);
						pr->SetDaytime(false);
					}
				}

				ListIter<OrbitalBody> moon_iter = planet->Satellites();
				while (++moon_iter) {
					OrbitalBody* moon = moon_iter.value();

					if (moon->rep) {
						PlanetRep* pr = (PlanetRep*)moon->rep;

						if (terrain) {
							pr->MoveTo(TerrainTransform(moon->Location()));
							pr->SetOrientation(terrain_orientation);

							if (moon == active_region->Primary()) {
								pr->Hide();
							}
							else {
								pr->Show();
								pr->SetDaytime(true);
							}
						}
						else {
							pr->Show();
							pr->TranslateBy(active_loc);
							pr->SetDaytime(false);
						}
					}
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

Orbital* StarSystem::FindOrbital(const char* in_name)
{
	if (!in_name || !in_name[0])
		return 0;

	ListIter<OrbitalBody> star = bodies;
	while (++star) {
		if (!FCStringAnsi::Stricmp(star->GetName(), in_name))
			return star.value();

		ListIter<OrbitalRegion> star_rgn = star->Regions();
		while (++star_rgn) {
			if (!FCStringAnsi::Stricmp(star_rgn->GetName(), in_name))
				return star_rgn.value();
		}

		ListIter<OrbitalBody> planet = star->Satellites();
		while (++planet) {
			if (!FCStringAnsi::Stricmp(planet->GetName(), in_name))
				return planet.value();

			ListIter<OrbitalRegion> planet_rgn = planet->Regions();
			while (++planet_rgn) {
				if (!FCStringAnsi::Stricmp(planet_rgn->GetName(), in_name))
					return planet_rgn.value();
			}

			ListIter<OrbitalBody> moon = planet->Satellites();
			while (++moon) {
				if (!FCStringAnsi::Stricmp(moon->GetName(), in_name))
					return moon.value();

				ListIter<OrbitalRegion> moon_rgn = moon->Regions();
				while (++moon_rgn) {
					if (!FCStringAnsi::Stricmp(moon_rgn->GetName(), in_name))
						return moon_rgn.value();
				}
			}
		}
	}

	ListIter<OrbitalRegion> region = regions;
	while (++region) {
		if (!FCStringAnsi::Stricmp(region->GetName(), in_name))
			return region.value();
	}

	return 0;
}

// +--------------------------------------------------------------------+

OrbitalRegion* StarSystem::FindRegion(const char* in_name)
{
	if (!in_name || !in_name[0])
		return 0;

	ListIter<OrbitalRegion> region = all_regions;
	while (++region) {
		if (!FCStringAnsi::Stricmp(region->GetName(), in_name))
			return region.value();
	}

	return 0;
}

// +--------------------------------------------------------------------+

bool StarSystem::HasLinkTo(StarSystem* s) const
{
	ListIter<OrbitalRegion> iter = ((StarSystem*)this)->all_regions;
	while (++iter) {
		OrbitalRegion* rgn = iter.value();

		ListIter<Text> lnk_iter = rgn->GetLinks();
		while (++lnk_iter) {
			Text* t = lnk_iter.value();

			if (s->FindRegion(*t))
				return true;
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

FVector StarSystem::TerrainTransform(const FVector& in_loc)
{
	FVector tmp = in_loc - active_region->Location();

	FVector result;
	result.X = FVector::DotProduct(tmp, tvrt);
	result.Z = FVector::DotProduct(tmp, tvup);
	result.Y = FVector::DotProduct(tmp, tvpn);

	return result;
}

FColor StarSystem::GetAmbient() const
{
	FColor result = ambient;
	const bool terrain = (active_region && active_region->GetType() == Orbital::TERRAIN);

	if (terrain) {
		TerrainRegion* trgn = static_cast<TerrainRegion*>(active_region);
		result = trgn->Ambient();

		if (trgn->IsEclipsed()) {
			result = ScaleColor(result, 0.3f);
		}
	}

	return result;
}

void StarSystem::SetSunlight(FColor color, double brightness)
{
	sun_color = color;
	sun_scale = brightness;

	ListIter<SimLight> sun_iter = sun_lights;
	while (++sun_iter) {
		SimLight* sun_light = sun_iter.value();
		sun_light->SetColor(color);
		sun_light->SetIntensity((float)(1.1 * sun_scale));
	}

	ListIter<SimLight> back_iter = back_lights;
	while (++back_iter) {
		SimLight* back_light = back_iter.value();
		back_light->SetIntensity((float)(0.5 * sun_scale));
	}
}

void StarSystem::SetBacklight(FColor color, double brightness)
{
	ListIter<SimLight> back_iter = back_lights;
	while (++back_iter) {
		SimLight* back_light = back_iter.value();
		back_light->SetColor(color);
		back_light->SetIntensity((float)(0.5 * brightness));
	}
}

void StarSystem::RestoreTrueSunColor()
{
	ListIter<OrbitalBody> iter = bodies;
	while (++iter) {
		OrbitalBody* star = iter.value();

		if (star) {
			if (star->light_rep) {
				star->light_rep->SetColor(star->LightColor());
				star->light_rep->SetIntensity(1.1f);
			}

			if (star->back_light) {
				star->back_light->SetColor(star->back);
				star->back_light->SetIntensity(0.5f);
			}
		}
	}
}

// +====================================================================+

FColor Star::GetColor() const
{
	return GetColor(seq);
}

int Star::GetSize() const
{
	return GetSize(seq);
}

FColor Star::GetColor(int s)
{
	switch (s) {
	case O:           return FColor(128, 128, 255);
	case B:           return FColor(192, 192, 255);
	case A:           return FColor(220, 220, 255);
	case F:           return FColor(255, 255, 255);
	case G:           return FColor(255, 255, 128);
	case K:           return FColor(255, 192, 100);
	case M:           return FColor(255, 100, 100);

	case RED_GIANT:   return FColor(255, 80, 80);
	case WHITE_DWARF: return FColor(255, 255, 255);
	case BLACK_HOLE:  return FColor(0, 0, 0);
	}

	return FColor::White;
}

int Star::GetSize(int s)
{
	switch (s) {
	case O:           return 4;
	case B:           return 4;
	case A:           return 3;
	case F:           return 3;
	case G:           return 2;
	case K:           return 2;
	case M:           return 1;

	case RED_GIANT:   return 4;
	case WHITE_DWARF: return 1;
	case BLACK_HOLE:  return 3;
	}

	return 3;
}

// +====================================================================+

Orbital::Orbital(StarSystem* s, const char* n, OrbitalType t, double m, double r, double o, Orbital* p)
	: name(n)
	, type(t)
	, subtype(0)
	, description("")
	, mass(m)
	, radius(r)
	, rotation(0)
	, theta(0)
	, orbit(o)
	, phase(0)
	, period(0)
	, velocity(0)
	, loc(0, 0, 0)
	, retro(false)
	, rep(0)
	, map_icon(nullptr)
	, system(s)
	, primary(p)
	, regions()
{
	if (system && primary && orbit > 0) {
		velocity = sqrt(GRAV * (primary->Mass() + FMath::Max(0.0,mass)) / orbit);
		period = 2 * PI * orbit / velocity;
	}

	Update();
}

// +--------------------------------------------------------------------+

Orbital::~Orbital()
{
	delete rep;
	rep = nullptr;

	// DO NOT DELETE regions here (owned by StarSystem)
	regions.clear();
}

// +--------------------------------------------------------------------+

void Orbital::SetOrbitShape(double AuthoredEccentricity, double InclinationDegrees)
{
    authored_eccentricity = FMath::IsFinite(AuthoredEccentricity)
        ? FMath::Clamp(AuthoredEccentricity, 0.0, 0.75) : 0.0;
    eccentricity = authored_eccentricity;
    orbit_inclination = FMath::IsFinite(InclinationDegrees)
        ? FMath::DegreesToRadians(FMath::Clamp(InclinationDegrees,-180.0,180.0)) : 0.0;
}

void Orbital::BoundOrbit(ListIter<OrbitalBody> Siblings)
{
    eccentricity = authored_eccentricity;
    if(primary && orbit>0)
        eccentricity=FMath::Min(eccentricity,FMath::Max(0.0,1.0-(primary->Radius()+ClearanceRadius()+FMath::Max(1000.0,(primary->Radius()+ClearanceRadius())*.01))/orbit));
    if (orbit <= 0)
        return;

    while (++Siblings)
    {
        const OrbitalBody* Sibling = Siblings.value();
        if (Sibling && Sibling != this && Sibling->Orbit() > 0)
        {
            const double Gap = FMath::Abs(Sibling->Orbit() - orbit);
            const double Radii=ClearanceRadius()+Sibling->ClearanceRadius();
            const double Free=FMath::Max(0.0,Gap-Radii-FMath::Max(1000.0,Radii*.01));
            eccentricity = FMath::Min(eccentricity, 0.45 * Free / orbit);
        }
    }
}

FVector Orbital::OrbitOffset(double MeanAnomaly) const
{
    if(!FMath::IsFinite(MeanAnomaly) || !FMath::IsFinite(orbit) || orbit<=0)return FVector::ZeroVector;
    const double Mean = FMath::Fmod(MeanAnomaly, 2.0 * PI);
    double Anomaly = Mean;
    if (eccentricity > 0.0)
    {
        for (int Iteration = 0; Iteration < 16; ++Iteration)
        {
            Anomaly -= (Anomaly - eccentricity * FMath::Sin(Anomaly) - Mean) /
                (1.0 - eccentricity * FMath::Cos(Anomaly));
        }
    }
    const double X = orbit * (FMath::Cos(Anomaly) - eccentricity);
    const double Y = orbit * FMath::Sqrt(1.0 - eccentricity * eccentricity) * FMath::Sin(Anomaly);
    return FVector(X, Y * FMath::Cos(orbit_inclination), Y * FMath::Sin(orbit_inclination));
}

void Orbital::Update()
{
    const auto* Environment = UStarshatterEnvironmentSubsystem::Get();
    const UWorld* World = Environment ? Environment->GetWorld() : nullptr;
    if (World && World->GetNetMode() == NM_Client)
    {
        if (const auto* State = World->GetGameState<ASSWSessionGameState>())
        {
            if (const auto* Position = State->FindOrbitalPosition(*this))
            {
                loc = Position->Position;
                phase = Position->Phase;
            }
        }
    }
    else if (system && primary && period > 0) {
		const double grade = (retro) ? -1 : 1;

		// orbits are counter clockwise:
		phase = -2 * PI * grade * StarSystem::GetStardate() / period;

		loc = primary->Location() + OrbitOffset(phase);
	}

	ListIter<OrbitalRegion> region = regions;
	while (++region)
		region->Update();

	if (rep)
		rep->MoveTo(OtherHand(loc));
}

// +--------------------------------------------------------------------+

FVector Orbital::PredictLocation(double delta_t)
{
	FVector predicted_loc = Location();

	if (system && primary && period > 0) {
		predicted_loc = primary->PredictLocation(delta_t);

		const double grade = (retro) ? -1 : 1;

		// orbits are(?) counter clockwise:
		const double predicted_phase = (double)(-2 * PI * grade * (StarSystem::GetStardate() + delta_t) / period);

		predicted_loc += OrbitOffset(predicted_phase);
	}

	return predicted_loc;
}

// +====================================================================+

OrbitalBody::OrbitalBody(StarSystem* s, const char* n, OrbitalType t, double m, double r, double o, Orbital* p)
	: Orbital(s, n, t, m, r, o, p)
	, map_name("")
	, tex_name("")
	, tex_high_res("")
	, tex_ring("")
	, tex_glow("")
	, tex_glow_high_res("")
	, tex_gloss("")
	, tscale(1.0)
	, light(0)
	, ring_min(0)
	, ring_max(0)
	, tilt(0)
	, subtype(0)
	, light_rep(0)
	, back_light(0)
	, color(FColor::White)
	, back(FColor::Black)
	, atmosphere(FColor::Black)
	, luminous(false)
	, satellites()
{
	Update();
}

// +--------------------------------------------------------------------+

OrbitalBody::~OrbitalBody()
{
	// DO NOT DELETE satellites here
	satellites.clear();
}

// +--------------------------------------------------------------------+

void OrbitalBody::Update()
{
	Orbital::Update();

	theta = 0.0;

	if (rotation > 0.0)
		theta = -2.0 * PI * StarSystem::GetStardate() / rotation;

	ListIter<OrbitalBody> BodyIter = satellites;
	while (++BodyIter)
		BodyIter->Update();

	if (rep && theta != 0.0) {
		// Unreal uses degrees, Starshatter math is radians
		const float PitchDeg = FMath::RadiansToDegrees(tilt);
		const float RollDeg = FMath::RadiansToDegrees(tilt * 0.5);
		const float YawDeg = FMath::RadiansToDegrees(theta);

		const FRotator OrientationRotator(PitchDeg, YawDeg, RollDeg);
		rep->SetOrientation(FRotationMatrix(OrientationRotator));
	}

	if (light_rep) {
		const FVector BodyLocation = OtherHand(loc);
		light_rep->MoveTo(BodyLocation);
	}
}

// +====================================================================+

OrbitalRegion::OrbitalRegion(StarSystem* s, const char* n, double m, double r, double o, Orbital* p)
	: Orbital(s, n, REGION, m, r, o, p)
	, grid(25.0e3f)
	, inclination(0)
	, asteroids(0)
	, links()
{
}

// +--------------------------------------------------------------------+

OrbitalRegion::~OrbitalRegion()
{
	// DO NOT DELETE links here
	links.clear();
}

void StarSystem::SetSimulationTime(double t)
{
	sim_time = t;
}

double StarSystem::GetSimulationTime()
{
	return sim_time;
}

Orbital* StarSystem::GetCenter() const
{
	return center;
}

void StarSystem::AddBody(OrbitalBody* Body)
{
	if (!Body)
	{
		return;
	}

	bodies.append(Body);

	if (Body->Orbit() > radius)
	{
		radius = Body->Orbit();
	}
}

void StarSystem::AddRegion(OrbitalRegion* Region)
{
	if (!Region)
	{
		return;
	}

	regions.append(Region);
	all_regions.append(Region);

	if (Region->Orbit() > radius)
	{
		radius = Region->Orbit();
	}
}

void StarSystem::RecalculateRadius()
{
    radius=0;
    const auto BoundGroup=[&](OrbitalBody* Parent)
    {
        TArray<OrbitalBody*> Children;for(auto It=Parent->Satellites();++It;)if(It.value())Children.Add(It.value());
        Children.Sort([](const OrbitalBody& A,const OrbitalBody& B){return A.Orbit()==B.Orbit()?FCStringAnsi::Strcmp(A.GetName(),B.GetName())<0:A.Orbit()<B.Orbit();});
        OrbitalBody* Previous=nullptr;
        for(auto* Body:Children)
        {
            const double ParentRadii=Parent->Radius()+Body->ClearanceRadius();
            double Minimum=ParentRadii+FMath::Max(1000.0,ParentRadii*.01);
            if(Previous){const double Radii=Previous->ClearanceRadius()+Body->ClearanceRadius();Minimum=FMath::Max(Minimum,Previous->Orbit()+Radii+FMath::Max(1000.0,Radii*.01));}
            if(!FMath::IsFinite(Body->orbit) || Body->orbit<Minimum)
            {UE_LOG(LogTemp,Display,TEXT("[Orbit] Corrected %hs/%hs semi-major axis from %.0f to %.0f m for clearance."),GetName(),Body->GetName(),Body->orbit,Minimum);Body->orbit=Minimum;}
            const double Mu=GRAV*(Parent->Mass()+FMath::Max(0.0,Body->Mass()));
            Body->velocity=Mu>0?FMath::Sqrt(Mu/Body->orbit):0;
            Body->period=Body->velocity>0?2*PI*Body->orbit/Body->velocity:0;Previous=Body;
        }
        for(auto* Body:Children){Body->BoundOrbit(Parent->Satellites());Body->Update();}
    };
    for(ListIter<OrbitalBody> Stars=bodies;++Stars;)
    {
        auto* Star=Stars.value();if(!Star)continue;
        for(auto Planets=Star->Satellites();++Planets;)
        {
            auto* Planet=Planets.value();if(!Planet)continue;
            BoundGroup(Planet);Planet->clearance_radius=Planet->Radius();
            for(auto Moons=Planet->Satellites();++Moons;)if(Moons.value())Planet->clearance_radius=FMath::Max(Planet->clearance_radius,Moons->Apoapsis()+Moons->Radius());
        }
        BoundGroup(Star);radius=FMath::Max(radius,Star->Apoapsis()+Star->Radius());
        for(auto Planets=Star->Satellites();++Planets;)if(Planets.value())
        {Planets->Update();radius=FMath::Max(radius,Planets->Apoapsis()+Planets->ClearanceRadius());for(auto Moons=Planets->Satellites();++Moons;)if(Moons.value())Moons->Update();}
    }

	ListIter<OrbitalRegion> RegionIter = all_regions;
	while (++RegionIter)
	{
		OrbitalRegion* Region = RegionIter.value();
		if (!Region)
		{
			continue;
		}

		radius = FMath::Max(radius, Region->Orbit());
	}
}

void StarSystem::AddRootRegion(OrbitalRegion* Region)
{
	if (!Region)
	{
		return;
	}

	regions.append(Region);
	all_regions.append(Region);

	if (Region->Orbit() > radius)
	{
		radius = Region->Orbit();
	}
}

void StarSystem::ResetHydratedContents()
{
	active_region = nullptr;
	radius = 0.0;

	if (instantiated)
	{
		Deactivate();
		Destroy();
	}

	// SAFE DELETE: bodies
	{
		ListIter<OrbitalBody> BodyIter = bodies;
		while (++BodyIter)
		{
			OrbitalBody* Body = BodyIter.value();
			if (Body)
			{
				delete Body;
			}
		}
		bodies.clear();
	}

	// SAFE DELETE: root regions only
	{
		ListIter<OrbitalRegion> RegionIter = regions;
		while (++RegionIter)
		{
			OrbitalRegion* Region = RegionIter.value();
			if (Region)
			{
				delete Region;
			}
		}
		regions.clear();
	}

	// NON-OWNING CACHE
	all_regions.clear();
}

int32 StarSystem::ToLegacyStarClass(ESPECTRAL_CLASS InClass) const
{
	switch (InClass)
	{
	case ESPECTRAL_CLASS::O:            return Star::O;
	case ESPECTRAL_CLASS::B:            return Star::B;
	case ESPECTRAL_CLASS::A:            return Star::A;
	case ESPECTRAL_CLASS::F:            return Star::F;
	case ESPECTRAL_CLASS::G:            return Star::G;
	case ESPECTRAL_CLASS::K:            return Star::K;
	case ESPECTRAL_CLASS::M:            return Star::M;
	case ESPECTRAL_CLASS::RED_GIANT:    return Star::RED_GIANT;
	case ESPECTRAL_CLASS::WHITE_DWARF:  return Star::WHITE_DWARF;
	case ESPECTRAL_CLASS::BLACK_HOLE:   return Star::BLACK_HOLE;
	default:                            return Star::G;
	}
}

void StarSystem::HydrateFromEnvironment(
	const FS_Galaxy& GalaxyRow,
	const FStarSystem* OptionalSystemMeta)
{
	ResetHydratedContents();

	SetName(TCHAR_TO_ANSI(*GalaxyRow.Name));
	SetLocation(GalaxyRow.Location);
	SetAffiliation(GalaxyRow.Iff);
	SetSequence(ToLegacyStarClass(GalaxyRow.Class));

	for (const FStarSystem& StarRow : GalaxyRow.Stellar)
	{
		OrbitalBody* StarBody = HydrateStar(StarRow);
		if (!StarBody)
		{
			continue;
		}

		for (const FRegion& RegionRow : StarRow.Region)
		{
			HydrateRegion(StarBody, RegionRow);
		}

		for (const FPlanet& PlanetRow : StarRow.Planet)
		{
			OrbitalBody* PlanetBody = HydratePlanet(StarBody, PlanetRow);
			if (!PlanetBody)
			{
				continue;
			}

			for (const FRegion& RegionRow : PlanetRow.Region)
			{
				HydrateRegion(PlanetBody, RegionRow);
			}

			for (const FMoon& MoonRow : PlanetRow.Moon)
			{
				OrbitalBody* MoonBody = HydrateMoon(PlanetBody, MoonRow);
				if (!MoonBody)
				{
					continue;
				}

				for (const FRegion& RegionRow : MoonRow.Region)
				{
					HydrateRegion(MoonBody, RegionRow);
				}
			}
		}
	}

	RecalculateRadius();
}

OrbitalBody* StarSystem::HydrateStar(const FStarSystem& Row)
{
	OrbitalBody* StarBody = new OrbitalBody(
		this,
		TCHAR_TO_ANSI(*Row.Name),
		Orbital::STAR,
		Row.Mass,
		Row.Radius,
		Row.Orbit,
		GetCenter());

	StarBody->map_name = TCHAR_TO_ANSI(*Row.Map);
	StarBody->tex_name = TCHAR_TO_ANSI(*Row.Image);
	StarBody->light = Row.Light;
	StarBody->tscale = Row.Tscale;
	StarBody->retro = Row.Retro;
	StarBody->rotation = Row.Rot * 3600.0;
	StarBody->color = Row.Color;
	StarBody->back = Row.Back;
	StarBody->subtype = ToLegacyStarClass(Row.Class);

	AddBody(StarBody);
	return StarBody;
}

OrbitalBody* StarSystem::HydratePlanet(OrbitalBody* ParentStar, const FPlanet& Row)
{
	if (!ParentStar)
	{
		return nullptr;
	}

	OrbitalBody* PlanetBody = new OrbitalBody(
		this,
		TCHAR_TO_ANSI(*Row.Name),
		Orbital::PLANET,
		Row.Mass,
		Row.Radius,
		Row.Orbit,
		ParentStar);

	PlanetBody->map_name = TCHAR_TO_ANSI(*Row.Icon);
	PlanetBody->tex_name = TCHAR_TO_ANSI(*Row.Texture);
	PlanetBody->tex_gloss = TCHAR_TO_ANSI(*Row.Gloss);
	PlanetBody->tex_ring = TCHAR_TO_ANSI(*Row.Ring);
	PlanetBody->ring_min = Row.Minrad;
	PlanetBody->ring_max = Row.Maxrad;
	PlanetBody->tscale = Row.Tscale;
	PlanetBody->tilt = Row.Tilt;
	PlanetBody->SetOrbitShape(Row.Eccentricity, Row.Inclination);
	PlanetBody->retro = Row.Retro;
	PlanetBody->rotation = Row.Rot * 3600.0;
	PlanetBody->atmosphere = Row.Atmos;

	ParentStar->satellites.append(PlanetBody);
	return PlanetBody;
}

OrbitalBody* StarSystem::HydrateMoon(OrbitalBody* ParentPlanet, const FMoon& Row)
{
	if (!ParentPlanet)
	{
		return nullptr;
	}

	OrbitalBody* MoonBody = new OrbitalBody(
		this,
		TCHAR_TO_ANSI(*Row.Name),
		Orbital::MOON,
		Row.Mass,
		Row.Radius,
		Row.Orbit,
		ParentPlanet);

	MoonBody->map_name = TCHAR_TO_ANSI(*Row.Icon);
	MoonBody->tex_name = TCHAR_TO_ANSI(*Row.Texture);
	MoonBody->tscale = Row.Tscale;
	MoonBody->tilt = Row.Tilt;
	MoonBody->SetOrbitShape(Row.Eccentricity, Row.Inclination);
	MoonBody->retro = Row.Retro;
	MoonBody->rotation = Row.Rot * 3600.0;
	MoonBody->atmosphere = Row.Atmos;

	ParentPlanet->satellites.append(MoonBody);
	return MoonBody;
}

OrbitalRegion* StarSystem::HydrateRegion(Orbital* Parent, const FRegion& Row)
{
	if (!Parent)
	{
		return nullptr;
	}

	OrbitalRegion* Region = new OrbitalRegion(
		this,
		TCHAR_TO_ANSI(*Row.Name),
		0,
		Row.Size,
		Row.Orbit,
		Parent);

	Region->grid = Row.Grid;
	Region->inclination = Row.Inclination;
	Region->asteroids = Row.Asteroids;

	for (const FString& LinkName : Row.Link)
	{
		Region->links.append(new Text(TCHAR_TO_ANSI(*LinkName)));
	}

	Parent->regions.append(Region);
	all_regions.append(Region);

	return Region;
}