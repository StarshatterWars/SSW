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

#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

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
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float ModelYawFix = 90.0f;

    const FRotator SpawnRotation(
        0.0f,
        (float)HeadingDegrees + ModelYawFix,
        0.0f);

    // -----------------------------------------
    // 1. Try Blueprint first
    // -----------------------------------------
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
                TEXT("[CampaignSceneActor] BP Spawned PreScale=%s"),
                *SpawnedBPActor->GetActorScale3D().ToString());


            // -----------------------------------------
            if (!SpawnedBPActor->IsA(APlanetActor::StaticClass()))
            {
                float FinalScale = MissionElementScaleMultiplier;

                if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
                {
                    FinalScale *= 4.0f;
                }

                SpawnedBPActor->SetActorScale3D(FVector(FinalScale));

                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] BP PostScale=%s"),
                    *SpawnedBPActor->GetActorScale3D().ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] PlanetActor detected — skipping MissionElementScaleMultiplier"));
            }

            return SpawnedBPActor;
        }
    }

    // -----------------------------------------
    // 2. Static mesh fallback
    // -----------------------------------------
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
    if (!MeshComp)
    {
        return SpawnedActor;
    }

    // -----------------------------------------
    if (!SpawnedActor->IsA(APlanetActor::StaticClass()))
    {
        float FinalScale = MissionElementScaleMultiplier;

        if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
        {
            FinalScale *= 4.0f;
        }

        SpawnedActor->SetActorScale3D(FVector(FinalScale));
    }

    MeshComp->SetVisibility(true, true);
    MeshComp->SetHiddenInGame(false, true);

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
                    Nav ? *Nav->Location().ToString() : TEXT("NULL"),
                    Nav ? Nav->Speed() : -1);

                if (Nav)
                {
                    const FVector StartLegacy = Elem->GetLocation();
                    const FVector TargetLegacy = Nav->Location();
                    const float Speed = Nav->Speed() > 0 ? (float)Nav->Speed() : 1000.0f;

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

        const FShipDesign* Ship = ShipDesignRegistry::Find(DesignName);
        if (Ship && !Ship->Model.IsEmpty())
        {
            ModelName = Ship->Model;
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
        // SCALE CONVERSION
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

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Spawn '%s' Region='%s' Local=%s World=%s"),
            *ElementName,
            *RegionName,
            *LocalOffset.ToString(),
            *WorldLoc.ToString());

        //--------------------------------------------------
        // SPAWN
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
        // SHIP MOVEMENT (FIXED SCALE)
        //--------------------------------------------------

        if (AShipActor* ShipActor = Cast<AShipActor>(Spawned))
        {
            if (Elem.Navpoint.Num() > 0)
            {
                const FS_MissionInstruction& Nav = Elem.Navpoint[0];

                const FVector StartLocal = LocalOffset;

                const FVector TargetLocal =
                    ConvertLegacyRegionOffsetToSceneOffset(Nav.Location);

                const float Speed =
                    Nav.Speed > 0
                    ? (float)Nav.Speed
                    : ((Ship && Ship->Vlimit > 0.0f)
                        ? Ship->Vlimit
                        : 1000.0f);

                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] NAV '%s' Start=%s Target=%s Speed=%.2f"),
                    *ElementName,
                    *StartLocal.ToString(),
                    *TargetLocal.ToString(),
                    Speed);

                ShipActor->SetCutsceneLocalMovement(
                    StartLocal,
                    TargetLocal,
                    Speed);
            }
        }

        //--------------------------------------------------
        // TRACK
        //--------------------------------------------------

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = ElementName;
        Entry.DesignName = ModelName;
        Entry.RegionName = RegionName;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = Spawned->GetActorLocation();
        Entry.HeadingDegrees = Elem.Heading;
        Entry.CommanderName = Elem.Commander;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

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

bool ACampaignSceneActor::FocusCameraOnSceneActorGroup(
    const TArray<FString>& ElementNames,
    float BlendSeconds) const
{
    ASSWCameraManager* Cam = ResolveSSWCameraManager();
    if (!Cam)
    {
        return false;
    }

    FBox GroupBox(ForceInit);

    int32 FoundCount = 0;

    for (const FString& Name : ElementNames)
    {
        AActor* Actor = FindSceneActorByName(Name);
        if (!Actor)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SceneActor Camera] Group target missing '%s'"),
                *Name);
            continue;
        }

        GroupBox += Actor->GetActorLocation();
        FoundCount++;
    }

    if (FoundCount <= 0 || !GroupBox.IsValid)
    {
        return false;
    }

    const FVector Center = GroupBox.GetCenter();
    const float Radius = FMath::Max(GroupBox.GetExtent().Size(), 1500.0f);

    const FVector CameraLocation =
        Center + FVector(-Radius * 2.5f, Radius * 1.1f, Radius * 0.75f);

    const FRotator CameraRotation =
        (Center - CameraLocation).Rotation();

    Cam->SetStaticView(CameraLocation, CameraRotation);
    Cam->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] GroupCamera Count=%d Center=%s Radius=%.2f Cam=%s"),
        FoundCount,
        *Center.ToString(),
        Radius,
        *CameraLocation.ToString());

    return true;
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

    //--------------------------------------------------
    // FIND COMMANDER + SUBORDINATES
    //--------------------------------------------------

    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (!Entry.Actor)
        {
            continue;
        }

        if (Entry.ElementName.Equals(CommanderName, ESearchCase::IgnoreCase))
        {
            GroupActors.Add(Entry.Actor);
        }
        else
        {
            // match commander relationship
            const FString Commander = GetCommanderForElement(Entry.ElementName);

            if (Commander.Equals(CommanderName, ESearchCase::IgnoreCase))
            {
                GroupActors.Add(Entry.Actor);
            }
        }
    }

    if (GroupActors.Num() <= 0)
    {
        return false;
    }

    //--------------------------------------------------
    // BUILD GROUP BOX
    //--------------------------------------------------

    FBox Box(ForceInit);

    for (AActor* Actor : GroupActors)
    {
        Box += Actor->GetActorLocation();
    }

    const FVector Center = Box.GetCenter();
    float Radius = FMath::Max(Box.GetExtent().Size(), 1500.0f);

    //--------------------------------------------------
    // CAMERA
    //--------------------------------------------------

    const FVector CamLoc =
        Center + FVector(-Radius * 2.5f, Radius * 1.2f, Radius * 0.8f);

    const FRotator CamRot =
        (Center - CamLoc).Rotation();

    Cam->SetStaticView(CamLoc, CamRot);
    Cam->ActivateCamera(BlendSeconds);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] CommanderGroup '%s' Count=%d Center=%s"),
        *CommanderName,
        GroupActors.Num(),
        *Center.ToString());

    return true;
}