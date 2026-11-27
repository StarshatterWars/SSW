#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs_economy.h"
#include "GameStructs_products.h"
#include "GameStructs_skills.h"
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
// Read-only installation preview; amounts already include runs and blueprint efficiencies.
struct FPlayerIndustryQuote
{
    FName Input, Output;
    TArray<FCommodityAmount> Materials, Products;
    TArray<FCommodityAmount> NominalMaterials, NominalProducts;
    double NominalDuration = 0;
    double Duration = 0;
    int64 InstallationFee = 0;
    FName ServiceProfile;
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
    bool RequestPurchase(FName Recipe,bool Original,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    bool RequestBeginPlayerJob(const FSSWProductionFacility&,FGuid Blueprint,int32 Runs,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    bool RequestCancelPlayerJob(FGuid Job,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    bool CanBlueprintService(const FSSWProductionFacility&,FGuid Blueprint,FName Activity,int32 Runs,double& Seconds,int64& Price,FText&)const;
    bool RequestBlueprintService(const FSSWProductionFacility&,FGuid Blueprint,FName Activity,int32 Runs,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    bool CanBeginPlayerJob(const FSSWProductionFacility&, FGuid Blueprint, int32 Runs, FPlayerIndustryQuote&, FText& Reason) const;
    static bool MeetsRecipeSkills(const FSSWProductionRecipe&, const TMap<FName, FSkillProgress>* PilotSkills, FText& Reason,
        const TMap<FName, FSkillDefinition>* SkillCatalog = nullptr);
    static bool QuotePlayerJob(const FEconomyState&, const FSSWProductionFacility&, const FSSWProductionRecipe&, FGuid Blueprint, int32 Runs, FPlayerIndustryQuote&, FText& Reason,
        const TMap<FName, FSkillProgress>* PilotSkills = nullptr, const TMap<FName, FSkillDefinition>* SkillCatalog = nullptr);
    bool CancelPlayerJob(FGuid Job, FText& Reason);
    FGuid BeginPlayerJob(const FSSWProductionFacility&, FGuid Blueprint, int32 Runs, FText& Reason);
    static FGuid PurchaseLicense(FEconomyState&, const FBlueprintOffer&, bool bOriginal);
private:
    TMap<FName, FBlueprintOffer> Offers;
    FBlueprintServiceTerms ServiceTerms;
};
