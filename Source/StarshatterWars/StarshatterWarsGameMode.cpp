// Copyright Epic Games, Inc. All Rights Reserved.

#include "StarshatterWarsGameMode.h"
#include "StarshatterWarsCharacter.h"
#include "UObject/ConstructorHelpers.h"

AStarshatterWarsGameMode::AStarshatterWarsGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
