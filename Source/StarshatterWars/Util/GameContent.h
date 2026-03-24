/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    System
	FILE:         GameContent.h
	AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"

#include "Types.h"
#include "Dictionary.h"
#include "Text.h"
//#include "Locale_ss.h"

/**
 * 
 */
class STARSHATTERWARS_API GameContent
{
public:
	static const char* TYPENAME() { return "ContentBundle"; }

	const			  Text& GetName()          const { return ContentName; }
	Text              GetText(const char* key) const;

	bool              IsLoaded()                const { return !ContentValues.IsEmpty(); }

	Text              ContentName;
	Dictionary<Text>  ContentValues;
};
