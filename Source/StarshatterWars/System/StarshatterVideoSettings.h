#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameStructs.h"
#include "StarshatterVideoSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class STARSHATTERWARS_API UStarshatterVideoSettings : public UObject
{
    GENERATED_BODY()

public:
    static UStarshatterVideoSettings* Get();

    // -----------------------------
    // Config lifecycle
    // -----------------------------
    void Load();
    void Save();
    void Sanitize();

    // -----------------------------
    // Accessors
    // -----------------------------
    const FStarshatterVideoConfig& GetConfig() const { return Config; }
    void SetConfig(const FStarshatterVideoConfig& InConfig);

    // -----------------------------
    // Runtime apply hook
    // -----------------------------
    void ApplyToRuntimeVideo();

private:
    UPROPERTY(Config)
    FStarshatterVideoConfig Config;
};
