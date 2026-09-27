/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Terrain.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
*/

#include "Terrain.h"
#include "TerrainApron.h"
#include "TerrainClouds.h"
#include "TerrainLayer.h"
#include "TerrainPatch.h"
#include "TerrainRegion.h"
#include "Water.h"

#include "CameraView.h"
#include "SimProjector.h"
#include "SimScene.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"

// Unreal:
#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "Math/RandomStream.h"
#include "HAL/UnrealMemory.h"

// +--------------------------------------------------------------------+

int Terrain::detail_level = 3; // default = MEDIUM DETAIL
;

// +--------------------------------------------------------------------+

Terrain::Terrain(TerrainRegion* Trgn)
	: region(Trgn)
	, patches(nullptr)
	, water_patches(nullptr)
	, water(nullptr)
	, aprons(nullptr)
	, clouds(nullptr)
	, nclouds(0)
	, terrain_texture(nullptr)
	, apron_texture(nullptr)
	, water_texture(nullptr)
	, terrain_normals(nullptr)
	, scale(1.0)
	, mtnscale(1.0)
	, subdivisions(0)
	, patch_size(0)
	, detail_frame(0)
	, fog_fade(0.0)
{
	datapath = "Galaxy/";

	for (int i = 0; i < 6; i++)
		env_texture[i] = nullptr;

	for (int i = 0; i < 256; i++)
		tiles[i] = nullptr;

	for (int i = 0; i < 2; i++) {
		cloud_texture[i] = nullptr;
		shade_texture[i] = nullptr;
		noise_texture[i] = nullptr;
	}

	if (region) {
		scale = region->LateralScale();
		mtnscale = region->MountainScale();

		ListIter<TerrainLayer> Iter = region->GetLayers();
		while (++Iter) {
			TerrainLayer* Orig = Iter.value();
			TerrainLayer* Copy = new TerrainLayer;
			*Copy = *Orig;
			layers.append(Copy);
		}

		layers.sort();
	}
	else {
		scale = 1.0;
		mtnscale = 1.0;
	}
}

// +--------------------------------------------------------------------+

Terrain::~Terrain()
{
	if (patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				GRAPHIC_DESTROY(patches[i * subdivisions + j]);
			}
		}
	}

	if (water_patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				GRAPHIC_DESTROY(water_patches[i * subdivisions + j]);
			}
		}
	}

	if (aprons) {
		for (int i = 0; i < 8; i++)
			GRAPHIC_DESTROY(aprons[i]);
	}

	if (clouds) {
		for (int i = 0; i < nclouds; i++)
			GRAPHIC_DESTROY(clouds[i]);
	}

	if (water) {
		for (int i = 0; i < 6; i++)
			delete water[i];
	}

	delete[] aprons;
	delete[] clouds;
	delete[] patches;
	delete[] water_patches;
	delete[] water;
	delete[] terrain_normals;

	terrain_patch.ClearImage();
	terrain_apron.ClearImage();

	layers.destroy();
}

// +--------------------------------------------------------------------+

void
Terrain::BuildTerrain()
{
	DataLoader* Loader = DataLoader::GetLoader();

	// Text -> const char* for legacy loader API:
	Loader->SetDataPath(datapath.data());

	Loader->LoadGameBitmap(region->PatchName(), terrain_patch);
	Loader->LoadGameBitmap(region->ApronName(), terrain_apron);

	Loader->LoadTexture(region->PatchTexture(), terrain_texture);
	Loader->LoadTexture(region->ApronTexture(), apron_texture);

	if (region->WaterTexture().length()) {
		Loader->LoadTexture(region->WaterTexture(), water_texture);

		if (region->EnvironmentTexture(0).length() > 0) {
			Loader->LoadTexture(region->EnvironmentTexture(0), env_texture[0]);
			Loader->LoadTexture(region->EnvironmentTexture(1), env_texture[1]);
			Loader->LoadTexture(region->EnvironmentTexture(2), env_texture[2]);
			Loader->LoadTexture(region->EnvironmentTexture(3), env_texture[3]);
			Loader->LoadTexture(region->EnvironmentTexture(4), env_texture[4]);
			Loader->LoadTexture(region->EnvironmentTexture(5), env_texture[5]);
		}
	}

	Loader->LoadTexture(region->CloudsHigh(), cloud_texture[0], Bitmap::BMP_TRANSLUCENT);
	Loader->LoadTexture(region->CloudsLow(), cloud_texture[1], Bitmap::BMP_TRANSLUCENT);
	Loader->LoadTexture(region->ShadesLow(), shade_texture[1], Bitmap::BMP_TRANSLUCENT);

	if (region->DetailTexture0().length())
		Loader->LoadTexture(region->DetailTexture0(), noise_texture[0], Bitmap::BMP_TRANSLUCENT, false, true);

	if (region->DetailTexture1().length())
		Loader->LoadTexture(region->DetailTexture1(), noise_texture[1], Bitmap::BMP_TRANSLUCENT, false, true);

	subdivisions = terrain_patch.Width() / PATCH_SIZE;
	patch_size = terrain_patch.Width() / subdivisions;

	BuildNormals();

	double Dx = scale * patch_size;
	double Dz = scale * patch_size;
	double Offset = -subdivisions / 2.0;

	if (water_texture) {
		water = new Water * [6];
		for (int i = 0; i < 6; i++) {
			water[i] = new Water();
			const int n = (1 << i) + 1;
			water[i]->Init(n, (float)(scale * patch_size), 90.0f);
		}
	}

	// load tile textures:
	for (int i = 0; i < layers.size(); i++) {
		TerrainLayer* Layer = layers.at(i);

		if (i < layers.size() - 1)
			Layer->max_height = layers.at(i + 1)->min_height;
		else
			Layer->max_height = 1.0e6;

		if (Layer->tile_name.length())
			Loader->LoadTexture(Layer->tile_name, Layer->tile_texture);

		if (Layer->detail_name.length())
			Loader->LoadTexture(Layer->detail_name, Layer->detail_texture);
	}

	patches = new TerrainPatch * [subdivisions * subdivisions];

	if (water_texture)
		water_patches = new TerrainPatch * [subdivisions * subdivisions];

	for (int i = 0; i < subdivisions; i++) {
		for (int j = 0; j < subdivisions; j++) {
			const int j1 = subdivisions - j;

			Rect  RectPatch(j * patch_size, i * patch_size, patch_size, patch_size);
			Point P1((j1 + Offset) * Dx, 0, (i + Offset) * Dz);
			Point P2((j1 + Offset + 1) * Dx, mtnscale, (i + Offset + 1) * Dz);

			const int Index = i * subdivisions + j;
			patches[Index] = new TerrainPatch(this, &terrain_patch, RectPatch, P1, P2);

			if (water_texture && patches[Index]->MinHeight() < 3)
				water_patches[Index] = new TerrainPatch(this, RectPatch, P1, P2, 30);
			else if (water_patches != nullptr)
				water_patches[Index] = nullptr;
		}
	}

	int a = 0;

	Dx = scale * terrain_patch.Width();
	Dz = scale * terrain_patch.Height();
	Offset = -3.0 / 2.0;

	const double XOffset = Offset + 1.0 / 16.0;

	aprons = new TerrainApron * [8];

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			const int j1 = 2 - j;
			if (i != 1 || j1 != 1) {
				Rect  RectApron(j * subdivisions, i * subdivisions, subdivisions, subdivisions);
				Point P1((j1 + XOffset) * Dx, 0, (i + Offset) * Dz);
				Point P2((j1 + XOffset + 1) * Dx, mtnscale, (i + Offset + 1) * Dz);

				aprons[a++] = new TerrainApron(this, &terrain_apron, RectApron, P1, P2);
			}
		}
	}

	Weather::STATE State = region->GetWeather().State();

	if (State == Weather::HIGH_CLOUDS || State == Weather::MODERATE_CLOUDS) {
		double Altitude = region->CloudAltHigh();
		nclouds = 9;

		if (State == Weather::MODERATE_CLOUDS)
			nclouds *= 2;

		clouds = new TerrainClouds * [nclouds];

		// Deterministic per-terrain-instance random stream (UE internal):
		const int32 SeedValue = (int32)((uintptr_t)this);
		FRandomStream RandomStream(SeedValue);

		a = 0;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				clouds[a] = new TerrainClouds(this, 0);

				const double XLoc = (j - 1) * 75000.0 + (RandomStream.FRand() - 0.5) * 50000.0;
				const double YLoc = (i - 1) * 75000.0 + (RandomStream.FRand() - 0.5) * 50000.0;

				clouds[a]->MoveTo(Point(XLoc, Altitude, YLoc));
				a++;
			}
		}

		if (State == Weather::MODERATE_CLOUDS) {
			Altitude = region->CloudAltLow();

			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					clouds[a] = new TerrainClouds(this, 1);

					const double XLoc = (j - 1) * 75000.0 + (RandomStream.FRand() - 0.5) * 50000.0;
					const double YLoc = (i - 1) * 75000.0 + (RandomStream.FRand() - 0.5) * 50000.0;

					clouds[a]->MoveTo(Point(XLoc, Altitude, YLoc));
					a++;
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
Terrain::BuildNormals()
{
	if (terrain_normals) {
		delete[] terrain_normals;
		terrain_normals = nullptr;
	}

	const int w = terrain_patch.Width();
	const int h = terrain_patch.Height();

	// UE COLOR TYPE:
	// This requires Bitmap::HiPixels() to return FColor* (or be typedef’d accordingly).
	FColor* Pix = terrain_patch.HiPixels();

	BYTE* Alt = new BYTE[h * w];

	const int    NVerts = w * h;
	const double NormScale = region->MountainScale() / (region->LateralScale() * 2.0);

	terrain_normals = new Vec3B[NVerts];
	FMemory::Memzero(terrain_normals, sizeof(Vec3B) * NVerts);

	for (int i = 0; i < w; i++) {
		Alt[i] = 0;
		Alt[(h - 1) * w + i] = 0;
		terrain_normals[i] = Vec3B(128, 128, 255);
		terrain_normals[(h - 1) * w + i] = Vec3B(128, 128, 255);
	}

	for (int i = 0; i < h; i++) {
		Alt[i * w] = 0;
		Alt[i * w + (w - 1)] = 0;
		terrain_normals[i * w] = Vec3B(128, 128, 255);
		terrain_normals[i * w + (w - 1)] = Vec3B(128, 128, 255);
	}

	for (int y = 1; y < h - 1; y++) {
		for (int x = 1; x < w - 1; x++) {
			Alt[y * w + x] = Pix[y * w + x].R; // UE byte channel
		}
	}

	for (int y = 1; y < h - 1; y++) {
		for (int x = 1; x < w - 1; x++) {
			const double Dx =
				(Alt[y * w + (x - 1)] - Alt[y * w + (x + 1)]) * NormScale +
				(Alt[(y - 1) * w + (x - 1)] - Alt[(y - 1) * w + (x + 1)]) * NormScale * 0.5 +
				(Alt[(y + 1) * w + (x - 1)] - Alt[(y + 1) * w + (x + 1)]) * NormScale * 0.5;

			const double Dy =
				(Alt[(y - 1) * w + x] - Alt[(y + 1) * w + x]) * NormScale +
				(Alt[(y - 1) * w + (x - 1)] - Alt[(y + 1) * w + (x - 1)]) * NormScale * 0.5 +
				(Alt[(y - 1) * w + (x + 1)] - Alt[(y + 1) * w + (x + 1)]) * NormScale * 0.5;

			Point Norm(Dx, Dy, 1);
			Norm.Normalize();

			Vec3B* TN = &terrain_normals[y * w + x];

			TN->x = (BYTE)(Norm.X * 127 + 128);
			TN->y = (BYTE)(Norm.Y * 127 + 128);
			TN->z = (BYTE)(Norm.Z * 127 + 128);
		}
	}

	delete[] Alt;
}

// +--------------------------------------------------------------------+

void
Terrain::Activate(SimScene& scene)
{
	StarSystem* System = region->System();
	if (System)
		datapath = System->GetDataPath();

	region->GetWeather().Update();

	if (!patches)
		BuildTerrain();

	if (patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				if (patches[i * subdivisions + j])
					scene.AddGraphic(patches[i * subdivisions + j]);
			}
		}
	}

	if (water_patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				if (water_patches[i * subdivisions + j])
					scene.AddGraphic(water_patches[i * subdivisions + j]);
			}
		}
	}

	if (aprons) {
		for (int i = 0; i < 8; i++) {
			if (aprons[i])
				scene.AddGraphic(aprons[i]);
		}
	}

	if (clouds) {
		for (int i = 0; i < nclouds; i++) {
			if (clouds[i])
				scene.AddGraphic(clouds[i]);
		}
	}
}

void
Terrain::Deactivate(SimScene& scene)
{
	if (patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				TerrainPatch* p = patches[i * subdivisions + j];
				if (p) {
					p->DeletePrivateData();
					scene.DelGraphic(p);
				}
			}
		}
	}

	if (water_patches) {
		for (int i = 0; i < subdivisions; i++) {
			for (int j = 0; j < subdivisions; j++) {
				TerrainPatch* p = water_patches[i * subdivisions + j];
				if (p) {
					p->DeletePrivateData();
					scene.DelGraphic(p);
				}
			}
		}
	}

	if (aprons) {
		for (int i = 0; i < 8; i++) {
			if (aprons[i])
				scene.DelGraphic(aprons[i]);
		}
	}

	if (clouds) {
		for (int i = 0; i < nclouds; i++) {
			if (clouds[i])
				scene.DelGraphic(clouds[i]);
		}
	}

	StarSystem* System = region->System();

	// restore sunlight color and brightness on exit:
	if (System) {
		System->RestoreTrueSunColor();
	}
}

// +--------------------------------------------------------------------+

void
Terrain::ExecFrame(double seconds)
{
	if (water) {
		for (int i = 0; i < 6; i++) {
			if (water[i])
				water[i]->CalcWaves(seconds);
		}
	}
}

// +--------------------------------------------------------------------+

void
Terrain::SelectDetail(SimProjector* projector)
{
	if (!patches)
		return;

	if (detail_frame >= Game::Frame())
		return;

	for (int z = 0; z < subdivisions; z++) {
		for (int x = 0; x < subdivisions; x++) {
			TerrainPatch* Patch = patches[z * subdivisions + x];
			int           NDetail = 0;

			const FVector Loc = Patch->Location();
			const float   Radius = Patch->Radius();

			if (Loc.Size() < 2.0f * Radius) {
				NDetail = detail_level;
			}
			else {
				const double Threshold = 4.0;

				for (int Level = 1; Level <= detail_level; Level++) {
					const double FeatureSize = Radius / (1 << Level);

					if (projector->ApparentRadius(Loc, (float)FeatureSize) > Threshold)
						NDetail = Level;
				}
			}

			Patch->SetDetailLevel(NDetail);

			if (water_patches) {
				Patch = water_patches[z * subdivisions + x];
				if (Patch)
					Patch->SetDetailLevel(NDetail);
			}
		}
	}

	const double Hour = region->DayPhase();

	if (Hour < 12)
		fog_fade = (Hour) / 12.0;
	else
		fog_fade = (24 - Hour) / 12.0;

	fog_fade = fog_fade * (1 - region->HazeFade()) + region->HazeFade();

	detail_frame = Game::Frame();
}

// +--------------------------------------------------------------------+

double
Terrain::Height(double x, double y) const
{
	double h = 0.0;

	if (patches) {
		int ix = (int)floor(x / (patch_size * scale));
		int iy = (int)floor(y / (patch_size * scale));

		const double px = x - ix * patch_size * scale;
		const double py = y - iy * patch_size * scale;

		ix = subdivisions / 2 - ix;
		iy = subdivisions / 2 + iy;

		TerrainPatch* Patch = nullptr;

		if (ix >= 0 && ix < subdivisions &&
			iy >= 0 && iy < subdivisions)
			Patch = patches[iy * subdivisions + ix];

		if (Patch)
			h = Patch->Height(px, py);
	}

	if (water_patches && h < 30)
		h = 30;

	return h;
}

// +--------------------------------------------------------------------+

void
Terrain::SetDetailLevel(int32 DetailLevel)
{
	const bool bValidDetailLevel = (DetailLevel >= 1) && (DetailLevel <= 4);
	if (bValidDetailLevel) {
		detail_level = DetailLevel;
	}
}

// +--------------------------------------------------------------------+

bool
Terrain::IsFirstPatch(TerrainPatch* p) const
{
	return (patches && *patches == p);
}
