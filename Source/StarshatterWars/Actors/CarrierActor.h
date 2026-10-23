#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "CarrierActor.generated.h"

class USceneComponent;
class Ship;
class Hangar;
class FlightDeck;
class UCarrierFlightDeckComponent;
class UCarrierHangarComponent;

/** Unreal presentation hierarchy for a carrier. The legacy Ship owns its Hangar and FlightDeck instances. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACarrierActor : public ACapitalShipActor
{
    GENERATED_BODY()

public:
    ACarrierActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Read-only pointers; never delete or replace the simulation-owned objects. */
    Ship* GetCarrierSimulation() const;
    Hangar* GetCarrierHangar() const;
    FlightDeck* GetCarrierFlightDeck(int32 Index) const;

    /** Converts a simulation world position via the carrier's rotated and scaled UE hull frame. */
    FVector ConvertDeckWorldToUE(const FVector& LegacyWorld) const;

    /** Locked to the authored launch-slot lane while the flight-deck animation runs. */
    FVector ConvertDeckFighterToUE(Ship* Fighter);

    /** Converts a presentation position back to the legacy camera frame. */
    FVector ConvertDeckWorldToLegacyCamera(const FVector& LegacyWorld) const;
    FVector ConvertDeckFighterToLegacyCamera(Ship* Fighter);

    UFUNCTION(BlueprintPure, Category="Carrier|Runtime")
    int32 GetRuntimeFlightDeckCount() const;

    UFUNCTION(BlueprintPure, Category="Carrier|Runtime")
    bool HasRuntimeHangar() const;

    UFUNCTION(BlueprintPure, Category="Carrier|Runtime")
    bool IsRuntimeLaunchDeck(int32 Index) const;

    UFUNCTION(BlueprintPure, Category="Carrier|Runtime")
    bool IsRuntimeRecoveryDeck(int32 Index) const;

    /** Individual per-deck components. Indices match Ship::GetFlightDeck(index). */
    UFUNCTION(BlueprintPure, Category="Carrier|Decks")
    UCarrierFlightDeckComponent* GetFlightDeckComponent(int32 DeckIndex) const;

    /** Primary hangar; there is exactly one runtime Hangar per carrier. */
    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    UCarrierHangarComponent* GetHangarComponent() const;

    /** One visual hangar bay per deck; all reference the same simulation Hangar. */
    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    UCarrierHangarComponent* GetHangarBayComponent(int32 DeckIndex) const;

    /** Recreates presentation-only deck markers from the ship's authored point definitions. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Carrier|Decks")
    void RebuildCarrierPresentation();

    UFUNCTION(BlueprintPure, Category="Carrier|Decks")
    USceneComponent* GetDeckMarker(int32 DeckIndex) const;

    UFUNCTION(BlueprintPure, Category="Carrier|Decks")
    USceneComponent* GetLaunchSpotMarker(int32 DeckIndex, int32 SpotIndex) const;

    UFUNCTION(BlueprintPure, Category="Carrier|Decks")
    USceneComponent* GetRecoveryMarker(int32 DeckIndex) const;

    /** Per-carrier point groups, all attached to unscaled PointRoot and inheriting actual actor scale exactly once. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<USceneComponent> CarrierDeckRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<USceneComponent> CarrierHangarRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<USceneComponent> CarrierLaunchRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<USceneComponent> CarrierRecoveryRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<USceneComponent> CarrierCameraRoot;

    /** Runtime/authoring deck presentation objects, not just grouping roots. */
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<UCarrierFlightDeckComponent>> FlightDeckComponents;

    /** One per-deck bay; references the primary Hangar's existing simulation object. */
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<UCarrierHangarComponent>> HangarBayComponents;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TObjectPtr<UCarrierHangarComponent> HangarComponent;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<USceneComponent>> DeckMarkers;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<USceneComponent>> LaunchSpotMarkers;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<USceneComponent>> RecoveryMarkers;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Components")
    TArray<TObjectPtr<USceneComponent>> BayCameraMarkers;

protected:
    /** Native components created by a concrete carrier's C++ constructor. */
    UCarrierFlightDeckComponent* AddNativeFlightDeck(int32 DeckIndex, bool bRecovery);
    UCarrierHangarComponent* AddNativeHangarBay(int32 DeckIndex);
    void SynchronizeNativeDeckLocations();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Native Components")
    TArray<TObjectPtr<UCarrierFlightDeckComponent>> NativeFlightDecks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Native Components")
    TArray<TObjectPtr<UCarrierHangarComponent>> NativeHangarBays;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Native Components")
    TObjectPtr<UCarrierHangarComponent> NativePrimaryHangar;

private:
    // Presentation state only. Pointers are not owned; entries are retired after launch.
    TMap<Ship*, double> CapturedLaunchLanes;
    UPROPERTY(Transient)
    TArray<int32> LaunchMarkerDeckIndices;
    UPROPERTY(Transient)
    TArray<int32> LaunchMarkerSpotIndices;
    UPROPERTY(Transient)
    TArray<int32> RecoveryMarkerDeckIndices;

    void RefreshRuntimeComponentBindings();
    const Ship* LastBoundCarrier = nullptr;
    int32 LastBoundDeckCount = INDEX_NONE;

    USceneComponent* CreateCarrierMarker(USceneComponent* Parent, const FString& Name, const FVector& LocalPosition, const FRotator& LocalRotation);
    void ClearCarrierMarkers();
};
