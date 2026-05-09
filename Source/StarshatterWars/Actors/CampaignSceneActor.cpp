#include "CampaignSceneActor.h"
#include "SSWCameraManager.h"

#include "SceneMeshActor.h"
#include "SystemSceneBuilder.h"

#include "Instruction.h"
#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

#include "PlanetActor.h"
#include "ShipActor.h"
#include "CameraPodActor.h"
#include "ShipDesign.h"

#include "Sim.h"
#include "Ship.h"

#include "StarshatterEnvironmentSubsystem.h"
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

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
        {
            Timer->ManualMissionTick(DeltaSeconds);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] TIMER TICK Delta=%.4f MissionMS=%d State=%d"),
                DeltaSeconds,
                Timer->GetMissionTimeMS(),
                (int32)Timer->GetMissionClockState());
        }
    }

    if (bEnableRuntimeAITick)
    {
        TickRuntimeShips(DeltaSeconds);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Runtime AI tick DISABLED"));
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
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
        {
            Timer->ManualMissionTick(DeltaSeconds);
        }
    }

    const double SimSeconds =
        FMath::Clamp((double)DeltaSeconds * (double)RuntimeAITimeScale, 0.0, (double)MaxRuntimeTickSeconds);

    for (Ship* RuntimeShip : RuntimeShips)
    {
        if (!RuntimeShip)
        {
            continue;
        }

        RuntimeShip->ExecFrame(SimSeconds);
       
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] RuntimeAI Tick Ship='%s' Loc=%s Vel=%s"),
            ANSI_TO_TCHAR(RuntimeShip->GetName()),
            *RuntimeShip->GetLocation().ToString(),
            *RuntimeShip->GetVelocity().ToString());
    }

    for (const TPair<FString, AShipActor*>& Pair : ShipActorByElementName)
    {
        AShipActor* ShipActor = Pair.Value;
        Ship* RuntimeShip = RuntimeShipByElementName.FindRef(Pair.Key);

        if (!ShipActor || !RuntimeShip)
        {
            continue;
        }

        ShipActor->UpdateFromRuntimeShip(DeltaSeconds);
    }
}

void ACampaignSceneActor::ClearRuntimeShips()
{
    for (Ship* RuntimeShip : RuntimeShips)
    {
        delete RuntimeShip;
    }

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

FString ACampaignSceneActor::ResolveModelNameForDesign(const FString& DesignName) const
{
    const FShipDesign* Row = ShipDesignRegistry::Find(DesignName);
    if (!Row)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] No ship design row for '%s'"),
            *DesignName);
        return FString();
    }

    return Row->Model;
}

FString ACampaignSceneActor::ResolveMeshPathForDesign(const FString& DesignName) const
{
    const FString Model = ResolveModelNameForDesign(DesignName);
    if (Model.IsEmpty())
    {
        return FString();
    }

    return FString::Printf(
        TEXT("/Script/Engine.StaticMesh'/Game/Models/%s/%s.%s'"),
        *Model,
        *Model,
        *Model);
}

UStaticMesh* ACampaignSceneActor::ResolveStaticMeshFromPath(const FString& MeshPath) const
{
    return LoadObject<UStaticMesh>(nullptr, *MeshPath);
}

AActor* ACampaignSceneActor::FindRegionActorByName(const FString& RegionName) const
{
    const FString SearchName = RegionName.TrimStartAndEnd();
    if (SearchName.IsEmpty())
    {
        return nullptr;
    }

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();
    if (!Builder)
    {
        return nullptr;
    }

    FSpawnedSystemRegion Region;
    if (Builder->GetRegionByName(SearchName + TEXT("_REGION"), Region) ||
        Builder->GetRegionByName(SearchName, Region))
    {
        return Region.Actor;
    }

    return nullptr;
}


AActor* ACampaignSceneActor::SpawnSceneElementActor(
    const FString& ElementName,
    const FString& DesignName,
    const FVector& WorldLocation,
    int32 HeadingDegrees)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: World is null"));
        return nullptr;
    }

    const FString ResolvedName = DesignName.TrimStartAndEnd();

    if (ResolvedName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: DesignName is empty for '%s'"),
            *ElementName);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float ModelYawFix = -90.0f;

    const FRotator SpawnRotation(
        0.0f,
        (float)HeadingDegrees + ModelYawFix,
        0.0f);

    // --------------------------------------------------
    // 1. Blueprint path
    // --------------------------------------------------

    {
        const FString BPClassPath = FString::Printf(
            TEXT("/Game/Models/%s/BP_%s.BP_%s_C"),
            *ResolvedName,
            *ResolvedName,
            *ResolvedName);

        UClass* BPClass = LoadClass<AActor>(nullptr, *BPClassPath);

        if (BPClass)
        {
            AActor* SpawnedBPActor = World->SpawnActor<AActor>(
                BPClass,
                WorldLocation,
                SpawnRotation,
                Params);

            if (!SpawnedBPActor)
            {
                return nullptr;
            }

#if WITH_EDITOR
            SpawnedBPActor->SetActorLabel(ElementName);
#endif

            SpawnedBPActor->AttachToActor(
                this,
                FAttachmentTransformRules::KeepWorldTransform);

            SpawnedBPActor->SetActorHiddenInGame(false);
            SpawnedBPActor->SetActorEnableCollision(false);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BP Spawned '%s' Class=%s PreScale=%s"),
                *ElementName,
                *GetNameSafe(SpawnedBPActor->GetClass()),
                *SpawnedBPActor->GetActorScale3D().ToString());

            if (!SpawnedBPActor->IsA(APlanetActor::StaticClass()))
            {
                float FinalScale = MissionElementScaleMultiplier;

                //if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
                //{
                //    FinalScale *= 4.0f;
                //}

                FinalScale = FMath::Max(FinalScale, 1.0f);

                SpawnedBPActor->SetActorScale3D(FVector(FinalScale));

                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] BP PostScale '%s' Scale=%s FinalScale=%.6f"),
                    *ElementName,
                    *SpawnedBPActor->GetActorScale3D().ToString(),
                    FinalScale);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] PlanetActor detected, skipping mission element scale"));
            }

            return SpawnedBPActor;
        }
    }

    // --------------------------------------------------
    // 2. Static mesh fallback path
    // --------------------------------------------------

    TSubclassOf<ASceneMeshActor> SpawnClass = DefaultSceneMeshActorClass;

    if (!SpawnClass)
    {
        SpawnClass = ASceneMeshActor::StaticClass();
    }

    const FString MeshPath = FString::Printf(
        TEXT("/Script/Engine.StaticMesh'/Game/Models/%s/%s.%s'"),
        *ResolvedName,
        *ResolvedName,
        *ResolvedName);

    UStaticMesh* Mesh = ResolveStaticMeshFromPath(MeshPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: mesh not found '%s'"),
            *MeshPath);
        return nullptr;
    }

    ASceneMeshActor* SpawnedActor = World->SpawnActor<ASceneMeshActor>(
        SpawnClass,
        WorldLocation,
        SpawnRotation,
        Params);

    if (!SpawnedActor)
    {
        return nullptr;
    }

#if WITH_EDITOR
    SpawnedActor->SetActorLabel(ElementName);
#endif

    SpawnedActor->AttachToActor(
        this,
        FAttachmentTransformRules::KeepWorldTransform);

    SpawnedActor->SetSceneMesh(Mesh);

    UStaticMeshComponent* MeshComp = SpawnedActor->GetMeshComponent();

    if (!SpawnedActor->IsA(APlanetActor::StaticClass()))
    {
        float FinalScale = MissionElementScaleMultiplier;

        if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
        {
            FinalScale *= 4.0f;
        }

        FinalScale = FMath::Max(FinalScale, 1.0f);

        SpawnedActor->SetActorScale3D(FVector(FinalScale));

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Mesh PostScale '%s' Scale=%s FinalScale=%.6f"),
            *ElementName,
            *SpawnedActor->GetActorScale3D().ToString(),
            FinalScale);
    }

    if (MeshComp)
    {
        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
    }

    SpawnedActor->SetActorHiddenInGame(false);
    SpawnedActor->SetActorEnableCollision(false);

    return SpawnedActor;
}

void ACampaignSceneActor::BuildSceneActorsFromRuntimeMission(Mission* MissionPtr)
{
    ClearSceneActors();

    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] MissionPtr is null"));
        return;
    }

    int32 Count = 0;

    ListIter<MissionElement> It = MissionPtr->GetElements();
    while (++It)
    {
        MissionElement* Elem = It.value();
        if (!Elem)
        {
            continue;
        }

        const FString ElementName = ANSI_TO_TCHAR(Elem->GetName());
        const FString RegionName = ANSI_TO_TCHAR(Elem->GetRegion());

        const FShipDesign* Ship = Elem->GetShipDesign();
        if (!Ship)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] '%s' has no ship design"),
                *ElementName);
            continue;
        }

        const FString ModelName = Ship->Model;
        if (ModelName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] '%s' has empty model"),
                *ElementName);
            continue;
        }

        const FVector LegacyLoc = Elem->GetLocation();
        const int32 Heading = (int32)Elem->GetHeading();

        AActor* RegionActor = FindRegionActorByName(RegionName);

        FVector WorldLoc = FVector::ZeroVector;

        if (RegionActor)
        {
            WorldLoc = RegionActor->GetActorTransform().TransformPosition(LegacyLoc);
        }
        else
        {
            WorldLoc = ConvertLegacySceneLocToWorld(LegacyLoc);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Spawn '%s' Model='%s' Region='%s' World=%s"),
            *ElementName,
            *ModelName,
            *RegionName,
            *WorldLoc.ToString());

        AActor* Spawned = SpawnSceneElementActor(
            ElementName,
            ModelName,
            WorldLoc,
            Heading);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Failed to spawn '%s'"),
                *ElementName);
            continue;
        }

        if (RegionActor)
        {
            Spawned->AttachToActor(
                RegionActor,
                FAttachmentTransformRules::KeepWorldTransform);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Attached '%s' -> '%s'"),
                *ElementName,
                *RegionName);
        }

        //--------------------------------------------------
        // MOVEMENT
        //--------------------------------------------------

        if (AShipActor* ShipActor = Cast<AShipActor>(Spawned))
        {
            List<Instruction>& Navs = Elem->NavList();

            ListIter<Instruction> NavIt = Navs;
            if (++NavIt)
            {
                Instruction* Nav = NavIt.value();

                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] NAV DEBUG '%s' Action=%d Loc=%s Speed=%d"),
                    *ElementName,
                    Nav ? (int32)Nav->GetAction() : -1,
                    Nav ? *Nav->GetLocation().ToString() : TEXT("NULL"),
                    Nav ? Nav->GetSpeed() : -1);

                if (Nav)
                {
                    const FVector StartLegacy = Elem->GetLocation();
                    const FVector TargetLegacy = Nav->GetLocation();
                    const float Speed = Nav->GetSpeed() > 0 ? (float)Nav->GetSpeed() : 1000.0f;

                    ShipActor->SetCutsceneNavMovement(
                        StartLegacy,
                        TargetLegacy,
                        Speed);

                    UE_LOG(LogTemp, Warning,
                        TEXT("[CampaignSceneActor] MOVEMENT SET '%s' Start=%s Target=%s Speed=%.2f"),
                        *ElementName,
                        *StartLegacy.ToString(),
                        *TargetLegacy.ToString(),
                        Speed);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] NO NAVPOINTS '%s'"),
                    *ElementName);
            }
        }

        //--------------------------------------------------
        // Store
        //--------------------------------------------------

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = ElementName;
        Entry.DesignName = ModelName;
        Entry.RegionName = RegionName;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = Spawned->GetActorLocation();
        Entry.HeadingDegrees = Heading;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Build complete: %d actors"),
        Count);
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

void ACampaignSceneActor::BuildSceneActorsFromMission(const FS_CampaignMission& MissionData)
{
    ClearSceneActors();

    //-------------------------------------------------------------
    // START MISSION CLOCK 
    //-------------------------------------------------------------
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
        {
            Timer->StartMissionRun(true);
        }
    }

    CurrentMissionRegionName = MissionData.MissionRegion.TrimStartAndEnd();

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] CurrentMissionRegionName SET '%s'"),
        *CurrentMissionRegionName);

    int32 Count = 0;

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();

    //-------------------------------------------------------------
    // BUILD RUNTIME SIM REGIONS
    //-------------------------------------------------------------
    BuildSimRegionsFromEnvironment();
    
    //-------------------------------------------------------------
    // PASS 1: Spawn + Create Runtime Ships
    //-------------------------------------------------------------
    for (const FS_MissionElement& Elem : MissionData.Element)
    {
        if (Elem.Name.IsEmpty())
        {
            continue;
        }

        const FString ElementName = Elem.Name.TrimStartAndEnd();
        const FString RegionName = Elem.RegionName.TrimStartAndEnd();
        const FString DesignName = Elem.Design.TrimStartAndEnd();

        //--------------------------------------------------
        // Capture first valid element region as fallback.
        // Needed when MissionRegion is empty and player
        // CameraPod/Falcon has no explicit region.
        //--------------------------------------------------
        if (CurrentMissionRegionName.IsEmpty() && !RegionName.IsEmpty())
        {
            CurrentMissionRegionName = RegionName;

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] CurrentMissionRegionName AUTO-SET '%s'"),
                *CurrentMissionRegionName);
        }

        FString ModelName = DesignName;

        const FShipDesign* ShipRow = ShipDesignRegistry::Find(DesignName);
        if (ShipRow && !ShipRow->Model.IsEmpty())
        {
            ModelName = ShipRow->Model;
        }

        if (ModelName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] '%s' missing model/design"),
                *ElementName);
            continue;
        }

        //--------------------------------------------------
        // REGION RESOLVE
        //--------------------------------------------------
        AActor* RegionActor = nullptr;
        FVector RegionCenter = FVector::ZeroVector;
        bool bHasRegion = false;

        if (Builder && !RegionName.IsEmpty())
        {
            FSpawnedSystemRegion Region;

            if (Builder->GetRegionByName(RegionName + TEXT("_REGION"), Region) ||
                Builder->GetRegionByName(RegionName, Region))
            {
                RegionActor = Region.Actor;
                RegionCenter = Region.Actor
                    ? Region.Actor->GetActorLocation()
                    : Region.SpawnLocation;

                bHasRegion = true;
            }
            else if (Builder->GetBodyWorldLocationByName(RegionName, RegionCenter))
            {
                bHasRegion = true;
            }
        }

        //--------------------------------------------------
        // LOCATION CONVERSION
        //--------------------------------------------------
        const FVector LocalOffset =
            ConvertLegacyRegionOffsetToSceneOffset(Elem.Location);

        FVector WorldLoc = FVector::ZeroVector;

        if (RegionActor)
        {
            WorldLoc =
                RegionActor->GetActorTransform().TransformPosition(LocalOffset);
        }
        else if (bHasRegion)
        {
            WorldLoc = RegionCenter + LocalOffset;
        }
        else
        {
            WorldLoc = ConvertMissionElementLocToWorld(Elem);
        }

        //--------------------------------------------------
        // SPAWN ACTOR
        //--------------------------------------------------
        AActor* Spawned = SpawnSceneElementActor(
            ElementName,
            ModelName,
            WorldLoc,
            Elem.Heading);

        if (!Spawned)
        {
            continue;
        }

        if (RegionActor)
        {
            Spawned->AttachToActor(
                RegionActor,
                FAttachmentTransformRules::KeepWorldTransform);
        }

        //--------------------------------------------------
        // RUNTIME SHIP BINDING
        //--------------------------------------------------
        if (AShipActor* ShipActor = Cast<AShipActor>(Spawned))
        {
            Ship* RuntimeShip = CreateRuntimeShipForMissionElement(Elem, WorldLoc);

            if (RuntimeShip)
            {
                ShipActor->BindRuntimeShip(RuntimeShip);

                RegisterRuntimeShipForElement(
                    Elem,
                    RuntimeShip,
                    ShipActor);

                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] Ship bound '%s' -> Actor '%s'"),
                    *ElementName,
                    *ShipActor->GetName());
            }
        }

        //--------------------------------------------------
        // TRACKING
        //--------------------------------------------------
        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = ElementName;
        Entry.DesignName = ModelName;
        Entry.RegionName = RegionName;
        Entry.CommanderName = Elem.Commander;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = Spawned->GetActorLocation();
        Entry.HeadingDegrees = Elem.Heading;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

    //-------------------------------------------------------------
    // PASS 2: COMMANDER LINKING
    //-------------------------------------------------------------
    LinkRuntimeShipCommanders(MissionData.Element);
    ApplyRuntimeFormationOffsets(MissionData.Element);

    //-------------------------------------------------------------
    // FINAL LOG
    //-------------------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] BuildSceneActorsFromMission COMPLETE Count=%d RuntimeShips=%d"),
        Count,
        RuntimeShips.Num());
}

AActor* ACampaignSceneActor::FindSceneActorByName(const FString& ElementName) const
{
    const FString SearchName = ElementName.TrimStartAndEnd();
    if (SearchName.IsEmpty())
    {
        return nullptr;
    }

    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (!Entry.Actor)
        {
            continue;
        }

        if (Entry.ElementName.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            return Entry.Actor;
        }
    }

    return nullptr;
}

bool ACampaignSceneActor::FocusCameraOnSceneActorByName(
    const FString& ElementName,
    const FVector& CameraOffset,
    const FRotator& CameraRotator,
    float BlendSeconds) const
{
    AActor* TargetActor = FindSceneActorByName(ElementName);
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneActor Camera] Target not found '%s'"),
            *ElementName);
        return false;
    }

    ASSWCameraManager* CameraManager = ResolveSSWCameraManager();
    if (!CameraManager)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SceneActor Camera] Existing SSWCameraManager not found"));
        return false;
    }

    FBox Bounds = TargetActor->GetComponentsBoundingBox(true);

    float TargetSize = 500.0f;
    if (Bounds.IsValid)
    {
        TargetSize = FMath::Max(250.0f, Bounds.GetExtent().Size());
    }

    FVector FinalOffset = CameraOffset;

    if (FinalOffset.IsNearlyZero())
    {
        FinalOffset = FVector(-TargetSize * 4.0f, TargetSize * 1.5f, TargetSize * 0.75f);
    }
    else
    {
        FinalOffset *= TargetSize;
    }

    CameraManager->SetActorFollowView(
        TargetActor,
        FinalOffset,
        CameraRotator);

    CameraManager->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] ExistingCamera=%s Target='%s' Size=%.2f Offset=%s Rot=%s"),
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

    const float Radius = FMath::Max(Extent.Size(), 400.0f);

    const float Distance = FMath::Clamp(
        Radius * 1.05f,
        450.0f,
        1800.0f);

    const FVector LocalOffset(
        -Distance,
        Distance * 0.22f,
        Distance * 0.12f);

    Cam->SetGroupFollowView(
        GroupActors,
        LocalOffset,
        VelocityDir,
        0.0f);

    Cam->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] CommanderGroup FOLLOW '%s' Count=%d Center=%s Radius=%.2f Distance=%.2f Offset=%s"),
        *CommanderName,
        GroupActors.Num(),
        *Center.ToString(),
        Radius,
        Distance,
        *LocalOffset.ToString());

    return true;
}

Ship* ACampaignSceneActor::CreateRuntimeShipForMissionElement(
    const FS_MissionElement& Elem,
    const FVector& WorldLoc)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] CreateRuntimeShipForMissionElement Name='%s' Design='%s' WorldLoc=%s"),
        *Elem.Name,
        *Elem.Design,
        *WorldLoc.ToString());

    //-------------------------------------------------------------
    // 1. Resolve legacy ShipDesign
    //-------------------------------------------------------------
    ShipDesign* Design = ShipDesignRegistry::FindLegacy(Elem.Design);

    if (!Design)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignSceneActor] Legacy ShipDesign NOT FOUND '%s' for '%s'. RegistryCount=%d"),
            *Elem.Design,
            *Elem.Name,
            ShipDesignRegistry::Num());

        return nullptr;
    }

    //-------------------------------------------------------------
    // 2. Safe constructor strings
    //-------------------------------------------------------------
    const FString SafeShipName = Elem.Name.Left(63);
    const FString SafeRegistry = Elem.Name.Left(15);

    const FTCHARToUTF8 ShipNameUtf8(*SafeShipName);
    const FTCHARToUTF8 RegistryUtf8(*SafeRegistry);

    //-------------------------------------------------------------
    // 3. Construct Ship
    //-------------------------------------------------------------
    Ship* NewShip = new Ship(
        ShipNameUtf8.Get(),
        RegistryUtf8.Get(),
        Design,
        Elem.IFFCode,
        Elem.CommandAI,
        nullptr,
        true
    );

    if (!NewShip)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignSceneActor] Failed to create Ship '%s'"),
            *Elem.Name);

        return nullptr;
    }

    //-------------------------------------------------------------
    // 4. Runtime control setup
    //-------------------------------------------------------------
    if (Elem.Player == 1)
    {
        // CameraPod / player ship. No AI director.
        NewShip->SetNetworkControl(nullptr);
        NewShip->SetPlayerShip(Elem.Player == 1);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Player ship control setup '%s' Dir=%p NetControl=%p"),
            *Elem.Name,
            NewShip->GetDirector(),
            NewShip->GetNetworkControl());
    }
    else
    {
        // AI ships. Passing nullptr allows Ship::SetNetworkControl()
        // to create SteerAI internally.
        NewShip->SetNetworkControl(nullptr);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] AI ship control setup '%s' IFF=%d Dir=%p NetControl=%p Static=%d Starship=%d"),
            *Elem.Name,
            NewShip->GetIFF(),
            NewShip->GetDirector(),
            NewShip->GetNetworkControl(),
            NewShip->IsStatic() ? 1 : 0,
            NewShip->IsStarship() ? 1 : 0);
    }

    //-------------------------------------------------------------
    // 5. Seed runtime transform
    //-------------------------------------------------------------
    NewShip->MoveTo(WorldLoc);

    const double HeadingRad = FMath::DegreesToRadians((double)Elem.Heading);

    NewShip->SetHeading(0.0, 0.0, HeadingRad + PI);
    NewShip->SetHelmHeading(HeadingRad);

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Seeded RuntimeShip '%s' Loc=%s Heading=%d"),
        *Elem.Name,
        *WorldLoc.ToString(),
        Elem.Heading);

    //-------------------------------------------------------------
    // 6. Initial state
    //-------------------------------------------------------------
    NewShip->SetInvulnerable(Elem.Invulnerable);

    NewShip->SetFlightPhase(
        Elem.Alert ? Ship::ALERT : Ship::ACTIVE
    );

    //-------------------------------------------------------------
    // 7. Temporary movement bridge
    //-------------------------------------------------------------
    if (Elem.Navpoint.Num() > 0)
    {
        NewShip->SetThrottle(100.0);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] FORCE throttle '%s' NavPoints=%d"),
            *Elem.Name,
            Elem.Navpoint.Num());
    }

    //-------------------------------------------------------------
    // 8. Assign region
    //-------------------------------------------------------------
    Sim* SimInst = Sim::GetSim();

    if (SimInst)
    {
        SimRegion* Region = nullptr;

        if (!Elem.RegionName.IsEmpty())
        {
            Region = SimInst->FindRegion(Elem.RegionName);
        }

        if (!Region && !CurrentMissionRegionName.IsEmpty())
        {
            Region = SimInst->FindRegion(CurrentMissionRegionName);
        }

        if (!Region)
        {
            Region = SimInst->FindNearestSpaceRegionAt(WorldLoc);
        }

        if (Region)
        {
            NewShip->SetRegion(Region);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor:Region] Ship '%s' assigned to Region='%hs' Region=%p"),
                *Elem.Name,
                Region->GetName(),
                Region);
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[CampaignSceneActor:Region] No region found for ship '%s' ElemRegion='%s' MissionRegion='%s'"),
                *Elem.Name,
                *Elem.RegionName,
                *CurrentMissionRegionName);
        }
    }

    //-------------------------------------------------------------
    // 9. Player ship assignment
    //-------------------------------------------------------------
    if (!CurrentPlayerShip && Elem.Player == 1)
    {
        CurrentPlayerShip = NewShip;

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] CurrentPlayerShip SET '%s' Design='%s'"),
            *Elem.Name,
            *Elem.Design);

        SimRegion* PlayerRegion = NewShip->GetRegion();

        if (PlayerRegion)
        {
            PlayerRegion->SetPlayerShip(CurrentPlayerShip);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Region PlayerShip SET '%hs' Region='%hs'"),
                CurrentPlayerShip->GetName(),
                PlayerRegion->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("[CampaignSceneActor] Player ship '%s' has no region - cannot assign PlayerShip"),
                *Elem.Name);
        }
    }

    //-------------------------------------------------------------
    // 10. Navpoints -> Instructions
    //-------------------------------------------------------------
    for (const FS_MissionInstruction& Nav : Elem.Navpoint)
    {
        const FString RegionName =
            !Nav.OrderRegionName.IsEmpty()
            ? Nav.OrderRegionName
            : Elem.RegionName;

        Instruction* Inst = new Instruction(
            TCHAR_TO_ANSI(*RegionName),
            Nav.Location,
            INSTRUCTION_ACTION::VECTOR
        );

        Inst->SetSpeed(Nav.Speed);
        Inst->SetHoldTime((double)Nav.Hold);
        Inst->SetPriority(Nav.Priority);
        Inst->SetFarcast(Nav.Farcast);
        Inst->SetEMCON(Nav.EMCON);
        Inst->SetFormation(ResolveInstructionFormation(Nav.Formation));

        if (!Nav.StatusName.IsEmpty())
        {
            if (Nav.StatusName.Equals(TEXT("ACTIVE"), ESearchCase::IgnoreCase))
            {
                Inst->SetStatus(INSTRUCTION_STATUS::ACTIVE);
            }
            else if (Nav.StatusName.Equals(TEXT("COMPLETE"), ESearchCase::IgnoreCase))
            {
                Inst->SetStatus(INSTRUCTION_STATUS::COMPLETE);
            }
        }

        if (!Nav.TargetName.IsEmpty())
        {
            Inst->SetTarget(Nav.TargetName);
        }

        if (!Nav.TargetDesc.IsEmpty())
        {
            Inst->SetTargetDesc(TCHAR_TO_ANSI(*Nav.TargetDesc));
        }

        NewShip->AddNavPoint(Inst);

        UE_LOG(LogTemp, Warning,
            TEXT("[Nav] Ship='%s' Region='%s' Loc=%s Speed=%d Formation=%d Priority=%d"),
            *Elem.Name,
            *RegionName,
            *Nav.Location.ToString(),
            Nav.Speed,
            Nav.Formation,
            Nav.Priority);
    }

   
    //-------------------------------------------------------------
    // 11. Final log
    //-------------------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Runtime Ship CREATED '%s' Design='%s' Loc=%s Heading=%d NavPoints=%d"),
        *Elem.Name,
        *Elem.Design,
        *WorldLoc.ToString(),
        Elem.Heading,
        Elem.Navpoint.Num());

    return NewShip;
}

void ACampaignSceneActor::RegisterRuntimeShipForElement(
    const FS_MissionElement& Elem,
    Ship* RuntimeShip,
    AShipActor* ShipActor)
{
    if (!RuntimeShip || Elem.Name.IsEmpty())
    {
        return;
    }

    RuntimeShips.Add(RuntimeShip);
    RuntimeShipByElementName.Add(Elem.Name, RuntimeShip);

    if (ShipActor)
    {
        ShipActorByElementName.Add(Elem.Name, ShipActor);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Registered RuntimeShip Elem='%s' Ship=%p Actor='%s'"),
        *Elem.Name,
        RuntimeShip,
        ShipActor ? *ShipActor->GetName() : TEXT("NULL"));
}

void ACampaignSceneActor::LinkRuntimeShipCommanders(
    const TArray<FS_MissionElement>& Elements)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] LinkRuntimeShipCommanders: Elements=%d RuntimeShips=%d"),
        Elements.Num(),
        RuntimeShips.Num());

    //-------------------------------------------------------------
    // Commander / leader linking
    //-------------------------------------------------------------
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

        if (!ChildShip)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] CommanderLink SKIP child missing Elem='%s' Commander='%s'"),
                *Elem.Name,
                *Elem.Commander);

            continue;
        }

        if (!CommanderShip)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] CommanderLink SKIP commander missing Elem='%s' Commander='%s'"),
                *Elem.Name,
                *Elem.Commander);

            continue;
        }

        if (ChildShip == CommanderShip)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] CommanderLink SKIP self commander Elem='%s'"),
                *Elem.Name);

            continue;
        }

        ChildShip->SetLeader(CommanderShip);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] CommanderLink Child='%s' Leader='%s'"),
            *Elem.Name,
            *Elem.Commander);
    }

    //-------------------------------------------------------------
    // AI hostility diagnostics
    //-------------------------------------------------------------
    for (Ship* TestShip : RuntimeShips)
    {
        if (!TestShip)
        {
            continue;
        }

        for (Ship* OtherShip : RuntimeShips)
        {
            if (!OtherShip || OtherShip == TestShip)
            {
                continue;
            }

            const bool bHostile =
                TestShip->IsHostileTo(OtherShip);

            UE_LOG(LogTemp, Warning,
                TEXT("[AI TARGET TEST] Ship='%hs' IFF=%d Region=%p Other='%hs' OtherIFF=%d OtherRegion=%p Hostile=%d"),
                TestShip->GetName(),
                TestShip->GetIFF(),
                TestShip->GetRegion(),
                OtherShip->GetName(),
                OtherShip->GetIFF(),
                OtherShip->GetRegion(),
                bHostile ? 1 : 0);
        }
    }

    //-------------------------------------------------------------
    // Temporary forced target test
    //-------------------------------------------------------------
    Ship* Kitts =
        RuntimeShipByElementName.FindRef(TEXT("Kitts"));

    Ship* Lovo =
        RuntimeShipByElementName.FindRef(TEXT("Lovo"));

    Ship* Courier =
        RuntimeShipByElementName.FindRef(TEXT("Blockade Runner"));

    if (Kitts && Courier)
    {
        Kitts->SetTarget(Courier);

        UE_LOG(LogTemp, Warning,
            TEXT("[AI FORCE TARGET] Kitts -> Blockade Runner"));
    }

    if (Lovo && Courier)
    {
        Lovo->SetTarget(Courier);

        UE_LOG(LogTemp, Warning,
            TEXT("[AI FORCE TARGET] Lovo -> Blockade Runner"));
    }
}

void ACampaignSceneActor::ApplyRuntimeFormationOffsets(const TArray<FS_MissionElement>& Elements)
{
    TMap<FString, TArray<const FS_MissionElement*>> FollowersByCommander;

    for (const FS_MissionElement& Elem : Elements)
    {
        if (!Elem.Commander.IsEmpty())
        {
            FollowersByCommander.FindOrAdd(Elem.Commander).Add(&Elem);
        }
    }

    for (const TPair<FString, TArray<const FS_MissionElement*>>& Pair : FollowersByCommander)
    {
        const FString& CommanderName = Pair.Key;
        const TArray<const FS_MissionElement*>& Followers = Pair.Value;

        Ship* CommanderShip = RuntimeShipByElementName.FindRef(CommanderName);
        AShipActor* CommanderActor = ShipActorByElementName.FindRef(CommanderName);

        if (!CommanderShip || !CommanderActor)
        {
            continue;
        }

        const FVector CommanderLoc = CommanderActor->GetActorLocation();
        const FRotator CommanderRot = CommanderActor->GetActorRotation();

        for (int32 Index = 0; Index < Followers.Num(); ++Index)
        {
            const FS_MissionElement* FollowerElem = Followers[Index];

            if (!FollowerElem)
            {
                continue;
            }

            Ship* FollowerShip = RuntimeShipByElementName.FindRef(FollowerElem->Name);
            AShipActor* FollowerActor = ShipActorByElementName.FindRef(FollowerElem->Name);

            if (!FollowerShip || !FollowerActor)
            {
                continue;
            }

            const FVector LocalOffset =
                GetFormationOffsetForElement(*FollowerElem, Index, Followers.Num());

            const FVector WorldOffset =
                CommanderRot.RotateVector(LocalOffset);

            const FVector DesiredWorldLoc =
                CommanderLoc + WorldOffset;

            FollowerShip->SetFormationOffset(LocalOffset);

            FollowerShip->MoveTo(DesiredWorldLoc);
            FollowerShip->SetHelmHeading(CommanderShip->GetHelmHeading());

            FollowerActor->SetActorLocation(DesiredWorldLoc);
            FollowerActor->SetActorRotation(CommanderRot);

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