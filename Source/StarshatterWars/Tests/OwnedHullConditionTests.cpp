#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OwnedHullCondition.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedConditionIdentityTest,"Starshatter.Hangar.Condition.OwnerIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOwnedConditionIdentityTest::RunTest(const FString&)
{
    FPlayerHangarShip Hull; Hull.HullId=FGuid::NewGuid(); Hull.Design=TEXT("Falcon");
    const FName Cargo(*(TEXT("Ship/")+Hull.HullId.ToString(EGuidFormats::Digits)));
    TestTrue(TEXT("Explicit owned binding eligible"),FOwnedHullCondition::IsOwnedLive(Hull,Hull.HullId,Hull.Design,Cargo,true));
    TestFalse(TEXT("Mission loaner never writes owned condition"),FOwnedHullCondition::IsOwnedLive(Hull,Hull.HullId,Hull.Design,Cargo,false));
    Hull.bAssigned=true;
    TestFalse(TEXT("Assigned row not owned"),FOwnedHullCondition::IsOwnedLive(Hull,Hull.HullId,Hull.Design,Cargo,true));
    Hull.bAssigned=false;
    TestFalse(TEXT("Same design with another live identity rejected"),FOwnedHullCondition::IsOwnedLive(Hull,Hull.HullId,Hull.Design,TEXT("Ship/Other"),true));
    TestFalse(TEXT("Different owned identity rejected"),FOwnedHullCondition::IsOwnedLive(Hull,FGuid::NewGuid(),Hull.Design,Cargo,true));
    TestFalse(TEXT("Different design rejected"),FOwnedHullCondition::IsOwnedLive(Hull,Hull.HullId,TEXT("Raptor"),Cargo,true));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedConditionPersistenceTest,"Starshatter.Hangar.Condition.SaveRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOwnedConditionPersistenceTest::RunTest(const FString&)
{
    auto* Save=NewObject<UPlayerSaveGame>(); auto& Hull=Save->PlayerInfo.HangarShips.AddDefaulted_GetRef();
    Hull.HullId=FGuid::NewGuid(); Hull.bHasCondition=true; Hull.HullCondition=.6f; Hull.RepairQueue={2};
    auto& Slot=Hull.Components.AddDefaulted_GetRef(); Slot.bHasCondition=true; Slot.ConditionPercent=70;
    Slot.ConditionStatus=static_cast<int32>(SYSTEM_STATUS::REPAIR); Slot.MaintenanceSeconds=12; Slot.SpareCount=4; Slot.JerryCount=2; Slot.Level=5;
    auto& System=Hull.SystemConditions.AddDefaulted_GetRef(); System.SystemIndex=2; System.Availability=.7f;
    TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Condition serialized"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Condition restored from save"),Loaded))return false;
    const auto& Copy=Loaded->PlayerInfo.HangarShips[0];
    TestEqual(TEXT("Hull fraction survives"),Copy.HullCondition,.6f);
    TestEqual(TEXT("Raw module health survives"),Copy.Components[0].ConditionPercent,70.f);
    TestEqual(TEXT("Maintenance timer survives"),Copy.Components[0].MaintenanceSeconds,12.f);
    TestEqual(TEXT("Jerry count survives"),Copy.Components[0].JerryCount,2);
    TestEqual(TEXT("Quality independent of damage"),Copy.Components[0].Level,5);
    TestEqual(TEXT("Repair priority survives"),Copy.RepairQueue[0],2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedConditionRepairRollbackTest,"Starshatter.Hangar.Condition.RepairRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOwnedConditionRepairRollbackTest::RunTest(const FString&)
{
    const auto BeforeDesigns=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Entry:BeforeDesigns)ShipDesignRegistry::RegisterDesign(Entry.Key,Entry.Value);};
    FShipDesign DesignRow; DesignRow.ShipName=TEXT("ConditionFixture"); DesignRow.ShipClass=TEXT("Fighter"); DesignRow.Mass=20; DesignRow.Integrity=100;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("ConditionFixture")),DesignRow);
    Ship Craft("Condition fixture","",ShipDesignRegistry::FindLegacy(FName(TEXT("ConditionFixture"))),0,0,nullptr,false);
    auto* System=new SimSystem(SYSTEM_CATEGORY::COMPUTER,0,"Condition system",100);
    ComponentDesign Definition; Definition.name="Condition part"; Definition.spares=4; Definition.repair_time=30; Definition.affects=SimComponent::DAMAGE_EFFICIENCY;
    Definition.performance_multiplier=1.3;
    auto* Part=new SimComponent(Definition,System); System->GetComponents().append(Part); Craft.GetSystems().append(System);
    Craft.SetIntegrity(60); Part->ApplyDamage(30); Part->Repair(); Craft.RepairQueue().append(System);
    FS_PlayerGameInfo Player; Player.InsuranceCredits=500;
    auto& Owned=Player.HangarShips.AddDefaulted_GetRef(); Owned.HullId=FGuid::NewGuid(); FOwnedHullCondition::Capture(Craft,Owned);
    const bool Saved=FOwnedHullCondition::RepairTransaction(Craft,Player,&Owned,
        [&](){Player.InsuranceCredits-=100;return true;},[&](){FOwnedHullCondition::ApplyPristine(Craft);return true;},[](){return false;});
    TestFalse(TEXT("Failed save rejects repairs"),Saved);
    TestEqual(TEXT("Credit state rolled back"),Player.InsuranceCredits,int64(500));
    TestEqual(TEXT("Hull restored"),Craft.GetIntegrity(),60.0);
    FPlayerHangarShip Raw; FOwnedHullCondition::Capture(Craft,Raw);
    const auto* Restored=Raw.Components.FindByPredicate([](const auto& Row){return Row.Component==TEXT("Condition part");});
    if(TestNotNull(TEXT("Module snapshot present"),Restored))
    {
        TestEqual(TEXT("Uncapped raw health restored"),Restored->ConditionPercent,70.f);
        TestEqual(TEXT("Repair state restored"),Restored->ConditionStatus,static_cast<int32>(SYSTEM_STATUS::REPAIR));
        TestEqual(TEXT("Repair timer restored"),Restored->MaintenanceSeconds,30.f);
    }
    TestEqual(TEXT("Grade performance unchanged"),Part->PerformanceMultiplier(),1.3);
    TestTrue(TEXT("Repair queue restored"),Craft.RepairQueue().contains(System));
    auto* SavedOwned=&Player.HangarShips[0];
    TestTrue(TEXT("Successful repair commits"),FOwnedHullCondition::RepairTransaction(Craft,Player,SavedOwned,
        [&](){Player.InsuranceCredits-=100;return true;},[&](){FOwnedHullCondition::ApplyPristine(Craft);return true;},[](){return true;}));
    TestEqual(TEXT("Repaired persistent hull stored"),Player.HangarShips[0].HullCondition,1.f);
    TestEqual(TEXT("Exactly one charge retained"),Player.InsuranceCredits,int64(400));
    return true;
}
#endif
