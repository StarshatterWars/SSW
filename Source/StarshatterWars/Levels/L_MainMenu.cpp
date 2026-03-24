// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "L_MainMenu.h"

void AL_MainMenu::BeginPlay()
{
	Super::BeginPlay();
	ShowMainMenu();
}

void AL_MainMenu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AL_MainMenu::ShowMainMenu()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->InitializeAudioSystem();
	SSWInstance->PlayMenuMusic();
	SSWInstance->ShowMainMenuScreen();
}



