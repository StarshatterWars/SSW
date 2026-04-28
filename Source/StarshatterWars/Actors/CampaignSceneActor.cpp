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
#include "ShipDesign.h"

#include "Ship.h"

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
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void ACampaignSceneActor::BeginPlay()
{
    Super::BeginPlay();
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

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Cleared scene actors"));
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

    const float ModelYawFix = 90.0f;

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

    int32 Count = 0;

    ASystemSceneBuilder* Builder = ResolveSystemSceneBuilder();

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
        // RUNTIME SHIP BINDING (THIS IS THE KEY ADDITION)
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
                    TEXT("[CampaignSceneActor] Ship bound '%s' ? Actor '%s'"),
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
    // PASS 2: COMMANDER LINKING (CRITICAL)
    //-------------------------------------------------------------
    LinkRuntimeShipCommanders(MissionData.Element);

    //-------------------------------------------------------------
    // FINAL LOG
    //-------------------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] BuildSceneActorsFromMission COMPLETE Count=%d"),
        Count);
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

    const float Radius = FMath::Max(Extent.Size(), 800.0f);
    const float Distance = FMath::Clamp(Radius * 4.0f, 1800.0f, 6500.0f);

    const FVector LocalOffset =
        FVector(-Distance, Distance * 0.45f, Distance * 0.30f);

    Cam->SetGroupFollowView(
        GroupActors,
        LocalOffset,
        VelocityDir,
        0.0f);

    Cam->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] CommanderGroup FOLLOW '%s' Count=%d Center=%s Radius=%.2f Offset=%s"),
        *CommanderName,
        GroupActors.Num(),
        *Center.ToString(),
        Radius,
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
    // 3. Construct Ship (NO AI for cutscene/runtime)
    //-------------------------------------------------------------
    Ship* NewShip = new Ship(
        ShipNameUtf8.Get(),
        RegistryUtf8.Get(),
        Design,
        Elem.IFFCode,
        Elem.CommandAI,
        nullptr,
        false
    );

    if (!NewShip)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignSceneActor] Failed to create Ship '%s'"),
            *Elem.Name);
        return nullptr;
    }

    //-------------------------------------------------------------
    // 4. CRITICAL: Seed runtime transform
    //-------------------------------------------------------------
    NewShip->MoveTo(WorldLoc);
    NewShip->SetHelmHeading((double)Elem.Heading);

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Seeded RuntimeShip '%s' Loc=%s Heading=%d"),
        *Elem.Name,
        *WorldLoc.ToString(),
        Elem.Heading);

    //-------------------------------------------------------------
    // 5. Initial state
    //-------------------------------------------------------------
    NewShip->SetInvulnerable(Elem.Invulnerable);

    NewShip->SetFlightPhase(
        Elem.Alert ? Ship::ALERT : Ship::ACTIVE
    );

    //-------------------------------------------------------------
    // 6. Navpoints -> Instructions
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

        //---------------------------------------------------------
        // Core movement
        //---------------------------------------------------------
        Inst->SetSpeed(Nav.Speed);
        Inst->SetHoldTime((double)Nav.Hold);

        //---------------------------------------------------------
        // Tactical settings
        //---------------------------------------------------------
        Inst->SetPriority(Nav.Priority);
        Inst->SetFarcast(Nav.Farcast);
        Inst->SetEMCON(Nav.EMCON);

        //---------------------------------------------------------
        // Formation
        //---------------------------------------------------------
        Inst->SetFormation(ResolveInstructionFormation(Nav.Formation));

        //---------------------------------------------------------
        // Status
        //---------------------------------------------------------
        if (!Nav.StatusName.IsEmpty())
        {
            if (Nav.StatusName.Equals("ACTIVE", ESearchCase::IgnoreCase))
            {
                Inst->SetStatus(INSTRUCTION_STATUS::ACTIVE);
            }
            else if (Nav.StatusName.Equals("COMPLETE", ESearchCase::IgnoreCase))
            {
                Inst->SetStatus(INSTRUCTION_STATUS::COMPLETE);
            }
        }

        //---------------------------------------------------------
        // Targeting
        //---------------------------------------------------------
        if (!Nav.TargetName.IsEmpty())
        {
            Inst->SetTarget(Nav.TargetName);
        }

        if (!Nav.TargetDesc.IsEmpty())
        {
            Inst->SetTargetDesc(TCHAR_TO_ANSI(*Nav.TargetDesc));
        }

        //---------------------------------------------------------
        // Add to ship
        //---------------------------------------------------------
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
    // 7. Final log
    //-------------------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Runtime Ship CREATED '%s' Loc=%s Heading=%d NavPoints=%d"),
        *Elem.Name,
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

void ACampaignSceneActor::LinkRuntimeShipCommanders(const TArray<FS_MissionElement>& Elements)
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

        Ship* ChildShip = RuntimeShipByElementName.FindRef(Elem.Name);
        Ship* CommanderShip = RuntimeShipByElementName.FindRef(Elem.Commander);

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

        /*
            Temporary clean bridge:
            Ship.h currently exposes SetWard(Ship*) but not SetLeader(Ship*).
            Use SetWard for now as the runtime relationship hook.

            Later, if you expose SetLeader or SimElement-based formation logic,
            swap this single line only.
        */
        ChildShip->SetWard(CommanderShip);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] CommanderLink Child='%s' Commander='%s'"),
            *Elem.Name,
            *Elem.Commander);
    }
}


