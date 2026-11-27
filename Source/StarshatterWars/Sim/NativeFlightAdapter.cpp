#include "NativeFlightAdapter.h"
#include "Physical.h"
#include "Ship.h"
#include "Sim.h"
#include "SimDirector.h"
#include "NativeSimulationWorker.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include <cmath>
#include <cstdlib>
namespace
{
TAutoConsoleVariable<int32> CVarThreadedFlight(TEXT("ssw.Simulation.ThreadedFlight"),0,
    TEXT("Run copied inertial/atmospheric/arcade physics on the simulation worker. Preserves director substep barriers; profile before enabling."),ECVF_Default);
}
FNativeFlightState FNativeFlightAdapter::Capture(const Physical& P)
{
    FNativeFlightState S;
    S.cam=P.cam;
    S.velocity=P.velocity;
    S.arcade_velocity=P.arcade_velocity;
    S.accel=P.accel;
    S.primary_loc=P.primary_loc;
    S.vibration=P.vibration;
    S.thrust=P.thrust;
    S.trans_x=P.trans_x;
    S.trans_y=P.trans_y;
    S.trans_z=P.trans_z;
    S.drag=P.drag;
    S.mass=P.mass;
    S.roll=P.roll;
    S.pitch=P.pitch;
    S.yaw=P.yaw;
    S.dr=P.dr;
    S.dp=P.dp;
    S.dy=P.dy;
    S.dr_acc=P.dr_acc;
    S.dp_acc=P.dp_acc;
    S.dy_acc=P.dy_acc;
    S.dr_drg=P.dr_drg;
    S.dp_drg=P.dp_drg;
    S.dy_drg=P.dy_drg;
    S.flight_path_yaw=P.flight_path_yaw;
    S.flight_path_pitch=P.flight_path_pitch;
    S.g_accel=P.g_accel;
    S.Do=P.Do;
    S.CL=P.CL;
    S.CD=P.CD;
    S.alpha=P.alpha;
    S.stall=P.stall;
    S.shake=P.shake;
    S.primary_mass=P.primary_mass;
    S.lat_thrust=P.lat_thrust;
    S.straight=P.straight;
    if(P.GetType()==ESimObject::SHIP)
    {const auto& Vessel=static_cast<const Ship&>(P);S.bAirborneLCADamping=Vessel.IsAirborne() && Vessel.GetClassification()==CLASSIFICATION::LCA;}
    return S;
}
void FNativeFlightAdapter::Commit(Physical& P,const FNativeFlightState& S)
{
    P.cam=S.cam;
    P.velocity=S.velocity;
    P.arcade_velocity=S.arcade_velocity;
    P.accel=S.accel;
    P.vibration=S.vibration;
    P.roll=S.roll;
    P.pitch=S.pitch;
    P.yaw=S.yaw;
    P.dr=S.dr;
    P.dp=S.dp;
    P.dy=S.dy;
    P.flight_path_yaw=S.flight_path_yaw;
    P.flight_path_pitch=S.flight_path_pitch;
    P.alpha=S.alpha;
    P.shake=S.shake;
}
TArray<FVector> FNativeFlightAdapter::Samples(const FNativeFlightState& S,double Seconds,double Subframe)
{
    TArray<FVector> Values;float Shake=S.shake;double Left=Seconds;
    while(Left>0)
    {
        const double Slice=Left>Subframe?Subframe:Left;FVector Raw=FVector::ZeroVector;
        if(!S.straight && Shake>.01f)
        {
            // Preserve the legacy expression and caller-thread CRT RNG order.
            Raw=FVector((float)(std::rand()-16384),(float)(std::rand()-16384),(float)(std::rand()-16384));
            Shake*=(float)std::exp(-1.5*Slice);
        }
        else Shake=0;
        Values.Add(Raw);Left-=Slice;
    }
    return Values;
}
bool FNativeFlightAdapter::TryFrame(Physical& P,ENativeFlightModel Model,double Seconds)
{
    if (!IsInGameThread() || !FCommandLine::IsInitialized()) return false;
    // Monolithic clients initialize this translation unit before command-line setup.
    // Read startup overrides only after startup; keep the console variable overridable.
    static const bool bStartupOverrideApplied = []()
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("SSWThreadedFlight")))
            CVarThreadedFlight->Set(1, ECVF_SetByConstructor);
        return true;
    }();
    (void)bStartupOverrideApplied;
    if (CVarThreadedFlight.GetValueOnGameThread()==0) return false;
    auto* Simulation=Sim::GetSim();const double Subframe=P.sub_frame;
    if(!Simulation || Seconds<=0 || !FMath::IsFinite(Seconds) || Subframe<=0 || !FMath::IsFinite(Subframe) || Seconds/Subframe>4096 || P.mass<=0)return false;
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_FlightPhysicsAdapter);
    const FVector Original=P.GetVelocity();
    if(Model!=ENativeFlightModel::Arcade)P.arcade_velocity=FVector::ZeroVector;
    if(P.dir && !P.dir->GetSubframe())P.dir->ExecFrame(Seconds);
    if(P.life>0)P.life-=Seconds;
    auto& Worker=Simulation->GetSimulationWorker();double Left=Seconds;
    // A subframe director can inspect/mutate the newly integrated state each slice.
    // Whole-frame batching is used only where the old loop has no such callbacks.
    const bool PerSlice=P.dir && P.dir->GetSubframe();
    while(Left>0)
    {
        const double Duration=PerSlice?(Left>Subframe?Subframe:Left):Left;
        if(PerSlice && P.dir && P.dir->GetSubframe())P.dir->ExecFrame(Duration);
        FNativeFlightRequest Request;Request.Sequence=Simulation->NextSimulationRequest();Request.State=Capture(P);
        Request.Seconds=Duration;Request.Subframe=Subframe;Request.Model=Model;Request.OriginalVelocity=Original;Request.bFinalize=Duration>=Left;
        Request.RandomSamples=Samples(Request.State,Duration,Subframe);
        const uint64 Sequence=Request.Sequence;
        TUniquePtr<FNativeFlightResult> Result;
        if(Worker.SubmitFlight(MoveTemp(Request)))Result=Worker.FinishFlight();
        else Result=MakeUnique<FNativeFlightResult>(SSWFlightPhysics::Calculate(MoveTemp(Request)));
        // Request and live object are owned by this blocked game-thread call; no
        // event loop, UObject callback or frame delay occurs across the barrier.
        checkf(Result && Result->bValid && Result->Sequence==Sequence,TEXT("Invalid native flight result"));
        if(!Result || !Result->bValid || Result->Sequence!=Sequence)return true;
        Commit(P,Result->State);Left-=Duration;
    }
    return true;
}
