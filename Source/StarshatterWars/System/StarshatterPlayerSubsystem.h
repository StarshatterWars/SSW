#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PlayerSaveGame.h"
#include "GameStructs.h"

#include "StarshatterPlayerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerSaveLoaded, bool, bSuccess);

UCLASS()
class STARSHATTERWARS_API UStarshatterPlayerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool LoadFromBoot();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    void SetSaveSlot(const FString& InSlotName, int32 InUserIndex);

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    FString GetSlotName() const { return SlotName; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    int32 GetUserIndex() const { return UserIndex; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool HasLoaded() const { return bLoaded; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool HadExistingSaveOnLoad() const { return bHadExistingSave; }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    void MarkDirty() { bDirty = true; }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    void ResetToDefaults();

    // AUTHORITATIVE ACCESS
    const FS_PlayerGameInfo& GetPlayerInfo() const;
    FS_PlayerGameInfo& GetMutablePlayerInfo();

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    FS_PlayerGameInfo GetPlayerInfoCopy() const { return PlayerInfo; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool DoesSaveExistNow() const;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool LoadPlayer();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool SavePlayer(bool bForce = false);

    UPROPERTY(BlueprintAssignable, Category = "Starshatter|PlayerSave")
    FOnPlayerSaveLoaded OnPlayerSaveLoaded;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    void DebugDump(const FString& Tag = TEXT("")) const;

private:
    bool SaveGameInternal(const FString& InSlotName, int32 InUserIndex,
        const FS_PlayerGameInfo& InPlayerData, int32 InSaveVersion);

    bool LoadGameInternal(const FString& InSlotName, int32 InUserIndex,
        FS_PlayerGameInfo& OutPlayerData, int32& OutSaveVersion);

    bool MigratePlayerSave(int32 FromVersion, int32 ToVersion, FS_PlayerGameInfo& InOutPlayerInfo);

private:
    FString SlotName = TEXT("PlayerSave");
    int32   UserIndex = 0;

    FS_PlayerGameInfo PlayerInfo;

    bool bLoaded = false;
    bool bDirty = false;
    bool bHadExistingSave = false;

    static constexpr int32 CURRENT_SAVE_VERSION = 1;
};
