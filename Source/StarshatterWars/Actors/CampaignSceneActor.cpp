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

        const FString Name = ANSI_TO_TCHAR(Elem->GetName());
        const FString Region = ANSI_TO_TCHAR(Elem->GetRegion());

        const FShipDesign* Ship = Elem->GetShipDesign();
        if (!Ship)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Elem '%s' has no ship design"),
                *Name);
            continue;
        }

        const FString ModelName = Ship->Model;
        if (ModelName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Elem '%s' has empty model"),
                *Name);
            continue;
        }

        FS_MissionElement Temp;
        Temp.Name = Name;
        Temp.Design = ModelName;
        Temp.RegionName = Region;
        Temp.Location = Elem->GetLocation();
        Temp.Heading = (int32)Elem->GetHeading();

        const FVector WorldLoc = ConvertMissionElementLocToWorld(Temp);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Runtime Elem Name='%s' Model='%s' Region='%s' Local=%s World=%s Heading=%d"),
            *Temp.Name,
            *Temp.Design,
            *Temp.RegionName,
            *Temp.Location.ToString(),
            *WorldLoc.ToString(),
            Temp.Heading);

        AActor* Spawned = SpawnSceneElementActor(
            Temp.Name,
            Temp.Design,
            WorldLoc,
            Temp.Heading);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Failed spawn Name='%s' Model='%s'"),
                *Temp.Name,
                *Temp.Design);
            continue;
        }

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = Temp.Name;
        Entry.DesignName = Temp.Design;
        Entry.RegionName = Temp.RegionName;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = WorldLoc;
        Entry.HeadingDegrees = Temp.Heading;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Spawned actors = %d"),
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
            TEXT("Name='%s' Design='%s' Actor=%s"),
            *E.ElementName,
            *E.DesignName,
            *GetNameSafe(E.Actor));
    }
}

void ACampaignSceneActor::BuildSceneActorsFromMission(const FS_CampaignMission& MissionData)
{
    ClearSceneActors();

    int32 Count = 0;

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

        const FVector WorldLoc = ConvertMissionElementLocToWorld(Elem);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] FS Elem Name='%s' Design='%s' Model='%s' Region='%s' Local=%s World=%s Heading=%d"),
            *ElementName,
            *DesignName,
            *ModelName,
            *RegionName,
            *Elem.Location.ToString(),
            *WorldLoc.ToString(),
            Elem.Heading);

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

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = ElementName;
        Entry.DesignName = ModelName;
        Entry.RegionName = RegionName;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = WorldLoc;
        Entry.HeadingDegrees = Elem.Heading;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        Count++;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] BuildSceneActorsFromMission: spawned=%d"),
        Count);
}

