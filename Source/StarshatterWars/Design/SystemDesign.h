/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SystemDesign.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic ship System Design class
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"

// +--------------------------------------------------------------------+

class ComponentDesign;

// +--------------------------------------------------------------------+

class SystemDesign
{
public:
	static const char* TYPENAME() { return "SystemDesign"; }

	SystemDesign();
	~SystemDesign();

	int operator == (const SystemDesign& rhs) const { return name == rhs.name; }

	static void          Initialize(const char* filename);
	static void          Close();
	static SystemDesign* Find(const char* name);

	// Unique ID:
	Text name;

	// Sub-components:
	List<ComponentDesign> components;

	static List<SystemDesign> catalog;
};
