#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ShipAI.h"
#include "StarshipAI.h"
#include "ShipManager.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "SimRegion.h"
#include "Asteroid.h"
#include "Instruction.h"
#include "FlightDeck.h"
#include "Physical.h"
#include "Camera.h"
#include "Game.h"
#include "DataLoader.h"
#include <type_traits>
#include "TimerSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "ShipActor.h"
#include "CarrierActor.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"

namespace {
// Exercise the production subsystem lifecycle without initializing unrelated game systems.
// Member pointers retain the actual UTimerSubsystem dynamic type.
struct FClockLifecycleAccess : UTimerSubsystem {
    static void Start(UTimerSubsystem& Clock,FSubsystemCollectionBase& C){auto Fn=&FClockLifecycleAccess::Initialize;(Clock.*Fn)(C);}
    static void Stop(UTimerSubsystem& Clock){auto Fn=&FClockLifecycleAccess::Deinitialize;(Clock.*Fn)();}
};
struct FRemainingDesigns {
    std::decay_t<decltype(ShipDesignRegistry::GetAll())> Original=ShipDesignRegistry::GetAll();
    ~FRemainingDesigns(){ShipDesignRegistry::Clear();for(const auto& E:Original)ShipDesignRegistry::RegisterDesign(E.Key,E.Value);}
    ShipDesign* Add(const TCHAR* Name,CLASSIFICATION Class) {
        FShipDesign Row;Row.ShipName=Name;Row.ShipType=static_cast<uint32>(Class);Row.Mass=20;Row.Integrity=100;Row.CommitRange=100000;
        ShipDesignRegistry::RegisterDesign(FName(Name),Row);return ShipDesignRegistry::FindLegacy(FName(Name));
    }
};
class FRouteProbe : public ShipAI {
public:
    explicit FRouteProbe(Ship* S):ShipAI(S){}
    void Resolve(Instruction* N){navpt=N;FindObjectiveNavPoint();}
};
class FCapitalProbe : public StarshipAI {
public:
    explicit FCapitalProbe(Ship* S):StarshipAI(S){}
    void ThrottleAt(Instruction* N,const FVector& Goal,double D){navpt=N;obj_w=Goal;distance=D;seconds=.01;ThrottleControl();}
    void Follow(Instruction* N){navpt=N;SeekTarget();}
};
class FPhysicalProbe : public Physical {
public:
    void Configure(SimDirector* D=nullptr){dir=D;mass=7.25f;thrust=123.75f;trans_x=-18.25f;trans_y=7.5f;trans_z=13.125f;drag=.03125f;g_accel=.125f;cam.Aim(.13,-.29,.37);}
    void ConfigureCollision(float M,const FVector& V){mass=M;velocity=V;}
    void Aerodynamic(){Do=.01f;CL=.0001f;CD=.00001f;stall=.3f;drag=.0001f;}
    void ArcadeVelocity(const FVector& V){arcade_velocity=V;}
};
class FRegionProbe : public SimRegion {
public:
    using SimRegion::SimRegion;
    using SimRegion::UpdateShips;
};
class FRegionShipProbe : public Ship {
public:
    TArray<int> Modes;
    FRegionShipProbe(const char* N,ShipDesign* D):Ship(N,"",D,1,0,nullptr,false){}
    void ExecFrame(double) override {Modes.Add(GetAIMode());}
};
class FSliceProbe : public ShipAI {
public:
    TArray<double> Calls;
    FSliceProbe():ShipAI(nullptr){}
    void ExecFrame(double S) override {Calls.Add(S);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingCameraTest,"Starshatter.AIParity.Remaining.CameraReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingCameraTest::RunTest(const FString& Parameters)
{
    Camera C(100000001.125,-200000002.25,300000003.5);
    TestTrue(TEXT("Scalar camera constructor preserves double coordinates"),C.Pos().Equals(FVector(100000001.125,-200000002.25,300000003.5),1.e-9));
    C.MoveTo(100000001.125,-200000002.25,300000003.5);C.MoveBy(.125,.25,.5);
    TestTrue(TEXT("Legacy scalar MoveBy swaps only its Y/Z arguments"),C.Pos().Equals(FVector(100000001.25,-200000001.75,300000003.75),1.e-9));
    C.MoveBy(FVector(1,2,3));TestTrue(TEXT("Vector MoveBy remains conventional"),C.Pos().Equals(FVector(100000002.25,-199999999.75,300000006.75),1.e-9));
    // Legacy Camera.h forwards rotations directly to Matrix, without renormalization.
    Matrix Reference;Reference.Identity();
    FRandomStream Stream(75991);
    for(int32 I=0;I<10000;++I){const double R=Stream.FRandRange(-.01f,.01f),P=Stream.FRandRange(-.01f,.01f),Y=Stream.FRandRange(-.01f,.01f);C.Aim(R,P,Y);Reference.Rotate(R,P,Y);C.Yaw(Y);Reference.Yaw(Y);C.Pitch(P);Reference.Pitch(P);C.Roll(R);Reference.Roll(R);}
    double Error=0;for(int I=0;I<3;++I)for(int J=0;J<3;++J)Error=FMath::Max(Error,FMath::Abs(C.Orientation()(I,J)-Reference(I,J)));
    TestTrue(TEXT("10000 camera steps match legacy matrix forwarding"),Error<1.e-12);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingRouteTest,"Starshatter.AIParity.Remaining.AuthoredRoutesAndArrival",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingRouteTest::RunTest(const FString& Parameters)
{
    FRemainingDesigns Designs;ShipDesign* Design=Designs.Add(TEXT("RemainingCapital"),CLASSIFICATION::CRUISER);
    SimRegion Region(nullptr,"Route fixture",SimRegion::REAL_SPACE);
    Ship* S=new Ship("Capital","",Design,1,0,nullptr,false);Ship* Named=new Ship("Named object","",Design,2,0,nullptr,false);Region.InsertObject(S);Region.InsertObject(Named);
    S->MoveTo(FVector::ZeroVector);Named->MoveTo(FVector(90000,80000,70000));
    Instruction Route(&Region,FVector(23000,-17000,12000));Route.SetTarget(Named);Route.SetFarcast(1);
    FRouteProbe AI(S);AI.Resolve(&Route);TestTrue(TEXT("NPC named route retains authored point rather than target origin"),AI.GetObjective().Equals(OtherHand(Route.GetLocation())));
    Route.SetLocation(FVector(-1000,0,0));AI.Resolve(&Route);TestTrue(TEXT("Exactly 1000 units does not complete legacy route"),Route.GetStatus()!=INSTRUCTION_STATUS::COMPLETE);
    Route.SetLocation(FVector(-999.9,0,0));AI.Resolve(&Route);TestTrue(TEXT("Below 1000 units completes route"),Route.GetStatus()==INSTRUCTION_STATUS::COMPLETE);
    Route.SetStatus(INSTRUCTION_STATUS::PENDING);Route.SetLocation(FVector(-3000,0,0));
    FCapitalProbe Capital(S);Capital.ThrottleAt(&Route,FVector(3000,0,0),3000);Capital.Follow(&Route);
    TestTrue(TEXT("Farcast route does not auto-complete at 3000 units"),Route.GetStatus()!=INSTRUCTION_STATUS::COMPLETE);
    Capital.SetTarget(Named);Capital.FindObjective();
    TestTrue(TEXT("Capital can acquire combat goal after previous waypoint completion"),Capital.GetTarget()==Named && !Capital.GetObjective().IsNearlyZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingDeckTest,"Starshatter.AIParity.Remaining.DeckQueueBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingDeckTest::RunTest(const FString& Parameters)
{
    const bool OwnLoader=DataLoader::GetLoader()==nullptr; if(OwnLoader)DataLoader::Initialize();
    ON_SCOPE_EXIT {if(OwnLoader)DataLoader::Close();};
    FRemainingDesigns Designs;auto* Design=Designs.Add(TEXT("RemainingFighter"),CLASSIFICATION::FIGHTER);auto* CarrierDesign=Designs.Add(TEXT("RemainingCarrier"),CLASSIFICATION::CARRIER);
    SimRegion Region(nullptr,"Deck fixture",SimRegion::REAL_SPACE);
    Ship* Carrier=new Ship("Carrier","",CarrierDesign,1,0,nullptr,false);Ship* First=new Ship("First","",Design,1,0,nullptr,false);Ship* Second=new Ship("Second","",Design,1,0,nullptr,false);
    for(Ship* S:{Carrier,First,Second})Region.InsertObject(S);
    FlightDeck Deck;Deck.SetRecoveryDeck();Deck.SetCarrier(Carrier);Deck.AddSlot(FVector::ZeroVector,static_cast<uint32>(CLASSIFICATION::FIGHTER));Deck.AddSlot(FVector(100,0,0),static_cast<uint32>(CLASSIFICATION::FIGHTER));Deck.SetMaxShips(1);Deck.SetStartPoint(FVector(0,0,-2000));Deck.SetEndPoint(FVector(0,0,-500));Deck.SetCamLoc(FVector::ZeroVector);Deck.SetBoundingBox(FVector(1000,500,2000));Deck.Orient(Carrier);
    TestEqual(TEXT("SpaceLeft counts all compatible physical spots, as legacy"),Deck.SpaceLeft(static_cast<int>(CLASSIFICATION::FIGHTER)),2);
    int A=0,B=1;TestTrue(TEXT("First spot accepted"),Deck.Spot(First,A));TestTrue(TEXT("Second authored spot is usable despite presentation max_ships"),Deck.Spot(Second,B));Deck.Clear(0);Deck.Clear(1);
    // Clear sets a five-second cycle; one frame advances it to two seconds.
    Deck.ExecFrame(3.0);First->MoveTo(Deck.GetMountLocation()+FVector(46000,0,0));
    InboundSlot* Slot=new InboundSlot(First,&Deck,0,7);Deck.Inbound(Slot);
    TestTrue(TEXT("First inbound clears at cycle<=3 even beyond 35km"),Slot->Cleared()!=0);
    Deck.GrantClearance();TestFalse(TEXT("Clearance revoked beyond 45km"),Slot->Cleared()!=0);
    First->MoveTo(Deck.GetMountLocation()+FVector(36000,0,0));Deck.GrantClearance();TestFalse(TEXT("Uncleared aircraft at 36km holds"),Slot->Cleared()!=0);
    First->MoveTo(Deck.GetMountLocation()+FVector(34000,0,0));Deck.GrantClearance();TestTrue(TEXT("Aircraft inside 35km clears"),Slot->Cleared()!=0);
    Second->MoveTo(Deck.GetMountLocation()+FVector(1000,0,0));InboundSlot* Other=new InboundSlot(Second,&Deck,0,8);Deck.Inbound(Other);
    TestTrue(TEXT("Already-cleared aircraft retains priority over nearer newcomer"),Deck.GetRecoveryQueue()[0]==Slot && !Other->Cleared());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingPhysicsTest,"Starshatter.AIParity.Remaining.FixedStepPhysicsReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingPhysicsTest::RunTest(const FString& Parameters)
{
    const double SavedStep=Physical::GetSubFrameLength();ON_SCOPE_EXIT{Physical::SetSubFrameLength(SavedStep);};Physical::SetSubFrameLength(1.0/60.0);
    FPhysicalProbe Actual;auto* Director=new FSliceProbe;Actual.Configure(Director);
    const FVector Start(100000001.125,-70000000.5,200000000.75),Velocity(97.123456789,-43.3125,17.8125);Actual.MoveTo(Start);Actual.SetVelocity(Velocity);
    FVector P=Start,V=Velocity,Acc=FVector::ZeroVector;const Camera& Cam=Actual.GetCam();int ExpectedCalls=0;
    volatile float Mass=7.25f,Thrust=123.75f,TX=-18.25f,TY=7.5f,TZ=13.125f;
    const float Drag=.03125f,Gravity=.125f;
    // Reference equations and float scalar inputs: legacy nGenEx Physical.cpp 9972413.
    for(int Frame=0;Frame<600;++Frame){double Left=Frame%3==0?.0417:(Frame%3==1?.001:.0333);const double Dt=Left;const FVector Before=V;double Slice=Left;
        while(Left>0){Slice=Left>1.0/60.0?1.0/60.0:Left;++ExpectedCalls;V+=Cam.vpn()*((Thrust/Mass)*Slice);V+=Cam.vrt()*((TX/Mass)*Slice);V+=Cam.vpn()*((TY/Mass)*Slice);V+=Cam.vup()*((TZ/Mass)*Slice);V+=FVector(0,-Gravity,0)*Slice;V*=exp(-Drag*Slice);P+=V*Slice;Left-=Slice;}
        Acc=(V-Before)*(1/Slice);Actual.ExecFrame(Dt);
        if(!TestTrue(TEXT("Fixed-step position matches legacy reference"),Actual.GetLocation().Equals(P,1.e-7)))return false;
        if(!TestTrue(TEXT("Fixed-step velocity matches legacy reference"),Actual.GetVelocity().Equals(V,1.e-10))){AddError(FString::Printf(TEXT("Frame=%d actual=(%.17g %.17g %.17g) reference=(%.17g %.17g %.17g)"),Frame,Actual.GetVelocity().X,Actual.GetVelocity().Y,Actual.GetVelocity().Z,V.X,V.Y,V.Z));return false;}
        if(!TestTrue(TEXT("Legacy acceleration uses final integration slice"),Actual.GetAcceleration().Equals(Acc,1.e-8)))return false;
    }
    TestEqual(TEXT("ShipAI legacy Subframe dispatch occurs once per physics slice"),Director->Calls.Num(),ExpectedCalls);
    ShipManager Manual(nullptr,nullptr);TestTrue(TEXT("ShipManager retains legacy subframe dispatch"),static_cast<SimDirector*>(&Manual)->GetSubframe());
    FPhysicalProbe A,B;const FVector VA(103.123456789,-37.5,12.25),VB(-77.987654321,5.125,99.875);const float MA=8.25f,MB=15.75f;A.ConfigureCollision(MA,VA);B.ConfigureCollision(MB,VB);
    const double Sum=MA+MB,Delta=MA-MB;Physical::ElasticCollision(A,B);
    TestTrue(TEXT("Elastic collision preserves legacy double vector arithmetic"),A.GetVelocity().Equals((VB*(2*MB)+VA*Delta)*(1/Sum),1.e-12));
    A.ConfigureCollision(MA,VA);B.ConfigureCollision(MB,VB);Physical::InelasticCollision(A,B);
    TestTrue(TEXT("Inelastic collision preserves legacy momentum"),A.GetVelocity().Equals((VA*MA+VB*MB)*(1/Sum),1.e-12));
    A.ConfigureCollision(MA,VA);B.ConfigureCollision(MB,VB);Physical::SemiElasticCollision(A,B);
    TestTrue(TEXT("Semi-elastic collision preserves legacy mix"),A.GetVelocity().Equals((VB*(2*MB)+VA*Delta)*(1/Sum)*.65+(VA*MA+VB*MB)*(1/Sum)*.35,1.e-12));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingFlightModelsTest,"Starshatter.AIParity.Remaining.AtmosphericAndArcadeReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingFlightModelsTest::RunTest(const FString& Parameters)
{
    const double Saved=Physical::GetSubFrameLength();ON_SCOPE_EXIT{Physical::SetSubFrameLength(Saved);};Physical::SetSubFrameLength(1./60.);
    for(bool Air:{true,false}) {
        FPhysicalProbe Actual;Actual.Configure();if(Air)Actual.Aerodynamic();
        FVector P(100000000.125,10000.375,-200000000.25),V(220.125,15.375,350.875),Arc=Air?FVector::ZeroVector:Actual.GetCam().vpn()*32.125;
        Actual.MoveTo(P);Actual.SetVelocity(V);Actual.ArcadeVelocity(Arc);
        volatile float Mass=7.25f,Thrust=123.75f,TX=-18.25f,TY=7.5f,TZ=13.125f,Gravity=.125f,Drag=Air?.0001f:.03125f,CL=.0001f,CD=.00001f,Stall=.3f,Density=.01f;
        const Camera& Cam=Actual.GetCam();
        for(int32 Frame=0;Frame<400;++Frame) {
            double Left=Frame%2?.0237:.0371;const double Dt=Left;
            while(Left>0) {
                const double H=Left>1./60.?1./60.:Left;
                if(Air) {
                    V+=Cam.vpn()*((Thrust/Mass)*H);V+=FVector(0,-Gravity,0)*H;
                    const double Speed=V.Size(),Rho=.75*Density*(250000-P.Y)/250000;
                    FVector Dir=V/Speed;
                    if(Speed>150) {
                        const double Sq=(Speed-150)*(Speed-150);FVector Flat=Dir-Cam.vrt()*FVector::DotProduct(Dir,Cam.vrt());Flat/=Flat.Size();
                        const double Cos=FVector::DotProduct(Flat,Cam.vpn());float Alpha=Cos>=1?0.f:static_cast<float>(acos(Cos));if(FVector::DotProduct(Flat,Cam.vup())>0)Alpha=-Alpha;
                        const double Lift=Alpha<=Stall?(CL*Alpha)*Rho*Sq:(CL*(2*Stall-Alpha))*Rho*Sq;
                        V+=Cam.vup()*Lift*H;const double AlphaSq=Alpha*Alpha;const double Eff=(Drag+CD*AlphaSq)*Rho*Sq;
                        Dir=V/V.Size();V+=Dir*-Eff*H;
                    } else V*=exp(-Drag*H);
                    P+=V*H;
                } else {
                    const double Speed=Arc.Size();FVector Unit=Speed>1.e-9?Arc/Speed:FVector::ZeroVector;
                    Arc=Cam.vpn()*(Speed*pow(FVector::DotProduct(Unit,Cam.vpn()),30));Arc+=Cam.vpn()*((Thrust/Mass)*H);Arc*=exp(-Drag*H);
                    V+=Cam.vrt()*((TX/Mass)*H);V+=Cam.vpn()*((TY/Mass)*H);V+=Cam.vup()*((TZ/Mass)*H);V+=FVector(0,-Gravity,0)*H;V*=exp(-Drag*H);P+=Arc*H+V*H;
                }
                Left-=H;
            }
            if(Air)Actual.AeroFrame(Dt);else Actual.ArcadeFrame(Dt);
            if(!TestTrue(Air?TEXT("Atmospheric trajectory matches legacy equations"):TEXT("Arcade trajectory matches legacy equations"),Actual.GetLocation().Equals(P,1.e-6) && Actual.GetVelocity().Equals(V+Arc,1.e-7))) {
                AddError(FString::Printf(TEXT("Mode=%s Frame=%d position error=%.12g velocity error=%.12g"),Air?TEXT("Air"):TEXT("Arcade"),Frame,(Actual.GetLocation()-P).Size(),(Actual.GetVelocity()-V-Arc).Size()));return false;
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingRegionTest,"Starshatter.AIParity.Remaining.RegionSchedulingAndDocking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingRegionTest::RunTest(const FString& Parameters)
{
    FRemainingDesigns Designs;auto* D=Designs.Add(TEXT("SchedulingFixture"),CLASSIFICATION::FIGHTER);
    FRegionProbe R(nullptr,"Scheduling fixture",SimRegion::REAL_SPACE);
    auto* A=new FRegionShipProbe("A",D);auto* B=new FRegionShipProbe("B",D);auto* C=new FRegionShipProbe("C",D);
    for(auto* S:{A,B,C})R.InsertObject(S);R.SetPlayerShip(C);
    for(int I=0;I<5;++I)R.UpdateShips(.02);
    const TArray<int> ExpectA={2,1,1,1,2},ExpectB={1,2,1,1,1},ExpectPlayer={2,2,2,2,2};
    TestTrue(TEXT("Legacy sensor round robin and wrap gap preserved"),A->Modes==ExpectA && B->Modes==ExpectB);
    TestTrue(TEXT("Player receives full sensor assessment every frame"),C->Modes==ExpectPlayer);
    B->SetFlightPhase(EOPSMode::DOCKED);R.ExecFrame(.02);
    TestFalse(TEXT("Normal region frame consumes a docked ship"),R.GetShips().contains(B));
    TestTrue(TEXT("Other ships remain owned by the region"),R.GetShips().contains(A)&&R.GetShips().contains(C));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAsteroidTumbleTest, "Starshatter.Asteroid.ActiveRegionTumble",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAsteroidTumbleTest::RunTest(const FString& Parameters)
{
    FRegionProbe Region(nullptr, "Asteroid fixture", SimRegion::REAL_SPACE);
    const FVector Position(30000.0, 100.0, 500.0);
    auto* Rock = new Asteroid(3, Position, 5e7);
    Region.InsertObject(Rock);
    const FVector OriginalHeading = Rock->GetCam().vpn();
    const FVector OriginalUp = Rock->GetCam().vup();

    Region.ExecFrame(0.1);
    TestTrue(TEXT("Inactive region leaves the rock orientation alone"),
        Rock->GetCam().vpn().Equals(OriginalHeading) && Rock->GetCam().vup().Equals(OriginalUp));

    Region.Activate();
    for (int Step = 0; Step < 20; ++Step)
        Region.ExecFrame(0.05);

    TestTrue(TEXT("The active asteroid visibly tumbles within a second"),
        FVector::Distance(Rock->GetCam().vpn(), OriginalHeading) > 0.08 ||
        FVector::Distance(Rock->GetCam().vup(), OriginalUp) > 0.08);
    TestTrue(TEXT("Spin does not move the asteroid"), Rock->GetLocation().Equals(Position));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemainingCollisionPassTest,"Starshatter.AIParity.Remaining.RegionCollisionDispatch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRemainingCollisionPassTest::RunTest(const FString& Parameters)
{
    if (UStarshatterEnvironmentSubsystem::Get()) {AddError(TEXT("Run collision replay outside an active play session"));return false;}
    auto* Clock=NewObject<UStarshatterEnvironmentSubsystem>(NewObject<UGameInstance>());
    UStarshatterEnvironmentSubsystem::ActiveInstance=Clock;
    ON_SCOPE_EXIT {UStarshatterEnvironmentSubsystem::ActiveInstance=nullptr;};
    Clock->SetGameTime(60000); // Ships wait a minute in the hangar before launch.
    FRemainingDesigns Designs;auto* D=Designs.Add(TEXT("CollisionFixture"),CLASSIFICATION::FIGHTER);
    SimRegion R(nullptr,"Collision fixture",SimRegion::REAL_SPACE);
    auto* A=new FRegionShipProbe("Pilot",D);auto* B=new FRegionShipProbe("AI",D);R.InsertObject(A);R.InsertObject(B);R.SetPlayerShip(A);
    A->SetFlightPhase(EOPSMode::ACTIVE);B->SetFlightPhase(EOPSMode::ACTIVE);A->MoveTo(FVector::ZeroVector);B->MoveTo(FVector::ZeroVector);
    A->SetVelocity(FVector(5,0,0));B->SetVelocity(FVector::ZeroVector);
    R.CollideShips();TestTrue(TEXT("Legacy ten-second launch collision grace retained"),A->GetVelocity().Equals(FVector(5,0,0)));
    TestEqual(TEXT("A delayed launch starts its own clock at zero"),A->GetMissionClockMS(),0);
    Clock->SetGameTime(69999);R.CollideShips();
    TestTrue(TEXT("Hangar time cannot consume launch grace"),A->GetVelocity().Equals(FVector(5,0,0)));
    Clock->SetGameTime(80000);R.SetPlayerShip(nullptr);R.CollideShips();
    TestTrue(TEXT("AI fighter pair collisions remain excluded"),A->GetVelocity().Equals(FVector(5,0,0)));
    R.SetPlayerShip(A);A->SetCarrier(B,nullptr);A->BeginCarrierLaunchClearance();R.CollideShips();
    TestTrue(TEXT("Unreal launch corridor protects only own carrier beyond grace"),A->GetVelocity().Equals(FVector(5,0,0)));
    TestFalse(TEXT("Launch protection cannot suppress another ship"),A->IsClearingLaunchCarrier(A));
    A->MoveTo(FVector(100000,0,0));TestFalse(TEXT("Launch protection ends after first hull exit"),A->IsClearingLaunchCarrier(B));
    A->MoveTo(FVector::ZeroVector);TestFalse(TEXT("Returning to the carrier cannot reactivate launch protection"),A->IsClearingLaunchCarrier(B));
    R.CollideShips();TestTrue(TEXT("Eligible player collision invokes legacy physical response"),A->GetVelocity().IsNearlyZero() && B->GetVelocity().IsNearlyZero());
    TestTrue(TEXT("Collision preserves surviving ships in the region"),R.GetShips().contains(A)&&R.GetShips().contains(B));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryNativeScaleTest,"Starshatter.AIParity.Remaining.LegacyDeckCoordinates",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryNativeScaleTest::RunTest(const FString& Parameters)
{
    auto* Table=NewObject<UDataTable>();Table->RowStruct=FShipDesign::StaticStruct();
    auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());Importer->ShipDesignDataTable=Table;
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Ships/Orion.def");
    const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadShipDesign(Ansi.Get());
    const auto* Row=Table->FindRow<FShipDesign>(TEXT("Orion"),TEXT("Legacy deck coordinates"));
    if(!TestNotNull(TEXT("Real Orion definition parses"),Row) || !TestEqual(TEXT("All four decks parse"),Row->FlightDeck.Num(),4))return false;
    TestTrue(TEXT("Legacy parser scales recovery start exactly once"),Row->FlightDeck[2].StartPoint.Equals(FVector(1220,0,-7800),0.001));
    TestTrue(TEXT("Legacy parser scales revised launch spot exactly once"),Row->FlightDeck[0].Slots[0].Location.Equals(FVector(180,-80,3100),0.001));
    FRemainingDesigns Designs;
    const bool OwnLoader=DataLoader::GetLoader()==nullptr; if(OwnLoader)DataLoader::Initialize();
    ON_SCOPE_EXIT {if(OwnLoader)DataLoader::Close();};
    Ship Carrier("Carrier","",Designs.Add(TEXT("RecoveryScaleCarrier"),CLASSIFICATION::CARRIER),1,0,nullptr,false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false);
    ON_SCOPE_EXIT {Carrier.BindNativePresentation(nullptr);World->DestroyWorld(false);};
    auto* Actor=World->SpawnActor<ACarrierActor>();Actor->SetActorScale3D(FVector(4));Actor->BindRuntimeShip(&Carrier);Carrier.BindNativePresentation(Actor);
    FlightDeck Deck;Deck.SetRecoveryDeck();Deck.SetCarrier(&Carrier);
    Deck.SetStartPoint(FVector(1220,0,-7800));Deck.SetEndPoint(FVector(1220,-48,-2000));
    Deck.SetApproachPoint(0,FVector(1120,0,-9200));Deck.SetApproachPoint(1,FVector(6000,0,-4000));
    Deck.Orient(&Carrier);
    const auto WorldPoint=[&](FVector Local){return Carrier.GetLocation()+Carrier.GetCam().vrt()*Local.X+Carrier.GetCam().vup()*Local.Y+Carrier.GetCam().vpn()*Local.Z;};
    TestTrue(TEXT("AI outer marker uses actual scale-four carrier lane"),Deck.StartPoint().Equals(WorldPoint(FVector(1220,0,-7800)),0.001));
    TestTrue(TEXT("Approach routes around full-sized hull rather than centerline"),Deck.ApproachPoint(1).Equals(WorldPoint(FVector(6000,0,-4000)),0.001));
    FVector Corners[4];TestTrue(TEXT("Recovery hoop geometry exists"),Deck.GetILSHoopCorners(3,Corners));
    TestTrue(TEXT("Last hoop and AI outer marker coincide"),((Corners[0]+Corners[1]+Corners[2]+Corners[3])*0.25).Equals(Deck.StartPoint(),0.001));
    const FVector CameraPoint=Actor->ConvertDeckWorldToUE(Deck.StartPoint());
    const FVector AuthoredVisual=Actor->GetActorTransform().TransformPosition(FVector(-1950,305,0));
    TestTrue(TEXT("Camera deck conversion applies native scale exactly once"),CameraPoint.Equals(AuthoredVisual,0.001));
    const FVector FighterVisual(Deck.StartPoint().Z,Deck.StartPoint().X,Deck.StartPoint().Y);
    TestTrue(TEXT("Fighter world placement and camera deck focus coincide"),FighterVisual.Equals(CameraPoint,0.001));
    const FVector Once=Deck.StartPoint();Deck.Orient(&Carrier);TestTrue(TEXT("Repeated orientation never compounds scale"),Deck.StartPoint().Equals(Once,0.001));
    Actor->SetActorScale3D(FVector::OneVector);Deck.Orient(&Carrier);
    TestTrue(TEXT("Changing Unreal actor scale cannot alter simulation coordinates"),Deck.StartPoint().Equals(Once,0.001));
    Deck.SetLaunchDeck();Actor->SetActorScale3D(FVector(4));Deck.Orient(&Carrier);
    TestTrue(TEXT("Launch simulation uses the same scaled hull metric as recovery"),Deck.StartPoint().Equals(WorldPoint(FVector(1220,0,-7800)),0.001));
    return true;
}
#endif
