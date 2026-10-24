#include "FighterActor.h"
#include "Sim.h"
#include "FighterLandingGearComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ChildActorComponent.h"
#include "Ship.h"
#include "LandingGear.h"

AFighterActor::AFighterActor()
{
    FighterCockpitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterCockpitRoot"));
    FighterCockpitRoot->SetupAttachment(PointRoot);
    FighterLandingGearRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLandingGearRoot"));
    FighterLandingGearRoot->SetupAttachment(PointRoot);
    FighterLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLaunchRoot"));
    FighterLaunchRoot->SetupAttachment(PointRoot);
    CockpitActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("CockpitActor"));
    CockpitActor->SetupAttachment(FighterCockpitRoot);
    CockpitActor->bEditableWhenInherited = true;
    CockpitActor->SetMobility(EComponentMobility::Movable);
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
        UFighterLandingGearComponent* Gear = CreateDefaultSubobject<UFighterLandingGearComponent>(
            FName(*FString::Printf(TEXT("LandingGear_%02d"), Index + 1)));
        Gear->SetupAttachment(FighterLandingGearRoot);
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
        CockpitActor->SetVisibility(Visible,true);
        if(auto* Child=CockpitActor->GetChildActor())Child->SetActorHiddenInGame(!Visible);
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
    // Native components survive reconstruction. Never replace a BP-assigned child actor class.
    for (int32 Index = 0; Index < LandingGearActors.Num(); ++Index)
    {
        UFighterLandingGearComponent* Gear = LandingGearActors[Index];
        if (!Gear) continue;
        if (LandingGearBlueprintClasses.IsValidIndex(Index) && LandingGearBlueprintClasses[Index]
            && Gear->GetChildActorClass() != LandingGearBlueprintClasses[Index])
            Gear->SetChildActorClass(LandingGearBlueprintClasses[Index]);
    }
    UpdateLandingGearPresentation();
}

void AFighterActor::UpdateLandingGearPresentation()
{
    const float Deployment = GetLandingGearDeployment();
    for (int32 Index = 0; Index < LandingGearActors.Num(); ++Index)
        if (UFighterLandingGearComponent* Gear = LandingGearActors[Index])
            Gear->ApplyDeployment(Deployment, !GearDefinitions.IsValidIndex(Index) || GearDefinitions[Index].bEnabled);
}
