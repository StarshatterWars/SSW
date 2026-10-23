#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "FighterActor.generated.h"

class USceneComponent;

/** A single landing-gear assembly, in the same presentation coordinate frame as PointRoot. */
USTRUCT(BlueprintType)
struct FFighterLandingGearDefinition
{
    GENERATED_BODY()

    /** Whether this native gear slot is used by this fighter design. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fighter|Landing Gear")
    bool bEnabled = false;

    /** Legacy gear.start, converted to the ship actor's presentation frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fighter|Landing Gear")
    FVector RetractedPosition = FVector::ZeroVector;

    /** Legacy gear.end, converted to the ship actor's presentation frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fighter|Landing Gear")
    FVector ExtendedPosition = FVector::ZeroVector;
};

/** Common fighter presentation attachments. Gear physics and simulation stay with legacy Ship. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFighterActor : public AShipActor
{
    GENERATED_BODY()

public:
    AFighterActor();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Components")
    TObjectPtr<USceneComponent> FighterCockpitRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Components")
    TObjectPtr<USceneComponent> FighterLandingGearRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Components")
    TObjectPtr<USceneComponent> FighterLaunchRoot;

    /** Four fixed native gear assemblies. Unused slots remain present in the Blueprint hierarchy. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear4;

    /** End point is relative to its assembly; start point is the assembly origin. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear1Extended;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear2Extended;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear3Extended;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter|Landing Gear")
    TObjectPtr<USceneComponent> LandingGear4Extended;

    /** Set from each concrete fighter's authored .def gear.start / gear.end entries. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fighter|Landing Gear", meta = (EditFixedSize))
    TArray<FFighterLandingGearDefinition> GearDefinitions;

    /** Apply the authored gear marker coordinates; does not animate or alter simulation. */
    UFUNCTION(BlueprintCallable, Category = "Fighter|Landing Gear")
    void SynchronizeLandingGearMarkers();
};
