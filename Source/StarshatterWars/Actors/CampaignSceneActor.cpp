#include "CampaignSceneActor.h"

#include "SceneMeshActor.h"
#include "SystemSceneBuilder.h"

#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesignRegistry.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

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
    const FString& ModelName,
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

    TSubclassOf<ASceneMeshActor> SpawnClass = DefaultSceneMeshActorClass;

    if (!SpawnClass)
    {
        SpawnClass = ASceneMeshActor::StaticClass();

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: DefaultSceneMeshActorClass is null, using native ASceneMeshActor"));
    }

    const FString MeshPath = FString::Printf(
        TEXT("/Script/Engine.StaticMesh'/Game/Models/%s/%s.%s'"),
        *ModelName,
        *ModelName,
        *ModelName);

    if (MeshPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: empty mesh path for model '%s'"),
            *ModelName);
        return nullptr;
    }

    UStaticMesh* Mesh = ResolveStaticMeshFromPath(MeshPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed mesh load Name='%s' Model='%s' Path='%s'"),
            *ElementName,
            *ModelName,
            *MeshPath);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const FRotator SpawnRotation(0.0f, (float)HeadingDegrees, 0.0f);

    ASceneMeshActor* SpawnedActor = World->SpawnActor<ASceneMeshActor>(
        SpawnClass,
        WorldLocation,
        SpawnRotation,
        Params);

    if (!SpawnedActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed spawn Name='%s' Model='%s'"),
            *ElementName,
            *ModelName);
        return nullptr;
    }

#if WITH_EDITOR
    SpawnedActor->SetActorLabel(ElementName);
#endif

    SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

    if (!SpawnedActor->SetSceneMesh(Mesh))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: SetSceneMesh failed Name='%s' Model='%s' Mesh='%s'"),
            *ElementName,
            *ModelName,
            *GetNameSafe(Mesh));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Spawned scene mesh actor Name='%s' Model='%s' Mesh='%s' Loc=%s Heading=%d"),
        *ElementName,
        *ModelName,
        *GetNameSafe(Mesh),
        *WorldLocation.ToString(),
        HeadingDegrees);

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