// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "L_Campaign.h"

void AL_Campaign::BeginPlay()
{
	Super::BeginPlay();
	ShowCampaignScreen();
}

void AL_Campaign::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AL_Campaign::ShowCampaignScreen()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	//SSWInstance->ShowCampaignScreen();
}
