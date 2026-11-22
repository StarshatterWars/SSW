#pragma once
#include "CoreMinimal.h"

enum class ECommercePurpose : uint8 { None, Purchase, Service, MarketSale, Brokerage };
struct FCommercePrice
{
    int64 Base=0,Tax=0,Fee=0,Total=0;
    bool bValid=false;
    int64 Net() const {return Base;}
    FText Breakdown() const;
};
namespace CommerceCharges
{
    // Catalog prices are GST-inclusive final prices, never a surprise surcharge.
    FCommercePrice Quote(int64 FinalPrice,ECommercePurpose Purpose);
    int64 BrokerFee(int64 OrderValue);
    FName TaxAccount();
    FName FeeAccount();
}
