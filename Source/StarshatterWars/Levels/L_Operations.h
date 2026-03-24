// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "SSWGameInstance.h"
#include "L_Operations.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API AL_Operations : public ALevelScriptActor
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void ShowOperationsScreen();
	
};
