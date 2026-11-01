#include "FighterActor.h"
#include "Sim.h"
#include "FighterLandingGearComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Ship.h"
#include "LandingGear.h"
#include "ShipDesign.h"
#include "Components/PrimitiveComponent.h"
#include "Misc/PackageName.h"

AFighterActor::AFighterActor()
{
    FighterLandingGearRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLandingGearRoot"));
    FighterLandingGearRoot->SetupAttachment(PointRoot);
    LeftGear = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftGear"));
    RightGear = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightGear"));
    NoseGear = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NoseGear"));
    Cockpit = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cockpit"));
    for (UStaticMeshComponent* Mesh : {LeftGear.Get(), RightGear.Get(), NoseGear.Get(), Cockpit.Get()})
    {
        Mesh->SetupAttachment(FighterLandingGearRoot);
        Mesh->bEditableWhenInherited = true;
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
    }
    FighterCockpitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterCockpitRoot"));
    FighterCockpitRoot->SetupAttachment(PointRoot);
    FighterLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLaunchRoot"));
    FighterLaunchRoot->SetupAttachment(PointRoot);
    CockpitComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CockpitComponent"));
    CockpitComponent->SetupAttachment(FighterCockpitRoot);
    CockpitComponent->bEditableWhenInherited = true;
    CockpitComponent->SetMobility(EComponentMobility::Movable);
    Cockpit->SetupAttachment(CockpitComponent);
}

void AFighterActor::InitializeNativeFighterAttachments(const TCHAR* CockpitModel, float CockpitScale,
    const FVector& CockpitPosition, const TArray<FFighterLandingGearDefinition>& Definitions)
{
    AuthoredCockpitModel = CockpitModel;
    AuthoredCockpitScale = CockpitScale;
    FighterCockpitRoot->SetRelativeLocation(CockpitPosition);
    GearDefinitions = Definitions;
    for (int32 Index = 0; Index < Definitions.Num(); ++Index)
    {
        const FFighterLandingGearDefinition& Def = Definitions[Index];
        UStaticMeshComponent* GearMount = Index == 0 ? NoseGear.Get() : Index == 1 ? RightGear.Get() : Index == 2 ? LeftGear.Get() : nullptr;
        const FName ComponentName = GearMount
            ? FName(*(GearMount->GetName() + TEXT("Actor")))
            : FName(*FString::Printf(TEXT("LandingGear_%02d"), Index + 1));
        UFighterLandingGearComponent* Gear = CreateDefaultSubobject<UFighterLandingGearComponent>(ComponentName);
        Gear->SetupAttachment(FighterLandingGearRoot);
        Gear->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        if (GearMount) GearMount->SetupAttachment(Gear);
        if (GearMount)
        {
            // DEF positions are relative to FighterLandingGearRoot, independent of mesh mount edits.
            const FString AssetName = GearMount->GetName();
            const FString Package = FString::Printf(TEXT("/Game/Models/LandingGear/BP_%s"), *AssetName);
            if (FPackageName::DoesPackageExist(Package))
            {
                const FString ClassPath = Package + TEXT(".BP_") + AssetName + TEXT("_C");
                Gear->SetChildActorClass(LoadClass<AActor>(nullptr, *ClassPath));
            }
        }
        Gear->AuthoredIndex = Index;
        Gear->AuthoredModel = Def.Model;
        Gear->RetractedPosition = Def.RetractedPosition;
        Gear->ExtendedPosition = Def.ExtendedPosition;
        Gear->SetRelativeLocation(Def.RetractedPosition);
        LandingGearActors.Add(Gear);
    }
}

void AFighterActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SynchronizeLandingGearMarkers();
}

void AFighterActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateLandingGearPresentation();
    if(Ship* Runtime=GetRuntimeShip())
    {
        const bool Visible=Sim::GetSim() && Runtime==Sim::GetSim()->GetPlayerShip() && Runtime->IsNativeCockpitRequested();
        Cockpit->SetVisibility(Visible, true);
        CockpitComponent->SetVisibility(Visible,true);
        if(auto* Child=CockpitComponent->GetChildActor())Child->SetActorHiddenInGame(!Visible || Cockpit->GetStaticMesh());
    }
}

float AFighterActor::GetLandingGearDeployment() const
{
    if (Ship* Runtime = GetRuntimeShip())
        return Runtime->GetGear() ? static_cast<float>(Runtime->GetGear()->GetDeployment()) : 0.f;
    return FMath::IsFinite(PreviewGearDeployment) ? FMath::Clamp(PreviewGearDeployment, 0.f, 1.f) : 0.f;
}

void AFighterActor::SynchronizeLandingGearMarkers()
{
    if (Cockpit && Cockpit->GetAttachParent() != CockpitComponent)
        Cockpit->AttachToComponent(CockpitComponent, FAttachmentTransformRules::KeepRelativeTransform);
    // Native components survive reconstruction. Never replace a BP-assigned child actor class.
    for (int32 Index = 0; Index < LandingGearActors.Num(); ++Index)
    {
        UFighterLandingGearComponent* Gear = LandingGearActors[Index];
        if (!Gear) continue;
        // Correct attachment overrides serialized by existing fighter Blueprints.
        if (Gear->GetAttachParent() != FighterLandingGearRoot)
            Gear->AttachToComponent(FighterLandingGearRoot, FAttachmentTransformRules::KeepRelativeTransform);
        Gear->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        UStaticMeshComponent* Mount = Index == 0 ? NoseGear.Get() : Index == 1 ? RightGear.Get() : Index == 2 ? LeftGear.Get() : nullptr;
        if (Mount && Mount->GetAttachParent() != Gear)
            Mount->AttachToComponent(Gear, FAttachmentTransformRules::KeepRelativeTransform);
        if (LandingGearBlueprintClasses.IsValidIndex(Index) && LandingGearBlueprintClasses[Index]
            && Gear->GetChildActorClass() != LandingGearBlueprintClasses[Index])
            Gear->SetChildActorClass(LandingGearBlueprintClasses[Index]);
    }
    UpdateLandingGearPresentation();
}

void AFighterActor::UpdateLandingGearPresentation()
{
    const float Deployment = GetLandingGearDeployment();
    const LandingGear* RuntimeGear = GetRuntimeShip() ? GetRuntimeShip()->GetGear() : nullptr;
    if (RuntimeGear && LastReportedGearState != int32(RuntimeGear->GetState()))
    {
        LastReportedGearState = int32(RuntimeGear->GetState());
        UE_LOG(LogTemp, Display, TEXT("[FighterGear] Actor=%s State=%d Deployment=%.3f Components=%d"),
            *GetName(), LastReportedGearState, Deployment, LandingGearActors.Num());
    }
    // Blueprints based on the generic FighterShipActor have their own gear
    // assemblies, rather than the concrete native design's LandingGearActors.
    if (RuntimeGear && LandingGearActors.IsEmpty())
    {
        const ShipDesign* Design = GetRuntimeShip()->Design();
        const double Scale = Design && FMath::Abs(Design->scale) > SMALL_NUMBER ? Design->scale : 1.0;
        auto Position = [&](int32 Index)
        {
            const FVector P = FMath::Lerp(RuntimeGear->GetGearStartLocal(Index),
                RuntimeGear->GetGearEndLocal(Index), Deployment) / Scale;
            return FVector(P.Z, P.X, P.Y);
        };
        for (const auto& Pair : {TPair<UStaticMeshComponent*, int32>(NoseGear.Get(),0),
            TPair<UStaticMeshComponent*, int32>(LeftGear.Get(),2),
            TPair<UStaticMeshComponent*, int32>(RightGear.Get(),1)})
        {
            if (Pair.Key && Pair.Key->GetStaticMesh())
            {
                Pair.Key->SetRelativeLocation(Position(Pair.Value));
                Pair.Key->SetRelativeRotation(FRotator(0,-90,0));
            }
        }
        TArray<UChildActorComponent*> Assemblies;
        GetComponents(Assemblies);
        for (UChildActorComponent* Assembly : Assemblies)
        {
            if (Assembly == CockpitComponent) continue;
            const FString Name = Assembly->GetName() + TEXT(" ") + GetNameSafe(Assembly->GetChildActorClass());
            int32 Index = Name.Contains(TEXT("NoseGear")) ? 0 : Name.Contains(TEXT("LeftGear")) ? 2 : Name.Contains(TEXT("RightGear")) ? 1 : INDEX_NONE;
            if (Index == INDEX_NONE) continue;
            if (Assembly->GetAttachParent() != FighterLandingGearRoot)
                Assembly->AttachToComponent(FighterLandingGearRoot, FAttachmentTransformRules::KeepRelativeTransform);
            Assembly->SetRelativeLocation(Position(Index));
            Assembly->SetRelativeRotation(FRotator(0,-90,0));
            const bool Visible = RuntimeGear->IsGearVisible(Index);
            Assembly->SetVisibility(Visible, true);
            Assembly->SetHiddenInGame(!Visible, true);
            if (AActor* Child = Assembly->GetChildActor())
            {
                Child->SetActorHiddenInGame(!Visible);
                TArray<UPrimitiveComponent*> Meshes;
                Child->GetComponents(Meshes);
                for (UPrimitiveComponent* Mesh : Meshes)
                {
                    Mesh->SetVisibility(Visible, true);
                    Mesh->SetHiddenInGame(!Visible, true);
                }
            }
        }
    }
    for (UStaticMeshComponent* Mesh : {LeftGear.Get(), RightGear.Get(), NoseGear.Get()})
        if (Mesh && Mesh->GetStaticMesh())
        {
            Mesh->SetVisibility(Deployment > 0.f, true);
            Mesh->SetHiddenInGame(Deployment <= 0.f, true);
        }
    for (int32 Index = 0; Index < LandingGearActors.Num(); ++Index)
        if (UFighterLandingGearComponent* Gear = LandingGearActors[Index])
            Gear->ApplyDeployment(Deployment, !GearDefinitions.IsValidIndex(Index) || GearDefinitions[Index].bEnabled,
                !RuntimeGear || RuntimeGear->IsGearVisible(Index));
}
