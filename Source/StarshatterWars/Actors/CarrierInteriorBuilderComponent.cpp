#include "CarrierInteriorBuilderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

UCarrierInteriorBuilderComponent::UCarrierInteriorBuilderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    BlockMesh = Cube.Object;
    bEditableWhenInherited = true;
}

void UCarrierInteriorBuilderComponent::LoadOrionStarterLayout()
{
    Modify();
    Bays.Reset();
    // Floor elevations and lane centers use Orion.def after (right,up,forward)->(forward,right,up).
    // Room lengths/widths/heights are editable starter architecture, not DEF-authored room bounds.
    for (int32 Index = 0; Index < 4; ++Index)
    {
        FCarrierInteriorBay Bay;
        Bay.Name = FName(*FString::Printf(TEXT("Deck_%d_%s"), Index+1, Index<2 ? TEXT("LaunchHangar") : TEXT("RecoveryHangar")));
        const float Side = Index%2 == 0 ? 1.f : -1.f;
        if (Index < 2)
        {
            Bay.FloorTransform.SetLocation(FVector(650, Side*60, -20));
            Bay.Size = FVector(400,90,70);
            Bay.FrontOpening = FVector2D(80,60);
            Bay.RearOpening = FVector2D(80,60);
        }
        else
        {
            Bay.FloorTransform.SetLocation(FVector(-650, Side*305, -12));
            Bay.Size = FVector(400,120,80);
            // The aft opening admits the original -1950 -> -500 recovery path.
            Bay.RearOpening = FVector2D(110,70);
            Bay.FrontOpening = FVector2D(90,65);
        }
        Bays.Add(Bay);
    }
    if (bAutoRebuild && IsRegistered()) RebuildInterior();
}

void UCarrierInteriorBuilderComponent::LoadOrionFullInteriorLayout()
{
    {
        TGuardValue<bool> RebuildGuard(bAutoRebuild,false);
        LoadOrionStarterLayout();
        // Keep assigned assets and edited transforms when revisiting this preset.
        auto AddAnchor = [&](const TCHAR* Name, const FVector& Position)
        {
            const FName AnchorName(Name);
            if (Modules.ContainsByPredicate([&](const FCarrierInteriorModule& Existing) { return Existing.Name == AnchorName; })) return;
            FCarrierInteriorModule Module;
            Module.Name = AnchorName;
            Module.Transform.SetLocation(Position);
            Module.bAnchorOnly = true;
            Module.bFitMeshToSize = false;
            Module.bStarterAnchor = true;
            Modules.Add(Module);
        };
        // Bridge/drive/reactor references follow Orion's authored structural points.
        // Other rooms are conceptual placement anchors for the user's modular kit.
        AddAnchor(TEXT("Bridge_Room"), FVector(600,0,100));
        AddAnchor(TEXT("Engineering_Room"), FVector(-1240,0,48));
        AddAnchor(TEXT("Reactor_Room"), FVector(-180,0,-48));
        AddAnchor(TEXT("Main_Corridor"), FVector(100,0,-20));
        AddAnchor(TEXT("Crew_Quarters"), FVector(150,150,15));
        AddAnchor(TEXT("Medbay"), FVector(200,-150,15));
        AddAnchor(TEXT("Flight_Control"), FVector(885,0,0));
        AddAnchor(TEXT("Pilot_Ready_Room"), FVector(750,0,-20));
        AddAnchor(TEXT("Cargo_And_Stores"), FVector(-850,0,-48));
        AddAnchor(TEXT("Port_Recovery_Transfer"), FVector(-500,305,-12));
        AddAnchor(TEXT("Starboard_Recovery_Transfer"), FVector(-500,-305,-12));
        AddAnchor(TEXT("Vertical_Access"), FVector(-300,0,-20));
    }
    if (bAutoRebuild && IsRegistered()) RebuildInterior();
}

void UCarrierInteriorBuilderComponent::RemoveGeneratedPieces()
{
    // The tag lets reconstruction find old generated pieces even if a transient array was reset.
    if (AActor* Owner = GetOwner())
    {
        TArray<USceneComponent*> Existing;
        Owner->GetComponents(Existing);
        const FName BuilderTag(*FString::Printf(TEXT("InteriorBuilder_%s"), *GetFName().ToString()));
        for (USceneComponent* Piece : Existing)
            if (IsValid(Piece) && Piece->ComponentHasTag(BuilderTag)) Piece->DestroyComponent();
    }
    GeneratedPieces.Reset();
}

void UCarrierInteriorBuilderComponent::AddPiece(const FString& Label, const FTransform& Frame,
    const FVector& Center, const FVector& Dimensions, UMaterialInterface* Material, UStaticMesh* Mesh)
{
    Mesh = Mesh ? Mesh : BlockMesh.Get();
    if (!GetOwner() || !Mesh || Dimensions.ContainsNaN() || Dimensions.GetMin() <= 0) return;
    const FVector MeshDimensions = Mesh->GetBoundingBox().GetSize();
    if (MeshDimensions.GetMin() <= KINDA_SMALL_NUMBER) return;
    UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(GetOwner(),
        MakeUniqueObjectName(GetOwner(), UStaticMeshComponent::StaticClass(), FName(*Label)));
    Piece->CreationMethod = EComponentCreationMethod::UserConstructionScript;
    Piece->ComponentTags.Add(FName(*FString::Printf(TEXT("InteriorBuilder_%s"), *GetFName().ToString())));
    Piece->SetupAttachment(this);
    Piece->SetMobility(EComponentMobility::Movable);
    Piece->SetStaticMesh(Mesh);
    // Size is a bounding-box size; account for imported mesh pivots without an extra actor-scale factor.
    const FVector LocalScale = Dimensions / MeshDimensions;
    const FVector PivotOffset = -Mesh->GetBoundingBox().GetCenter() * LocalScale;
    const FTransform Local(Frame.GetRotation(), Frame.TransformPosition(Center + PivotOffset), Frame.GetScale3D() * LocalScale);
    Piece->SetRelativeTransform(Local);
    if (Material) Piece->SetMaterial(0, Material);
    Piece->SetCollisionEnabled(bWalkingCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Piece->SetCollisionProfileName(bWalkingCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Piece->SetGenerateOverlapEvents(false);
    Piece->RegisterComponent();
    GeneratedPieces.Add(Piece);
}

void UCarrierInteriorBuilderComponent::BuildEndWall(const FCarrierInteriorBay& Bay, float Direction, const FVector2D& Opening)
{
    const float Width = Bay.Size.Y, Height = Bay.Size.Z, Thickness = Bay.Thickness;
    const float DoorWidth = FMath::Clamp(float(Opening.X), 0.f, Width);
    const float DoorHeight = FMath::Clamp(float(Opening.Y), 0.f, Height);
    const float X = Direction * (Bay.Size.X*0.5f + Thickness*0.5f);
    const FString Prefix = Bay.Name.ToString() + (Direction>0 ? TEXT("_Front") : TEXT("_Rear"));
    if (DoorWidth <= 0 || DoorHeight <= 0)
    {
        AddPiece(Prefix, Bay.FloorTransform, FVector(X,0,Height*0.5f), FVector(Thickness,Width,Height), WallMaterial, WallMesh);
        return;
    }
    const float JambWidth = (Width-DoorWidth)*0.5f;
    if (JambWidth > KINDA_SMALL_NUMBER)
        for (float Side : {-1.f,1.f})
            AddPiece(Prefix+FString::SanitizeFloat(Side), Bay.FloorTransform,
                FVector(X,Side*(DoorWidth+JambWidth)*0.5f,Height*0.5f), FVector(Thickness,JambWidth,Height), WallMaterial, WallMesh);
    if (DoorHeight < Height)
        AddPiece(Prefix+TEXT("_Lintel"), Bay.FloorTransform, FVector(X,0,(Height+DoorHeight)*0.5f),
            FVector(Thickness,DoorWidth,Height-DoorHeight), WallMaterial, WallMesh);
}

void UCarrierInteriorBuilderComponent::RebuildInterior()
{
    if (bRebuilding || !GetOwner() || !GetWorld() ||
        GetOwner()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return;
    TGuardValue<bool> Guard(bRebuilding,true);
    RemoveGeneratedPieces();
    if (!bBuildInterior) return;
    for (const FCarrierInteriorBay& Bay : Bays)
    {
        if (!Bay.bEnabled || Bay.Size.ContainsNaN() || Bay.Size.GetMin() <= 0 ||
            !FMath::IsFinite(Bay.Thickness) || Bay.Thickness <= 0 || Bay.FloorTransform.ContainsNaN()) continue;
        const float T = Bay.Thickness;
        AddPiece(Bay.Name.ToString()+TEXT("_Floor"), Bay.FloorTransform, FVector(0,0,-T*0.5f), FVector(Bay.Size.X,Bay.Size.Y,T), FloorMaterial, FloorMesh);
        if (Bay.bCeiling && bShowCeilings)
            AddPiece(Bay.Name.ToString()+TEXT("_Ceiling"), Bay.FloorTransform, FVector(0,0,Bay.Size.Z+T*0.5f), FVector(Bay.Size.X,Bay.Size.Y,T), CeilingMaterial, CeilingMesh);
        if (Bay.bLeftWall)
            AddPiece(Bay.Name.ToString()+TEXT("_Left"), Bay.FloorTransform, FVector(0,-(Bay.Size.Y+T)*0.5f,Bay.Size.Z*0.5f), FVector(Bay.Size.X,T,Bay.Size.Z), WallMaterial, WallMesh);
        if (Bay.bRightWall)
            AddPiece(Bay.Name.ToString()+TEXT("_Right"), Bay.FloorTransform, FVector(0,(Bay.Size.Y+T)*0.5f,Bay.Size.Z*0.5f), FVector(Bay.Size.X,T,Bay.Size.Z), WallMaterial, WallMesh);
        BuildEndWall(Bay,1,Bay.FrontOpening);
        BuildEndWall(Bay,-1,Bay.RearOpening);
    }
    for (const FCarrierInteriorModule& Module : Modules)
    {
        if (!Module.bEnabled || Module.Transform.ContainsNaN()) continue;
        if (Module.RoomBlueprint || (Module.bAnchorOnly && !Module.Mesh))
        {
            USceneComponent* Placement = nullptr;
            const FName PieceName = MakeUniqueObjectName(GetOwner(), USceneComponent::StaticClass(), Module.Name);
            if (Module.RoomBlueprint)
            {
                UChildActorComponent* Prefab = NewObject<UChildActorComponent>(GetOwner(), PieceName);
                Prefab->SetChildActorClass(Module.RoomBlueprint);
                Placement = Prefab;
            }
            else
            {
                UArrowComponent* Anchor = NewObject<UArrowComponent>(GetOwner(), PieceName);
                Anchor->SetHiddenInGame(true);
                Anchor->ArrowColor = FColor(70,170,255);
                Placement = Anchor;
            }
            Placement->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            Placement->ComponentTags.Add(FName(*FString::Printf(TEXT("InteriorBuilder_%s"), *GetFName().ToString())));
            Placement->SetupAttachment(this);
            Placement->SetRelativeTransform(Module.Transform);
            Placement->RegisterComponent();
            if (UChildActorComponent* Prefab = Cast<UChildActorComponent>(Placement))
                if (AActor* Room = Prefab->GetChildActor())
                {
                    TArray<USceneComponent*> RoomComponents;
                    Room->GetComponents(RoomComponents);
                    for (USceneComponent* RoomComponent : RoomComponents)
                        RoomComponent->SetMobility(EComponentMobility::Movable);
                }
            GeneratedPieces.Add(Placement);
        }
        else
        {
            const FVector DesiredSize = !Module.bFitMeshToSize && Module.Mesh
                ? Module.Mesh->GetBoundingBox().GetSize() : Module.Size;
            AddPiece(Module.Name.ToString(), Module.Transform, FVector::ZeroVector, DesiredSize, Module.Material, Module.Mesh);
        }
    }
}

void UCarrierInteriorBuilderComponent::ClearInterior()
{
    Modify();
    bBuildInterior = false;
    RemoveGeneratedPieces();
}

void UCarrierInteriorBuilderComponent::BakeToEditableActors()
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || World->WorldType != EWorldType::Editor || !Owner || GeneratedPieces.IsEmpty()) return;
    const FScopedTransaction Transaction(NSLOCTEXT("CarrierInterior", "Bake", "Bake carrier interior modules"));
    Owner->Modify(); Modify(); Owner->GetLevel()->Modify();
    TArray<AActor*> NewActors;
    bool bFailed = false;
    for (USceneComponent* Piece : GeneratedPieces)
    {
        if (!IsValid(Piece)) continue;
        FActorSpawnParameters Params;
        Params.ObjectFlags = RF_Transactional;
        Params.OverrideLevel = Owner->GetLevel();
        UStaticMeshComponent* SourceMesh = Cast<UStaticMeshComponent>(Piece);
        UChildActorComponent* SourcePrefab = Cast<UChildActorComponent>(Piece);
        if (!SourceMesh && !SourcePrefab) continue; // Empty placement anchors have no architecture to bake.
        const FTransform BakedTransform = SourcePrefab && SourcePrefab->GetChildActor()
            ? SourcePrefab->GetChildActor()->GetActorTransform() : Piece->GetComponentTransform();
        UClass* ActorClass = AStaticMeshActor::StaticClass();
        if (SourcePrefab) ActorClass = SourcePrefab->GetChildActorClass().Get();
        AActor* Baked = ActorClass ? World->SpawnActor(ActorClass, &BakedTransform, Params) : nullptr;
        if (!Baked) { bFailed = true; break; }
        NewActors.Add(Baked);
        Baked->Modify();
        Baked->SetActorLabel(TEXT("OrionInterior_")+Piece->GetName());
        if (SourceMesh)
        {
            UStaticMeshComponent* Mesh = CastChecked<AStaticMeshActor>(Baked)->GetStaticMeshComponent();
            Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(SourceMesh->GetStaticMesh());
            for (int32 Index=0; Index<SourceMesh->GetNumMaterials(); ++Index) Mesh->SetMaterial(Index,SourceMesh->GetMaterial(Index));
            Mesh->SetCollisionProfileName(SourceMesh->GetCollisionProfileName());
        }
        else
        {
            TArray<USceneComponent*> RoomComponents;
            Baked->GetComponents(RoomComponents);
            for (USceneComponent* RoomComponent : RoomComponents)
                RoomComponent->SetMobility(EComponentMobility::Movable);
        }
        Baked->AttachToActor(Owner,FAttachmentTransformRules::KeepWorldTransform);
        Baked->MarkPackageDirty();
    }
    if (bFailed)
    {
        for (AActor* Baked : NewActors) Baked->Destroy();
        UE_LOG(LogTemp, Warning, TEXT("[OrionInterior] Bake cancelled; generated preview retained."));
        return;
    }
    if (NewActors.IsEmpty()) return;
    ClearInterior();
    Owner->MarkPackageDirty();
#endif
}

void UCarrierInteriorBuilderComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    RemoveGeneratedPieces();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UCarrierInteriorBuilderComponent::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    if (IsValid(this) && bAutoRebuild && IsRegistered()) RebuildInterior();
}
#endif
