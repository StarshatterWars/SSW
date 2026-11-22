#include "CreditPricing.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

int64 CreditPricing::CreditsPerUsd()
{
    static const int64 Rate=[]()
    {
        FString Json;
        TSharedPtr<FJsonObject> Root;
        double Value=0,Version=0;
        if(FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/credit_pricing.json"))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) && Root &&
            Root->TryGetNumberField(TEXT("schemaVersion"),Version) && Version==1 &&
            Root->TryGetNumberField(TEXT("CreditsPerUsd"),Value) && FMath::IsFinite(Value) &&
            Value>=1 && Value<=1000000000 && Value==FMath::FloorToDouble(Value))return int64(Value);
        UE_LOG(LogTemp,Error,TEXT("[CreditPricing] Invalid credit_pricing.json; using 1000 credits per USD."));
        return int64(1000);
    }();
    return Rate;
}

double CreditPricing::UsdEquivalent(int64 Credits)
{
    return double(Credits)/double(CreditsPerUsd());
}
