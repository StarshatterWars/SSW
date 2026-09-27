/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SystemDesign.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic ship System Design class
*/

#include "SystemDesign.h"
#include "CoreMinimal.h" // UE_LOG, ANSI_TO_TCHAR
#include "SimComponent.h"

#include "Game.h"
// Bitmap removed: render assets are Unreal UTexture2D*
#include "DataLoader.h"
#include "ParseUtil.h"
#include "List.h"

// +--------------------------------------------------------------------+

List<SystemDesign> SystemDesign::catalog;

#define GET_DEF_TEXT(p,d,x) if(p->name()->value()==(#x))GetDefText(d->x,p,filename)
#define GET_DEF_NUM(p,d,x)  if(p->name()->value()==(#x))GetDefNumber(d->x,p,filename)

// +--------------------------------------------------------------------+

SystemDesign::SystemDesign()
{
}

SystemDesign::~SystemDesign()
{
	components.destroy();
}

// +--------------------------------------------------------------------+

void
SystemDesign::Close()
{
	catalog.destroy();
}

// +--------------------------------------------------------------------+

SystemDesign*
SystemDesign::Find(const char* name)
{
	SystemDesign test;
	test.name = name;
	return catalog.find(&test);
}
