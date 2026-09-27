/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    System
	FILE:         GameContent.cpp
	AUTHOR:       Carlos Bott
*/


#include "GameContent.h"

Text
GameContent::GetText(const char* key) const
{
	return ContentValues.Find(key, Text(key));
}

