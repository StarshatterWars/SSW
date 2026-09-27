#include "CampaignSceneActor.h"
#include "SSWCameraManager.h"

#include "SceneMeshActor.h"
#include "SystemSceneBuilder.h"
#include "Game.h"
#include "Drive.h"
#include "Instruction.h"
#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"
#include "ShipAI.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

#include "PlanetActor.h"
#include "ShipActor.h"
#include "CameraPodActor.h"
#include "ShipDesign.h"
#include "GameStructs.h"

#include "Sim.h"
#include "SimObject.h"
#include "Ship.h"

#include "StarshatterEnvironmentSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "SimRegion.h"
#include "Orbital.h"
#include "OrbitalRegion.h"

#include "SimDirector.h"
#include "TimerSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
    static INSTRUCTION_FORMATION ResolveInstructionFormation(int32 Formation)
    {
        switch (Formation)
        {
        case 1:
            return INSTRUCTION_FORMATION::DIAMOND;
        case 2:
            return INSTRUCTION_FORMATION::SPREAD;
        case 3:
            return INSTRUCTION_FORMATION::BOX;
        case 4:
            return INSTRUCTION_FORMATION::TRAIL;
        case 0:
        default:
            return INSTRUCTION_FORMATION::NONE;
        }
    }
}

ACampaignSceneActor::ACampaignSceneActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void ACampaignSceneActor::BeginPlay()
{
    Super::BeginPlay();
}

void ACampaignSceneActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bEnableRuntimeAITick)
    {
        TickRuntimeShips(DeltaSeconds);
    }
}

void ACampaignSceneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearRuntimeShips();

    Super::EndPlay(EndPlayReason);
}

ASystemSceneBuilder* ACampaignSceneActor::ResolveSystemSceneBuilder() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
    {
        ASystemSceneBuilder* Builder = *It;
        if (IsValid(Builder))
        {
            return Builder;
        }
    }

    return nullptr;
}

void ACampaignSceneActor::ClearSceneActors()
{
    for (AActor* Actor : OwnedActors)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    OwnedActors.Empty();
    SpawnedSceneActors.Empty();

    ClearRuntimeShips();

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Cleared scene actors"));
}

void ACampaignSceneActor::TickRuntimeShips(float DeltaSeconds)
{
    bool bRuntimeSubsystemOwnsTick = false;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USSWRuntimeSubsystem* RuntimeSS =
            GI->GetSubsystem<USSWRuntimeSubsystem>())
        {
            bRuntimeSubsystemOwnsTick =
                RuntimeSS->IsRuntimeRunning();
        }
    }

    if (bRuntimeSubsystemOwnsTick)
    {
        return;
    }

    const double SimSeconds =
        FMath::Clamp(
            (double)DeltaSeconds * (double)RuntimeAITimeScale,
            0.0,
            (double)MaxRuntimeTickSeconds);

    if (Sim* RuntimeSim = Sim::GetSim())
    {
        RuntimeSim->ExecFrame(SimSeconds);

        UE_LOG(LogTemp, Verbose,
            TEXT("[CampaignSceneActor] Fallback Sim Tick Seconds=%.4f"),
            SimSeconds);
    }
}

void ACampaignSceneActor::ClearRuntimeShips()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] ClearRuntimeShips skipped. Runtime ships are owned by Sim/SimRegion."));

    RuntimeShips.Empty();
    RuntimeShipByElementName.Empty();
    ShipActorByElementName.Empty();
}

FVector ACampaignSceneActor::ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const
{
    return GetActorLocation() + SceneOriginOffset + (LegacyLoc * LegacyUnitsPerKm);
}

FVector ACampaignSceneActor::ConvertMissionElementLocToWorld(const FS_MissionElement& Elem) const
{
    return ConvertLegacySceneLocToWorld(Elem.Location);
}

bool ACampaignSceneActor::ResolveRegionCenterLocation(
    const FString& RegionName,
    FVector& OutWorldLocation) const
{
    OutWorldLocation = FVector::ZeroVector;

    const FString SearchName = RegionName.TrimStartAndEnd();
    if (SearchName.IsEmpty())
    {
        return false;
    }

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();
    if (!Builder)
    {
        return false;
    }

    FSpawnedSystemRegion Region;
    if (Builder->GetRegionByName(SearchName + TEXT("_REGION"), Region) ||
        Builder->GetRegionByName(SearchName, Region))
    {
        OutWorldLocation = Region.Actor
            ? Region.Actor->GetActorLocation()
            : Region.SpawnLocation;
        return true;
    }

    if (Builder->GetBodyWorldLocationByName(SearchName, OutWorldLocation))
    {
        return true;
    }

    return false;
}

bool ACampaignSceneActor::ResolveRegionAnchorLocation(
    const FString& RegionName,
    FVector& OutWorldLocation) const
{
    return ResolveRegionCenterLocation(RegionName, OutWorldLocation);
}

void ACampaignSceneActor::DumpSceneActors() const
{
    UE_LOG(LogTemp, Warning,
        TEXT("SceneActor count = %d"),
        SpawnedSceneActors.Num());

    for (const FCampaignSceneSpawnedActor& E : SpawnedSceneActors)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Name='%s' Design='%s' Region='%s' Actor=%s"),
            *E.ElementName,
            *E.DesignName,
            *E.RegionName,
            *GetNameSafe(E.Actor));
    }
}

void ACampaignSceneActor::BuildSceneActorsFromMission(
    const FS_CampaignMission& MissionData)
{
    ClearSceneActors();

    //-------------------------------------------------------------
    // START MISSION CLOCK
    //-------------------------------------------------------------
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UTimerSubsystem* Timer =
            GI->GetSubsystem<UTimerSubsystem>())
        {
            Timer->StartMissionRun(true);
        }
    }

    CurrentMissionRegionName =
        MissionData.MissionRegion.TrimStartAndEnd();

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] CurrentMissionRegionName SET '%s'"),
        *CurrentMissionRegionName);

    //-------------------------------------------------------------
    // BUILD RUNTIME SIM REGIONS
    //-------------------------------------------------------------
    BuildSimRegionsFromEnvironment();

    //-------------------------------------------------------------
    // EXECUTE LEGACY MISSION
    //-------------------------------------------------------------
    if (Sim* SimInst = Sim::GetSim())
    {
        Mission* LegacyMission =
            new Mission();

        if (LegacyMission)
        {
            //-----------------------------------------------------
            // Build legacy mission from data
            //-----------------------------------------------------
            LegacyMission->LoadMissionCommon(
                MissionData,
                true);

            //-----------------------------------------------------
            // Load into sim
            //-----------------------------------------------------
            SimInst->LoadMission(
                LegacyMission,
                false);

            //-----------------------------------------------------
            // Legacy runtime creation happens here:
            //
            // Sim::ExecMission()
            //   -> CreateElements()
            //   -> CreateShip()
            //   -> RuntimeSubsystem spawns visuals
            //-----------------------------------------------------
            SimInst->ExecMission();

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Legacy Sim mission executed"));
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[CampaignSceneActor] Failed creating legacy mission"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignSceneActor] No Sim instance"));
    }
}

AActor*
ACampaignSceneActor::FindSceneActorByName(
    const FString& ElementName) const
{
    const FString SearchName =
        ElementName.TrimStartAndEnd();

    if (SearchName.IsEmpty())
    {
        return nullptr;
    }

    //-------------------------------------------------------------
    // 1. Old CampaignSceneActor-tracked actors
    //-------------------------------------------------------------

    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (!Entry.Actor)
        {
            continue;
        }

        if (Entry.ElementName.Equals(
            SearchName,
            ESearchCase::IgnoreCase))
        {
            return Entry.Actor;
        }
    }

    //-------------------------------------------------------------
    // 2. Runtime-spawned visual actors
    //-------------------------------------------------------------

    UWorld* World =
        GetWorld();

    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor =
            *It;

        if (!IsValid(Actor))
        {
            continue;
        }

        const FString RuntimeActorName =
            Actor->GetName();

#if WITH_EDITOR
        const FString RuntimeActorLabel =
            Actor->GetActorLabel();
#else
        const FString RuntimeActorLabel =
            RuntimeActorName;
#endif

        if (RuntimeActorName.Equals(SearchName, ESearchCase::IgnoreCase) ||
            RuntimeActorLabel.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            return Actor;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] FindSceneActorByName failed '%s'"),
        *SearchName);

    return nullptr;
}

bool ACampaignSceneActor::FocusCameraOnSceneActorByName(
    const FString& ElementName,
    const FVector& CameraOffset,
    const FRotator& CameraRotator,
    float BlendSeconds) const
{
    AActor* TargetActor =
        FindSceneActorByName(ElementName);

    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneActor Camera] Target not found '%s'"),
            *ElementName);

        return false;
    }

    ASSWCameraManager* CameraManager =
        ResolveSSWCameraManager();

    if (!CameraManager)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneActor Camera] Existing SSWCameraManager not found"));

        return false;
    }

    //---------------------------------------------------------
    // Compute cinematic bounds
    //---------------------------------------------------------
    FBox Bounds =
        TargetActor->GetComponentsBoundingBox(true);

    float TargetSize = 500.0f;

    if (Bounds.IsValid)
    {
        TargetSize =
            FMath::Max(
                250.0f,
                Bounds.GetExtent().Size());
    }

    //---------------------------------------------------------
    // Default cinematic follow framing
    //---------------------------------------------------------
    FVector FinalOffset = CameraOffset;

    if (FinalOffset.IsNearlyZero())
    {
        FinalOffset = FVector(
            -TargetSize * 6.0f,
            TargetSize * 2.2f,
            TargetSize * 1.0f);
    }
    else
    {
        //-----------------------------------------------------
        // Treat custom offset as scale multipliers
        //-----------------------------------------------------
        FinalOffset.X *= TargetSize;
        FinalOffset.Y *= TargetSize;
        FinalOffset.Z *= TargetSize;
    }

    //---------------------------------------------------------
    // Apply follow view
    //---------------------------------------------------------
    CameraManager->SetActorFollowView(
        TargetActor,
        FinalOffset,
        CameraRotator);

    CameraManager->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] "
            "ExistingCamera=%s "
            "Target='%s' "
            "Size=%.2f "
            "Offset=%s "
            "Rot=%s"),
        *GetNameSafe(CameraManager),
        *ElementName,
        TargetSize,
        *FinalOffset.ToString(),
        *CameraRotator.ToString());

    return true;
}

ASSWCameraManager* ACampaignSceneActor::ResolveSSWCameraManager() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ASSWCameraManager> It(World); It; ++It)
    {
        ASSWCameraManager* CameraManager = *It;
        if (IsValid(CameraManager))
        {
            return CameraManager;
        }
    }

    return nullptr;
}

FVector ACampaignSceneActor::ConvertLegacyRegionOffsetToSceneOffset(
    const FVector& LegacyOffset) const
{
    /*
     * Legacy mission locs are large Starshatter region coordinates.
     * Example: -120000, -90000.
     *
     * Region anchors in the visual scene use ring radius scale around
     * a few thousand units, so mission offsets must be compressed into
     * the same visual scene scale.
     */

    const float LegacyReference = 200000.0f;

    const float Scale =
        LegacyReference > 0.0f
        ? MissionRegionOffsetScale / LegacyReference
        : 1.0f;

    return FVector(
        LegacyOffset.X * Scale,
        LegacyOffset.Y * Scale,
        LegacyOffset.Z * Scale);
}

FVector ACampaignSceneActor::ConvertLegacyRegionOffsetToWorld(
    AActor* RegionActor,
    const FVector& LegacyOffset) const
{
    const FVector SceneOffset =
        ConvertLegacyRegionOffsetToSceneOffset(LegacyOffset);

    if (RegionActor)
    {
        return RegionActor->GetActorTransform().TransformPosition(SceneOffset);
    }

    return GetActorLocation() + SceneOffset;
}

float ACampaignSceneActor::ConvertLegacyRegionSpeedToSceneSpeed(float LegacySpeed) const
{
    const float LegacyReference = 200000.0f;

    const float Scale =
        LegacyReference > 0.0f
        ? MissionRegionOffsetScale / LegacyReference
        : 1.0f;

    return LegacySpeed * Scale;
}

FString ACampaignSceneActor::GetCommanderForElement(const FString& ElementName) const
{
    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (Entry.ElementName.Equals(ElementName, ESearchCase::IgnoreCase))
        {
            return Entry.CommanderName; // <- ADD THIS FIELD (see below)
        }
    }

    return FString();
}


int32 ACampaignSceneActor::GetCommanderGroupActorCount(
    const FString& CommanderName) const
{
    int32 Count = 0;

    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (!Entry.Actor)
        {
            continue;
        }

        if (Entry.ElementName.Equals(CommanderName, ESearchCase::IgnoreCase) ||
            Entry.CommanderName.Equals(CommanderName, ESearchCase::IgnoreCase))
        {
            Count++;
        }
    }

    return Count;
}

bool ACampaignSceneActor::FocusCameraOnCommanderGroup(
    const FString& CommanderName,
    float BlendSeconds) const
{
    ASSWCameraManager* Cam = ResolveSSWCameraManager();
    if (!Cam)
    {
        return false;
    }

    TArray<AActor*> GroupActors;

    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (!Entry.Actor)
        {
            continue;
        }

        if (Entry.ElementName.Equals(CommanderName, ESearchCase::IgnoreCase) ||
            Entry.CommanderName.Equals(CommanderName, ESearchCase::IgnoreCase))
        {
            GroupActors.Add(Entry.Actor);
        }
    }

    if (GroupActors.Num() < 2)
    {
        return false;
    }

    FBox Box(ForceInit);
    FVector AverageVelocity = FVector::ZeroVector;

    for (AActor* Actor : GroupActors)
    {
        Box += Actor->GetActorLocation();
        AverageVelocity += Actor->GetVelocity();
    }

    const FVector Center = Box.GetCenter();
    const FVector Extent = Box.GetExtent();

    FVector VelocityDir = AverageVelocity.GetSafeNormal();

    if (VelocityDir.IsNearlyZero())
    {
        VelocityDir = GroupActors[0]->GetActorForwardVector();
    }

    const float Radius =
        FMath::Max(
            Extent.Size(),
            400.0f);

    //---------------------------------------------------------
    // Pull camera farther back for cinematic fleet framing
    //---------------------------------------------------------
    const float Distance =
        FMath::Clamp(
            Radius * 2.25f,
            900.0f,
            3600.0f);

    //---------------------------------------------------------
    // Wider side offset + slightly higher elevation
    //---------------------------------------------------------
    const FVector LocalOffset(
        -Distance,
        Distance * 0.35f,
        Distance * 0.18f);

    Cam->SetGroupFollowView(
        GroupActors,
        LocalOffset,
        VelocityDir,
        0.0f);

    Cam->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] CommanderGroup FOLLOW '%s' "
            "Count=%d Center=%s Radius=%.2f Distance=%.2f Offset=%s"),
        *CommanderName,
        GroupActors.Num(),
        *Center.ToString(),
        Radius,
        Distance,
        *LocalOffset.ToString());

    return true;
}

void
ACampaignSceneActor::LinkRuntimeShipCommanders(
    const TArray<FS_MissionElement>& Elements)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] LinkRuntimeShipCommanders: Elements=%d RuntimeShips=%d"),
        Elements.Num(),
        RuntimeShips.Num());

    for (const FS_MissionElement& Elem : Elements)
    {
        if (Elem.Name.IsEmpty() || Elem.Commander.IsEmpty())
        {
            continue;
        }

        Ship* ChildShip =
            RuntimeShipByElementName.FindRef(Elem.Name);

        Ship* CommanderShip =
            RuntimeShipByElementName.FindRef(Elem.Commander);

        if (!ChildShip || !CommanderShip || ChildShip == CommanderShip)
        {
            continue;
        }

        //-------------------------------------------------------------
        // 1. Element first.
        // SetElement may rebuild/reset formation state, so do this
        // before assigning ward.
        //-------------------------------------------------------------
        if (CommanderShip->GetElement())
        {
            ChildShip->SetElement(
                CommanderShip->GetElement());
        }

        //-------------------------------------------------------------
        // 2. Leader.
        //-------------------------------------------------------------
        ChildShip->SetLeader(
            CommanderShip);

        //-------------------------------------------------------------
        // 3. Ward direct pointer.
        //-------------------------------------------------------------
        ChildShip->SetWard(
            CommanderShip);

        //-------------------------------------------------------------
        // 4. Ward through AI so formation_delta is also initialized.
        //-------------------------------------------------------------
        if (ChildShip->GetDirector())
        {
            ShipAI* ChildAI =
                dynamic_cast<ShipAI*>(
                    ChildShip->GetDirector());

            if (ChildAI)
            {
                ChildAI->SetWard(
                    CommanderShip);
            }
        }

        //-------------------------------------------------------------
        // 5. Match leader initial motion.
        // Wingmen should inherit heading and speed.
        //-------------------------------------------------------------

        ChildShip->SetVelocity(
            CommanderShip->GetVelocity());

        ChildShip->SetAngularVelocity(
            CommanderShip->GetAngularVelocity());

        ChildShip->SetThrottle(
            CommanderShip->GetThrottle());

        ChildShip->SetThrottleRequest(
            CommanderShip->GetThrottleRequest());

        UE_LOG(LogTemp, Warning,
            TEXT("[WARD VERIFY LINK] Child='%hs' Leader='%hs' Ward='%hs' Element=%p ElementIndex=%d Director=%p ChildHeading=%s LeaderHeading=%s ChildVel=%s LeaderVel=%s"),
            ChildShip ? ChildShip->GetName() : "NULL",
            ChildShip && ChildShip->GetLeader() ?
            ChildShip->GetLeader()->GetName() : "NULL",
            ChildShip && ChildShip->GetWard() ?
            ChildShip->GetWard()->GetName() : "NULL",
            ChildShip ? ChildShip->GetElement() : nullptr,
            ChildShip ? ChildShip->GetElementIndex() : -1,
            ChildShip ? ChildShip->GetDirector() : nullptr,
            ChildShip ? *ChildShip->GetHeading().ToString() : TEXT("NULL"),
            CommanderShip ? *CommanderShip->GetHeading().ToString() : TEXT("NULL"),
            ChildShip ? *ChildShip->GetVelocity().ToString() : TEXT("NULL"),
            CommanderShip ? *CommanderShip->GetVelocity().ToString() : TEXT("NULL"));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] LinkRuntimeShipCommanders COMPLETE"));
}

void
ACampaignSceneActor::ApplyRuntimeFormationOffsets(
    const TArray<FS_MissionElement>& Elements)
{
    TMap<FString, TArray<const FS_MissionElement*>> FollowersByCommander;

    for (const FS_MissionElement& Elem : Elements)
    {
        if (!Elem.Commander.IsEmpty())
        {
            FollowersByCommander
                .FindOrAdd(Elem.Commander)
                .Add(&Elem);
        }
    }

    for (const TPair<FString, TArray<const FS_MissionElement*>>& Pair :
        FollowersByCommander)
    {
        const FString& CommanderName =
            Pair.Key;

        const TArray<const FS_MissionElement*>& Followers =
            Pair.Value;

        Ship* CommanderShip =
            RuntimeShipByElementName.FindRef(CommanderName);

        AShipActor* CommanderActor =
            ShipActorByElementName.FindRef(CommanderName);

        if (!CommanderShip || !CommanderActor)
        {
            continue;
        }

        const FVector CommanderLoc =
            CommanderActor->GetActorLocation();

        const FRotator CommanderRot =
            CommanderActor->GetActorRotation();

        for (int32 Index = 0;
            Index < Followers.Num();
            ++Index)
        {
            const FS_MissionElement* FollowerElem =
                Followers[Index];

            if (!FollowerElem)
            {
                continue;
            }

            Ship* FollowerShip =
                RuntimeShipByElementName.FindRef(
                    FollowerElem->Name);

            AShipActor* FollowerActor =
                ShipActorByElementName.FindRef(
                    FollowerElem->Name);

            if (!FollowerShip || !FollowerActor)
            {
                continue;
            }

            const FVector LocalOffset =
                GetFormationOffsetForElement(
                    *FollowerElem,
                    Index,
                    Followers.Num());

            const FVector WorldOffset =
                CommanderRot.RotateVector(LocalOffset);

            const FVector DesiredWorldLoc =
                CommanderLoc + WorldOffset;

            //-----------------------------------------------------
            // Formation offset
            //-----------------------------------------------------
            FollowerShip->SetFormationOffset(LocalOffset);

            UE_LOG(LogTemp, Warning,
                TEXT("[WARD BEFORE MoveTo] Ship='%hs' Leader='%hs' Ward='%hs'"),
                FollowerShip ?
                FollowerShip->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetLeader() ?
                FollowerShip->GetLeader()->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetWard() ?
                FollowerShip->GetWard()->GetName() : "NULL");

            //-----------------------------------------------------
            // Runtime move
            //-----------------------------------------------------
            FollowerShip->MoveTo(DesiredWorldLoc);

            UE_LOG(LogTemp, Warning,
                TEXT("[WARD AFTER MoveTo] Ship='%hs' Leader='%hs' Ward='%hs'"),
                FollowerShip ?
                FollowerShip->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetLeader() ?
                FollowerShip->GetLeader()->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetWard() ?
                FollowerShip->GetWard()->GetName() : "NULL");

            //-----------------------------------------------------
            // Match heading
            //-----------------------------------------------------
            FollowerShip->SetHelmHeading(
                CommanderShip->GetHelmHeading());

            //-----------------------------------------------------
            // Actor placement
            //-----------------------------------------------------
            FollowerActor->SetActorLocation(
                DesiredWorldLoc);

            FollowerActor->SetActorRotation(
                CommanderRot);

            //-----------------------------------------------------
            // Re-assert formation linkage AFTER MoveTo
            //-----------------------------------------------------
            FollowerShip->SetLeader(CommanderShip);
            FollowerShip->SetWard(CommanderShip);

            if (FollowerShip->GetDirector())
            {
                ShipAI* AI =
                    dynamic_cast<ShipAI*>(
                        FollowerShip->GetDirector());

                if (AI)
                {
                    AI->SetWard(CommanderShip);
                }
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[WARD AFTER FORMATION APPLY] Ship='%hs' Leader='%hs' Ward='%hs' Element=%p ElementIndex=%d"),
                FollowerShip ?
                FollowerShip->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetLeader() ?
                FollowerShip->GetLeader()->GetName() : "NULL",
                FollowerShip &&
                FollowerShip->GetWard() ?
                FollowerShip->GetWard()->GetName() : "NULL",
                FollowerShip ?
                FollowerShip->GetElement() : nullptr,
                FollowerShip ?
                FollowerShip->GetElementIndex() : -1);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] FormationOffset Commander='%s' Follower='%s' Offset=%s World=%s"),
                *CommanderName,
                *FollowerElem->Name,
                *LocalOffset.ToString(),
                *DesiredWorldLoc.ToString());
        }
    }
}

FVector ACampaignSceneActor::GetFormationOffsetForElement(
    const FS_MissionElement& Elem,
    int32 FollowerIndex,
    int32 FollowerCount) const
{
    const float Spacing = 1800.0f;

    const int32 SafeIndex = FollowerIndex + 1;

    INSTRUCTION_FORMATION Formation = INSTRUCTION_FORMATION::DIAMOND;

    if (Elem.Navpoint.Num() > 0)
    {
        Formation = ResolveInstructionFormation(Elem.Navpoint[0].Formation);
    }

    switch (Formation)
    {
        case INSTRUCTION_FORMATION::TRAIL:
            return FVector(-Spacing * SafeIndex, 0.0f, 0.0f);

        case INSTRUCTION_FORMATION::SPREAD:
        {
            const float Side = (FollowerIndex % 2 == 0) ? -1.0f : 1.0f;
            const float Rank = FMath::FloorToFloat((float)FollowerIndex / 2.0f) + 1.0f;
            return FVector(-Spacing * Rank, Side * Spacing * Rank, 0.0f);
        }

        case INSTRUCTION_FORMATION::BOX:
        {
            const int32 Col = FollowerIndex % 2;
            const int32 Row = FollowerIndex / 2;
            return FVector(
                -Spacing * (Row + 1),
                (Col == 0 ? -Spacing : Spacing),
                0.0f);
        }

        case INSTRUCTION_FORMATION::DIAMOND:
        default:
        {
            switch (FollowerIndex)
            {
                case 0: return FVector(-Spacing, -Spacing, 0.0f);
                case 1: return FVector(-Spacing, Spacing, 0.0f);
                case 2: return FVector(-Spacing * 2.0f, 0.0f, 0.0f);
                default:
                {
                    const float Side = (FollowerIndex % 2 == 0) ? -1.0f : 1.0f;
                    const float Rank = FMath::FloorToFloat((float)FollowerIndex / 2.0f) + 1.0f;
                    return FVector(-Spacing * (Rank + 1.0f), Side * Spacing, 0.0f);
                }
            }
        }
    }
}

void ACampaignSceneActor::BuildSimRegionsFromEnvironment()
{
    Sim* SimInst = Sim::GetSim();

    UStarshatterEnvironmentSubsystem* Env =
        UStarshatterEnvironmentSubsystem::Get();

    if (!SimInst || !Env)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignSceneActor] BuildSimRegionsFromEnvironment failed Sim=%p Env=%p"),
            SimInst,
            Env);
        return;
    }

    Env->BuildSimRegionsForSim(SimInst);
}




