// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h"
#include "CombatGroupObject.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UCombatGroupObject : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintReadOnly)
	FS_CombatGroup GroupData;

	UPROPERTY(BlueprintReadOnly)
	TArray<UCombatGroupObject*> Children;

	UPROPERTY(BlueprintReadOnly)
	int32 IndentLevel = 0;

	void Init(const FS_CombatGroup& InData, int32 InIndentLevel);
};
	
	
	

