#pragma once
#include "CoreMinimal.h"

/**
 * Minimal bridge so Campaign remains plain C++ and still interacts with UE systems.
 * Subsystem (UCampaignSubsystem) should implement this interface.
 */
class ICampaignHost
{
public:
	virtual ~ICampaignHost() = default;

	// Clock
	virtual int64 GetUniverseSeconds() const = 0;

	// Player context
	virtual int32 GetPlayerRank() const = 0;

	// Cutscene state (stub until you wire it)
	virtual bool IsInCutscene() const = 0;

	// Logging/intel
	virtual void EmitLog(const FString& Line) = 0;
	virtual void EmitIntel(const FString& Title, const FString& Body) = 0;
};
