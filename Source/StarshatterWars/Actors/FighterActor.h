#pragma once
#include "CoreMinimal.h"
#include "ShipActor.h"
#include "FighterActor.generated.h"

class UStaticMeshComponent;
class UChildActorComponent;
class UFighterLandingGearComponent;

/** Authoring data only; coordinates are unscaled ship-local (forward, right, up). */
USTRUCT(BlueprintType)
struct FFighterLandingGearDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fighter|Landing Gear") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fighter|Landing Gear") FVector RetractedPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fighter|Landing Gear") FVector ExtendedPosition = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Landing Gear") FString Model;
    FFighterLandingGearDefinition() = default;
    FFighterLandingGearDefinition(const TCHAR* InModel, const FVector& Start, const FVector& End)
        : RetractedPosition(Start), ExtendedPosition(End), Model(InModel) {}
};

/** Shared fighter presentation. Legacy Ship/LandingGear remain the flight and gear authority. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFighterActor : public AShipActor
{
    GENERATED_BODY()
public:
    AFighterActor();
    // Assign meshes and relative transforms directly in each fighter Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Meshes") TObjectPtr<UStaticMeshComponent> LeftGear;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Meshes") TObjectPtr<UStaticMeshComponent> RightGear;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Meshes") TObjectPtr<UStaticMeshComponent> NoseGear;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Meshes") TObjectPtr<UStaticMeshComponent> Cockpit;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components") TObjectPtr<USceneComponent> FighterCockpitRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components") TObjectPtr<USceneComponent> FighterLandingGearRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Components") TObjectPtr<USceneComponent> FighterLaunchRoot;
    /** Select this native component and set Child Actor Class to your cockpit BP. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Cockpit") TObjectPtr<UChildActorComponent> CockpitActor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Cockpit") FString AuthoredCockpitModel;
    /** Legacy source metadata, not an additional UE scale multiplier. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Cockpit") float AuthoredCockpitScale = 1;
    /** Exactly one native child-actor component per nested .def gear entry. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Landing Gear") TArray<TObjectPtr<UFighterLandingGearComponent>> LandingGearActors;
    /** Optional class overrides for compatibility; empty entries preserve component-level BP classes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fighter|Landing Gear") TArray<TSubclassOf<AActor>> LandingGearBlueprintClasses;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fighter|Landing Gear") TArray<FFighterLandingGearDefinition> GearDefinitions;
    /** Editor/unbound-actor preview only. Bound ships always use the legacy deployment fraction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fighter|Landing Gear", meta=(ClampMin="0", ClampMax="1")) float PreviewGearDeployment = 0;
    UFUNCTION(BlueprintPure, Category="Fighter|Landing Gear") float GetLandingGearDeployment() const;
    /** Compatibility entry point: synchronizes native components without creating markers or slots. */
    UFUNCTION(BlueprintCallable, Category="Fighter|Landing Gear") void SynchronizeLandingGearMarkers();
protected:
    /** Called once in each concrete native constructor with its authored definitions. */
    void InitializeNativeFighterAttachments(const TCHAR* CockpitModel, float CockpitScale,
        const FVector& CockpitPosition, const TArray<FFighterLandingGearDefinition>& Definitions);
    void UpdateLandingGearPresentation();
};
