// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "CombatGroupObject.h"

void UCombatGroupObject::Init(const FS_CombatGroup& InData, int32 InIndentLevel)
{
	GroupData = InData;
	IndentLevel = InIndentLevel;
}
