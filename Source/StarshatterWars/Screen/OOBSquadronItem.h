// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_OOBForce definition
#include "OOBSquadronItem.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBSquadronItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	ECOMBATGROUP_TYPE Type;

	UPROPERTY()
	FS_OOBAttack AttackData;

	UPROPERTY()
	FS_OOBIntercept InterceptData;

	UPROPERTY()
	FS_OOBLanding LandingData;

	UPROPERTY()
	FS_OOBFighter FighterData;

	UFUNCTION()
	FString GetDisplayName() const
	{
		switch (Type)
		{
		case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
			return AttackData.Name;
		case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
			return InterceptData.Name;
		case ECOMBATGROUP_TYPE::LCA_SQUADRON:
			return LandingData.Name;
		case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
			return FighterData.Name;
		default:
			return TEXT("Unknown Squadron");
		}
	}
};
	
	

