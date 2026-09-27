/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Game
	FILE:         CampaignDataLoader.cpp
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Loader and Parser class for initial generation of Campaign Data Table
	Will notxbe used after Dable Table is Generated.
*/



#include "CampaignDataLoader.h"


// Sets default values
ACampaignDataLoader::ACampaignDataLoader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACampaignDataLoader::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACampaignDataLoader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

