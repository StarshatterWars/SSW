#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Offline maintenance only. Does not initialize or reset a live pilot.
namespace PilotSaveRecovery
{
    TSharedRef<FJsonObject> Execute(const FJsonObject& Request, const FString& SaveDirectory);
    bool IsRequested();
    void RunRequested();
}
