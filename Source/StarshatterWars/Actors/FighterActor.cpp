#include "FighterActor.h"
#include "Components/SceneComponent.h"
#include "Components/ChildActorComponent.h"

AFighterActor::AFighterActor()
{
    FighterCockpitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterCockpitRoot"));
    FighterCockpitRoot->SetupAttachment(PointRoot);

    FighterLandingGearRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLandingGearRoot"));
    FighterLandingGearRoot->SetupAttachment(PointRoot);

    FighterLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLaunchRoot"));
    FighterLaunchRoot->SetupAttachment(PointRoot);


}

void AFighterActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SynchronizeLandingGearMarkers();
}

void AFighterActor::SynchronizeLandingGearMarkers()
{
    // GearDefinitions are set by concrete fighter definitions, not padded to four.
    const int32 Count = GearDefinitions.Num();
    for (USceneComponent* Marker : LandingGearExtendedMarkers)
        if (IsValid(Marker)) Marker->DestroyComponent();
    LandingGearExtendedMarkers.Empty();
    for (UChildActorComponent* Gear : LandingGearActors)
        if (IsValid(Gear)) Gear->DestroyComponent();
    LandingGearActors.Empty();

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FFighterLandingGearDefinition& Def = GearDefinitions[Index];
        UChildActorComponent* Gear = NewObject<UChildActorComponent>(this, *FString::Printf(TEXT("LandingGear_%02d"), Index + 1));
        Gear->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Gear->SetupAttachment(FighterLandingGearRoot);
        Gear->RegisterComponent();
        Gear->SetRelativeLocation(Def.RetractedPosition);
        if (LandingGearBlueprintClasses.IsValidIndex(Index) && LandingGearBlueprintClasses[Index])
            Gear->SetChildActorClass(LandingGearBlueprintClasses[Index]);
        LandingGearActors.Add(Gear);

        USceneComponent* Extended = NewObject<USceneComponent>(this, *FString::Printf(TEXT("LandingGearExtended_%02d"), Index + 1));
        Extended->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Extended->SetupAttachment(Gear);
        Extended->RegisterComponent();
        Extended->SetRelativeLocation(Def.ExtendedPosition - Def.RetractedPosition);
        LandingGearExtendedMarkers.Add(Extended);
    }
}
