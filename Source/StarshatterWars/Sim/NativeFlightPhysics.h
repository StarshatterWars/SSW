#pragma once
#include "CoreMinimal.h"
#include "Camera.h"
// Owned native values only. Camera here is SSW's value-only mathematical pose,
// not a UE camera/actor; it owns its matrix and has no live object references.
enum class ENativeFlightModel:uint8 { Inertial,Atmospheric,Arcade };
struct FNativeFlightState
{
    Camera cam;
    FVector velocity=FVector::ZeroVector,arcade_velocity=FVector::ZeroVector,accel=FVector::ZeroVector;
    FVector primary_loc=FVector::ZeroVector,vibration=FVector::ZeroVector;
    float thrust=0,trans_x=0,trans_y=0,trans_z=0,drag=0,mass=1;
    float roll=0,pitch=0,yaw=0,dr=0,dp=0,dy=0,dr_acc=0,dp_acc=0,dy_acc=0,dr_drg=0,dp_drg=0,dy_drg=0;
    float flight_path_yaw=0,flight_path_pitch=0,g_accel=0,Do=0,CL=0,CD=0,alpha=0,stall=0,shake=0;
    double primary_mass=0;
    bool lat_thrust=false,straight=false,bAirborneLCADamping=false;
    FVector GetVelocity()const{return velocity+arcade_velocity;}
    double GetDensity()const;
    void AngularFrame(double SecondsThisSlice,const FVector& RawRandom);
    void LinearFrame(double SecondsThisSlice);
    void CalcFlightPath();
    void InertialStep(double SecondsThisSlice);
    void AtmosphericStep(double SecondsThisSlice);
    void ArcadeStep(double SecondsThisSlice);
};
struct FNativeFlightRequest
{
    uint64 Sequence=0;
    FNativeFlightState State;
    ENativeFlightModel Model=ENativeFlightModel::Inertial;
    double Seconds=0,Subframe=1.0/60.0;
    FVector OriginalVelocity=FVector::ZeroVector;
    bool bFinalize=true;
    TArray<FVector> RandomSamples; // sampled in original order on game thread
};
struct FNativeFlightResult
{
    uint64 Sequence=0;
    FNativeFlightState State;
    bool bValid=false,bOffGameThread=false;
    double CalculationSeconds=0;
};
namespace SSWFlightPhysics
{
    FNativeFlightResult Calculate(FNativeFlightRequest&& Request);
}
