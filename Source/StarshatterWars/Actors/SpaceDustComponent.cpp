#include "SpaceDustComponent.h"
#include "StarshatterVideoSettings.h"
#include "StarshatterVideoSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
USpaceDustComponent::USpaceDustComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);SetGenerateOverlapEvents(false);SetCastShadow(false);
    bAffectDynamicIndirectLighting=false;SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if(Sphere.Succeeded())SetStaticMesh(Sphere.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/Space/M_SSW_SpaceDust.M_SSW_SpaceDust"));
    if(Material.Succeeded())SetMaterial(0,Material.Object);
}
void USpaceDustComponent::SetPlayerActive(bool Active)
{
    if(bPlayerActive==Active)return;bPlayerActive=Active;
    SetComponentTickEnabled(Active);SetVisibility(Active);
    if(!Active)bHaveCamera=false;
}
void USpaceDustComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime,TickType,TickFunction);
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SpaceDust);
    if(!bPlayerActive || !GetWorld())return;
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC || !PC->PlayerCameraManager)return;
    const FVector Camera=PC->PlayerCameraManager->GetCameraLocation();
    const UStarshatterVideoSubsystem* Video = UStarshatterVideoSubsystem::Get(this);
    const int32 Level = FMath::Clamp(Video ? Video->GetConfig().DustLevel :
        GetDefault<UStarshatterVideoSettings>()->GetConfig().DustLevel, 0, 3);
    const int32 Count=Level*64;
    if(Specks.Num()!=Count)
    {
        ClearInstances();Specks.SetNum(Count);DustTransforms.SetNum(Count);bHaveCamera=false;
        for(int32 I=0;I<Count;++I)AddInstance(FTransform::Identity);
    }
    if (Count == 0) return;
    const FVector Motion=bHaveCamera ? Camera-PreviousCamera : FVector::ZeroVector;
    const bool Reset=!bHaveCamera || Motion.SizeSquared()>FMath::Square(DustRadius);
    const FVector Direction=Motion.IsNearlyZero()?FVector::ForwardVector:Motion.GetSafeNormal();
    const FQuat Rotation=FRotationMatrix::MakeFromX(Direction).ToQuat();
    const float Length=FMath::Clamp(.15f+float(Motion.Size())*.15f,.15f,2.f);
    for(int32 I=0;I<Count;++I)
    {
        if(Reset)Specks[I]=Camera+FVector(Random.FRandRange(-DustRadius,DustRadius),Random.FRandRange(-DustRadius,DustRadius),Random.FRandRange(-DustRadius,DustRadius));
        FVector Offset=Specks[I]-Camera;
        for(int32 Axis=0;Axis<3;++Axis)
            Offset[Axis]=FMath::Fmod(FMath::Fmod(Offset[Axis]+DustRadius,2*DustRadius)+2*DustRadius,2*DustRadius)-DustRadius;
        Specks[I]=Camera+Offset;
        DustTransforms[I]=FTransform(Rotation,Specks[I],FVector(Length,.15f,.15f)/100.f);
    }
    if(Count)BatchUpdateInstancesTransforms(0,DustTransforms,true,true,true);
    PreviousCamera=Camera;bHaveCamera=true;
}
