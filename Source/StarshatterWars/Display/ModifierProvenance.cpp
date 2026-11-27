#include "ModifierProvenance.h"
#include "BlueprintCatalogSubsystem.h"
namespace ModifierProvenance
{
void Price(FRecordDetails& D,int64 FinalPrice,ECommercePurpose Purpose,double CatalogReference)
{
    const auto Q=CommerceCharges::Quote(FinalPrice,Purpose);if(!Q.bValid)return;
    if(CatalogReference>0)D.Fact(INVTEXT("CATALOG REFERENCE"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(CatalogReference)),INVTEXT("Authored SSW catalog reference; market supply, demand and location can change the quote."));
    D.Fact(INVTEXT("PRICE / SELLER BASE"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Q.Base)),INVTEXT("Seller share of this GST-inclusive quote, calculated using the current SSW transaction policy."));
    D.Fact(INVTEXT("PRICE / TAX"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Q.Tax)),INVTEXT("Tax authority: included in the displayed total, never added again."));
    D.Fact(INVTEXT("PRICE / SERVICE FEE"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Q.Fee)),INVTEXT("Current service or transaction fee policy; included in this total."));
    D.Fact(INVTEXT("PRICE / FINAL"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Q.Total)),INVTEXT("Seller base plus included tax and fees. The saved transaction records the actual charge."));
}
void Industry(FRecordDetails& D,const FSSWProductionRecipe& R,const FSSWProductionFacility& S,const FBlueprintInstance* BP,const FPlayerIndustryQuote* Q)
{
    D.Fact(INVTEXT("TIME / RECIPE"),FText::Format(INVTEXT("{0} S / BATCH"),FText::AsNumber(R.SecondsPerBatch)),INVTEXT("Authored SSW recipe duration before facility specialization, blueprint research and run count."));
    D.Fact(INVTEXT("TIME / FACILITY"),FText::Format(INVTEXT("x {0}"),FText::AsNumber(S.PlayerTimeMultiplier)),INVTEXT("This facility's specialization multiplier applies to player installations."));
    D.Fact(INVTEXT("MATERIALS / FACILITY"),FText::Format(INVTEXT("x {0}"),FText::AsNumber(S.PlayerMaterialMultiplier)),INVTEXT("Applied to the nominal bill before blueprint material efficiency and whole-unit rounding."));
    if(BP)D.Fact(INVTEXT("BLUEPRINT RESEARCH"),FText::Format(INVTEXT("MATERIALS -{0}% / TIME -{1}%"),FText::AsNumber(BP->MaterialEfficiency*100),FText::AsNumber(BP->TimeEfficiency*100)),INVTEXT("Research completed on this original or inherited by its licensed copy. Whole materials round up per run."));
    double Output=0;TArray<FCommodityAmount> Products;if(R.ResolveOutputs(Products))for(const auto& P:Products)Output+=P.Quantity*P.UnitTonnes;
    D.Fact(INVTEXT("YIELD / RECIPE"),FText::Format(INVTEXT("{0} T INPUT / {1} T OUTPUT"),FText::AsNumber(R.InputTonnes),FText::AsNumber(Output)),INVTEXT("Nominal recipe bill and every co-product; no EVE prices, yields or combat modifiers are used."));
    if(Q){D.Fact(INVTEXT("TIME / QUOTED TOTAL"),FText::Format(INVTEXT("{0} S"),FText::AsNumber(Q->Duration)),INVTEXT("Exact installation quote after facility, blueprint and run modifiers."));Price(D,Q->InstallationFee,ECommercePurpose::Service);}
}
}
