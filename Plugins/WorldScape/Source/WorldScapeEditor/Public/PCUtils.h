// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "EditorViewportClient.h"
#include "Math/Vector.h"

#include "PCUtils.generated.h"


UCLASS()
class WORLDSCAPEEDITOR_API UWorldScapeEditorUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	static FVector GetViewPortCameraLookAt(float Distance);
	static FVector GetViewPortCameraPosition();
	static bool IsInViewPort();
};


