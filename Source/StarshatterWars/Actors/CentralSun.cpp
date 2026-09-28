#include "CentralSun.h"

#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

ACentralSun::ACentralSun()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(
        TEXT("SceneRoot"));

    SetRootComponent(SceneRoot);

    SunComponent = CreateDefaultSubobject<UChildActorComponent>(
        TEXT("CentralSun"));

    SunComponent->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FClassFinder<AActor> StarBP(
        TEXT(
            "/Game/Space_Creator/Star_Creator/"
            "StarCreator_Update_1/Blueprints/BP_Star"
        )
    );

    if (StarBP.Succeeded())
    {
        SunComponent->SetChildActorClass(
            StarBP.Class);
    }

    SunComponent->SetRelativeLocation(
        FVector::ZeroVector);

    SunComponent->SetRelativeRotation(
        FRotator::ZeroRotator);
}

void ACentralSun::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[CentralSun] BeginPlay Sun=%s"),
        *GetNameSafe(GetSunActor()));
}

void ACentralSun::ShowSun()
{
    SetSunVisible(true);
}

void ACentralSun::HideSun()
{
    SetSunVisible(false);
}

void ACentralSun::SetSunVisible(bool bVisible)
{
    if (!SunComponent)
    {
        return;
    }

    SunComponent->SetVisibility(
        bVisible,
        true);

    if (AActor* SunActor = GetSunActor())
    {
        SunActor->SetActorHiddenInGame(
            !bVisible);
    }
}

AActor* ACentralSun::GetSunActor() const
{
    return SunComponent
        ? SunComponent->GetChildActor()
        : nullptr;
}