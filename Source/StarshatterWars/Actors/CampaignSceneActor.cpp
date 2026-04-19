#include "CampaignSceneActor.h"

#include "SceneMeshActor.h"
#include "StarshatterShipDesignSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

#include "Engine/World.h"
#include "ShipDesignRegistry.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"

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

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Cleared scene actors"));
    }
}

FVector ACampaignSceneActor::ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const
{
    return GetActorLocation() + SceneOriginOffset + (LegacyLoc * LegacyUnitsPerKm);
}

FString ACampaignSceneActor::ResolveModelNameForDesign(const FString& DesignName) const
{
    if (DesignName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveModelNameForDesign: empty design name"));
        return FString();
    }

    const FShipDesign* ShipRow = ShipDesignRegistry::Find(DesignName);
    if (!ShipRow)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveModelNameForDesign: no ship row for design '%s'"),
            *DesignName);
        return FString();
    }

    if (ShipRow->Model.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveModelNameForDesign: row found for '%s' but Model is empty"),
            *DesignName);
        return FString();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] ResolveModelNameForDesign: Design='%s' Model='%s'"),
        *DesignName,
        *ShipRow->Model);

    return ShipRow->Model;
}

FString ACampaignSceneActor::ResolveMeshPathForDesign(const FString& DesignName) const
{
    const FString ModelName = ResolveModelNameForDesign(DesignName);
    if (ModelName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveMeshPathForDesign: no model for design '%s'"),
            *DesignName);
        return FString();
    }

    const FString MeshPath = FString::Printf(
        TEXT("/Script/Engine.StaticMesh'/Game/Models/%s/%s.%s'"),
        *ModelName,
        *ModelName,
        *ModelName);

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] ResolveMeshPathForDesign: Design='%s' Path='%s'"),
        *DesignName,
        *MeshPath);

    return MeshPath;
}

UStaticMesh* ACampaignSceneActor::ResolveStaticMeshFromPath(const FString& MeshPath) const
{
    const FString CleanPath = MeshPath.TrimStartAndEnd();

    if (CleanPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveStaticMeshFromPath: empty path"));
        return nullptr;
    }

    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *CleanPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] ResolveStaticMeshFromPath: failed to load '%s'"),
            *CleanPath);
        return nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] ResolveStaticMeshFromPath: loaded '%s'"),
        *CleanPath);

    return Mesh;
}

AActor* ACampaignSceneActor::SpawnSceneElementActor(
    const FString& ElementName,
    const FString& DesignName,
    const FVector& WorldLocation)
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

    const FString MeshPath = ResolveMeshPathForDesign(DesignName);
    if (MeshPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: no mesh path for design '%s'"),
            *DesignName);
        return nullptr;
    }

    UStaticMesh* Mesh = ResolveStaticMeshFromPath(MeshPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed mesh load Name='%s' Design='%s' Path='%s'"),
            *ElementName,
            *DesignName,
            *MeshPath);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASceneMeshActor* SpawnedActor = World->SpawnActor<ASceneMeshActor>(
        SpawnClass,
        WorldLocation,
        FRotator::ZeroRotator,
        Params);

    if (!SpawnedActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: failed spawn Name='%s' Design='%s'"),
            *ElementName,
            *DesignName);
        return nullptr;
    }

#if WITH_EDITOR
    SpawnedActor->SetActorLabel(ElementName);
#endif

    SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

    if (!SpawnedActor->SetSceneMesh(Mesh))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] SpawnSceneElementActor: SetSceneMesh failed Name='%s' Design='%s' Mesh='%s'"),
            *ElementName,
            *DesignName,
            *GetNameSafe(Mesh));
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Spawned scene mesh actor Name='%s' Design='%s' Mesh='%s' Loc=%s"),
        *ElementName,
        *DesignName,
        *GetNameSafe(Mesh),
        *WorldLocation.ToString());

    return SpawnedActor;
}

void ACampaignSceneActor::BuildSceneActorsFromMission(const FS_CampaignMission& MissionData)
{
    ClearSceneActors();

    for (const FS_MissionElement& Elem : MissionData.Element)
    {
        if (Elem.Name.IsEmpty())
        {
            continue;
        }

        const FVector WorldLoc = ConvertLegacySceneLocToWorld(Elem.Location);

        AActor* Spawned = SpawnSceneElementActor(
            Elem.Name,
            Elem.Design,
            WorldLoc);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Failed spawn Name='%s' Design='%s'"),
                *Elem.Name,
                *Elem.Design);
            continue;
        }

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = Elem.Name;
        Entry.DesignName = Elem.Design;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = WorldLoc;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] BuildSceneActorsFromMission: spawned=%d"),
        SpawnedSceneActors.Num());
}

bool ACampaignSceneActor::FindSceneActorByName(
    const FString& TargetName,
    FCampaignSceneSpawnedActor& OutEntry) const
{
    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (Entry.ElementName.Equals(TargetName, ESearchCase::IgnoreCase))
        {
            OutEntry = Entry;
            return true;
        }
    }

    return false;
}

bool ACampaignSceneActor::FocusCameraOnSceneActorByName(
    const FString& TargetName,
    const FVector& CameraOffset,
    float BlendSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }

    FCampaignSceneSpawnedActor Entry;
    if (!FindSceneActorByName(TargetName, Entry) || !Entry.Actor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Target not found '%s'"),
            *TargetName);
        return false;
    }

    const FVector Focus = Entry.Actor->GetActorLocation();

    FVector Offset = CameraOffset;
    if (Offset.IsNearlyZero())
    {
        Offset = FVector(-2500.0f, -750.0f, 1200.0f);
    }

    const FVector CamLoc = Focus + Offset;
    const FRotator CamRot = (Focus - CamLoc).Rotation();

    PC->SetInitialLocationAndRotation(CamLoc, CamRot);
    PC->SetControlRotation(CamRot);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Camera focus '%s' Blend=%.2f"),
        *TargetName,
        BlendSeconds);

    return true;
}

void ACampaignSceneActor::DumpSceneActors() const
{
    UE_LOG(LogTemp, Warning,
        TEXT("========================================"));
    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] DumpSceneActors: count=%d"),
        SpawnedSceneActors.Num());

    for (int32 i = 0; i < SpawnedSceneActors.Num(); ++i)
    {
        const FCampaignSceneSpawnedActor& Entry = SpawnedSceneActors[i];

        UE_LOG(LogTemp, Warning,
            TEXT("  [%d] Name='%s' Design='%s' Actor=%s Loc=%s"),
            i,
            *Entry.ElementName,
            *Entry.DesignName,
            *GetNameSafe(Entry.Actor),
            *Entry.SpawnLocation.ToString());
    }

    UE_LOG(LogTemp, Warning,
        TEXT("========================================"));
}