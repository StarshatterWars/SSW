#pragma once
#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "FighterLandingGearComponent.generated.h"

/** One authored gear assembly. The child class is assigned directly in the fighter Blueprint. */
UCLASS(ClassGroup=(Starshatter), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UFighterLandingGearComponent : public UChildActorComponent
{
    GENERATED_BODY()
public:
    UFighterLandingGearComponent();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Landing Gear|Definition") FString AuthoredModel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Landing Gear|Definition") int32 AuthoredIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landing Gear|Animation") FVector RetractedPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landing Gear|Animation") FVector ExtendedPosition = FVector::ZeroVector;
    /** Disable translation if the child Blueprint animates its own strut; Deployment still updates. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landing Gear|Animation") bool bAnimateTranslation = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landing Gear|Animation") bool bHideWhenRetracted = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Landing Gear|Animation") float Deployment = 0;
    void ApplyDeployment(float Fraction, bool bEnabled = true);
};
