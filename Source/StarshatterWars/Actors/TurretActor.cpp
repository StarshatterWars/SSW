#include "TurretActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Weapon.h"
#include "Ship.h"
#include "Camera.h"
#include "Math/RotationMatrix.h"
namespace {
FQuat DisplayRotation(const Camera& Cam)
{
    const FVector F=Cam.vpn(), U=Cam.vup();
    return FRotationMatrix::MakeFromXZ(FVector(F.Z,F.X,F.Y),FVector(U.Z,U.X,U.Y)).ToQuat();
}
}
ATurretActor::ATurretActor()
{
    PrimaryActorTick.bCanEverTick=false;
    MountRoot=CreateDefaultSubobject<USceneComponent>(TEXT("MountRoot")); SetRootComponent(MountRoot);
    BasePivot=CreateDefaultSubobject<USceneComponent>(TEXT("BasePivot")); BasePivot->SetupAttachment(MountRoot);
    GunPivot=CreateDefaultSubobject<USceneComponent>(TEXT("GunPivot")); GunPivot->SetupAttachment(MountRoot);
    BaseMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh")); BaseMesh->SetupAttachment(BasePivot);
    GunMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh")); GunMesh->SetupAttachment(GunPivot);
    for (UStaticMeshComponent* Mesh : {BaseMesh.Get(),GunMesh.Get()}) {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
    }
}
void ATurretActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BasePivot->SetRelativeScale3D(FVector(LegacyModelScale));
    GunPivot->SetRelativeScale3D(FVector(LegacyModelScale));
}
void ATurretActor::SetPreviewAim(double Azimuth,double Elevation)
{
    Camera Gun; Gun.Yaw(Azimuth); Gun.Pitch(Elevation);
    Camera Base;
    if(BaseAxis==1) Base.Pitch(Elevation); else Base.Yaw(Azimuth);
    GunPivot->SetRelativeRotation(DisplayRotation(Gun));
    BasePivot->SetRelativeRotation(DisplayRotation(Base));
}
void ATurretActor::SyncFromWeapon(const Weapon& InWeapon,const Ship& InShip)
{
    bHasRuntimeMuzzles=true;
    RuntimeMuzzles.SetNum(InWeapon.GetTurretBarrelCount());
    const double ModelScale=InWeapon.Design() && FMath::Abs(InWeapon.Design()->scale)>SMALL_NUMBER ? InWeapon.Design()->scale : 1.0;
    for(int32 I=0;I<RuntimeMuzzles.Num();++I) RuntimeMuzzles[I]=InWeapon.GetTurretBarrelOffset(I)/ModelScale;
    // Remove simulation world pose; the parent supplies the displayed ship pose.
    const FQuat ShipRotation=DisplayRotation(InShip.GetCam());
    GunPivot->SetRelativeRotation(ShipRotation.Inverse()*DisplayRotation(InWeapon.GetTurretAimCamera()));
    Camera Base; Base.Clone(InShip.GetCam());
    if(BaseAxis==1) Base.Pitch(InWeapon.GetElevation()+InWeapon.GetTurretElevationOffset());
    else Base.Yaw(InWeapon.GetAzimuth()+InWeapon.GetTurretAzimuthOffset());
    BasePivot->SetRelativeRotation(ShipRotation.Inverse()*DisplayRotation(Base));
}

bool ATurretActor::GetMuzzleTransform(int32 BarrelIndex,FTransform& OutTransform) const
{
    const TArray<FVector>& Points=bHasRuntimeMuzzles ? RuntimeMuzzles : LegacyMuzzles;
    if(!Points.IsValidIndex(BarrelIndex) || !GunPivot) { OutTransform=FTransform::Identity; return false; }
    const FVector P=Points[BarrelIndex];
    OutTransform=FTransform(GunPivot->GetComponentQuat(),GunPivot->GetComponentTransform().TransformPosition(FVector(P.Z,P.X,P.Y)));
    return true;
}
