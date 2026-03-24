// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBFleet definition
#include "OOBFleetItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBFleetItem : public UObject
{
	GENERATED_BODY()
	
public:
	FS_OOBFleet Data;	
};
