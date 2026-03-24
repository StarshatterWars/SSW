// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "SSWGameInstance.h"
#include "Sound/SoundBase.h"
#include "L_MainMenu.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API AL_MainMenu : public ALevelScriptActor
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:	
	void ShowMainMenu();
	
	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* MenuMusic;
};
