#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs_economy.h"
#include "GameStructs_products.h"
#include "BlueprintCatalogSubsystem.generated.h"
USTRUCT(BlueprintType)
struct FBlueprintOffer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Recipe;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Image = TEXT("Production");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 OriginalPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 CopyPrice = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CopyRuns = 10;
};
UCLASS()
class STARSHATTERWARS_API UBlueprintCatalogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase&) override;
    bool LoadCatalog();
    const TMap<FName, FBlueprintOffer>& GetOffers() const { return Offers; }
    bool CanPurchase(FName Recipe, bool bOriginal, FText& Reason) const;
    bool Purchase(FName Recipe, bool bOriginal, FText& Reason);
    FGuid BeginPlayerJob(const FSSWProductionFacility&, FGuid Blueprint, int32 Runs, FText& Reason);
    static FGuid PurchaseLicense(FEconomyState&, const FBlueprintOffer&, bool bOriginal);
private:
    TMap<FName, FBlueprintOffer> Offers;
};
