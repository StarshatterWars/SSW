/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo / Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Grid.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Tactical Grid
*/

#pragma once

#include "Types.h"
#include "Graphic.h"
#include "Geometry.h"

#include "Math/Vector.h" // FVector
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class Window;
class SimProjector;
class PolyRender;

// +--------------------------------------------------------------------+

class Grid : public Graphic
{
public:
	Grid(int size, int step);
	virtual ~Grid();

	virtual void      Render(Video* video, DWORD flags) override;
	virtual int       CollidesWith(Graphic& o) override;
	void              ShowGrid(bool bShow) { bVisible = bShow; }
protected:
	virtual void      DrawLine(Video* video, FVector& p1, FVector& p2, FColor c);

	int               size;
	int               step;
	int               drawn;
	bool			  bVisible = true;
};

// +--------------------------------------------------------------------+
