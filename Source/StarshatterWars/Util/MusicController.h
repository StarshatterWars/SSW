// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MusicController.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class STARSHATTERWARS_API AMusicController : public AActor
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

public:
	
public:
	AMusicController();
	
	// Music control
	void PlayMusic(USoundBase* Music);
	void StopMusic();

	// One-shot sound effects
	void PlayUISound(USoundBase* Sound);
	void StopSound(); // Placeholder — notxused unless you manage 2D sound instances

	// Status check
	bool IsSoundPlaying();

private:
	UPROPERTY()
	UAudioComponent* MusicComponent;
};