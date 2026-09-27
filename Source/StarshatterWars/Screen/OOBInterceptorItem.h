// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBWing definition
#include "OOBInterceptorItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBInterceptorItem : public UObject
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, Category = "Data")
    FS_OOBIntercept Data;		
};
