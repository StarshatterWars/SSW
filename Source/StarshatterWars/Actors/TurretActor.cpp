#include "TurretActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Weapon.h"
#include "Ship.h"
#include "Camera.h"
#include "Math/RotationMatrix.h"
#include "WeaponPresentationFrame.h"
#include "WeaponDesignRegistry.h"
namespace {
FQuat DisplayRotation(const Camera& Cam)
{
    const FVector F=Cam.vpn(), U=Cam.vup();
    return FRotationMatrix::MakeFromXZ(FVector(F.Z,F.X,F.Y),FVector(U.Z,U.X,U.Y)).ToQuat();
}
}
ATurretActor::ATurretActor()
{
    PrimaryActorTick.bCanEverTick=false;SetActorEnableCollision(false);
    MountRoot=CreateDefaultSubobject<USceneComponent>(TEXT("MountRoot")); SetRootComponent(MountRoot);
    BasePivot=CreateDefaultSubobject<USceneComponent>(TEXT("BasePivot")); BasePivot->SetupAttachment(MountRoot);
    GunPivot=CreateDefaultSubobject<USceneComponent>(TEXT("GunPivot")); GunPivot->SetupAttachment(MountRoot);
    BaseMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh")); BaseMesh->SetupAttachment(BasePivot);
    GunMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh")); GunMesh->SetupAttachment(GunPivot);
    // Imported turret models use +Y forward; aim frames use Unreal +X.
    BaseMesh->SetRelativeRotation(FRotator(0.f,-90.f,0.f));
    GunMesh->SetRelativeRotation(FRotator(0.f,-90.f,0.f));
    for (UStaticMeshComponent* Mesh : {BaseMesh.Get(),GunMesh.Get()}) {
        Mesh->bEditableWhenInherited=true;
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
    }
}
void ATurretActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyWeaponDefinition(WeaponType);
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
    const AActor* Parent = GetParentActor() ? GetParentActor() : GetAttachParentActor();
    const FQuat DisplayShip = Parent ? Parent->GetActorQuat() : WeaponPresentationFrame::Rotation(InShip.GetCam());
    SyncFromWeaponFrame(InWeapon, InShip.GetCam(), DisplayShip);
}

void ATurretActor::SyncFromWeaponFrame(const Weapon& InWeapon, const Camera& SimShip, const FQuat& DisplayShip)
{
    bHasRuntimeMuzzles=true;
    RuntimeMuzzles.SetNum(InWeapon.GetTurretBarrelCount());
    const double ModelScale=InWeapon.Design() && FMath::Abs(InWeapon.Design()->scale)>SMALL_NUMBER ? InWeapon.Design()->scale : 1.0;
    for(int32 I=0;I<RuntimeMuzzles.Num();++I) RuntimeMuzzles[I]=InWeapon.GetTurretBarrelOffset(I)/ModelScale;
    // The native mount can already carry the aim pose. Remove this actor's
    // actual world rotation, not only the simulated ship rotation, to avoid double aim.
    GunPivot->SetRelativeRotation(GetActorQuat().Inverse() *
        WeaponPresentationFrame::Display(InWeapon.GetTurretAimCamera(), SimShip, DisplayShip));
    Camera Base; Base.Clone(SimShip);
    const int32 RuntimeAxis = InWeapon.Design() ? InWeapon.Design()->turret_axis : BaseAxis;
    if(RuntimeAxis==1) Base.Pitch(InWeapon.GetElevation()+InWeapon.GetTurretElevationOffset());
    else Base.Yaw(InWeapon.GetAzimuth()+InWeapon.GetTurretAzimuthOffset());
    BasePivot->SetRelativeRotation(GetActorQuat().Inverse() *
        WeaponPresentationFrame::Display(Base, SimShip, DisplayShip));
}

bool ATurretActor::GetMuzzleTransform(int32 BarrelIndex,FTransform& OutTransform) const
{
    const TArray<FVector>& Points=bHasRuntimeMuzzles ? RuntimeMuzzles : LegacyMuzzles;
    if(!Points.IsValidIndex(BarrelIndex) || !GunPivot) { OutTransform=FTransform::Identity; return false; }
    const FVector P=Points[BarrelIndex];
    OutTransform=FTransform(GunPivot->GetComponentQuat(),GunPivot->GetComponentTransform().TransformPosition(FVector(P.Z,P.X,P.Y)));
    return true;
}

void ATurretActor::SetSeparateBasePresentation(bool bSeparate)
{
    if(!BaseMesh || bSeparate==bSeparateBaseActive)return;
    if(bSeparate){bBaseVisibilityBeforeSplit=BaseMesh->IsVisible();BaseMesh->SetVisibility(false);}
    else BaseMesh->SetVisibility(bBaseVisibilityBeforeSplit);
    bSeparateBaseActive=bSeparate;
}

void ATurretActor::ApplyWeaponDefinition(FName InWeaponType)
{
    WeaponType=InWeaponType;
    if(const auto* Row=WeaponDesignRegistry::Find(InWeaponType))
    {
        LegacyGunModel=Row->Turret;LegacyBaseModel=Row->TurretBase;
        LegacyModelScale=Row->Scale;BaseAxis=Row->TurretAxis;
        LegacyMuzzles.Reset();
        for(int32 I=0;I<FMath::Min(Row->NBarrels,Row->MuzzlePoints.Num());++I)LegacyMuzzles.Add(Row->MuzzlePoints[I]);
    }
    if(BasePivot)BasePivot->SetRelativeScale3D(FVector(LegacyModelScale));
    if(GunPivot)GunPivot->SetRelativeScale3D(FVector(LegacyModelScale));
}
