// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBForce definition
#include "OOBForceItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBForceItem : public UObject
{
	GENERATED_BODY()
	
public:
	FS_OOBForce Data;
};
