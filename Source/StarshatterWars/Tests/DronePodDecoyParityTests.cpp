#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "ShipDesignRegistry.h"
#include "ShipDesign.h"
#include "WeaponDesign.h"
#include "Ship.h"
#include "Weapon.h"
#include "Drone.h"
#include "SeekerAI.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimEvent.h"
#include "ShipActor.h"
#include "ShipProbeComponent.h"
#include "ShipDecoyComponent.h"

namespace {
struct FOrdnanceCatalog {
 TMap<FName,FWeaponDesign> Before=WeaponDesignRegistry::GetAll();
 FOrdnanceCatalog(){auto* L=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto Ansi=StringCast<ANSICHAR>(*File);L->LoadWeaponDesign(Ansi.Get());}
 ~FOrdnanceCatalog(){WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);}
};
class FArmedPulse : public SimShot {
public: FArmedPulse(const FVector& P,WeaponDesign* D,const Ship* O):SimShot(P,Camera(),D,O){armed=true;}
};
class FRegionSteps : public SimRegion {
public: using SimRegion::SimRegion; using SimRegion::UpdateShots;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneProfilesParity,"Starshatter.Weapons.Drones.ProfilesAndLifetime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDroneProfilesParity::RunTest(const FString&){
 FOrdnanceCatalog Catalog;TSet<FName> Seen;int Count=0;
 for(const auto& Pair:WeaponDesignRegistry::GetAll()){
  const FName Name(Pair.Value.Name);if(Seen.Contains(Name))continue;Seen.Add(Name);
  auto* D=WeaponDesignRegistry::FindLegacy(Name);if(!D || !D->drone)continue;++Count;
  Drone S(FVector::ZeroVector,Camera(),D);TestTrue(*Pair.Value.Name,S.IsDrone());
  TestEqual(TEXT("Decoy classification retained"),S.IsDecoy(),D->decoy_type!=0);
  TestEqual(TEXT("Probe classification retained"),S.IsProbe(),D->probe!=0);
  TestEqual(TEXT("Integrity retained"),S.GetIntegrity(),double(D->integrity));
  TestEqual(TEXT("Legacy passive signature"),S.PCS(),D->decoy_type||D->probe?0.0:10000.0);
  TestEqual(TEXT("Legacy active signature"),S.ACS(),D->decoy_type||D->probe?0.0:1000.0);
  S.ExecFrame(0);const double Before=S.GetLife();S.ExecFrame(.1);
  TestTrue(TEXT("Lifetime advances by simulation delta"),FMath::IsNearlyEqual(S.GetLife(),Before-.1,.0001));
 }
 TestEqual(TEXT("Eight deployed drone/decoy/probe definitions"),Count,8);
 auto* Probe=WeaponDesignRegistry::FindLegacy(FName(TEXT("Sensor Drone")));
 if(TestNotNull(TEXT("Sensor Drone"),Probe)){TestEqual(TEXT("Probe legacy coverage"),Probe->lethal_radius,50000.0f);TestEqual(TEXT("Probe is harmless"),Probe->damage,0.0f);Drone P(FVector::ZeroVector,Camera(),Probe);P.SeekTarget(nullptr);TestNull(TEXT("Probe never gets a combat seeker"),P.GetDirector());}
 auto* Recon=WeaponDesignRegistry::FindLegacy(FName(TEXT("Recon Pod")));
 if(TestNotNull(TEXT("Recon Pod"),Recon)){TestFalse(TEXT("Recon pulse is not a deployed drone"),bool(Recon->drone));TestTrue(TEXT("Negative damage identifies scan pulse"),Recon->damage<0);TestEqual(TEXT("Two scans per press"),Recon->ripple_count,2);TestEqual(TEXT("Ten scans per pod"),Recon->ammo,10);TestFalse(TEXT("No fired visible pod store"),bool(Recon->visible_stores));}
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneCollisionParity,"Starshatter.Weapons.Drones.CollisionAndRecon",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDroneCollisionParity::RunTest(const FString&){
 TUniquePtr<Sim> OwnedSim;if(!Sim::GetSim())OwnedSim=MakeUnique<Sim>(nullptr);
 ShipDesign SD;SD.ship_class=static_cast<int>(CLASSIFICATION::FIGHTER);SD.integrity=1000;SD.radius=200;SD.scale=1;
 Ship Owner("Drone parity shooter","",&SD,1,0,nullptr,false);
 FRegionSteps Region(Sim::GetSim(),"Drone parity",SimRegion::REAL_SPACE);
 Ship* Target=new Ship("Drone parity target","",&SD,2,0,nullptr,false);Target->MoveTo(FVector(0,0,5000));Region.InsertObject(Target);
 WeaponDesign Scan;Scan.name="Parity scan";Scan.mass=1;Scan.damage=-1;Scan.charge=1;Scan.life=2;Scan.speed=0;Scan.target_type=0xffffffff;
 Region.InsertObject(new FArmedPulse(Target->GetLocation(),&Scan,&Owner));const double Before=Target->GetIntegrity();Region.DamageShips();
 TestEqual(TEXT("Scan leaves hull intact"),Target->GetIntegrity(),Before);
 TestTrue(TEXT("Scan records legacy mission event"),ShipStats::Find(Owner.GetName())->HasEvent(ESimEvent::SCAN_TARGET));
 Region.UpdateShots(0);
 WeaponDesign D;D.name="Parity combat drone";D.drone=1;D.mass=1;D.damage=20;D.charge=1;D.life=5;D.speed=0;D.integrity=100;
 Region.InsertObject(new Drone(Target->GetLocation(),Camera(),&D,&Owner));
 TestEqual(TEXT("Drone has one owner list"),Region.GetShots().size(),0);
 TestEqual(TEXT("Drone included in runtime/collision view"),Region.GetProjectiles().Num(),1);
 Region.DamageShips();TestTrue(TEXT("Combat drone damages Unreal hull without legacy rep"),Target->GetIntegrity()<Before);
 Region.UpdateShots(0);TestEqual(TEXT("Consumed drone removed once"),Region.GetDrones().size(),0);
 Drone* Victim=new Drone(FVector(20000,0,0),Camera(),&D,nullptr);Region.InsertObject(Victim);
 WeaponDesign Bolt;Bolt.name="Parity interceptor";Bolt.primary=1;Bolt.damage=200;Bolt.charge=1;Bolt.life=2;Bolt.mass=1;Bolt.target_type=static_cast<uint32>(CLASSIFICATION::DRONE);
 Region.InsertObject(new FArmedPulse(Victim->GetLocation(),&Bolt,&Owner));Region.DamageShips();
 TestTrue(TEXT("Interceptor destroys deployable drone"),Victim->GetLife()<=0);Region.UpdateShots(0);
 TestEqual(TEXT("Destroyed drone cleanup"),Region.GetDrones().size(),0);TestEqual(TEXT("Consumed interceptor cleanup"),Region.GetShots().size(),0);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneSeekerParity,"Starshatter.Weapons.Drones.SeekerDelayAndDecoyRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDroneSeekerParity::RunTest(const FString&){
 WeaponDesign D;D.name="Seeker parity";D.mass=1;D.life=10;D.guided=1;
 SimShot Shot(FVector::ZeroVector,Camera(),&D);SeekerAI Seeker(&Shot);Seeker.SetDelay(1);
 for(int I=0;I<4;++I)Seeker.ExecFrame(.25);TestEqual(TEXT("One second arming delay advances"),Seeker.GetDelay(),0.0);
 Seeker.SetDelay(1);Seeker.ExecFrame(0);TestEqual(TEXT("Zero delta does not arm"),Seeker.GetDelay(),1.0);
 ShipDesign SD;SD.ship_class=static_cast<int>(CLASSIFICATION::FIGHTER);Ship Original("Seeker original","",&SD,2,0,nullptr,false);
 D.drone=1;D.decoy_type=static_cast<int>(CLASSIFICATION::FIGHTER);Drone Decoy(FVector::ZeroVector,Camera(),&D);
 Seeker.SetTarget(&Original);Seeker.SetTarget(&Decoy);Seeker.Update(&Decoy);
 TestTrue(TEXT("Expired decoy restores original ship"),Seeker.GetTarget()==&Original);
 Seeker.Update(&Original);TestNull(TEXT("Destroyed original target clears seeker"),Seeker.GetTarget());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipLauncherParity,"Starshatter.Weapons.Drones.SeparateShipLaunchers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipLauncherParity::RunTest(const FString&){
 FOrdnanceCatalog Catalog;const auto Before=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& P:Before)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
 FShipDesign Row;FShipWeapon Decoy;Decoy.Name="Countermeasures";Decoy.WeaponType="Fighter Decoy";Decoy.Muzzles.Add(FVector::ZeroVector);Row.Weapon.Add(Decoy);
 FShipWeapon Probe;Probe.Name="Probe launcher";Probe.WeaponType="Sensor Drone";Probe.Muzzles.Add(FVector::ZeroVector);Row.Weapon.Add(Probe);
 const FName Key(TEXT("OrdnanceLauncherParity"));ShipDesignRegistry::RegisterDesign(Key,Row);auto* Design=ShipDesignRegistry::FindLegacy(Key);
 if(!TestNotNull(TEXT("Converted launcher ship"),Design))return false;
 TestNotNull(TEXT("Dedicated decoy design"),Design->decoy);TestNotNull(TEXT("Dedicated probe design"),Design->probe);TestEqual(TEXT("Launchers excluded from guns"),Design->weapons.size(),0);
 Ship S("Launcher parity","",Design,1,0,nullptr,false);TestNotNull(TEXT("Runtime decoy"),S.GetDecoy());TestNotNull(TEXT("Runtime probe"),S.GetProbeLauncher());
 TestTrue(TEXT("Launchers are separate objects"),S.GetDecoy()!=S.GetProbeLauncher());
 const auto* Actor=GetDefault<AShipActor>();TestNotNull(TEXT("Shared ship ProbeComponent"),Actor->ProbeComponent.Get());TestNotNull(TEXT("Shared ship DecoyComponent"),Actor->DecoyComponent.Get());
 TestFalse(TEXT("Unbound probe fails safely"),Actor->ProbeComponent->Deploy());TestFalse(TEXT("Unbound decoy fails safely"),Actor->DecoyComponent->Deploy());return true;
}
#endif
