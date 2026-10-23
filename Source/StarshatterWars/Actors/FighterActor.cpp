#include "FighterActor.h"
#include "Components/SceneComponent.h"

AFighterActor::AFighterActor()
{
    FighterCockpitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterCockpitRoot"));
    FighterCockpitRoot->SetupAttachment(PointRoot);

    FighterLandingGearRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLandingGearRoot"));
    FighterLandingGearRoot->SetupAttachment(PointRoot);

    FighterLaunchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FighterLaunchRoot"));
    FighterLaunchRoot->SetupAttachment(PointRoot);

    LandingGear1 = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear1"));
    LandingGear1->SetupAttachment(FighterLandingGearRoot);
    LandingGear1Extended = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear1Extended"));
    LandingGear1Extended->SetupAttachment(LandingGear1);

    LandingGear2 = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear2"));
    LandingGear2->SetupAttachment(FighterLandingGearRoot);
    LandingGear2Extended = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear2Extended"));
    LandingGear2Extended->SetupAttachment(LandingGear2);

    LandingGear3 = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear3"));
    LandingGear3->SetupAttachment(FighterLandingGearRoot);
    LandingGear3Extended = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear3Extended"));
    LandingGear3Extended->SetupAttachment(LandingGear3);

    LandingGear4 = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear4"));
    LandingGear4->SetupAttachment(FighterLandingGearRoot);
    LandingGear4Extended = CreateDefaultSubobject<USceneComponent>(TEXT("LandingGear4Extended"));
    LandingGear4Extended->SetupAttachment(LandingGear4);

    GearDefinitions.SetNum(4);
}

void AFighterActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SynchronizeLandingGearMarkers();
}

void AFighterActor::SynchronizeLandingGearMarkers()
{
    USceneComponent* GearRoots[4] = { LandingGear1.Get(), LandingGear2.Get(), LandingGear3.Get(), LandingGear4.Get() };
    USceneComponent* GearEnds[4] = { LandingGear1Extended.Get(), LandingGear2Extended.Get(), LandingGear3Extended.Get(), LandingGear4Extended.Get() };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (!GearRoots[Index] || !GearEnds[Index])
        {
            continue;
        }

        const bool bDefined = GearDefinitions.IsValidIndex(Index) && GearDefinitions[Index].bEnabled;
        const FVector Start = bDefined ? GearDefinitions[Index].RetractedPosition : FVector::ZeroVector;
        const FVector End = bDefined ? GearDefinitions[Index].ExtendedPosition : FVector::ZeroVector;

        GearRoots[Index]->SetRelativeLocation(Start);
        GearEnds[Index]->SetRelativeLocation(End - Start);
    }
}
