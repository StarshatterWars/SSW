// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBDestroyer definition
#include "OOBDestroyerItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBDestroyerItem : public UObject
{
	GENERATED_BODY()
	
public:
	FS_OOBDestroyer Data;
};
