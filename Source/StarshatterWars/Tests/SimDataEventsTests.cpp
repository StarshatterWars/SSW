#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "SimDataEvents.h"
#include "ChangeDrivenPanel.h"
#include "Styling/CoreStyle.h"
#include "GameStructs_economy.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimDataEventsTest,"Starshatter.Economy.ChangeEvents",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSimDataEventsTest::RunTest(const FString&)
{
 FEconomyState State;const FName Owner(TEXT("Fixture"));
 FEconomyLedger::CreateInventory(State,TEXT("A"),Owner,TEXT("Janus"),100);
 FEconomyLedger::CreateInventory(State,TEXT("B"),Owner,TEXT("Janus"),100);
 int32 Count=0;bool Atomic=true;
 const auto Handle=FSimDataEvents::Changed().AddLambda([&](const FEconomyState& Changed){if(&Changed==&State)++Count;});
 ON_SCOPE_EXIT {FSimDataEvents::Changed().Remove(Handle);};
 TestFalse(TEXT("Rejected inventory edit"),FEconomyLedger::Deposit(State,TEXT("Missing"),TEXT("Ore"),5,1));
 TestEqual(TEXT("Rejected edits do not dirty panels"),Count,0);
 TestTrue(TEXT("Successful deposit"),FEconomyLedger::Deposit(State,TEXT("A"),TEXT("Ore"),5,1));
 TestEqual(TEXT("Deposit produces one change"),Count,1);Count=0;
 const auto TransferHandle=FSimDataEvents::Changed().AddLambda([&](const FEconomyState& Changed){if(&Changed==&State)Atomic &= FEconomyLedger::Quantity(State,TEXT("A"),TEXT("Ore"))==3 && FEconomyLedger::Quantity(State,TEXT("B"),TEXT("Ore"))==2;});
 FEconomyLedger::Transfer(State,Owner,TEXT("A"),TEXT("B"),TEXT("Ore"),2);
 FSimDataEvents::Changed().Remove(TransferHandle);
 TestTrue(TEXT("Subscribers only see the committed transfer"),Atomic);
 TestEqual(TEXT("Nested mutations coalesce into one event"),Count,1);
 FCommodityAmount Item;Item.Item=TEXT("Ore");Item.Quantity=3;Item.UnitTonnes=1;
 FEconomyLedger::ReconcileManifest(State,TEXT("A"),{Item});Count=0;
 FEconomyLedger::ReconcileManifest(State,TEXT("A"),{Item});
 TestEqual(TEXT("Unchanged manifest emits no update"),Count,0);
 int32 Evaluations=0;
 auto Text=SNew(SChangeDrivenText).Font(FCoreStyle::GetDefaultFontStyle("Regular",12))
     .Text_Lambda([&](){++Evaluations;return FText::FromString(TEXT("UNCHANGED"));});
 const int32 InitialEvaluations=Evaluations;
 const auto Geometry=FGeometry::MakeRoot(FVector2D(100,30),FSlateLayoutTransform());
 for(int32 Frame=0;Frame<3600;++Frame)Text->Tick(Geometry,double(Frame)/60,1.f/60);
 TestEqual(TEXT("An unchanged data view makes zero simulation/text queries over 3600 UI frames"),Evaluations,InitialEvaluations);
 return true;
}
#endif
