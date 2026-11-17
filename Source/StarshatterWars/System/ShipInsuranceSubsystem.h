#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs_insurance.h"
#include "ShipInsuranceSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UShipInsuranceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static FShipInsuranceQuote Quote(const FPlayerHangarShip& Ship, EShipInsuranceTier Tier);
    static EShipInsuranceTier EffectiveTier(const FPlayerHangarShip& Ship, int64 Now);
    static FString TierName(EShipInsuranceTier Tier);
    void RefreshAssignment();
    void BeginMission();
    bool RememberLoadout(const TArray<int32>& Stations, const FString& Name);
    bool CanModifyHull(FGuid Hull) const;
    const TArray<FPlayerHangarShip>& GetShips() const;
    const FPlayerHangarShip* FindShip(FGuid Hull) const;
    UFUNCTION(BlueprintCallable) FGuid RegisterOwnedShip(FName Design, const FString& Name,
        int64 HullValue, int64 ComponentValue, bool bPurchasedWithCash);
    UFUNCTION(BlueprintCallable) bool PurchasePolicy(FGuid Hull, EShipInsuranceTier Tier, FText& Result);
    UFUNCTION(BlueprintCallable) bool SettleLoss(FGuid Hull, FText& Result);
    UFUNCTION(BlueprintCallable) bool SelectHull(FGuid Hull, FText& Result);
    bool IsSelected(FGuid Hull) const;
    bool ValidateFitting(FGuid Hull, int32 Slot, int32 Level, FText& Result) const;
    UFUNCTION(BlueprintCallable) bool FitComponent(FGuid Hull, int32 Slot, int32 Level, FText& Result);
    int64 GetBalance() const;
    bool CanInsure() const;
private:
    bool bRestoreLoadout = false;
};
