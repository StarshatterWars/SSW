#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ProductionCatalog.h"
#include "ProductionEvents.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundProductionTest,"Starshatter.Industry.GroundFactoriesStorageAndDeadlines",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGroundProductionTest::RunTest(const FString&)
{
    FString Error;if(!TestTrue(TEXT("Ground profiles load from JSON"),SSWProductionCatalog::Reload(Error)))return false;
    TestEqual(TEXT("Every existing building type reviewed"),SSWProductionCatalog::BuildingProfiles().Num(),30);
    auto* Save=NewObject<UPlayerSaveGame>();auto& State=Save->PlayerInfo.Economy;
    FGroundFactorySite Site;Site.BuildingDesign=TEXT("Fact00");Site.Facility=TEXT("Assembly 1");Site.System=TEXT("Janus");Site.Planet=TEXT("Janek");Site.Region=TEXT("Janek");Site.Empire=TEXT("Terellian Alliance");
    TestTrue(TEXT("Native ground building becomes a persistent facility"),SSWProductionCatalog::RegisterGround(State,Site));
    TestTrue(TEXT("Mission reload is idempotent"),SSWProductionCatalog::RegisterGround(State,Site));
    TestEqual(TEXT("No duplicate factory"),State.GroundFactories.Num(),1);
    FSSWProductionFacility Facility;SSWProductionCatalog::ResolveGround(Site,Facility);
    const FName Id=SSWProductionCatalog::GroundKey(Site),Input=FEconomyRuntime::Warehouse(Site.System,Facility.Facility,TEXT("Input")),Output=FEconomyRuntime::Warehouse(Site.System,Facility.Facility,TEXT("Output"));
    TMap<FName,FSSWProductionFacility> Sites;Sites.Add(Id,Facility);
    const auto& Recipes=SSWProductionCatalog::Recipes();const auto* Recipe=Recipes.Find(Facility.Recipe);if(!TestNotNull(TEXT("Assembly recipe exists"),Recipe))return false;
    EconomyAI Controller;State.EconomyClockSeconds=1;Controller.AdvanceProduction(State,Sites,Recipes);
    TestEqual(TEXT("No fabricated input"),State.Jobs.Num(),0);
    for(const auto& Material:Recipe->Inputs)TestTrue(TEXT("Delivered input accepted"),FEconomyLedger::Deposit(State,Input,Material.Item,Material.Quantity,Material.UnitTonnes));
    int32 Starts=0,Stops=0,Finishes=0;
    const auto Handle=ProductionEvents::Changed().AddLambda([&](FName,FGuid,EProductionEvent Phase,const FText&){Starts+=Phase==EProductionEvent::Started;Stops+=Phase==EProductionEvent::Stopped;Finishes+=Phase==EProductionEvent::Completed;});
    Controller.WakeProduction();Controller.AdvanceProduction(State,Sites,Recipes);
    if(!TestEqual(TEXT("Arrival event starts one job"),State.Jobs.Num(),1)){ProductionEvents::Changed().Remove(Handle);return false;}
    TestEqual(TEXT("Start notification"),Starts,1);const double Deadline=State.Jobs[0].CompletionAt;
    State.EconomyClockSeconds+=1;const double Remaining=State.Jobs[0].Remaining;Controller.AdvanceProduction(State,Sites,Recipes);
    TestEqual(TEXT("No per-tick countdown mutation"),State.Jobs[0].Remaining,Remaining);
    State.OfflineFacilities.Add(Id);Controller.WakeProduction();Controller.AdvanceProduction(State,Sites,Recipes);TestEqual(TEXT("Stop notification"),Stops,1);
    State.EconomyClockSeconds=Deadline+100;Controller.AdvanceProduction(State,Sites,Recipes);TestEqual(TEXT("Destroyed factory produces nothing"),FEconomyLedger::Quantity(State,Output,DataTableUtils::MakeValidName(Recipe->CargoItem)),0.0);
    TArray<uint8> Bytes;TestTrue(TEXT("Factory identities and loss save"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(TestNotNull(TEXT("Economy reloads"),Loaded)){TestEqual(TEXT("Factory persisted"),Loaded->PlayerInfo.Economy.GroundFactories.Num(),1);TestTrue(TEXT("Destruction persists"),Loaded->PlayerInfo.Economy.OfflineFacilities.Contains(Id));}
    State.OfflineFacilities.Remove(Id);Controller.WakeProduction();Controller.AdvanceProduction(State,Sites,Recipes);
    State.EconomyClockSeconds=State.Jobs[0].CompletionAt+1;Controller.AdvanceProduction(State,Sites,Recipes);TestEqual(TEXT("Scheduled completion event"),Finishes,1);
    TestTrue(TEXT("Finished goods credited only on completion"),FEconomyLedger::Quantity(State,Output,DataTableUtils::MakeValidName(Recipe->CargoItem))>0);
    ProductionEvents::Changed().Remove(Handle);
    FGroundFactorySite Tank=Site;Tank.BuildingDesign=TEXT("Tank00");Tank.Facility=TEXT("Cryo 1");FSSWProductionFacility Storage;TestTrue(TEXT("Tank resolves as storage"),SSWProductionCatalog::ResolveGround(Tank,Storage));
    const FName TankId=SSWProductionCatalog::GroundKey(Tank),Stock=FEconomyRuntime::Warehouse(Tank.System,Storage.Facility,TEXT("Stock"));Sites.Add(TankId,Storage);
    Controller.WakeProduction();Controller.AdvanceProduction(State,Sites,Recipes);
    const FName Hydrogen=DataTableUtils::MakeValidName(TEXT("liquid hydrogen"));
    TestTrue(TEXT("Cryogenic tank accepts liquid hydrogen"),FEconomyLedger::Deposit(State,Stock,Hydrogen,5,1));
    TestFalse(TEXT("Cryogenic tank rejects manufactured crates"),FEconomyLedger::Deposit(State,Stock,TEXT("avionics"),1,1));
    TestFalse(TEXT("Tank capacity enforced"),FEconomyLedger::Deposit(State,Stock,Hydrogen,Storage.CapacityTonnes,1));
    State.OfflineFacilities.Add(TankId);Controller.WakeProduction();Controller.AdvanceProduction(State,Sites,Recipes);TestFalse(TEXT("Destroyed tank refuses deliveries"),FEconomyLedger::Deposit(State,Stock,Hydrogen,1,1));
    return true;
}
#endif
