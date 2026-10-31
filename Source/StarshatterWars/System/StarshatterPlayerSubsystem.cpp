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
    PlayerInfo.CreateTime = FDateTime::UtcNow().ToUnixTimestamp();
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

bool UStarshatterPlayerSubsystem::IsManagedPlayerSlot(const FString& SavedSlot) const
{
    const FString Prefix=SlotName+TEXT("_Profile_");
    if(!SavedSlot.StartsWith(Prefix)) return false;
    FGuid Id;return FGuid::ParseExact(SavedSlot.Mid(Prefix.Len()),EGuidFormats::Digits,Id);
}
UPlayerSaveIndexGame* UStarshatterPlayerSubsystem::ReadPlayerIndex(bool bImportLegacy) const
{
    if(UGameplayStatics::DoesSaveGameExist(GetPlayerIndexSlot(),UserIndex))
    {
        auto* Index=Cast<UPlayerSaveIndexGame>(UGameplayStatics::LoadGameFromSlot(GetPlayerIndexSlot(),UserIndex));
        // A damaged or newer index must never be overwritten with an empty list.
        return Index && Index->Version==1?Index:nullptr;
    }
    auto* Index=NewObject<UPlayerSaveIndexGame>();
    if(bImportLegacy && UGameplayStatics::DoesSaveGameExist(SlotName,UserIndex))
    {
        auto* Legacy=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName,UserIndex));
        if(!Legacy || Legacy->SaveVersion>CURRENT_SAVE_VERSION) return nullptr;
        FPlayerSaveSlotInfo Entry;
        Entry.Slot=SlotName+TEXT("_Profile_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Entry.DisplayName=Legacy->PlayerInfo.Name.IsEmpty()?TEXT("Pilot"):Legacy->PlayerInfo.Name;
        Entry.SavedAt=FDateTime::UtcNow();Index->Players.Add(Entry);Index->ActiveSlot=Entry.Slot;
        // Import as a separate named save. The existing authoritative save is preserved.
        if(!UGameplayStatics::SaveGameToSlot(Legacy,Entry.Slot,UserIndex)) return nullptr;
        if(!WritePlayerIndex(Index)) {UGameplayStatics::DeleteGameInSlot(Entry.Slot,UserIndex);return nullptr;}
    }
    return Index;
}
bool UStarshatterPlayerSubsystem::WritePlayerIndex(UPlayerSaveIndexGame* Index) const
{
    return Index && UGameplayStatics::SaveGameToSlot(Index,GetPlayerIndexSlot(),UserIndex);
}
TArray<FPlayerSaveSlotInfo> UStarshatterPlayerSubsystem::GetSavedPlayers() const
{
    if(auto* Index=ReadPlayerIndex(true)) return Index->Players;
    return {};
}
FString UStarshatterPlayerSubsystem::GetActiveSavedPlayerSlot() const
{
    if(auto* Index=ReadPlayerIndex(false)) return Index->ActiveSlot;
    return FString();
}
bool UStarshatterPlayerSubsystem::SavePlayerAs(const FString& DisplayName,const FString& ExistingSlot)
{
    const FString Name=DisplayName.TrimStartAndEnd();if(Name.IsEmpty()) return false;
    auto* Index=ReadPlayerIndex(false);if(!Index) return false;
    int32 Position=INDEX_NONE;
    if(!ExistingSlot.IsEmpty())
    {
        if(!IsManagedPlayerSlot(ExistingSlot)) return false;
        Position=Index->Players.IndexOfByPredicate([&](const auto& Entry){return Entry.Slot==ExistingSlot;});
        if(Position==INDEX_NONE) return false;
    }
    for(int32 I=0;I<Index->Players.Num();++I)
        if(I!=Position && Index->Players[I].DisplayName.Equals(Name,ESearchCase::IgnoreCase)) return false;
    if(!SavePlayer(true)) return false;
    const bool bNew=Position==INDEX_NONE;
    FPlayerSaveSlotInfo Entry=bNew?FPlayerSaveSlotInfo():Index->Players[Position];
    if(bNew) Entry.Slot=SlotName+TEXT("_Profile_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Entry.DisplayName=Name;Entry.SavedAt=FDateTime::UtcNow();
    if(!SaveGameInternal(Entry.Slot,UserIndex,PlayerInfo,CURRENT_SAVE_VERSION)) return false;
    if(bNew) Index->Players.Add(Entry);else Index->Players[Position]=Entry;
    Index->ActiveSlot=Entry.Slot;
    if(!WritePlayerIndex(Index))
    {
        if(bNew) UGameplayStatics::DeleteGameInSlot(Entry.Slot,UserIndex);
        return false;
    }
    return true;
}
bool UStarshatterPlayerSubsystem::LoadSavedPlayer(const FString& SavedSlot)
{
    auto* Index=ReadPlayerIndex(false);
    if(!Index || !IsManagedPlayerSlot(SavedSlot) || !Index->Players.ContainsByPredicate([&](const auto& Entry){return Entry.Slot==SavedSlot;})) return false;
    FS_PlayerGameInfo Loaded;int32 Version=0;
    if(!LoadGameInternal(SavedSlot,UserIndex,Loaded,Version) || Version>CURRENT_SAVE_VERSION) return false;
    if(Version<CURRENT_SAVE_VERSION && !MigratePlayerSave(Version,CURRENT_SAVE_VERSION,Loaded)) return false;
    // Commit only a successfully decoded profile. A missing/corrupt selection leaves the pilot intact.
    if(!SaveGameInternal(SlotName,UserIndex,Loaded,CURRENT_SAVE_VERSION)) return false;
    PlayerInfo=MoveTemp(Loaded);bLoaded=true;bDirty=false;bHadExistingSave=true;
    Index->ActiveSlot=SavedSlot;
    if(!WritePlayerIndex(Index)) UE_LOG(LogStarshatterPlayerSubsystem,Warning,TEXT("Player loaded, but active-save selection could not be recorded."));
    OnPlayerSaveLoaded.Broadcast(true);return true;
}
bool UStarshatterPlayerSubsystem::DeleteSavedPlayer(const FString& SavedSlot)
{
    auto* Index=ReadPlayerIndex(false);if(!Index || !IsManagedPlayerSlot(SavedSlot)) return false;
    const int32 Position=Index->Players.IndexOfByPredicate([&](const auto& Entry){return Entry.Slot==SavedSlot;});
    if(Position==INDEX_NONE) return false;
    const auto OldEntries=Index->Players;const FString OldActive=Index->ActiveSlot;
    Index->Players.RemoveAt(Position);if(Index->ActiveSlot==SavedSlot) Index->ActiveSlot.Reset();
    if(!WritePlayerIndex(Index)) return false;
    if(UGameplayStatics::DoesSaveGameExist(SavedSlot,UserIndex) && !UGameplayStatics::DeleteGameInSlot(SavedSlot,UserIndex))
    {Index->Players=OldEntries;Index->ActiveSlot=OldActive;WritePlayerIndex(Index);return false;}
    return true;
}
