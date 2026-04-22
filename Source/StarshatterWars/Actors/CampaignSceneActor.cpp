#include "CampaignSceneActor.h"

#include "SceneMeshActor.h"
#include "SystemSceneBuilder.h"

#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

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

    const FRotator SpawnRotation(0.0f, (float)HeadingDegrees, 0.0f);

    /*
     * ---------------------------------------------------------
     * 1. Try Blueprint first
     * ---------------------------------------------------------
     *
     * Convention:
     *     DesignName = Courier
     *     Blueprint  = /Game/Models/Courier/BP_Courier.BP_Courier_C
     */
    {
        const FString BPClassPath = FString::Printf(
            TEXT("/Game/Models/%s/BP_%s.BP_%s_C"),
            *ResolvedName,
            *ResolvedName,
            *ResolvedName);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Trying BP: %s"),
            *BPClassPath);

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
                UE_LOG(LogTemp, Warning,
                    TEXT("[CampaignSceneActor] SpawnSceneElementActor: BP class found but spawn failed Name='%s' Design='%s' ClassPath='%s'"),
                    *ElementName,
                    *ResolvedName,
                    *BPClassPath);
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

            float FinalScale = MissionElementScaleMultiplier;

            /*
             * Per-model override for assets that import at a radically different size.
             */
            if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
            {
                FinalScale *= 4.0f;
            }

            SpawnedBPActor->SetActorScale3D(FVector(FinalScale));

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BP Spawned PostScale=%s"),
                *SpawnedBPActor->GetActorScale3D().ToString());

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] SpawnSceneElementActor: spawned Blueprint actor Name='%s' Design='%s' Class='%s' Loc=%s Heading=%d Scale=%.2f"),
                *ElementName,
                *ResolvedName,
                *GetNameSafe(BPClass),
                *WorldLocation.ToString(),
                HeadingDegrees,
                FinalScale);

            return SpawnedBPActor;
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BP NOT FOUND: %s"),
                *BPClassPath);
        }
    }

    /*
     * ---------------------------------------------------------
     * 2. Fallback to existing static mesh actor flow
     * ---------------------------------------------------------
     */
    TSubclassOf<ASceneMeshActor> SpawnClass = DefaultSceneMeshActorClass;

    if (!SpawnClass)
    {
        SpawnClass = ASceneMeshActor::StaticClass();

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: DefaultSceneMeshActorClass is null, using native ASceneMeshActor"));
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
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed mesh load Name='%s' Model='%s' Path='%s'"),
            *ElementName,
            *ResolvedName,
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
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed static mesh actor spawn Name='%s' Model='%s'"),
            *ElementName,
            *ResolvedName);
        return nullptr;
    }

#if WITH_EDITOR
    SpawnedActor->SetActorLabel(ElementName);
#endif

    SpawnedActor->AttachToActor(
        this,
        FAttachmentTransformRules::KeepWorldTransform);

    if (!SpawnedActor->SetSceneMesh(Mesh))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: SetSceneMesh failed Name='%s' Model='%s' Mesh='%s'"),
            *ElementName,
            *ResolvedName,
            *GetNameSafe(Mesh));
    }

    UStaticMeshComponent* MeshComp = SpawnedActor->GetMeshComponent();
    if (!MeshComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: MeshComponent missing for '%s'"),
            *ElementName);
        return SpawnedActor;
    }

    float FinalScale = MissionElementScaleMultiplier;

    /*
     * Per-model override for assets that import at a radically different size.
     */
    if (ResolvedName.Equals(TEXT("Farcaster"), ESearchCase::IgnoreCase))
    {
        FinalScale *= 4.0f;
    }

    SpawnedActor->SetActorScale3D(FVector(FinalScale));

    MeshComp->SetVisibility(true, true);
    MeshComp->SetHiddenInGame(false, true);
    MeshComp->SetComponentTickEnabled(false);
    MeshComp->SetCastShadow(true);

    SpawnedActor->SetActorHiddenInGame(false);
    SpawnedActor->SetActorEnableCollision(false);

    const FBoxSphereBounds LocalBounds = Mesh->GetBounds();
    const FBox WorldBox = MeshComp->Bounds.GetBox();

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] SpawnSceneElementActor: spawned static mesh actor Name='%s' Model='%s' Mesh='%s' Loc=%s Heading=%d Scale=%.2f LocalOrigin=%s LocalExtent=%s WorldCenter=%s WorldExtent=%s"),
        *ElementName,
        *ResolvedName,
        *GetNameSafe(Mesh),
        *WorldLocation.ToString(),
        HeadingDegrees,
        FinalScale,
        *LocalBounds.Origin.ToString(),
        *LocalBounds.BoxExtent.ToString(),
        *WorldBox.GetCenter().ToString(),
        *WorldBox.GetExtent().ToString());

    return SpawnedActor;
}

void ACampaignSceneActor::BuildSceneActorsFromRuntimeMission(Mission* MissionPtr)
{
    ClearSceneActors();

    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] BuildSceneActorsFromRuntimeMission: MissionPtr is null"));
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
                TEXT("[CampaignSceneActor] BuildSceneActorsFromRuntimeMission: Elem '%s' has no ship design"),
                *ElementName);
            continue;
        }

        const FString ModelName = Ship->Model;
        if (ModelName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BuildSceneActorsFromRuntimeMission: Elem '%s' has empty model"),
                *ElementName);
            continue;
        }

        const FVector RelativeLoc = Elem->GetLocation();
        const int32 Heading = (int32)Elem->GetHeading();

        AActor* RegionActor = FindRegionActorByName(RegionName);

        FVector RegionCenter = FVector::ZeroVector;
        const bool bHasRegionCenter = ResolveRegionCenterLocation(RegionName, RegionCenter);

        const FVector WorldLoc = bHasRegionCenter
            ? (RegionCenter + RelativeLoc)
            : ConvertLegacySceneLocToWorld(RelativeLoc);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Runtime Elem Name='%s' Model='%s' Region='%s' Rel=%s World=%s Heading=%d Parent=%s"),
            *ElementName,
            *ModelName,
            *RegionName,
            *RelativeLoc.ToString(),
            *WorldLoc.ToString(),
            Heading,
            *GetNameSafe(RegionActor));

        AActor* Spawned = SpawnSceneElementActor(
            ElementName,
            ModelName,
            WorldLoc,
            Heading);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BuildSceneActorsFromRuntimeMission: failed spawn Name='%s' Model='%s'"),
                *ElementName,
                *ModelName);
            continue;
        }

        if (RegionActor)
        {
            Spawned->AttachToActor(
                RegionActor,
                FAttachmentTransformRules::KeepWorldTransform);

            Spawned->SetActorRelativeLocation(RelativeLoc);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Runtime attach '%s' -> Region='%s' Parent=%s Rel=%s"),
                *ElementName,
                *RegionName,
                *GetNameSafe(RegionActor),
                *RelativeLoc.ToString());
        }

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
        TEXT("[CampaignSceneActor] BuildSceneActorsFromRuntimeMission: spawned=%d"),
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
                TEXT("[CampaignSceneActor] BuildSceneActorsFromMission: '%s' has empty model/design"),
                *ElementName);
            continue;
        }

        AActor* RegionActor = nullptr;
        FVector RegionCenter = FVector::ZeroVector;
        bool bHasRegionCenter = false;

        if (Builder && !RegionName.IsEmpty())
        {
            FSpawnedSystemRegion Region;
            if (Builder->GetRegionByName(RegionName + TEXT("_REGION"), Region) ||
                Builder->GetRegionByName(RegionName, Region))
            {
                RegionActor = Region.Actor;
                RegionCenter = Region.Actor ? Region.Actor->GetActorLocation() : Region.SpawnLocation;
                bHasRegionCenter = true;
            }
            else if (Builder->GetBodyWorldLocationByName(RegionName, RegionCenter))
            {
                bHasRegionCenter = true;
                RegionActor = nullptr;
            }
        }

        const FVector RelativeLoc = Elem.Location;
        const FVector WorldLoc = bHasRegionCenter
            ? (RegionCenter + RelativeLoc)
            : ConvertMissionElementLocToWorld(Elem);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] FS Elem Name='%s' Design='%s' Model='%s' Region='%s' Rel=%s World=%s Heading=%d Parent=%s"),
            *ElementName,
            *DesignName,
            *ModelName,
            *RegionName,
            *RelativeLoc.ToString(),
            *WorldLoc.ToString(),
            Elem.Heading,
            *GetNameSafe(RegionActor));

        AActor* Spawned = SpawnSceneElementActor(
            ElementName,
            ModelName,
            WorldLoc,
            Elem.Heading);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] BuildSceneActorsFromMission: failed spawn Name='%s' Model='%s'"),
                *ElementName,
                *ModelName);
            continue;
        }

        if (RegionActor)
        {
            Spawned->AttachToActor(
                RegionActor,
                FAttachmentTransformRules::KeepWorldTransform);

            Spawned->SetActorRelativeLocation(RelativeLoc);

            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Attached '%s' to region '%s' Parent=%s Rel=%s"),
                *ElementName,
                *RegionName,
                *GetNameSafe(RegionActor),
                *RelativeLoc.ToString());
        }

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = ElementName;
        Entry.DesignName = ModelName;
        Entry.RegionName = RegionName;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = Spawned->GetActorLocation();
        Entry.HeadingDegrees = Elem.Heading;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] BuildSceneActorsFromMission: spawned=%d"),
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
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneActor] FocusCameraOnSceneActorByName: World is null"));
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneActor] FocusCameraOnSceneActorByName: PC is null"));
        return false;
    }

    AActor* TargetActor = FindSceneActorByName(ElementName);
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneActor] FocusCameraOnSceneActorByName: actor not found '%s'"),
            *ElementName);
        return false;
    }

    FVector SceneOffset = CameraOffset;
    if (SceneOffset.IsNearlyZero())
    {
        SceneOffset = FVector(0.0f, 0.0f, -2500.0f);
    }

    const FVector FocusPoint = TargetActor->GetActorLocation();

    // Rotate the offset using the full event rotator:
    const FVector RotatedOffset = CameraRotator.RotateVector(SceneOffset);

    const FVector CameraLocation = FocusPoint + RotatedOffset;

    // Always look back at the target from the final camera position:
    const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

    PC->SetInitialLocationAndRotation(CameraLocation, CameraRotation);
    PC->SetControlRotation(CameraRotation);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneActor Camera] Target='%s' Focus=%s RawOffset=%s RotatedOffset=%s EventRotator=%s Camera=%s Rotation=%s Blend=%.2f"),
        *ElementName,
        *FocusPoint.ToString(),
        *SceneOffset.ToString(),
        *RotatedOffset.ToString(),
        *CameraRotator.ToString(),
        *CameraLocation.ToString(),
        *CameraRotation.ToString(),
        BlendSeconds);

    return true;
}