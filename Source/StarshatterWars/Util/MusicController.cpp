// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "MusicController.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AMusicController::AMusicController()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create and configure the music audio component
	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	SetRootComponent(MusicComponent);
	MusicComponent->bAutoActivate = false;
	MusicComponent->bIsUISound = true;
}

void AMusicController::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> Found;

	UGameplayStatics::GetAllActorsOfClass(this, AMusicController::StaticClass(), Found);
	if (Found.Num() > 1)
	{
		Destroy(); // keep only one
	}
}

void AMusicController::PlayMusic(USoundBase* Music)
{
	if (!Music)
		return;

	// Optional: prevent replaying same track
	if (MusicComponent->IsPlaying() && MusicComponent->Sound == Music)
		return;

	MusicComponent->SetSound(Music);
	MusicComponent->Play();
}

void AMusicController::PlayUISound(USoundBase* Sound)
{
	if (!Sound)
		return;

	UGameplayStatics::PlaySound2D(this, Sound);
}

void AMusicController::StopMusic()
{
	MusicComponent->Stop();
}

void AMusicController::StopSound()
{
	// No audio component for 2D SFX, so nothing to stop
	// Add additional logic here if needed
}

bool AMusicController::IsSoundPlaying()
{
	if(MusicComponent)
		return MusicComponent->IsPlaying();
	else 
		return false;
}