#include "StarshatterPlayerSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterPlayerSubsystem, Log, All);

void UStarshatterPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bLoaded = false;
    bDirty = false;
    bHadExistingSave = false;

    PlayerInfo = FS_PlayerGameInfo();

    UE_LOG(LogStarshatterPlayerSubsystem, Log, TEXT("[PlayerSubsystem] Initialize. Slot=%s UserIndex=%d"),
        *SlotName, UserIndex);
}

void UStarshatterPlayerSubsystem::Deinitialize()
{
    UE_LOG(LogStarshatterPlayerSubsystem, Log, TEXT("[PlayerSubsystem] Deinitialize."));
    Super::Deinitialize();
}

void UStarshatterPlayerSubsystem::SetSaveSlot(const FString& InSlotName, int32 InUserIndex)
{
    SlotName = InSlotName;
    UserIndex = InUserIndex;

    UE_LOG(LogStarshatterPlayerSubsystem, Log, TEXT("[PlayerSubsystem] SetSaveSlot. Slot=%s UserIndex=%d"),
        *SlotName, UserIndex);
}

bool UStarshatterPlayerSubsystem::LoadFromBoot()
{
    const bool bOk = LoadPlayer();
    OnPlayerSaveLoaded.Broadcast(bOk);
    return bOk;
}

bool UStarshatterPlayerSubsystem::DoesSaveExistNow() const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

void UStarshatterPlayerSubsystem::ResetToDefaults()
{
    PlayerInfo = FS_PlayerGameInfo();
    bDirty = true;

    UE_LOG(LogStarshatterPlayerSubsystem, Log, TEXT("[PlayerSubsystem] ResetToDefaults (in-memory only)."));
}

const FS_PlayerGameInfo& UStarshatterPlayerSubsystem::GetPlayerInfo() const
{
    return PlayerInfo;
}

FS_PlayerGameInfo& UStarshatterPlayerSubsystem::GetMutablePlayerInfo()
{
    bDirty = true;
    return PlayerInfo;
}

bool UStarshatterPlayerSubsystem::LoadPlayer()
{
    bHadExistingSave = UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);

    UE_LOG(LogStarshatterPlayerSubsystem, Log, TEXT("[PlayerSubsystem] LoadPlayer. Slot=%s UserIndex=%d Exists=%s"),
        *SlotName, UserIndex, bHadExistingSave ? TEXT("YES") : TEXT("NO"));

    if (bHadExistingSave)
    {
        FS_PlayerGameInfo LoadedPlayerInfo;
        int32 LoadedSaveVersion = 0;

        if (!LoadGameInternal(SlotName, UserIndex, LoadedPlayerInfo, LoadedSaveVersion))
        {
            UE_LOG(LogStarshatterPlayerSubsystem, Error,
                TEXT("[PlayerSubsystem] LoadGameInternal FAILED. Slot=%s UserIndex=%d"),
                *SlotName, UserIndex);

            bLoaded = false;
            return false;
        }

        if (LoadedSaveVersion < CURRENT_SAVE_VERSION)
        {
            UE_LOG(LogStarshatterPlayerSubsystem, Warning,
                TEXT("[PlayerSubsystem] Migrating save from v%d to v%d..."),
                LoadedSaveVersion, CURRENT_SAVE_VERSION);

            if (!MigratePlayerSave(LoadedSaveVersion, CURRENT_SAVE_VERSION, LoadedPlayerInfo))
            {
                UE_LOG(LogStarshatterPlayerSubsystem, Error,
                    TEXT("[PlayerSubsystem] Migration FAILED. From=%d To=%d"),
                    LoadedSaveVersion, CURRENT_SAVE_VERSION);

                bLoaded = false;
                return false;
            }

            PlayerInfo = LoadedPlayerInfo;
            bLoaded = true;
            bDirty = true;

            const bool bRepaired = SavePlayer(true);

            UE_LOG(LogStarshatterPlayerSubsystem, Log,
                TEXT("[PlayerSubsystem] Migration complete. Slot repaired=%s"),
                bRepaired ? TEXT("YES") : TEXT("NO"));
        }
        else
        {
            PlayerInfo = LoadedPlayerInfo;
            bLoaded = true;
            bDirty = false;
        }

        UE_LOG(LogStarshatterPlayerSubsystem, Log,
            TEXT("[PlayerSubsystem] Player loaded: Name='%s' Callsign='%s' Empire=%d Rank=%d (SaveVersion=%d)"),
            *PlayerInfo.Name, *PlayerInfo.Callsign, PlayerInfo.Empire, PlayerInfo.Rank, LoadedSaveVersion);

        return true;
    }

    // No save exists: keep defaults; FirstRun expected to write
    bLoaded = true;
    bDirty = true;

    UE_LOG(LogStarshatterPlayerSubsystem, Log,
        TEXT("[PlayerSubsystem] No player save found. FirstRun required. Slot=%s UserIndex=%d"),
        *SlotName, UserIndex);

    return true;
}

bool UStarshatterPlayerSubsystem::SavePlayer(bool bForce)
{
    if (!bForce && !bDirty)
        return true;

    const bool bOk = SaveGameInternal(SlotName, UserIndex, PlayerInfo, CURRENT_SAVE_VERSION);
    if (bOk)
    {
        bDirty = false;

        UE_LOG(LogStarshatterPlayerSubsystem, Log,
            TEXT("[PlayerSubsystem] SavePlayer OK. Slot=%s UserIndex=%d Version=%d Name='%s' Callsign='%s'"),
            *SlotName, UserIndex, CURRENT_SAVE_VERSION, *PlayerInfo.Name, *PlayerInfo.Callsign);
    }
    else
    {
        UE_LOG(LogStarshatterPlayerSubsystem, Error,
            TEXT("[PlayerSubsystem] SavePlayer FAILED. Slot=%s UserIndex=%d Version=%d"),
            *SlotName, UserIndex, CURRENT_SAVE_VERSION);
    }

    return bOk;
}

bool UStarshatterPlayerSubsystem::SaveGameInternal(
    const FString& InSlotName, int32 InUserIndex,
    const FS_PlayerGameInfo& InPlayerData, int32 InSaveVersion)
{
    UPlayerSaveGame* SaveObject =
        Cast<UPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerSaveGame::StaticClass()));

    if (!SaveObject)
    {
        UE_LOG(LogStarshatterPlayerSubsystem, Error,
            TEXT("[PlayerSubsystem] CreateSaveGameObject FAILED. Slot=%s UserIndex=%d"),
            *InSlotName, InUserIndex);
        return false;
    }

    SaveObject->SaveVersion = InSaveVersion;
    SaveObject->PlayerInfo = InPlayerData;

    const bool bOk = UGameplayStatics::SaveGameToSlot(SaveObject, InSlotName, InUserIndex);
    if (!bOk)
    {
        UE_LOG(LogStarshatterPlayerSubsystem, Error,
            TEXT("[PlayerSubsystem] SaveGameToSlot FAILED. Slot=%s UserIndex=%d"),
            *InSlotName, InUserIndex);
    }

    return bOk;
}

bool UStarshatterPlayerSubsystem::LoadGameInternal(
    const FString& InSlotName, int32 InUserIndex,
    FS_PlayerGameInfo& OutPlayerData, int32& OutSaveVersion)
{
    USaveGame* LoadedSaveGameBase = UGameplayStatics::LoadGameFromSlot(InSlotName, InUserIndex);
    UPlayerSaveGame* LoadedSaveObject = Cast<UPlayerSaveGame>(LoadedSaveGameBase);

    if (!LoadedSaveObject)
    {
        UE_LOG(LogStarshatterPlayerSubsystem, Error,
            TEXT("[PlayerSubsystem] LoadGameFromSlot returned null or wrong class. Slot=%s UserIndex=%d"),
            *InSlotName, InUserIndex);
        return false;
    }

    OutSaveVersion = LoadedSaveObject->SaveVersion;
    OutPlayerData = LoadedSaveObject->PlayerInfo;
    return true;
}

bool UStarshatterPlayerSubsystem::MigratePlayerSave(
    int32 FromVersion, int32 ToVersion, FS_PlayerGameInfo& InOutPlayerInfo)
{
    if (FromVersion <= 0)
        FromVersion = 1;

    while (FromVersion < ToVersion)
    {
        switch (FromVersion)
        {
        case 1:
            FromVersion = 2;
            break;

        default:
            UE_LOG(LogStarshatterPlayerSubsystem, Error,
                TEXT("[PlayerSubsystem] No migration path from v%d to v%d."),
                FromVersion, ToVersion);
            return false;
        }
    }

    return true;
}

void UStarshatterPlayerSubsystem::DebugDump(const FString& Tag) const
{
    UE_LOG(LogStarshatterPlayerSubsystem, Warning,
        TEXT("[PlayerSubsystem][DUMP]%s Slot='%s' User=%d Loaded=%s Dirty=%s HadSaveOnLoad=%s ExistsNow=%s  Name='%s' Callsign='%s' Empire=%d Rank=%d CreateTime=%lld"),
        *Tag,
        *SlotName,
        UserIndex,
        bLoaded ? TEXT("YES") : TEXT("NO"),
        bDirty ? TEXT("YES") : TEXT("NO"),
        bHadExistingSave ? TEXT("YES") : TEXT("NO"),
        UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex) ? TEXT("YES") : TEXT("NO"),
        *PlayerInfo.Name,
        *PlayerInfo.Callsign,
        PlayerInfo.Empire,
        PlayerInfo.Rank,
        (long long)PlayerInfo.CreateTime);
}
