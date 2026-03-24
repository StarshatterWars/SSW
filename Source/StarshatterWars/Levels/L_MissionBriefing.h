// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "SSWGameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "L_MissionBriefing.generated.h"

class UMissionBriefing;
/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API AL_MissionBriefing : public ALevelScriptActor
{
	
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void ShowMissionBriefingScreen();
};
