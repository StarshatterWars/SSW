// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h"
#include "SSWGameInstance.h"
#include "CombatGroupListItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UCombatGroupListItem : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite)
	FS_CombatGroup Group;

	UPROPERTY(BlueprintReadWrite)
	int32 IndentLevel;

	UPROPERTY(BlueprintReadWrite)
	bool bIsExpanded = false;

	UPROPERTY(BlueprintReadWrite)
	bool bHasChildren = false;
};