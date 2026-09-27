// +----------------------------------------------------------------------+
// | UIntelListObject                                                     |
// +----------------------------------------------------------------------+
// | PURPOSE:                                                             |
// |   Data container for intel list entries.                             |
// +----------------------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IntelListObject.generated.h"

class CombatEvent;

UCLASS()
class STARSHATTERWARS_API UIntelListObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY() FString NewsTitle;
	UPROPERTY() FString NewsLocation;
	UPROPERTY() FString NewsSource;
	UPROPERTY() FString NewsDate;
	UPROPERTY() FString NewsInfoText;
	UPROPERTY() FString NewsImage;
	UPROPERTY() FString NewsAudio;

	UPROPERTY() bool NewsVisited = false;

	// Link back to source event
	CombatEvent* EventPtr = nullptr;
};