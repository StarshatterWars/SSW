// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBBattalion definition
#include "OOBBattalion.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBBattalion : public UObject
{
	GENERATED_BODY()
	
public:
	FS_OOBBattalion Data;
};
