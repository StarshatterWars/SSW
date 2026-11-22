#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "MissionRewardRules.h"
#include "CreditPricing.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreditPricingBalanceTest,"Starshatter.Economy.CreditPricingTrainingProgression",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCreditPricingBalanceTest::RunTest(const FString&)
{
    TestEqual(TEXT("Store credit conversion loads from JSON"),CreditPricing::CreditsPerUsd(),int64(1000));
    TestEqual(TEXT("Normal skin reference value"),CreditPricing::UsdEquivalent(5000),5.0);
    FString Json;
    TArray<TSharedPtr<FJsonValue>> Ships;
    if(!TestTrue(TEXT("Hull prices load"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Ships)))return false;
    int64 Stormhawk=0;
    for(const auto& Row:Ships)
        if(Row->AsObject()->GetStringField(TEXT("Row"))==TEXT("Stormhawk"))Stormhawk=int64(Row->AsObject()->GetNumberField(TEXT("BaseCost")));
    TestEqual(TEXT("Training fighter has the new credit price"),Stormhawk,int64(57400));
    const auto& Rewards=FMissionRewardCatalog::Get();
    const auto T1=Rewards.Quote(1,101,true,0,0),T2=Rewards.Quote(1,102,true,0,0);
    TestTrue(TEXT("Training pays credits without requiring optional bonuses"),T1.Base>0 && T2.Base>0);
    TestTrue(TEXT("New pilot can buy required T3 hull"),5000+T1.Base+T2.Base>=Stormhawk);
    TestEqual(TEXT("Safe first two lessons leave a useful reserve after purchase"),5000+T1.Total()+T2.Total()-Stormhawk,int64(5000));
    TestTrue(TEXT("Required hull is earned through both lessons"),5000+T1.Base<Stormhawk);
    TestTrue(TEXT("Recovery bonus rewards safe return"),T1.Recovery>0 && T2.Recovery>0);
    return true;
}
#endif
