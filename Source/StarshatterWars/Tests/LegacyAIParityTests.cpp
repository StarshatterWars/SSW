#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "SteerAI.h"
#include "ShipAI.h"
#include "Ship.h"
#include "TacticalAI.h"
#include "LegacyAIParity.h"
#include "ShipDesignRegistry.h"
#include "ShipDesign.h"
#include "SimRegion.h"
#include "SimContact.h"
#include "Game.h"
#include "RLoc.h"
#include <cstdlib>

namespace {
class FParitySteering : public SteerAI {
public:
    FParitySteering() : SteerAI(nullptr) {}
    using SteerAI::Seek;
    using SteerAI::Flee;
    using SteerAI::Avoid;
};
class FParityShipAI : public ShipAI {
public:
    explicit FParityShipAI(Ship* S) : ShipAI(S) {}
    FVector SteeringGoal() const { return objective; }
};
class FParityTactical : public TacticalAI {
public:
    explicit FParityTactical(ShipAI* AI) : TacticalAI(AI) {}
    using TacticalAI::SelectTargetOpportunity;
    using TacticalAI::SelectTargetDirected;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacySteeringParityTest,
 "Starshatter.AIParity.SteeringDifferential", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLegacySteeringParityTest::RunTest(const FString& Parameters)
{
    // Reference equations: Stars45/SteerAI.cpp, legacy commit 9972413.
    // Keep independent history; compare forward/reverse quadrants over a sequence.
    FParitySteering AI;
    double Az[3]={}, El[3]={};
    FRandomStream Samples(40921);
    for (int32 I=0; I<1024; ++I) {
        const FVector Input(Samples.FRandRange(-10000.f,10000.f), Samples.FRandRange(-10000.f,10000.f), Samples.FRandRange(-10000.f,10000.f));
        const FVector P=Input/Input.Size();
        Az[2]=Az[1]; Az[1]=Az[0]; El[2]=El[1]; El[1]=El[0];
        double Yaw, Pitch;
        if (P.Z>0) {
            Az[0]=atan2(fabs(P.X),P.Z)*20; El[0]=atan2(fabs(P.Y),P.Z)*20;
            if(P.X<0) Az[0]=-Az[0]; if(P.Y>0) El[0]=-El[0];
            Yaw=Az[0]-.5*(Az[1]+Az[2]*.5); Pitch=El[0]-.5*(El[1]+El[2]*.5);
        } else { Yaw=P.X>0?1:-1; Pitch=-P.Y*.5; }
        const Steer Actual=AI.Seek(Input);
        if (!TestTrue(TEXT("Seek preserves legacy gain, damping, history and signs"), fabs(Actual.yaw-Yaw)<1.e-9 && fabs(Actual.pitch-Pitch)<1.e-9)) return false;
        const Steer Flee=AI.Flee(Input);
        const double FY=P.Z>0?(P.X>0?-1:1):-P.X;
        const double FP=P.Z>0?0:P.Y;
        if(!TestTrue(TEXT("Flee preserves legacy forward/reverse decisions"), fabs(Flee.yaw-FY)<1.e-9 && fabs(Flee.pitch-FP)<1.e-9)) return false;
        const double Radius=700;
        double AY=0, AP=0;
        if(Input.Z>0) {
            const double X=Radius-fabs(Input.X), Y=Radius-fabs(Input.Y);
            if(X<Y) { AY=atan2(X,Input.Z)*20; if(Input.X>0) AY=-AY; }
            else { AP=atan2(Y,Input.Z)*20; if(Input.Y<0) AP=-AP; }
        }
        const Steer Avoid=AI.Avoid(Input,Radius);
        if(!TestTrue(TEXT("Avoid preserves legacy clearance axis and turn sign"), fabs(Avoid.yaw-AY)<1.e-9 && fabs(Avoid.pitch-AP)<1.e-9)) return false;
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacyTacticalParityTest,
 "Starshatter.AIParity.TacticalContactPriority", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLegacyTacticalParityTest::RunTest(const FString& Parameters)
{
    const auto Original=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT { ShipDesignRegistry::Clear(); for(const auto& E:Original) ShipDesignRegistry::RegisterDesign(E.Key,E.Value); };
    auto Design=[](const TCHAR* Name,const TCHAR* Class) {
        FShipDesign Row; Row.ShipName=Name; Row.ShipClass=Class; Row.ShipType=FCString::Stricmp(Class,TEXT("Frigate"))==0 ? static_cast<uint32>(CLASSIFICATION::FRIGATE) : (FCString::Stricmp(Class,TEXT("Cruiser"))==0 ? static_cast<uint32>(CLASSIFICATION::CRUISER) : static_cast<uint32>(CLASSIFICATION::FIGHTER)); Row.Mass=20; Row.Integrity=100; Row.CommitRange=100000;
        ShipDesignRegistry::RegisterDesign(FName(Name),Row); return ShipDesignRegistry::FindLegacy(FName(Name));
    };
    ShipDesign* Frigate=Design(TEXT("ParityFrigate"),TEXT("Frigate"));
    ShipDesign* Cruiser=Design(TEXT("ParityCruiser"),TEXT("Cruiser"));
    ShipDesign* Fighter=Design(TEXT("ParityFighter"),TEXT("Fighter"));
    SimRegion Region(nullptr,"AI parity fixture",SimRegion::REAL_SPACE);
    Ship* Observer=new Ship("Observer","",Frigate,1,0,nullptr,false);
    Ship* Large=new Ship("Large enemy","",Cruiser,2,0,nullptr,false);
    Ship* Small=new Ship("Fighter enemy","",Fighter,2,0,nullptr,false);
    Ship* Friend=new Ship("Friendly","",Fighter,1,0,nullptr,false);
    for(Ship* S:{Observer,Large,Small,Friend}) Region.InsertObject(S);
    Observer->MoveTo(FVector::ZeroVector); Large->MoveTo(FVector(4000,0,0)); Small->MoveTo(FVector(5000,0,0)); Friend->MoveTo(FVector(100,0,0));
    TestTrue(TEXT("Fixture is a frigate"),Observer->GetClassification()==CLASSIFICATION::FRIGATE);
    Observer->GetContactList().append(new SimContact(Large,1.e9f,1.e9f));
    Observer->GetContactList().append(new SimContact(Friend,1.e9f,1.e9f));
    Observer->GetContactList().append(new SimContact(Small,1.e9f,1.e9f));
    FParityShipAI AI(Observer); FParityTactical Tactical(&AI);
    Tactical.SelectTargetOpportunity();
    TestTrue(TEXT("Frigate selects eligible enemy aircraft instead of first/nearer cruiser or friendly"), AI.GetTarget()==Small);
    Tactical.SelectTargetDirected(Friend);
    TestNull(TEXT("Directed orders cannot attack a friendly non-rogue"),AI.GetTarget());
    Tactical.SelectTargetDirected(Large);
    TestTrue(TEXT("Explicit directed orders can target larger enemy ship"),AI.GetTarget()==Large);
    AI.SetTarget(Small); Observer->SetWard(Friend); Observer->SetHeading(.13,.29,-.71);
    AI.FindObjective();
    TestTrue(TEXT("Combat target retains priority over escort ward"),AI.GetObjective().Equals(Small->GetLocation()));
    const FVector Delta=Small->GetLocation()-Observer->GetLocation();
    const Camera& Cam=Observer->GetCam();
    const FVector LegacyAim(FVector::DotProduct(Delta,Cam.vrt()),FVector::DotProduct(Delta,Cam.vup()),FVector::DotProduct(Delta,Cam.vpn()));
    TestTrue(TEXT("Combat objective uses legacy camera-relative aiming"),AI.SteeringGoal().Equals(LegacyAim,1.e-8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacyRecoveryEnumsTest,
 "Starshatter.AIParity.RecoveryEnumsAndCoordinates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLegacyRecoveryEnumsTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Authored RTB remains RTB despite split Unreal enums"),LegacyAIParity::RecoveryAction(EInstruction::RTB,RadioMessageAction::NONE)==RadioMessageAction::RTB);
    TestTrue(TEXT("Authored Dock remains DOCK_WITH"),LegacyAIParity::RecoveryAction(EInstruction::Dock,RadioMessageAction::NONE)==RadioMessageAction::DOCK_WITH);
    TestTrue(TEXT("Explicit radio action retains priority"),LegacyAIParity::RecoveryAction(EInstruction::Dock,RadioMessageAction::RTB)==RadioMessageAction::RTB);
    const FVector Map(123,-456,789), Sim(-123,789,-456);
    TestTrue(TEXT("Map to simulation conversion uses legacy handedness and Y/Z swap"),OtherHand(Map).Equals(Sim));
    TestTrue(TEXT("Conversion round trip preserves all axes"),OtherHand(Sim).Equals(Map));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacyRouteRandomTest,
 "Starshatter.AIParity.RelativeRouteRandomSequence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLegacyRouteRandomTest::RunTest(const FString& Parameters)
{
    // Deterministic isolated automation run; legacy MSVC rand()/32768 stream.
    srand(6137);
    RLoc Route(FVector(321,-765,999),25000,5000);
    const FVector Actual=Route.Location();
    const int ActualNext=rand();
    srand(6137);
    const double D=25000.0-5000.0+10000.0*rand()/32768.0;
    const double AV=static_cast<float>(3.1415f), EV=static_cast<float>(.1f);
    const double A=-AV+(2*AV)*rand()/32768.0;
    const double E=-EV+(2*EV)*rand()/32768.0;
    const FVector Expected=FVector(321,-765,999)+FVector(D*sin(A),-D*cos(A),D*sin(E));
    const int ExpectedNext=rand();
    TestTrue(TEXT("Relative route has legacy sampled offset"),Actual.Equals(Expected,1.e-8));
    TestEqual(TEXT("Relative route consumes the same RNG stream"),ActualNext,ExpectedNext);
    TestTrue(TEXT("Resolved route is stable on repeated reads"),Route.Location().Equals(Actual));
    return true;
}
#endif
