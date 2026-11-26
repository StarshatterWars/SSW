#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "CarrierFighterSalesAI.h"
#include "ShipyardAI.h"
#include "SerializedHullStockAI.h"
#include "ShipDesignRegistry.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "FlightOpsAccessRules.h"
#include "Ship.h"
#include "DataLoader.h"
#include "EconomyRuntime.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarrierFighterSalesTest,"Starshatter.StationServices.CarrierTrainingFighterSales",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCarrierFighterSalesTest::RunTest(const FString&)
{
    const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
    FShipDesign Hull;Hull.ShipName=TEXT("Stormhawk");Hull.DisplayName=TEXT("Stormhawk");Hull.ShipType=int32(CLASSIFICATION::FIGHTER);Hull.Category=EShipCategory::Fighter;Hull.ShipEmpire=EShipEmpire::Terellian;Hull.BaseCost=574000;Hull.Mass=20;Hull.Integrity=100;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Stormhawk")),Hull);
    for(const TCHAR* Name:{TEXT("Falcon"),TEXT("Eagle"),TEXT("Thunderbolt"),TEXT("Zodiac")})
    {auto Other=Hull;Other.ShipName=Name;Other.DisplayName=Name;if(Other.ShipName==TEXT("Zodiac")){Other.Category=EShipCategory::Shuttle;Other.ShipType=int32(CLASSIFICATION::LCA);}ShipDesignRegistry::RegisterDesign(FName(Name),Other);}
    FShipDesign Carrier;Carrier.ShipName=TEXT("Orion");Carrier.ShipType=int32(CLASSIFICATION::CARRIER);Carrier.Mass=1000;Carrier.Integrity=10000;
    FShipSquadron Assigned;Assigned.Name=TEXT("Mission Assigned");Assigned.DesignName=TEXT("Stormhawk");Assigned.Count=Assigned.Avail=2;Carrier.Squadron.Add(Assigned);
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Orion")),Carrier);
    Ship Base("Archon","",ShipDesignRegistry::FindLegacy(FName(TEXT("Orion"))),1,0,nullptr,false);
    Base.InitializeRuntimeHangarFromDesign();if(!TestNotNull(TEXT("Carrier hangar available"),Base.GetHangar()))return false;
    Ship Observer("Fighter pilot","",ShipDesignRegistry::FindLegacy(FName(TEXT("Stormhawk"))),1,0,nullptr,false);
    TestFalse(TEXT("A fighter pilot cannot command its carrier"),FlightOpsAccessRules::CanCommand(&Observer,&Base));
    TestFalse(TEXT("No active pilot means read-only Flight Ops"),FlightOpsAccessRules::CanCommand(nullptr,&Base));
    TestFalse(TEXT("A fighter cannot become a command carrier"),FlightOpsAccessRules::CanCommand(&Observer,&Observer));
    TestTrue(TEXT("Piloting this carrier permits its own flight orders"),FlightOpsAccessRules::CanCommand(&Base,&Base));
    FS_PlayerGameInfo Pilot;Pilot.Campaign=1;Pilot.Empire=int32(EEMPIRE_NAME::Terellian);Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),425000);
    auto Found=CarrierFighterSalesAI::Offer(Pilot,Base,TEXT("Stormhawk"));if(!TestTrue(TEXT("Authored Stormhawk offer"),Found.IsSet()))return false;
    const auto* Offer=&Found.GetValue();
    TestEqual(TEXT("All five OOB aircraft designs are offered"),CarrierFighterSalesAI::Offers(Pilot,Base).Num(),5);
    TestEqual(TEXT("Stormhawk stock sums both OOB squadrons"),Offer->Quantity,48);
    TestEqual(TEXT("Training price is affordable at 425000 credits"),Offer->Price,400000LL);

    FText Why;const FName System(TEXT("Janus")),Key=CarrierFighterSalesAI::Inventory(System,TEXT("Archon"),TEXT("Stormhawk"));
    TestFalse(TEXT("Failed initial stock save rolls back"),CarrierFighterSalesAI::Prepare(Pilot,Base,System,[](){return false;},Why));
    TestFalse(TEXT("Failed initial save cannot leave sellable economic stock"),Pilot.Economy.Inventories.Contains(Key));
    TestTrue(TEXT("Prepare finite reserve"),CarrierFighterSalesAI::Prepare(Pilot,Base,System,[](){return true;},Why));
    int32 Squad=INDEX_NONE,Slot=INDEX_NONE;TestTrue(TEXT("Unassigned reserve found"),CarrierFighterSalesAI::FindReserve(*Base.GetHangar(),*Offer,Squad,Slot));
    TestEqual(TEXT("Mission squadron kept intact"),Base.GetHangar()->NumShipsReady(0),2);
    FlightDeck Deck;Deck.SetCarrier(&Base);Deck.SetLaunchDeck();Deck.AddSlot(FVector::ZeroVector,uint32(CLASSIFICATION::FIGHTER));
    TestTrue(TEXT("OOB squadron aircraft remain operational in Flight Ops"),Base.GetHangar()->GotoAlert(Squad,Slot,&Deck));
    TestEqual(TEXT("Assigned aircraft excluded from sale"),CarrierFighterSalesAI::Available(Pilot.Economy,Key,*Base.GetHangar(),*Offer),Offer->Quantity-1);
    TestTrue(TEXT("Stood-down fighter returns to squadron stock"),Base.GetHangar()->StandDown(Squad,Slot));
    SerializedHullStockAI::Ensure(Pilot.Economy,Key,TEXT("Stormhawk"),Offer->Quantity);
    const auto HullOffers=SerializedHullStockAI::Available(Pilot.Economy,Key,TEXT("Stormhawk"),Offer->Quantity);
    if(!TestEqual(TEXT("Reserve has serialized hull offers"),HullOffers.Num(),Offer->Quantity))return false;
    CarrierFighterSalesAI::BindHullOffers(Pilot.Economy,Key,*Base.GetHangar(),*Offer);
    const FGuid OfferedHullId=HullOffers.Last();
    const auto OfferedSlot=Pilot.Economy.SerializedHulls.FindChecked(OfferedHullId);
    TestFalse(TEXT("Specific carrier hull names its squadron slot"),OfferedSlot.Squadron.IsEmpty());
    const FGuid Receipt=FGuid::NewGuid();
    TestFalse(TEXT("Failed purchase save rolls back"),CarrierFighterSalesAI::Purchase(Pilot,Hull,*Base.GetHangar(),*Offer,Key,TEXT("Janek"),Receipt,[](){return false;},Why,OfferedHullId));
    TestEqual(TEXT("Failed sale preserves wallet"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),425000LL);
    TestEqual(TEXT("Failed sale preserves reserve hulls"),CarrierFighterSalesAI::Available(Pilot.Economy,Key,*Base.GetHangar(),*Offer),Offer->Quantity);
    TestTrue(TEXT("425000-credit cadet buys Stormhawk"),CarrierFighterSalesAI::Purchase(Pilot,Hull,*Base.GetHangar(),*Offer,Key,TEXT("Janek"),Receipt,[](){return true;},Why,OfferedHullId));
    TestEqual(TEXT("400000 charged exactly once"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),25000LL);
    TestEqual(TEXT("One owned fighter delivered"),Pilot.HangarShips.Num(),1);
    TestEqual(TEXT("Exact offered carrier hull delivered"),Pilot.HangarShips[0].HullId,OfferedHullId);
    for(int32 S=0;S<Base.GetHangar()->NumSquadrons();++S)
        if(OfferedSlot.Squadron==UTF8_TO_TCHAR(Base.GetHangar()->SquadronName(S).data()))
            TestEqual(TEXT("Exact offered squadron slot transferred"),Base.GetHangar()->GetState(Base.GetHangar()->GetSlot(S,OfferedSlot.Slot)),int32(Hangar::TRANSFERRED));
    TestEqual(TEXT("Transferred reserve no longer ready"),CarrierFighterSalesAI::Available(Pilot.Economy,Key,*Base.GetHangar(),*Offer),Offer->Quantity-1);
    TestEqual(TEXT("Reserve transfer is not a combat loss"),Base.GetHangar()->NumShipsDead(Squad),0);
    TestEqual(TEXT("Normal Stormhawk price remains unchanged"),ShipDesignRegistry::Find(FName(TEXT("Stormhawk")))->BaseCost,574000LL);
    TestTrue(TEXT("Duplicate receipt is idempotent"),CarrierFighterSalesAI::Purchase(Pilot,Hull,*Base.GetHangar(),*Offer,Key,TEXT("Janek"),Receipt,[](){return false;},Why,OfferedHullId));
    TestEqual(TEXT("Duplicate receipt never charges again"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),25000LL);
    TStrongObjectPtr<UPlayerSaveGame> Save(NewObject<UPlayerSaveGame>());Save->PlayerInfo=Pilot;TArray<uint8> Bytes;
    TestTrue(TEXT("Stock and purchase serialize"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
    TStrongObjectPtr<UPlayerSaveGame> Reload(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));if(!TestNotNull(TEXT("Purchase reloads"),Reload.Get()))return false;
    TestEqual(TEXT("Purchased carrier identity survives save"),Reload->PlayerInfo.HangarShips[0].HullId,OfferedHullId);
    Ship Reopened("Archon","",ShipDesignRegistry::FindLegacy(FName(TEXT("Orion"))),1,0,nullptr,false);Reopened.InitializeRuntimeHangarFromDesign();
    TestTrue(TEXT("Reserve restored after level reload"),CarrierFighterSalesAI::Prepare(Reload->PlayerInfo,Reopened,System,[](){return true;},Why));
    for(int32 S=0;S<Reopened.GetHangar()->NumSquadrons();++S)
        if(OfferedSlot.Squadron==UTF8_TO_TCHAR(Reopened.GetHangar()->SquadronName(S).data()))
            TestEqual(TEXT("Sold squadron slot stays transferred on reload"),Reopened.GetHangar()->GetState(Reopened.GetHangar()->GetSlot(S,OfferedSlot.Slot)),int32(Hangar::TRANSFERRED));
    TestTrue(TEXT("Remaining reserve found"),CarrierFighterSalesAI::FindReserve(*Reopened.GetHangar(),*Offer,Squad,Slot));
    TestEqual(TEXT("Reload cannot refill sold stock"),CarrierFighterSalesAI::Available(Reload->PlayerInfo.Economy,Key,*Reopened.GetHangar(),*Offer),Offer->Quantity-1);
    Pilot.bTrainingGraduate=true;
    auto Normal=CarrierFighterSalesAI::Offer(Pilot,Base,TEXT("Stormhawk"));
    TestTrue(TEXT("Carrier retains OOB stock after graduation"),Normal.IsSet());
    if(Normal.IsSet())TestEqual(TEXT("Graduated pilots pay the normal catalog price"),Normal.GetValue().Price,574000LL);
    TestEqual(TEXT("Only authored OOB squadrons plus the original assignment"),Base.GetHangar()->NumSquadrons(),8);
    return true;
}
#endif
