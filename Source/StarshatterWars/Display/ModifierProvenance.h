#pragma once
#include "RecordDetails.h"
#include "CommerceCharges.h"
struct FSSWProductionRecipe;struct FSSWProductionFacility;struct FBlueprintInstance;struct FPlayerIndustryQuote;
namespace ModifierProvenance
{
    void Price(FRecordDetails&,int64 FinalPrice,ECommercePurpose Purpose,double CatalogReference=0);
    void Industry(FRecordDetails&,const FSSWProductionRecipe&,const FSSWProductionFacility&,const FBlueprintInstance* Blueprint=nullptr,const FPlayerIndustryQuote* Quote=nullptr);
}
