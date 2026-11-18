#pragma once
#include "GameFramework/SaveGame.h"
#include "GameStructs_guilds.h"
#include "GuildDirectorySaveGame.generated.h"

USTRUCT()
struct FGuildDirectoryPilot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Identity;
    UPROPERTY(SaveGame) FString DisplayName;
    UPROPERTY(SaveGame) FName Empire;
    UPROPERTY(SaveGame) int32 Rank=0;
    UPROPERTY(SaveGame) TSet<FName> Skills;
    UPROPERTY(SaveGame) int64 LastSeenUtc=0;
};
// Shared by local profiles only. Player organizations never own personal money or starter rewards.
UCLASS()
class STARSHATTERWARS_API UGuildDirectorySaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FGuildState State;
    UPROPERTY(SaveGame) TMap<FName,FGuildDirectoryPilot> Pilots;
};
