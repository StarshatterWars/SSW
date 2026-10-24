#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CarrierHangarComponent.generated.h"

class Hangar;

/** Snapshot only: the simulation Hangar owns these states and service timers. */
USTRUCT(BlueprintType)
struct FCarrierHangarSlotStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Squadron = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 Slot = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FString SquadronName;
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(BlueprintReadOnly) int32 LegacyState = 0;
    UPROPERTY(BlueprintReadOnly) float SecondsRemaining = 0;
    UPROPERTY(BlueprintReadOnly) bool bCanPrepare = false;
    UPROPERTY(BlueprintReadOnly) bool bInTurnaround = false;
};

/** One component for the legacy Hangar, or an individual deck's presentation bay. */
UCLASS(ClassGroup=(Starshatter, Carrier), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UCarrierHangarComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UCarrierHangarComponent();

    /** DeckIndex INDEX_NONE means the actual carrier hangar, otherwise a bay within it. */
    void BindHangar(Hangar* InHangar, int32 InDeckIndex);

    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    int32 GetAssociatedDeckIndex() const { return AssociatedDeckIndex; }
    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    bool IsPrimaryHangar() const { return AssociatedDeckIndex == INDEX_NONE; }
    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    bool HasRuntimeHangar() const { return RuntimeHangar != nullptr; }

    UFUNCTION(BlueprintPure, Category="Carrier|Hangar")
    TArray<FCarrierHangarSlotStatus> GetSlotStatuses() const;

    /** Non-owning; per-deck bays refer to the SAME carrier Hangar, not copies. */
    Hangar* GetRuntimeHangar() const { return RuntimeHangar; }
private:
    UPROPERTY(VisibleAnywhere, Category="Carrier|Hangar")
    int32 AssociatedDeckIndex = INDEX_NONE;
    Hangar* RuntimeHangar = nullptr;
};
