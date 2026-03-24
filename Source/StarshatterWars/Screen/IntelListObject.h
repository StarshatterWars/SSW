// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IntelListObject.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UIntelListObject : public UObject
{
	GENERATED_BODY()

public:	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsTitle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsLocation;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsDate;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsSource;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsAudio;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsImage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString NewsInfoText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	bool NewsVisited;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	int32 NewsId;
	
};
