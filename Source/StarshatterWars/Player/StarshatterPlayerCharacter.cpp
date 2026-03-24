#include "StarshatterPlayerCharacter.h"
#include "StarshatterPlayerSubsystem.h"

void UStarshatterPlayerCharacter::Initialize(UStarshatterPlayerSubsystem* InOwnerSubsystem)
{
    OwnerSubsystem = InOwnerSubsystem;
}

static void NormalizeArrays(TArray<FString>& InOutChatMacros, TArray<int32>& InOutMfdModes)
{
    // Ensure 10 chat macros
    if (InOutChatMacros.Num() < 10)
    {
        const int32 AddCount = 10 - InOutChatMacros.Num();
        for (int32 i = 0; i < AddCount; ++i)
            InOutChatMacros.Add(TEXT(""));
    }
    else if (InOutChatMacros.Num() > 10)
    {
        InOutChatMacros.SetNum(10);
    }

    // Ensure 3 MFD modes
    if (InOutMfdModes.Num() < 3)
    {
        const int32 AddCount = 3 - InOutMfdModes.Num();
        for (int32 i = 0; i < AddCount; ++i)
            InOutMfdModes.Add(0);
    }
    else if (InOutMfdModes.Num() > 3)
    {
        InOutMfdModes.SetNum(3);
    }
}

void UStarshatterPlayerCharacter::FromPlayerInfo(const FS_PlayerGameInfo& InInfo)
{
    Id = InInfo.Id;
    Name = InInfo.Name;
    Callsign = InInfo.Callsign;
    Squadron = InInfo.Squadron;
    Signature = InInfo.Signature;
    Avatar = InInfo.Avatar;

    Campaign = InInfo.Campaign;
    CampaignRowName = InInfo.CampaignRowName;
    Mission = InInfo.Mission;

    Rank = InInfo.Rank;
    Empire = InInfo.Empire;

    ShipColor = InInfo.ShipColor;
    HudMode = InInfo.HudMode;
    GunMode = InInfo.GunMode;
    HudColor = InInfo.HudColor;
    FlightModel = InInfo.FlightModel;
    LandingMode = InInfo.LandingMode;

    FlyingStart = InInfo.FlyingStart;
    GridMode = InInfo.GridMode;
    TrainingMode = InInfo.TrainingMode;
    GunSightMode = InInfo.GunSightMode;

    AILevel = InInfo.AILevel;
    ForceFeedbackLevel = InInfo.ForceFeedbackLevel;

    CreateTime = InInfo.CreateTime;
    GameTime = InInfo.GameTime;
    CampaignTime = InInfo.CampaignTime;
    FlightTime = InInfo.FlightTime;

    PlayerKills = InInfo.PlayerKills;
    PlayerWins = InInfo.PlayerWins;
    PlayerLosses = InInfo.PlayerLosses;
    PlayerDeaths = InInfo.PlayerDeaths;
    PlayerMissions = InInfo.PlayerMissions;
    PlayerPoints = InInfo.PlayerPoints;
    PlayerLevel = InInfo.PlayerLevel;
    PlayerExperience = InInfo.PlayerExperience;

    PlayerStatus = InInfo.PlayerStatus;
    PlayerShip = InInfo.PlayerShip;
    PlayerRegion = InInfo.PlayerRegion;
    PlayerSystem = InInfo.PlayerSystem;

    PlayerSquadron = InInfo.PlayerSquadron;
    PlayerWing = InInfo.PlayerWing;
    PlayerDesronGroup = InInfo.PlayerDesronGroup;
    PlayerBattleGroup = InInfo.PlayerBattleGroup;
    PlayerCarrier = InInfo.PlayerCarrier;
    PlayerFleet = InInfo.PlayerFleet;
    PlayerForce = InInfo.PlayerForce;

    CampaignCompleteMask = InInfo.CampaignCompleteMask;
    CampaignComplete = InInfo.CampaignComplete;

    HighestTrainingMission = InInfo.HighestTrainingMission;
    TrainingMask = InInfo.TrainingMask;
    Trained = InInfo.Trained;

    MedalsMask = InInfo.MedalsMask;

    ChatMacros = InInfo.ChatMacros;
    MfdModes = InInfo.MfdModes;

    NormalizeArrays(ChatMacros, MfdModes);
}

void UStarshatterPlayerCharacter::ToPlayerInfo(FS_PlayerGameInfo& OutInfo) const
{
    OutInfo.Id = Id;
    OutInfo.Name = Name;
    OutInfo.Callsign = Callsign;
    OutInfo.Squadron = Squadron;
    OutInfo.Signature = Signature;
    OutInfo.Avatar = Avatar;

    OutInfo.Campaign = Campaign;
    OutInfo.CampaignRowName = CampaignRowName;
    OutInfo.Mission = Mission;

    OutInfo.Rank = Rank;
    OutInfo.Empire = Empire;

    OutInfo.ShipColor = ShipColor;
    OutInfo.HudMode = HudMode;
    OutInfo.GunMode = GunMode;
    OutInfo.HudColor = HudColor;
    OutInfo.FlightModel = FlightModel;
    OutInfo.LandingMode = LandingMode;

    OutInfo.FlyingStart = FlyingStart;
    OutInfo.GridMode = GridMode;
    OutInfo.TrainingMode = TrainingMode;
    OutInfo.GunSightMode = GunSightMode;

    OutInfo.AILevel = AILevel;
    OutInfo.ForceFeedbackLevel = ForceFeedbackLevel;

    OutInfo.CreateTime = CreateTime;
    OutInfo.GameTime = GameTime;
    OutInfo.CampaignTime = CampaignTime;
    OutInfo.FlightTime = FlightTime;

    OutInfo.PlayerKills = PlayerKills;
    OutInfo.PlayerWins = PlayerWins;
    OutInfo.PlayerLosses = PlayerLosses;
    OutInfo.PlayerDeaths = PlayerDeaths;
    OutInfo.PlayerMissions = PlayerMissions;
    OutInfo.PlayerPoints = PlayerPoints;
    OutInfo.PlayerLevel = PlayerLevel;
    OutInfo.PlayerExperience = PlayerExperience;

    OutInfo.PlayerStatus = PlayerStatus;
    OutInfo.PlayerShip = PlayerShip;
    OutInfo.PlayerRegion = PlayerRegion;
    OutInfo.PlayerSystem = PlayerSystem;

    OutInfo.PlayerSquadron = PlayerSquadron;
    OutInfo.PlayerWing = PlayerWing;
    OutInfo.PlayerDesronGroup = PlayerDesronGroup;
    OutInfo.PlayerBattleGroup = PlayerBattleGroup;
    OutInfo.PlayerCarrier = PlayerCarrier;
    OutInfo.PlayerFleet = PlayerFleet;
    OutInfo.PlayerForce = PlayerForce;

    OutInfo.CampaignCompleteMask = CampaignCompleteMask;
    OutInfo.CampaignComplete = CampaignComplete;

    OutInfo.HighestTrainingMission = HighestTrainingMission;
    OutInfo.TrainingMask = TrainingMask;
    OutInfo.Trained = Trained;

    OutInfo.MedalsMask = MedalsMask;

    OutInfo.ChatMacros = ChatMacros;
    OutInfo.MfdModes = MfdModes;
}

bool UStarshatterPlayerCharacter::Commit(bool bForceSave)
{
    if (!OwnerSubsystem)
        return false;

    FS_PlayerGameInfo& MutableInfo = OwnerSubsystem->GetMutablePlayerInfo();
    ToPlayerInfo(MutableInfo);

    return OwnerSubsystem->SavePlayer(bForceSave);
}

void UStarshatterPlayerCharacter::LoadFromPlayerInfo(const FS_PlayerGameInfo& Info)
{
    // ------------------------------------------------------------
    // Identity / profile
    // ------------------------------------------------------------
    Id = Info.Id;
    Name = Info.Name;
    Callsign = Info.Callsign;
    Squadron = Info.Squadron;
    Signature = Info.Signature;
    Avatar = Info.Avatar;

    // ------------------------------------------------------------
    // Campaign / progression routing
    // ------------------------------------------------------------
    Campaign = Info.Campaign;
    CampaignRowName = Info.CampaignRowName;
    Mission = Info.Mission;

    // ------------------------------------------------------------
    // Rank / faction
    // ------------------------------------------------------------
    Rank = Info.Rank;
    Empire = Info.Empire;

    // ------------------------------------------------------------
    // Visual / HUD / controls prefs
    // ------------------------------------------------------------
    ShipColor = Info.ShipColor;
    HudMode = Info.HudMode;
    GunMode = Info.GunMode;
    HudColor = Info.HudColor;
    FlightModel = Info.FlightModel;
    LandingMode = Info.LandingMode;

    FlyingStart = Info.FlyingStart;
    GridMode = Info.GridMode;
    TrainingMode = Info.TrainingMode;
    GunSightMode = Info.GunSightMode;

    AILevel = Info.AILevel;
    ForceFeedbackLevel = Info.ForceFeedbackLevel;

    // ------------------------------------------------------------
    // Time
    // ------------------------------------------------------------
    CreateTime = Info.CreateTime;
    GameTime = Info.GameTime;
    CampaignTime = Info.CampaignTime;
    FlightTime = Info.FlightTime;

    // ------------------------------------------------------------
    // Career / logbook stats
    // ------------------------------------------------------------
    PlayerKills = Info.PlayerKills;
    PlayerWins = Info.PlayerWins;
    PlayerLosses = Info.PlayerLosses;
    PlayerDeaths = Info.PlayerDeaths;
    PlayerMissions = Info.PlayerMissions;
    PlayerPoints = Info.PlayerPoints;
    PlayerLevel = Info.PlayerLevel;
    PlayerExperience = Info.PlayerExperience;

    PlayerStatus = Info.PlayerStatus;
    PlayerShip = Info.PlayerShip;
    PlayerRegion = Info.PlayerRegion;
    PlayerSystem = Info.PlayerSystem;

    // ------------------------------------------------------------
    // OOB / unit selection
    // ------------------------------------------------------------
    PlayerSquadron = Info.PlayerSquadron;
    PlayerWing = Info.PlayerWing;
    PlayerDesronGroup = Info.PlayerDesronGroup;
    PlayerBattleGroup = Info.PlayerBattleGroup;
    PlayerCarrier = Info.PlayerCarrier;
    PlayerFleet = Info.PlayerFleet;
    PlayerForce = Info.PlayerForce;

    // ------------------------------------------------------------
    // Campaign completion
    // ------------------------------------------------------------
    CampaignCompleteMask = Info.CampaignCompleteMask;
    CampaignComplete = Info.CampaignComplete;   // TArray<uint8> (0/1)

    // ------------------------------------------------------------
    // Training
    // ------------------------------------------------------------
    HighestTrainingMission = Info.HighestTrainingMission;
    TrainingMask = Info.TrainingMask;

    // Keep legacy compatibility field in sync:
    Trained = Info.Trained;

    // ------------------------------------------------------------
    // Awards / medals
    // ------------------------------------------------------------
    MedalsMask = Info.MedalsMask;

    // ------------------------------------------------------------
    // Chat / MFD
    // ------------------------------------------------------------
    ChatMacros = Info.ChatMacros;
    MfdModes = Info.MfdModes;

    // ------------------------------------------------------------
    // Optional: normalize array sizes for UI assumptions
    // ------------------------------------------------------------

    // Legacy expects exactly 10 chat macros:
    if (ChatMacros.Num() < 10)
    {
        const int32 AddCount = 10 - ChatMacros.Num();
        for (int32 i = 0; i < AddCount; ++i)
            ChatMacros.Add(TEXT(""));
    }
    else if (ChatMacros.Num() > 10)
    {
        ChatMacros.SetNum(10);
    }

    // Legacy expects exactly 3 MFD modes:
    if (MfdModes.Num() < 3)
    {
        const int32 AddCount = 3 - MfdModes.Num();
        for (int32 i = 0; i < AddCount; ++i)
            MfdModes.Add(0);
    }
    else if (MfdModes.Num() > 3)
    {
        MfdModes.SetNum(3);
    }

    // CampaignComplete is optional; if you want it to mirror the mask,
    // you can rebuild it here. (Leaving as-is unless you want strict sync.)
}
