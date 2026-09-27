// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBFleet definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBBatteryItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBBatteryItem : public UObject
{
	GENERATED_BODY()
	
public:
	FS_OOBBattery Data;
};
