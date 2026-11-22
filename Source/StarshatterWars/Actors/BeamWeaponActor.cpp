#include "BeamWeaponActor.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "WeaponDesignRegistry.h"
#include "Misc/PackageName.h"
#include "XRayBeamActor.h"
#include "GammaBeamActor.h"
#include "GraserBeamActor.h"
#include "ZolonBeamActor.h"
#include "MiningBeamActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

ABeamWeaponActor::ABeamWeaponActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    BeamFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamFX"));
    BeamFX->SetupAttachment(Root);
    BeamFX->SetAutoActivate(false);
    BeamFX->SetAutoDestroy(false);
    BeamFX->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

ABeamWeaponActor* ABeamWeaponActor::SpawnBeam(UObject* WorldContextObject, TSubclassOf<ABeamWeaponActor> BeamClass,
    FVector Start, FVector End, AActor* OwnerActor)
{
    UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World && GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (UWorld* Candidate = Context.World())
            {
                if (Candidate->WorldType == EWorldType::Game || Candidate->WorldType == EWorldType::PIE)
                {
                    World = Candidate;
                    break;
                }
            }
        }
    }
    if (!World) return nullptr;

    TSubclassOf<ABeamWeaponActor> ClassToSpawn = BeamClass ? BeamClass : TSubclassOf<ABeamWeaponActor>(ABeamWeaponActor::StaticClass());
    FActorSpawnParameters Params;
    Params.Owner = OwnerActor;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABeamWeaponActor* Beam = World->SpawnActor<ABeamWeaponActor>(ClassToSpawn, Start, FRotator::ZeroRotator, Params);
    if (Beam)
    {
        Beam->SetBeamPoints(Start, End);
        Beam->SetBeamActive(true);
    }
    return Beam;
}

void ABeamWeaponActor::BeginPlay()
{
    Super::BeginPlay();
    if (BeamSystem && BeamFX)
    {
        BeamFX->SetAsset(BeamSystem);
    }
    ApplyVisual();
}

void ABeamWeaponActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bSimulationControlled && LifeTime > 0.0f)
    {
        Age += DeltaSeconds;
        if (Age >= LifeTime)
        {
            Destroy();
        }
    }
}

void ABeamWeaponActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BeamFX)
    {
        BeamFX->DeactivateImmediate();
    }
    Super::EndPlay(EndPlayReason);
}

void ABeamWeaponActor::SetBeamPoints(FVector Start, FVector End)
{
    const bool bPointsChanged = BeamStart != Start || BeamEnd != End;
    BeamStart = Start;
    BeamEnd = End;
    if (GetActorLocation() != Start) SetActorLocation(Start);
    ApplyVisual();
    if (bPointsChanged) OnBeamUpdated(Start, End, FVector::Distance(Start, End));
}

void ABeamWeaponActor::SetBeamActive(bool bActive)
{
    bBeamActive = bActive;
    if (BeamFX)
    {
        if (bActive && !BeamFX->IsActive()) BeamFX->Activate(true);
        else if(!bActive) BeamFX->Deactivate();
    }
    if (!bActive && bAutoDestroyOnDeactivate)
    {
        Destroy();
    }
}

void ABeamWeaponActor::SetBeamColor(FLinearColor Color)
{
    BeamColor = Color;
    ApplyVisual();
}

void ABeamWeaponActor::SetBeamWidth(float Width)
{
    BeamWidth = Width;
    ApplyVisual();
}

void ABeamWeaponActor::ApplyVisual()
{
    if (!BeamFX) return;

    const FVector LocalEnd=BeamFX->GetComponentTransform().InverseTransformPosition(BeamEnd);
    const bool bSystemChanged=!bVisualParametersInitialized || PresentedSystem.Get()!=BeamSystem.Get();
    const bool bEndpointChanged=bSystemChanged || PresentedLocalEnd!=LocalEnd;
    const bool bColorChanged=bSystemChanged || PresentedColor!=BeamColor;
    const bool bWidthChanged=bSystemChanged || PresentedWidth!=BeamWidth || PresentedReferenceWidth!=PackReferenceWidth;
    if(bSystemChanged)
    {
        if(BeamFX->GetAsset()!=BeamSystem.Get()) BeamFX->SetAsset(BeamSystem);
        PackVectorParameters=0;
        if(BeamSystem)
        {
            const TCHAR* Names[]={TEXT("User.beamEnd"),TEXT("User.color"),TEXT("User.colorDeco"),TEXT("User.colorDeco2"),TEXT("User.coreColor")};
            for(uint8 Index=0;Index<UE_ARRAY_COUNT(Names);++Index)
                if(BeamSystem->GetExposedParameters().GetParameterData(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),FName(Names[Index]))))
                    PackVectorParameters|=uint8(1<<Index);
            const TCHAR* Decorative[]={TEXT("User.colorDeco"),TEXT("User.colorDeco2"),TEXT("User.coreColor")};
            for(uint8 Index=0;Index<UE_ARRAY_COUNT(Decorative);++Index)
                if(PackVectorParameters & (1<<(Index+2))) BeamFX->SetVariableVec3(FName(Decorative[Index]),FVector(8,8,8));
        }
        BeamFX->SetVariableVec3(TEXT("User.BeamStart"),FVector::ZeroVector);
    }
    if(bEndpointChanged)
    {
        if(PackVectorParameters & 1) BeamFX->SetVariableVec3(TEXT("User.beamEnd"),LocalEnd);
        BeamFX->SetVariableVec3(TEXT("User.BeamEnd"),LocalEnd);
        BeamFX->SetVariableFloat(TEXT("User.BeamLength"),FVector::Distance(BeamStart,BeamEnd));
    }
    if(bColorChanged)
    {
        if(PackVectorParameters & 2) BeamFX->SetVariableVec3(TEXT("User.color"),FVector(BeamColor.R,BeamColor.G,BeamColor.B));
        BeamFX->SetVariableLinearColor(TEXT("User.BeamColor"),BeamColor);
    }
    if(bWidthChanged)
    {
        if(BeamSystem) BeamFX->SetVariableFloat(TEXT("User.size"),BeamWidth/FMath::Max(.01f,PackReferenceWidth));
        BeamFX->SetVariableFloat(TEXT("User.BeamWidth"),BeamWidth);
    }
    if(bEndpointChanged || bWidthChanged)
    {
        FBox Bounds(ForceInit);Bounds+=FVector::ZeroVector;Bounds+=LocalEnd;
        BeamFX->SetSystemFixedBounds(Bounds.ExpandBy(FMath::Max(500.0f,BeamWidth*20.0f)));
    }
#if WITH_DEV_AUTOMATION_TESTS
    if(bEndpointChanged || bWidthChanged || bColorChanged) ++VisualUpdateCount;
#endif
    PresentedSystem=BeamSystem;PresentedLocalEnd=LocalEnd;PresentedColor=BeamColor;
    PresentedWidth=BeamWidth;PresentedReferenceWidth=PackReferenceWidth;bVisualParametersInitialized=true;
    if(bBeamActive && BeamSystem && !BeamFX->IsActive())BeamFX->Activate(true);
}

void ABeamWeaponActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);BeamFX->SetAsset(BeamSystem);
}
void ABeamWeaponActor::BeginSimulationBeam(const FString& Name)
{
 bSimulationControlled=true;SetActorTickEnabled(false);WeaponDefinitionName=Name;
 if(const auto* Row=WeaponDesignRegistry::Find(Name))BeamWidth=FMath::Max(.1f,Row->Width);
 ApplyVisual();
}
void ABeamWeaponActor::SetPresentationClock(bool bPaused,float Dilation)
{
 BeamFX->SetPaused(bPaused);BeamFX->SetCustomTimeDilation(Dilation);
}
TSubclassOf<ABeamWeaponActor> ABeamWeaponActor::ClassForWeapon(const FString& Name)
{
 if(Name==TEXT("Mining Laser"))return AMiningBeamActor::StaticClass();
 if(Name==TEXT("XRay Laser"))return AXRayBeamActor::StaticClass();
 if(Name==TEXT("Gamma Laser"))return AGammaBeamActor::StaticClass();
 if(Name==TEXT("Graser Turret"))return AGraserBeamActor::StaticClass();
 if(Name==TEXT("Zolon Laser"))return AZolonBeamActor::StaticClass();
 return StaticClass();
}
FString ABeamWeaponActor::BlueprintPathForWeapon(const FString& Name)
{
 if(Name==TEXT("XRay Laser"))return TEXT("/Game/Models/Weapons/Beams/XRay/BP_XRayBeam.BP_XRayBeam_C");
 if(Name==TEXT("Gamma Laser"))return TEXT("/Game/Models/Weapons/Beams/Gamma/BP_GammaBeam.BP_GammaBeam_C");
 if(Name==TEXT("Graser Turret"))return TEXT("/Game/Models/Weapons/Beams/Graser/BP_GraserBeam.BP_GraserBeam_C");
 if(Name==TEXT("Zolon Laser"))return TEXT("/Game/Models/Weapons/Beams/Zolon/BP_ZolonBeam.BP_ZolonBeam_C");
 return FString();
}
TSubclassOf<ABeamWeaponActor> ABeamWeaponActor::BlueprintClassForWeapon(const FString& Name)
{
 const FString P=BlueprintPathForWeapon(Name);
 if(!P.IsEmpty() && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(P)))
  if(UClass* C=LoadClass<ABeamWeaponActor>(nullptr,*P))return C;
 return ClassForWeapon(Name);
}
