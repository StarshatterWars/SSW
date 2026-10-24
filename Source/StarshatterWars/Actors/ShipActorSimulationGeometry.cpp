#include "ShipActor.h"
#include "NativeShipGeometry.h"
#include "Camera.h"
int32 AShipActor::QueryNativeHullRay(const Camera& Frame,const FVector& Origin,const FVector& Direction,double Length,FVector& Hit,Weapon*& HitWeapon)
{ if(!NativeGeometry) NativeGeometry=MakeShared<FNativeShipGeometry>();return NativeGeometry->Ray(*this,Frame,Origin,Direction,Length,Hit,HitWeapon); }
int32 AShipActor::QueryNativeHullOverlap(const Camera& Frame,AShipActor& Other,const Camera& OtherFrame)
{
    if(!NativeGeometry) NativeGeometry=MakeShared<FNativeShipGeometry>();
    if(!Other.NativeGeometry) Other.NativeGeometry=MakeShared<FNativeShipGeometry>();
    return NativeGeometry->Overlap(*this,Frame,*Other.NativeGeometry,Other,OtherFrame);
}
bool AShipActor::GetSimulationWeaponMuzzle(const Camera& Frame,const Weapon* InWeapon,int32 Index,FVector& Position,Camera& MuzzleFrame)
{
    // Simulation may fire before the actor presentation tick; synchronize turret aim first.
    UpdateTurretActors();FTransform Muzzle;
    if(!GetRuntimeWeaponMuzzleTransform(InWeapon,Index,Muzzle)) return false;
    const FQuat Inverse=GetActorQuat().Inverse();
    auto ToSim=[&](const FVector& V){return Frame.vpn()*V.X+Frame.vrt()*V.Y+Frame.vup()*V.Z;};
    Position=Frame.Pos()+ToSim(Inverse.RotateVector(Muzzle.GetLocation()-GetActorLocation()));
    const FVector Forward=ToSim(Inverse.RotateVector(Muzzle.GetRotation().GetAxisX())).GetSafeNormal();
    const FVector Up=ToSim(Inverse.RotateVector(Muzzle.GetRotation().GetAxisZ())).GetSafeNormal();
    const FVector Right=FVector::CrossProduct(Up,Forward).GetSafeNormal();
    if(Forward.IsNearlyZero() || Right.IsNearlyZero()) return false;
    MuzzleFrame.MoveTo(Position);MuzzleFrame.SetOrientation(Right,FVector::CrossProduct(Forward,Right).GetSafeNormal(),Forward);
    return true;
}
