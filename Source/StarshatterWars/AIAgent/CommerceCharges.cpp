#include "CommerceCharges.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FRates{int64 Tax=200,Service=100,Broker=100;};
const FRates& Rates()
{
    static const FRates Result=[]()
    {
        FRates Out;FString Json;TSharedPtr<FJsonObject> Root;double Version=0;
        const auto Read=[&](const TCHAR* Key,int64& Into)
        {double Value=0;if(!Root->TryGetNumberField(Key,Value) || !FMath::IsFinite(Value) || Value<0 || Value>10000 || Value!=FMath::FloorToDouble(Value))return false;Into=int64(Value);return true;};
        if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/transaction_fees.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root ||
            !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
            !Read(TEXT("GstBasisPoints"),Out.Tax) || !Read(TEXT("ServiceFeeBasisPoints"),Out.Service) || !Read(TEXT("BrokerFeeBasisPoints"),Out.Broker))
        {UE_LOG(LogTemp,Error,TEXT("[CommerceCharges] Invalid transaction_fees.json; using safe default rates."));return FRates();}
        return Out;
    }();return Result;
}
// Quotient/remainder multiplication avoids integer overflow even at the credit limit.
int64 Fraction(int64 Value,int64 Numerator,int64 Denominator)
{return (Value/Denominator)*Numerator+((Value%Denominator)*Numerator)/Denominator;}
}
FCommercePrice CommerceCharges::Quote(int64 Price,ECommercePurpose Purpose)
{
    FCommercePrice Out;if(Price<0)return Out;Out.Total=Price;Out.bValid=true;
    const auto& Policy=Rates();
    const int64 Tax=Purpose==ECommercePurpose::None?0:Policy.Tax;
    const int64 Fee=Purpose==ECommercePurpose::Service?Policy.Service:0;
    const int64 Denominator=10000+Tax+Fee;
    Out.Tax=Fraction(Price,Tax,Denominator);Out.Fee=Fraction(Price,Fee,Denominator);
    Out.Base=Price-Out.Tax-Out.Fee;
    if(Purpose==ECommercePurpose::Brokerage){Out.Fee+=Out.Base;Out.Base=0;}
    return Out;
}
int64 CommerceCharges::BrokerFee(int64 Value)
{return Value>0 && Rates().Broker>0?FMath::Max(int64(1),Fraction(Value,Rates().Broker,10000)):0;}
FName CommerceCharges::TaxAccount(){return TEXT("Revenue Authority / GST");}
FName CommerceCharges::FeeAccount(){return TEXT("Station service fees");}
FText FCommercePrice::Breakdown() const
{
    return FText::Format(INVTEXT("BASE COST: {0} CR\nINCLUDED GST: {1} CR\nINCLUDED SERVICE FEES: {2} CR\nFINAL TOTAL: {3} CR\nWhole-credit rounding; total is the exact amount charged."),
        FText::AsNumber(Base),FText::AsNumber(Tax),FText::AsNumber(Fee),FText::AsNumber(Total));
}
