#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CarrierFlightDeckComponent.generated.h"

class FlightDeck;

/** One Unreal presentation component per legacy carrier FlightDeck. No simulation ownership. */
UCLASS(ClassGroup=(Starshatter, Carrier), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UCarrierFlightDeckComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UCarrierFlightDeckComponent();
    void BindFlightDeck(FlightDeck* InDeck, int32 InIndex);

    UFUNCTION(BlueprintPure, Category="Carrier|Flight Deck")
    int32 GetDeckIndex() const { return DeckIndex; }
    UFUNCTION(BlueprintPure, Category="Carrier|Flight Deck")
    bool IsLaunchDeck() const;
    UFUNCTION(BlueprintPure, Category="Carrier|Flight Deck")
    bool IsRecoveryDeck() const;
    UFUNCTION(BlueprintPure, Category="Carrier|Flight Deck")
    int32 GetSlotCount() const;

    /** Raw, non-owning pointer. Only valid while the current simulation ship exists. */
    FlightDeck* GetRuntimeDeck() const { return RuntimeDeck; }
private:
    UPROPERTY(VisibleAnywhere, Category="Carrier|Flight Deck")
    int32 DeckIndex = INDEX_NONE;
    FlightDeck* RuntimeDeck = nullptr;
};
