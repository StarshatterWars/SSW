#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Game.h"
#include "Intel.h"
#include "TimerSubsystem.h"
#include "SimContact.h"
#include "Shield.h"
#include "ShipDesignRegistry.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "WeaponDesign.h"
#include "SimShot.h"
#include "Debris.h"
#include "Asteroid.h"
#include "NativeAsteroidActor.h"
#include "Engine/StaticMesh.h"
#include "SimRegion.h"
namespace {
struct FRestoreParityClock {
 double GUI=Game::GUITime(),Step=Game::FrameTime();bool Paused=Game::Paused();
 ~FRestoreParityClock(){Game::SetFrameClock(GUI,GUI>0?FMath::Max(1u,uint32(FMath::RoundToInt(Step/GUI))):1,Paused);}
};
class FParityDebris : public Debris {
public:FParityDebris(const FVector& P,double R,double Health=1000):Debris(nullptr,P,FVector::ZeroVector,100){radius=R;integrity=Health;}
};
class FParityAsteroid : public Asteroid {
public:FParityAsteroid(const FVector& P):Asteroid(0,P,100){radius=100;}
};
class FParitySegment : public SimShot {
public:FParitySegment(WeaponDesign* D,const FVector& A,const FVector& B):SimShot(A,Camera(),D){armed=true;origin=A;MoveTo(B);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParityClockTest,"Starshatter.Parity.P01.ClocksAndContactDecay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParityClockTest::RunTest(const FString&){
 FRestoreParityClock Restore;Game::SetFrameClock(.02,1,false);
 TestEqual(TEXT("Simulation step"),Game::FrameTime(),.02);TestEqual(TEXT("GUI step"),Game::GUITime(),.02);TestEqual(TEXT("FPS"),Game::FrameRate(),50.0);
 Game::SetFrameClock(.5,1,false);TestEqual(TEXT("Legacy normal frame cap"),Game::FrameTime(),Game::GetMaxFrameLength());TestEqual(TEXT("GUI uncapped"),Game::GUITime(),.5);
 Game::SetFrameClock(.1,10,false);TestEqual(TEXT("Compression applied once"),Game::FrameTime(),1.0);TestEqual(TEXT("GUI remains real time"),Game::GUITime(),.1);
 SimContact Contact(static_cast<Ship*>(nullptr),1,1);Contact.Reset();TestTrue(TEXT("Contact signal decays by sim step"),FMath::IsNearlyEqual(Contact.PasReturn(),.9,.00001));
 Game::SetFrameClock(.1,10,true);const double Before=Contact.PasReturn();Contact.Reset();TestEqual(TEXT("Paused physics"),Game::FrameTime(),0.0);TestEqual(TEXT("Paused UI advances"),Game::GUITime(),.1);TestEqual(TEXT("Paused contact decay stops"),Contact.PasReturn(),Before);
 Game::SetFrameClock(-1,1,false);TestEqual(TEXT("Negative delta rejected"),Game::FrameTime(),0.0);
 auto* Clock=NewObject<UTimerSubsystem>(NewObject<UGameInstance>());Clock->StartMissionRun(true);
 Game::SetFrameClock(.1,10,false);Clock->ManualMissionTick(float(Game::FrameTime()));TestEqual(TEXT("Mission timer uses compressed simulation step once"),Clock->GetMissionTimeSecondsInt(),1);
 Game::SetFrameClock(.1,10,true);Clock->ManualMissionTick(float(Game::FrameTime()));TestEqual(TEXT("Paused mission timer does not advance"),Clock->GetMissionTimeSecondsInt(),1);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParityShieldImportTest,"Starshatter.Parity.P02.ShieldImportDefaults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParityShieldImportTest::RunTest(const FString&){
 const auto Before=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& P:Before)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
 const EShieldType Types[]={EShieldType::DEFLECTOR,EShieldType::GRAV_SHIELD,EShieldType::HYPER_SHIELD};const double Caps[]={2000,7000,10000};const double Factors[]={.05,.01,.003};
 for(int I=0;I<3;++I){FShipDesign Row;FShipShield Source;Source.ShieldType=Types[I];Row.Shield.Add(Source);const FName Key(*FString::Printf(TEXT("P02Defaults%d"),I));ShipDesignRegistry::RegisterDesign(Key,Row);auto* Design=ShipDesignRegistry::FindLegacy(Key);if(!TestNotNull(TEXT("Converted shield"),Design->shield))continue;
 auto* S=Design->shield;TestEqual(TEXT("Legacy capacity retained"),S->GetCapacity(),Caps[I]);TestEqual(TEXT("Legacy consumption retained"),S->GetConsumption(),Caps[I]);TestTrue(TEXT("Legacy penetration factor"),FMath::IsNearlyEqual(S->GetShieldFactor(),Factors[I],.000001));TestTrue(TEXT("Noncapacitor curve retained"),FMath::IsNearlyEqual(S->GetShieldCurve(),.25,.00001));}
 FShipDesign Row;FShipShield Source;Source.bShieldCapacitor=true;Source.Capacity=3000;Source.Consumption=450;Source.Factor=.02;Source.Curve=.4;Source.Cutoff=.2;Source.DeflectionCost=2;Source.Location=FVector(11,22,33);Source.Size=44;Source.SourceIndex=2;Source.Emcon1=10;Source.Emcon2=50;Source.Emcon3=90;Row.Shield.Add(Source);
 const FName Key(TEXT("P02Overrides"));ShipDesignRegistry::RegisterDesign(Key,Row);auto* S=ShipDesignRegistry::FindLegacy(Key)->shield;
 TestTrue(TEXT("Mode preserves custom curve"),FMath::IsNearlyEqual(S->GetShieldCurve(),.4,.00001));TestEqual(TEXT("Capacity override"),S->GetCapacity(),3000.0);TestEqual(TEXT("Consumption override"),S->GetConsumption(),450.0);TestTrue(TEXT("Capacitor retained"),S->GetShieldCapacitor());TestEqual(TEXT("Mount retained"),S->GetMountRelativeLocation(),Source.Location);TestEqual(TEXT("Reactor retained"),S->GetSourceIndex(),2);TestEqual(TEXT("EMCON 2 retained"),S->GetEMCONPower(2),50);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParityShieldDamageTest,"Starshatter.Parity.P02.ShieldPowerAndDamage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParityShieldDamageTest::RunTest(const FString&){
 FRestoreParityClock Restore;Game::SetFrameClock(.1,1,false);WeaponDesign D;D.penetration=1;D.life=1;SimShot Shot(FVector::ZeroVector,Camera(),&D);
 for(auto Type:{EShieldType::DEFLECTOR,EShieldType::GRAV_SHIELD,EShieldType::HYPER_SHIELD}){
 Shield S(Type);S.SetShieldCapacitor(false);S.SetPowerLevel(100);S.ExecFrame(10);TestTrue(TEXT("Ten second ramp"),FMath::IsNearlyEqual(S.GetPowerLevel(),100.0,.001));S.Distribute(S.GetConsumption(),1);TestTrue(TEXT("Full powered shield"),FMath::IsNearlyEqual(S.GetShieldLevel(),100.0,.001));
 TestTrue(TEXT("Legacy penetration leakage"),FMath::IsNearlyEqual(S.DeflectDamage(&Shot,100),100*S.GetShieldFactor(),.001));S.SetPowerLevel(0);S.ExecFrame(5);TestTrue(TEXT("Five second power down"),S.GetPowerLevel()<.001);
 S.Distribute(0,1);TestEqual(TEXT("Unpowered shield passes damage"),S.DeflectDamage(&Shot,100),100.0);
 }
 Shield C(EShieldType::DEFLECTOR);C.SetShieldCapacitor(true);C.SetShieldCutoff(.2);C.Distribute(2000,1);const double Before=C.GetEnergy();const double Leak=C.DeflectDamage(&Shot,100);TestTrue(TEXT("Capacitor spends deflected energy"),FMath::IsNearlyEqual(C.GetEnergy(),Before-(100-Leak),.001));C.Distribute(-2000,1);TestEqual(TEXT("Empty capacitor passes full damage"),C.DeflectDamage(&Shot,100),100.0);C.Distribute(2200,1);TestTrue(TEXT("Capacitor recharges"),C.GetShieldLevel()>99);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParityObstacleTest,"Starshatter.Parity.P03.ProjectileObstacles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParityObstacleTest::RunTest(const FString&){
 FRestoreParityClock Restore;Game::SetFrameClock(.1,1,false);
 WeaponDesign D;D.primary=1;D.damage=10;D.mass=1;D.life=2;D.charge=1;D.target_type=0;
 {SimRegion Region(nullptr,"P03 debris",SimRegion::REAL_SPACE);auto* Piece=new FParityDebris(FVector(0,0,500),100);Region.InsertObject(Piece);auto* Shot=new FParitySegment(&D,FVector::ZeroVector,FVector(0,0,500));Region.InsertObject(Shot);Region.DamageShips();TestTrue(TEXT("Debris blocks projectile without DRONE mask"),Shot->GetLife()<=0);TestTrue(TEXT("Debris receives damage"),Piece->GetIntegrity()<1000);}
 {SimRegion Region(nullptr,"P03 small debris",SimRegion::REAL_SPACE);Region.InsertObject(new FParityDebris(FVector(0,0,500),49));auto* Shot=new FParitySegment(&D,FVector::ZeroVector,FVector(0,0,500));Region.InsertObject(Shot);Region.DamageShips();TestTrue(TEXT("Small debris ignored as legacy"),Shot->GetLife()>0);}
 {SimRegion Region(nullptr,"P03 rock",SimRegion::REAL_SPACE);Region.InsertObject(new FParityAsteroid(FVector(0,0,500)));auto* Shot=new FParitySegment(&D,FVector::ZeroVector,FVector(0,0,500));Region.InsertObject(Shot);Region.DamageShips();TestTrue(TEXT("Asteroid consumes shot"),Shot->GetLife()<=0);}
 {SimRegion Region(nullptr,"P03 destroyed debris",SimRegion::REAL_SPACE);Region.InsertObject(new FParityDebris(FVector(0,0,500),100,1));Region.InsertObject(new FParitySegment(&D,FVector::ZeroVector,FVector(0,0,500)));Region.DamageShips();TestEqual(TEXT("Destroyed debris removed safely"),Region.GetRocks().size(),0);}
 WeaponDesign BD;BD.primary=1;BD.beam=1;BD.graphic_type=3;BD.damage=20;BD.charge=1;BD.mass=1;BD.life=2;BD.length=10000;BD.min_range=1000;
 SimShot Beam(FVector::ZeroVector,Camera(),&BD);Beam.ExecFrame(.1);Beam.SetBeamPoints(FVector::ZeroVector,FVector(0,0,1000));FParityDebris Piece(FVector(0,0,500),100);FVector Impact;const double Before=Piece.GetIntegrity();TestTrue(TEXT("Beam segment hits obstacle before endpoint"),Piece.HitBy(&Beam,Impact)!=0);TestTrue(TEXT("Beam applies delta exactly once"),FMath::IsNearlyEqual(Before-Piece.GetIntegrity(),2.0,.001));TestTrue(TEXT("Beam remains alive"),Beam.GetLife()>0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParityIntelNamesTest,"Starshatter.Parity.P04.StableIntelNames",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParityIntelNamesTest::RunTest(const FString&){
 const char* Names[]={"Unknown","Reserve","Secret","Known","Located","Tracked","Active"};
 for(int I=0;I<7;++I){const char* Name=Intel::GetNameFromIntel(static_cast<EIntel>(I));
 // Disturb temporary allocations before consuming the returned pointer.
 TArray<FString> Noise;for(int J=0;J<20;++J)Noise.Add(FString::ChrN(64,TCHAR('a'+J)));
 TestEqual(TEXT("Stable legacy intel token"),FString(UTF8_TO_TCHAR(Name)),FString(UTF8_TO_TCHAR(Names[I])));
 if(I>0)TestEqual(TEXT("Name round trips"),Intel::GetIntelFromName(Name),static_cast<EIntel>(I));}
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeAsteroidScaleTest,"Starshatter.Parity.P03.AsteroidLibraryScale",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeAsteroidScaleTest::RunTest(const FString&){
 Asteroid::Initialize(); // Must work without parsing any legacy MAG geometry.
 const auto* Library=GetDefault<UAsteroidLibrarySettings>();TestEqual(TEXT("Six library variants configured"),Library->Meshes.Num(),6);
 for(const auto& Ref:Library->Meshes){auto* Mesh=Ref.LoadSynchronous();if(TestNotNull(TEXT("Imported asteroid library mesh"),Mesh)){
 const double Radius=Mesh->GetBounds().SphereRadius;TestTrue(TEXT("Nonzero imported bounds"),Radius>0);TestEqual(TEXT("Raw radius preserved"),ANativeAsteroidActor::RawRadius(Radius),Radius);
 AddInfo(FString::Printf(TEXT("Asteroid %s raw radius retained %.3f, mesh scale 1"),*Mesh->GetPathName(),Radius));}}
 for(int32 Index=0;Index<6 && Library->Meshes.Num()>0;++Index){
 if(auto* Mesh=Library->Meshes[Index%Library->Meshes.Num()].LoadSynchronous()){
 Asteroid Rock(Index,FVector::ZeroVector,100);
 TestEqual(TEXT("Simulation keeps the selected pack mesh raw radius"),double(Rock.GetRadius()),double(float(Mesh->GetBounds().SphereRadius)));
 TestEqual(TEXT("Stable pack variant"),Rock.GetMeshVariant(),Index);
 }}
 TestEqual(TEXT("Invalid radius rejected"),ANativeAsteroidActor::RawRadius(0),0.0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldHullIntegrationTest,"Starshatter.Parity.P02.ShieldHullIntegration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShieldHullIntegrationTest::RunTest(const FString&){
 FRestoreParityClock Restore;Game::SetFrameClock(.1,1,false);
 ShipDesign D;D.type=static_cast<int>(CLASSIFICATION::FIGHTER);D.integrity=1000;D.mass=20;D.radius=100;D.scale=1;
 auto* Template=new Shield(EShieldType::DEFLECTOR);Template->SetShieldCapacitor(true);D.shields.append(Template);D.shield=Template;
 Ship S("Shield hull fixture","",&D,1,0,nullptr,false);auto* Shield=S.GetShield();if(!TestNotNull(TEXT("Runtime shield copied"),Shield))return false;
 Shield->SetPowerLevel(100);Shield->ExecFrame(10);Shield->Distribute(Shield->GetCapacity(),1);
 WeaponDesign W;W.life=2;W.damage=100;W.penetration=1;W.mass=1;SimShot Shot(FVector::ZeroVector,Camera(),&W);
 const double Before=S.GetIntegrity();const double Applied=S.InflictDamage(100,&Shot,3,FVector::ZeroVector);
 TestTrue(TEXT("Shield leaks legacy five percent into hull"),FMath::IsNearlyEqual(Applied,5.0,.001));TestTrue(TEXT("Hull records only shield leakage"),FMath::IsNearlyEqual(Before-S.GetIntegrity(),5.0,.001));
 return true;
}
#endif
