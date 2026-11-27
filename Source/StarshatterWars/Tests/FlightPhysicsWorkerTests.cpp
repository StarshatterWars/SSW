#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "NativeFlightAdapter.h"
#include "NativeSimulationWorker.h"
#include "Physical.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimDirector.h"
#include "DataLoader.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <cstdlib>
namespace
{
struct FFlightTestEnvironment
{
    bool bOwnLoader=false;TUniquePtr<Sim> Owned;
    FFlightTestEnvironment()
    {bOwnLoader=!DataLoader::GetLoader();if(bOwnLoader)DataLoader::Initialize();if(!Sim::GetSim())Owned=MakeUnique<Sim>(nullptr);}
    ~FFlightTestEnvironment(){Owned.Reset();if(bOwnLoader)DataLoader::Close();}
};
FNativeFlightState FlightFixture()
{
    FNativeFlightState S;S.cam.MoveTo(FVector(100,12000,-200));S.cam.Aim(.13,-.29,.37);
    S.velocity=FVector(25,-3,265);S.arcade_velocity=FVector(5,2,120);S.mass=7.25f;S.thrust=123.75f;
    S.trans_x=-18.25f;S.trans_y=7.5f;S.trans_z=13.125f;S.drag=.0001f;
    S.dr=.02f;S.dp=-.03f;S.dy=.01f;S.dr_acc=.001f;S.dp_acc=.002f;S.dy_acc=-.003f;
    S.dr_drg=.2f;S.dp_drg=.3f;S.dy_drg=.4f;S.primary_loc=FVector(0,-6e6,0);S.primary_mass=5e20;
    S.g_accel=9.81f;S.Do=.01f;S.CL=.0001f;S.CD=.00001f;S.stall=.3f;S.alpha=.1f;S.shake=.15f;S.lat_thrust=true;return S;
}
class FFlightProbe:public Physical
{
public:
    void Configure(const FNativeFlightState& S,SimDirector* D=nullptr)
    {
        cam=S.cam;
        velocity=S.velocity;
        arcade_velocity=S.arcade_velocity;
        accel=S.accel;
        primary_loc=S.primary_loc;
        vibration=S.vibration;
        thrust=S.thrust;
        trans_x=S.trans_x;
        trans_y=S.trans_y;
        trans_z=S.trans_z;
        drag=S.drag;
        mass=S.mass;
        roll=S.roll;
        pitch=S.pitch;
        yaw=S.yaw;
        dr=S.dr;
        dp=S.dp;
        dy=S.dy;
        dr_acc=S.dr_acc;
        dp_acc=S.dp_acc;
        dy_acc=S.dy_acc;
        dr_drg=S.dr_drg;
        dp_drg=S.dp_drg;
        dy_drg=S.dy_drg;
        flight_path_yaw=S.flight_path_yaw;
        flight_path_pitch=S.flight_path_pitch;
        g_accel=S.g_accel;
        Do=S.Do;
        CL=S.CL;
        CD=S.CD;
        alpha=S.alpha;
        stall=S.stall;
        shake=S.shake;
        primary_mass=S.primary_mass;
        lat_thrust=S.lat_thrust;
        straight=S.straight;
        dir=D;life=100;
    }
};
class FFlightLCAProbe:public Ship
{
public:
    explicit FFlightLCAProbe(ShipDesign* D):Ship("Flight LCA fixture","",D,1,0,nullptr,false){}
    void Configure(const FNativeFlightState& S)
    {
        cam=S.cam;
        velocity=S.velocity;
        arcade_velocity=S.arcade_velocity;
        accel=S.accel;
        primary_loc=S.primary_loc;
        vibration=S.vibration;
        thrust=S.thrust;
        trans_x=S.trans_x;
        trans_y=S.trans_y;
        trans_z=S.trans_z;
        drag=S.drag;
        mass=S.mass;
        roll=S.roll;
        pitch=S.pitch;
        yaw=S.yaw;
        dr=S.dr;
        dp=S.dp;
        dy=S.dy;
        dr_acc=S.dr_acc;
        dp_acc=S.dp_acc;
        dy_acc=S.dy_acc;
        dr_drg=S.dr_drg;
        dp_drg=S.dp_drg;
        dy_drg=S.dy_drg;
        flight_path_yaw=S.flight_path_yaw;
        flight_path_pitch=S.flight_path_pitch;
        g_accel=S.g_accel;
        Do=S.Do;
        CL=S.CL;
        CD=S.CD;
        alpha=S.alpha;
        stall=S.stall;
        shake=S.shake;
        primary_mass=S.primary_mass;
        lat_thrust=S.lat_thrust;
        straight=S.straight;
        dir=nullptr;life=100;
    }
};
class FFeedbackDirector:public SimDirector
{
public:
    FFlightProbe& Body;bool bSlices;bool bGameThread=true;TArray<FVector> Positions;TArray<double> Durations;
    FFeedbackDirector(FFlightProbe& P,bool B):Body(P),bSlices(B){}
    int Subframe()const override{return bSlices?1:0;}
    void ExecFrame(double Dt)override
    {
        bGameThread&=IsInGameThread();Positions.Add(Body.GetLocation());Durations.Add(Dt);
        Body.SetThrust(125+Body.GetLocation().X*.001);Body.ApplyPitch(.02);Body.ApplyYaw(-.03);Body.ApplyRoll(.01);
        Body.SetTransX(Positions.Num()%2?7.5:-7.5);Body.SetTransY(3);Body.SetTransZ(-2);
    }
};
void RunFlight(Physical& P,ENativeFlightModel M,double Dt)
{if(M==ENativeFlightModel::Inertial)P.Physical::ExecFrame(Dt);else if(M==ENativeFlightModel::Atmospheric)P.Physical::AeroFrame(Dt);else P.Physical::ArcadeFrame(Dt);}
bool SameFlight(FAutomationTestBase& T,const FNativeFlightState& A,const FNativeFlightState& B)
{
    bool Same=true;
    const auto Vector=[&](const TCHAR* Name,const FVector& X,const FVector& Y){Same&=T.TestTrue(Name,X.Equals(Y,1.e-6));};
    Vector(TEXT("Position parity"),A.cam.Pos(),B.cam.Pos());Vector(TEXT("Forward parity"),A.cam.vpn(),B.cam.vpn());Vector(TEXT("Up parity"),A.cam.vup(),B.cam.vup());Vector(TEXT("Right parity"),A.cam.vrt(),B.cam.vrt());
    Vector(TEXT("velocity parity"),A.velocity,B.velocity);
    Vector(TEXT("arcade_velocity parity"),A.arcade_velocity,B.arcade_velocity);
    Vector(TEXT("accel parity"),A.accel,B.accel);
    Vector(TEXT("primary_loc parity"),A.primary_loc,B.primary_loc);
    Vector(TEXT("vibration parity"),A.vibration,B.vibration);
    Same&=T.TestTrue(TEXT("thrust parity"),FMath::Abs(double(A.thrust)-double(B.thrust))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.thrust))));
    Same&=T.TestTrue(TEXT("trans_x parity"),FMath::Abs(double(A.trans_x)-double(B.trans_x))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.trans_x))));
    Same&=T.TestTrue(TEXT("trans_y parity"),FMath::Abs(double(A.trans_y)-double(B.trans_y))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.trans_y))));
    Same&=T.TestTrue(TEXT("trans_z parity"),FMath::Abs(double(A.trans_z)-double(B.trans_z))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.trans_z))));
    Same&=T.TestTrue(TEXT("drag parity"),FMath::Abs(double(A.drag)-double(B.drag))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.drag))));
    Same&=T.TestTrue(TEXT("mass parity"),FMath::Abs(double(A.mass)-double(B.mass))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.mass))));
    Same&=T.TestTrue(TEXT("roll parity"),FMath::Abs(double(A.roll)-double(B.roll))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.roll))));
    Same&=T.TestTrue(TEXT("pitch parity"),FMath::Abs(double(A.pitch)-double(B.pitch))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.pitch))));
    Same&=T.TestTrue(TEXT("yaw parity"),FMath::Abs(double(A.yaw)-double(B.yaw))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.yaw))));
    Same&=T.TestTrue(TEXT("dr parity"),FMath::Abs(double(A.dr)-double(B.dr))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dr))));
    Same&=T.TestTrue(TEXT("dp parity"),FMath::Abs(double(A.dp)-double(B.dp))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dp))));
    Same&=T.TestTrue(TEXT("dy parity"),FMath::Abs(double(A.dy)-double(B.dy))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dy))));
    Same&=T.TestTrue(TEXT("dr_acc parity"),FMath::Abs(double(A.dr_acc)-double(B.dr_acc))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dr_acc))));
    Same&=T.TestTrue(TEXT("dp_acc parity"),FMath::Abs(double(A.dp_acc)-double(B.dp_acc))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dp_acc))));
    Same&=T.TestTrue(TEXT("dy_acc parity"),FMath::Abs(double(A.dy_acc)-double(B.dy_acc))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dy_acc))));
    Same&=T.TestTrue(TEXT("dr_drg parity"),FMath::Abs(double(A.dr_drg)-double(B.dr_drg))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dr_drg))));
    Same&=T.TestTrue(TEXT("dp_drg parity"),FMath::Abs(double(A.dp_drg)-double(B.dp_drg))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dp_drg))));
    Same&=T.TestTrue(TEXT("dy_drg parity"),FMath::Abs(double(A.dy_drg)-double(B.dy_drg))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.dy_drg))));
    Same&=T.TestTrue(TEXT("flight_path_yaw parity"),FMath::Abs(double(A.flight_path_yaw)-double(B.flight_path_yaw))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.flight_path_yaw))));
    Same&=T.TestTrue(TEXT("flight_path_pitch parity"),FMath::Abs(double(A.flight_path_pitch)-double(B.flight_path_pitch))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.flight_path_pitch))));
    Same&=T.TestTrue(TEXT("g_accel parity"),FMath::Abs(double(A.g_accel)-double(B.g_accel))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.g_accel))));
    Same&=T.TestTrue(TEXT("Do parity"),FMath::Abs(double(A.Do)-double(B.Do))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.Do))));
    Same&=T.TestTrue(TEXT("CL parity"),FMath::Abs(double(A.CL)-double(B.CL))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.CL))));
    Same&=T.TestTrue(TEXT("CD parity"),FMath::Abs(double(A.CD)-double(B.CD))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.CD))));
    Same&=T.TestTrue(TEXT("alpha parity"),FMath::Abs(double(A.alpha)-double(B.alpha))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.alpha))));
    Same&=T.TestTrue(TEXT("stall parity"),FMath::Abs(double(A.stall)-double(B.stall))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.stall))));
    Same&=T.TestTrue(TEXT("shake parity"),FMath::Abs(double(A.shake)-double(B.shake))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.shake))));
    Same&=T.TestTrue(TEXT("primary_mass parity"),FMath::Abs(double(A.primary_mass)-double(B.primary_mass))<=1.e-6*FMath::Max(1.0,FMath::Abs(double(A.primary_mass))));
    return Same;
}
double FlightMedian(TArray<double> V){V.Sort();return V[V.Num()/2];}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightControllerParityTest,"Starshatter.Domains.Flight.ControlsSubstepsModelsAndRng",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlightControllerParityTest::RunTest(const FString&)
{
    auto* Switch=IConsoleManager::Get().FindConsoleVariable(TEXT("ssw.Simulation.ThreadedFlight"));if(!TestNotNull(TEXT("Flight switch registered"),Switch))return false;
    const int Previous=Switch->GetInt();const double OldSlice=Physical::GetSubFrameLength();ON_SCOPE_EXIT{Switch->Set(Previous,ECVF_SetByCode);Physical::SetSubFrameLength(OldSlice);};Physical::SetSubFrameLength(1.0/60.0);
    FFlightTestEnvironment Environment;
    for(auto Mode:{ENativeFlightModel::Inertial,ENativeFlightModel::Atmospheric,ENativeFlightModel::Arcade})for(bool Slices:{false,true})for(bool Straight:{false,true})
    {
        auto Initial=FlightFixture();Initial.straight=Straight;FFlightProbe Serial,Threaded;auto* A=new FFeedbackDirector(Serial,Slices);auto* B=new FFeedbackDirector(Threaded,Slices);Serial.Configure(Initial,A);Threaded.Configure(Initial,B);
        Switch->Set(0,ECVF_SetByCode);std::srand(11931);
        for(int I=0;I<80;++I)RunFlight(Serial,Mode,I%3==0?.045:I%3==1?1.0/120.0:1.0/60.0);
        const int SerialNext=std::rand();const auto Expected=FNativeFlightAdapter::Capture(Serial);
        Switch->Set(1,ECVF_SetByCode);std::srand(11931);
        for(int I=0;I<80;++I)RunFlight(Threaded,Mode,I%3==0?.045:I%3==1?1.0/120.0:1.0/60.0);
        TestEqual(TEXT("Random draw count/order retained"),std::rand(),SerialNext);
        if(!SameFlight(*this,Expected,FNativeFlightAdapter::Capture(Threaded)))return false;
        TestEqual(TEXT("Object life decremented once"),Serial.GetLife(),Threaded.GetLife());TestTrue(TEXT("Controller callbacks stay on game thread"),A->bGameThread && B->bGameThread);
        TestEqual(TEXT("Controller callback count retained"),A->Positions.Num(),B->Positions.Num());
        for(int I=0;I<A->Positions.Num();++I){if(!TestTrue(TEXT("Every controller sees previous committed substep"),A->Positions[I].Equals(B->Positions[I],1.e-6)))return false;TestEqual(TEXT("Identical callback timestep"),A->Durations[I],B->Durations[I]);}
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightLCAParityTest,"Starshatter.Domains.Flight.AirborneLCADamping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlightLCAParityTest::RunTest(const FString&)
{
    auto* Switch=IConsoleManager::Get().FindConsoleVariable(TEXT("ssw.Simulation.ThreadedFlight"));const int Previous=Switch->GetInt();ON_SCOPE_EXIT{Switch->Set(Previous,ECVF_SetByCode);};
    const auto Designs=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& P:Designs)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
    FShipDesign Design;Design.ShipName=TEXT("Flight LCA fixture");Design.ShipType=uint32(CLASSIFICATION::LCA);Design.Mass=20;Design.Integrity=100;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Flight LCA fixture")),Design);auto* Legacy=ShipDesignRegistry::FindLegacy(FName(TEXT("Flight LCA fixture")));
    FFlightTestEnvironment Environment;SimRegion Surface(Sim::GetSim(),"Flight surface fixture",SimRegion::AIR_SPACE);
    for(auto Mode:{ENativeFlightModel::Inertial,ENativeFlightModel::Atmospheric,ENativeFlightModel::Arcade})for(bool Lateral:{false,true})for(bool Powered:{false,true})
    {
        FFlightLCAProbe Serial(Legacy),Threaded(Legacy);Serial.SimObject::SetRegion(&Surface);Threaded.SimObject::SetRegion(&Surface);
        auto Initial=FlightFixture();Initial.shake=0;Initial.lat_thrust=Lateral;Initial.trans_x=Initial.trans_y=Initial.trans_z=0;Initial.thrust=Powered?100.f:0.f;
        Serial.Configure(Initial);Threaded.Configure(Initial);TestTrue(TEXT("Actual LCA policy projects to native state"),FNativeFlightAdapter::Capture(Serial).bAirborneLCADamping);
        Switch->Set(0,ECVF_SetByCode);for(int I=0;I<40;++I)RunFlight(Serial,Mode,.037);
        Switch->Set(1,ECVF_SetByCode);for(int I=0;I<40;++I)RunFlight(Threaded,Mode,.037);
        if(!SameFlight(*this,FNativeFlightAdapter::Capture(Serial),FNativeFlightAdapter::Capture(Threaded)))return false;
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightWorkerIsolationTest,"Starshatter.Domains.Flight.MailboxIsolationAndLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlightWorkerIsolationTest::RunTest(const FString&)
{
    FNativeSimulationWorker Worker;FNativeFlightRequest R;R.Sequence=47;R.State=FlightFixture();R.State.shake=0;R.Seconds=.1;R.OriginalVelocity=R.State.GetVelocity();R.RandomSamples.SetNumZeroed(8);const auto Source=R.State;
    if(!TestTrue(TEXT("Native flight accepted"),Worker.SubmitFlight(MoveTemp(R))))return false;
    TestNull(TEXT("Collision receiver cannot consume flight mailbox"),Worker.Finish().Get());
    FNativeCollisionInput C;C.Epoch=1;C.Sequence=1;TestFalse(TEXT("Collision cannot overwrite pending flight"),Worker.Submit(MoveTemp(C)));
    auto Out=Worker.FinishFlight();if(!TestNotNull(TEXT("Typed flight response"),Out.Get()))return false;
    TestTrue(TEXT("Flight arithmetic actually ran off game thread"),Out->bOffGameThread);TestTrue(TEXT("Valid result"),Out->bValid);TestEqual(TEXT("Request sequence retained"),Out->Sequence,47ULL);
    TestTrue(TEXT("Worker owns copies, caller position unchanged"),Source.cam.Pos()==FlightFixture().cam.Pos());TestTrue(TEXT("Integrated position changed"),Source.cam.Pos()!=Out->State.cam.Pos());TestNull(TEXT("Response consumed once"),Worker.FinishFlight().Get());
    {FNativeSimulationWorker Closing;FNativeFlightRequest Last;Last.Sequence=48;Last.State=Source;Last.Seconds=.01;Last.RandomSamples.SetNumZeroed(1);TestTrue(TEXT("Shutdown has pending native flight"),Closing.SubmitFlight(MoveTemp(Last)));}
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightTimingTest,"Starshatter.Domains.Flight.ProductionBridgeTiming",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlightTimingTest::RunTest(const FString&)
{
    auto* Switch=IConsoleManager::Get().FindConsoleVariable(TEXT("ssw.Simulation.ThreadedFlight"));const int Previous=Switch->GetInt();ON_SCOPE_EXIT{Switch->Set(Previous,ECVF_SetByCode);};
    FFlightTestEnvironment Environment;TArray<TSharedPtr<FJsonValue>> Rows;
    for(auto Mode:{ENativeFlightModel::Inertial,ENativeFlightModel::Atmospheric,ENativeFlightModel::Arcade})for(int Count:{1,64,256})for(int Steps:{1,8})
    {
        TArray<TUniquePtr<FFlightProbe>> Bodies;for(int I=0;I<Count;++I)Bodies.Add(MakeUnique<FFlightProbe>());auto State=FlightFixture();State.shake=0;
        TArray<double> Serial,Threaded;
        for(int Trial=-3;Trial<21;++Trial)for(int Path:{0,1})
        {
            for(auto& Body:Bodies)Body->Configure(State);Switch->Set(Path,ECVF_SetByCode);
            const double Start=FPlatformTime::Seconds();for(auto& Body:Bodies)RunFlight(*Body,Mode,Steps/60.0);
            const double Elapsed=(FPlatformTime::Seconds()-Start)*1000;if(Trial>=0)(Path?Threaded:Serial).Add(Elapsed);
        }
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("model"),Mode==ENativeFlightModel::Inertial?TEXT("Inertial"):Mode==ENativeFlightModel::Atmospheric?TEXT("Atmospheric"):TEXT("Arcade"));Row->SetNumberField(TEXT("bodies"),Count);Row->SetNumberField(TEXT("nominalSubsteps"),Steps);Row->SetNumberField(TEXT("samples"),21);Row->SetNumberField(TEXT("serialMedianMs"),FlightMedian(Serial));Row->SetNumberField(TEXT("workerBridgeMedianMs"),FlightMedian(Threaded));Rows.Add(MakeShared<FJsonValueObject>(Row));
        AddInfo(FString::Printf(TEXT("Flight production bridge model %d / %d bodies/%d substeps: serial %.3f ms, worker + barrier %.3f ms; CPU microbenchmark, not FPS."),int(Mode),Count,Steps,FlightMedian(Serial),FlightMedian(Threaded)));
    }
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("scope"),TEXT("Actual inertial/atmospheric/arcade adapters with immediate per-object barriers. No artificial overlap; not rendered FPS. Directors absent; subframe feedback parity tested separately."));Report->SetArrayField(TEXT("workloads"),Rows);FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
    TestTrue(TEXT("Flight timing evidence written"),FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("Analysis/FlightPhysicsTiming.json"))));return true;
}
#endif
