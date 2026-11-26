#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ShipEquipment.h"
#include "ShipDesignRegistry.h"
#include "SystemDesign.h"
#include "Ship.h"
#include "Drive.h"
#include "Power.h"
#include "OwnedHullCondition.h"
#include "PlayerSaveGame.h"
#include "ShipStoreAI.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
namespace
{
struct FEquipmentFixture
{
 TMap<FName,FSystemDesign> OldCatalog=FShipEquipment::GetCatalog();
 TMap<FName,FShipDesign> OldShips=ShipDesignRegistry::GetAll();
 TUniquePtr<Ship> Craft;
 Drive* Engine=nullptr;
 PowerSource* Reactor=nullptr;
 FEquipmentFixture(FGuid Hull=FGuid())
 {
  FSystemDesign System;System.Name=TEXT("Equipment test");FComponentDesign Part;Part.Name=TEXT("Test module");Part.BaseCost=1000;Part.RepairTime=30;Part.ReplaceTime=60;Part.Affects=SimComponent::DAMAGE_EFFICIENCY;System.Components.Add(Part);
  for(int32 L=1;L<=5;++L){FSystemComponentGrade Grade;Grade.Level=L;Grade.PerformanceMultiplier=.85+.05*L;Grade.PowerDemandMultiplier=.925+.025*L;Grade.PriceMultiplier=L;Grade.RepairScale=Grade.ReplacementScale=1.;System.ComponentGrades.Add(Grade);}
  TMap<FName,FSystemDesign> Rows;Rows.Add(TEXT("Equipment test"),System);FShipEquipment::PublishCatalog(Rows);
  FShipDesign Design;Design.ShipName=TEXT("Equipment hull");Design.ShipType=int32(CLASSIFICATION::FIGHTER);Design.Mass=10;Design.Integrity=1000;Design.BaseCost=10000;ShipDesignRegistry::RegisterDesign(FName(TEXT("Equipment hull")),Design);
  Craft=MakeUnique<Ship>("Equipment craft","",ShipDesignRegistry::FindLegacy(FName(TEXT("Equipment hull"))),1,0,nullptr,false,Hull);
  Engine=new Drive(EDriveType::FUSION,100,0,false);Reactor=new PowerSource(EPowerSource::FUSION,100);
  SystemDesign Definition;Definition.name="Equipment test";auto* Native=new ComponentDesign;Native->name="Test module";Native->base_cost=1000;Native->repair_time=30;Native->replace_time=60;Native->affects=SimComponent::DAMAGE_EFFICIENCY;Definition.components.append(Native);
  Engine->SetDesign(&Definition);Reactor->SetDesign(&Definition);Craft->GetSystems().append(Engine);Craft->GetSystems().append(Reactor);
  Craft->GetEquipment()->InitializeStandard(3);
 }
 ~FEquipmentFixture(){Craft.Reset();ShipDesignRegistry::Clear();for(const auto& Pair:OldShips)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);FShipEquipment::PublishCatalog(OldCatalog);}
 double Thrust()
 {Engine->SetPowerOn();Engine->SetPowerLevel(100);Engine->SetThrottle(100,false,1);Engine->Distribute(200,1);return Engine->GetThrust(1);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipEquipmentOwnerTest,"Starshatter.Components.NativeOwnershipAndIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipEquipmentOwnerTest::RunTest(const FString&)
{
 FEquipmentFixture F;auto& Equipment=*F.Craft->GetEquipment();
 TestTrue(TEXT("Aggregate belongs to native ship"),&Equipment.GetOwner()==F.Craft.Get());
 auto* Part=F.Engine->GetComponents()[0];const FGuid Id=Part->GetItemId();
 TestTrue(TEXT("Physical component has an identity"),Id.IsValid());TestEqual(TEXT("Component owner is the physical hull"),Part->GetOwnerHullId(),F.Craft->GetHullId());
 TestTrue(TEXT("Legacy system ownership preserved"),F.Engine->GetShip()==F.Craft.Get() && Part->GetSystem()==F.Engine);
 TestTrue(TEXT("Distinct system slots have distinct identities"),Id!=F.Reactor->GetComponents()[0]->GetItemId());
 FPlayerHangarShip Snapshot;FOwnedHullCondition::Capture(*F.Craft,Snapshot);FOwnedHullCondition::Capture(*F.Craft,Snapshot);
 TestEqual(TEXT("Repeated snapshot does not manufacture another installed part"),Part->GetItemId(),Id);TestEqual(TEXT("One row per installed part"),Snapshot.Components.Num(),2);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipEquipmentBaselineTest,"Starshatter.Components.GradeIIIAndDriveParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipEquipmentBaselineTest::RunTest(const FString&)
{
 FEquipmentFixture F;const double OriginalThrust=F.Thrust(),OriginalSupply=F.Reactor->GetRatedOutput(),OriginalDemand=F.Engine->GetRatedDemand();
 TestTrue(TEXT("MILSPEC III matches the legacy drive formula"),FMath::IsNearlyEqual(OriginalThrust,10000.,.001));
 TestEqual(TEXT("MILSPEC III matches the authored reactor output"),OriginalSupply,100.);
 double LastQuality=0;
 for(int32 Level=1;Level<=5;++Level)
 {
  const auto Grade=FShipEquipment::GradeDesign(TEXT("Equipment test"),TEXT("Test module"),Level);
  if(!TestTrue(TEXT("Every component level resolves"),Grade.IsSet()))return false;
  TestTrue(TEXT("Quality increases from I through V"),Grade->performance_multiplier>LastQuality);LastQuality=Grade->performance_multiplier;
  if(Level==3)TestEqual(TEXT("III is the universal unmodified baseline"),Grade->performance_multiplier,1.);
 }
 F.Craft->GetEquipment()->InitializeStandard(1);
 TestTrue(TEXT("Level I has less thrust than MILSPEC III"),F.Thrust()<OriginalThrust);
 TestTrue(TEXT("Level I reactor output is below authored MILSPEC III"),F.Reactor->GetRatedOutput()<OriginalSupply);
 TestTrue(TEXT("Lower level has its authored lower power drain"),F.Engine->GetRatedDemand()<OriginalDemand);
 FPlayerHangarShip Saved;FOwnedHullCondition::Capture(*F.Craft,Saved);Saved.Components[0].Level=5;Saved.Components[0].InstalledValue=-1;
 FOwnedHullCondition::Apply(*F.Craft,Saved);
 TestTrue(TEXT("Installed component grade drives actual native thrust"),F.Thrust()>OriginalThrust);
 TestTrue(TEXT("Higher grade carries calculated native power demand"),F.Engine->GetRatedDemand()>OriginalDemand);
 TestTrue(TEXT("Propulsion refit does not upgrade the reactor"),F.Reactor->GetRatedOutput()<OriginalSupply);
 auto* Part=F.Engine->GetComponents()[0];Part->ApplyDamage(50);F.Engine->CalcStatus();
 TestTrue(TEXT("Existing damage still reduces upgraded thrust"),F.Thrust()<OriginalThrust);
 F.Craft->GetEquipment()->InitializeStandard(3);
 TestEqual(TEXT("MILSPEC III restores the exact authored reactor rating"),F.Reactor->GetRatedOutput(),OriginalSupply);
 TestEqual(TEXT("MILSPEC III restores the exact authored power drain"),F.Engine->GetRatedDemand(),OriginalDemand);
 TestEqual(TEXT("Fleet standard is Level III"),Part->GetLevel(),3);
 TestTrue(TEXT("Calibration never heals an installed part"),Part->Availability()<100);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipEquipmentSaveTest,"Starshatter.Components.SaveQualityDamageAndIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipEquipmentSaveTest::RunTest(const FString&)
{
 FEquipmentFixture F;F.Craft->GetEquipment()->InitializeStandard(1);FPlayerHangarShip Hull;Hull.HullId=F.Craft->GetHullId();FOwnedHullCondition::Capture(*F.Craft,Hull);
 Hull.Components[0].Level=5;Hull.Components[0].Component=TEXT("Test module V");FOwnedHullCondition::Apply(*F.Craft,Hull);
 auto* Part=F.Engine->GetComponents()[0];Part->ApplyDamage(25);FOwnedHullCondition::Capture(*F.Craft,Hull);
 TestEqual(TEXT("Legacy grade suffix migrates to stable component key"),Hull.Components[0].Component,FName(TEXT("Test module")));
 const auto Id=Part->GetItemId();const double Performance=Part->PerformanceMultiplier();
 TStrongObjectPtr<UPlayerSaveGame> Save(NewObject<UPlayerSaveGame>());Save->PlayerInfo.HangarShips.Add(Hull);TArray<uint8> Bytes;
 TestTrue(TEXT("Component snapshot saves"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));TStrongObjectPtr<UPlayerSaveGame> Reload(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));if(!TestNotNull(TEXT("Component snapshot reloads"),Reload.Get()))return false;
 F.Craft->GetEquipment()->InitializeStandard(3);FOwnedHullCondition::ApplyPristine(*F.Craft);FOwnedHullCondition::Apply(*F.Craft,Reload->PlayerInfo.HangarShips[0]);
 TestEqual(TEXT("Exact item identity restored"),Part->GetItemId(),Id);TestEqual(TEXT("Native installed level restored"),Part->GetLevel(),5);
 TestEqual(TEXT("Issued equipment level survives"),F.Craft->GetEquipment()->GetStandardLevel(),1);TestEqual(TEXT("Native quality restored independently of damage"),Part->PerformanceMultiplier(),Performance);TestEqual(TEXT("Damage restored independently of quality"),Part->Availability(),75.f);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipEquipmentValueTest,"Starshatter.Components.IncludedEquipmentValuation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipEquipmentValueTest::RunTest(const FString&)
{
 FEquipmentFixture F;F.Craft->GetEquipment()->InitializeStandard(1);FPlayerHangarShip Hull;Hull.HullValue=10000;FOwnedHullCondition::Capture(*F.Craft,Hull);
 FShipEquipment::SeparateValue(Hull);
 TestEqual(TEXT("Hull residual excludes included standard equipment"),Hull.HullValue,8000LL);TestEqual(TEXT("Installed components are tracked separately"),Hull.InstalledComponentValue,2000LL);
 TestEqual(TEXT("Standard equipment is included in advertised purchase total"),Hull.HullValue+Hull.InstalledComponentValue,10000LL);
 FShipEquipment::SeparateValue(Hull);TestEqual(TEXT("Migration is idempotent"),Hull.HullValue,8000LL);
 Hull.Components[0].Level=5;Hull.Components[0].InstalledValue=-1;FShipEquipment::Revalue(Hull);
 TestEqual(TEXT("Refit values the installed components once"),Hull.InstalledComponentValue,6000LL);TestEqual(TEXT("Refit leaves hull value unchanged"),Hull.HullValue,8000LL);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipEquipmentTransferTest,"Starshatter.Components.StoredPartRetainsIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipEquipmentTransferTest::RunTest(const FString&)
{
 FS_PlayerGameInfo Pilot;FHangarComponent Part;Part.ItemId=FGuid::NewGuid();Part.SystemDesign=TEXT("Engine");Part.Component=TEXT("Injector");Part.Level=3;Part.bHasCondition=true;Part.ConditionPercent=55;Part.InstalledValue=1200;
 ShipStoreAI::StoreComponent(Pilot,Part,TEXT("Dock"));FHangarComponent Taken;
 TestTrue(TEXT("Stored component can be fitted"),ShipStoreAI::TakeComponent(Pilot,Part.SystemDesign,Part.Component,3,TEXT("Dock"),Taken));
 TestEqual(TEXT("Moving an existing physical component preserves its ID"),Taken.ItemId,Part.ItemId);TestEqual(TEXT("Moving a part preserves damage"),Taken.ConditionPercent,55.f);TestEqual(TEXT("Moving a part preserves allocated value"),Taken.InstalledValue,1200LL);
 TestFalse(TEXT("One physical component cannot be taken twice"),ShipStoreAI::TakeComponent(Pilot,Part.SystemDesign,Part.Component,3,TEXT("Dock"),Taken));return true;
}
#endif
