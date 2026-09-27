/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainLayer.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	A blended detail texture applied to a terrain patch
	through a specific range of altitudes
*/

#pragma once

#include "Types.h"
#include "Text.h"

// Render assets:
class UTexture2D;

// +--------------------------------------------------------------------+

class Terrain;
class TerrainRegion;

// +--------------------------------------------------------------------+

class TerrainLayer
{
	friend class Terrain;
	friend class TerrainRegion;

public:
	static const char* TYPENAME() { return "TerrainLayer"; }

	TerrainLayer()
		: tile_texture(nullptr)
		, detail_texture(nullptr)
		, min_height(0)
		, max_height(-1)
	{
	}

	~TerrainLayer() = default;

	int operator <  (const TerrainLayer& t) const { return min_height < t.min_height; }
	int operator <= (const TerrainLayer& t) const { return min_height <= t.min_height; }
	int operator == (const TerrainLayer& t) const { return min_height == t.min_height; }

	// accessors:
	const char* GetTileName()      const { return tile_name; }
	const char* GetDetailName()    const { return detail_name; }
	Bitmap* GetTileTexture()   const { return tile_texture; }
	Bitmap* GetDetailTexture() const { return detail_texture; }
	double        GetMinHeight()     const { return min_height; }
	double        GetMaxHeight()     const { return max_height; }

private:
	Text          tile_name;
	Text          detail_name;

	Bitmap* tile_texture;
	Bitmap* detail_texture;

	double        min_height;
	double        max_height;
};
