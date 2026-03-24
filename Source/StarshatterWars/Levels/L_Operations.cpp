// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "L_Operations.h"

void AL_Operations::BeginPlay()
{
	Super::BeginPlay();
	ShowOperationsScreen();
}

void AL_Operations::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AL_Operations::ShowOperationsScreen()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	//SSWInstance->ShowOperationsScreen();
}
