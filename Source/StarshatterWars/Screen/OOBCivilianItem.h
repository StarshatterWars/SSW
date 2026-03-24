// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBCivilian definition
#include "OOBCivilianItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBCivilianItem : public UObject
{
	GENERATED_BODY()

public:
	FS_OOBCivilian Data;	
};
