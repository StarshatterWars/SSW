#include "MiningBeamActor.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "MiningCrystalCatalog.h"
#include "UObject/ConstructorHelpers.h"

AMiningBeamActor::AMiningBeamActor()
{
    WeaponDefinitionName = TEXT("Mining Laser");
    BeamColor = FLinearColor(0.35f, 3.2f, 2.4f, 1.0f);
    BeamWidth = 5.0f;
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Beam(
        TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_laser01.NS_ky_laser01"));
    BeamSystem = Beam.Object;
    OreContactFX=CreateDefaultSubobject<UNiagaraComponent>(TEXT("OreContactFX"));
    OreContactFX->SetupAttachment(Root);OreContactFX->SetAutoActivate(false);OreContactFX->SetAutoDestroy(false);
    OreContactFX->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OreContactFX->SetRelativeScale3D(FVector(.12f));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Contact(
        TEXT("/Game/Beam_and_laserFX01/Particles/Niagara/NS_ky_hit01.NS_ky_hit01"));
    OreContactSystem=Contact.Object;
    OreContactGlow=CreateDefaultSubobject<UPointLightComponent>(TEXT("OreContactGlow"));
    OreContactGlow->SetupAttachment(Root);OreContactGlow->SetCastShadows(false);
    OreContactGlow->SetAttenuationRadius(140);OreContactGlow->SetSourceRadius(8);OreContactGlow->SetIntensity(0);
    OreContactGlow->SetVisibility(false);
}

void AMiningBeamActor::BeginPlay()
{
    Super::BeginPlay();
    OreContactFX->SetAsset(OreContactSystem);
    OreContactFX->SetSystemFixedBounds(FBox(FVector(-150),FVector(150)));
    UpdateMiningVisual();
}
void AMiningBeamActor::BeginSimulationBeam(const FString& Name)
{
    Super::BeginSimulationBeam(Name);NominalWidth=BeamWidth;
    LastWorldTime=GetWorld()?GetWorld()->GetTimeSeconds():-1;
}
void AMiningBeamActor::SetPresentationClock(bool bPaused,float Dilation)
{
    Super::SetPresentationClock(bPaused,Dilation);
    bPresentationPaused=bPaused;
    PlaybackRate=FMath::IsFinite(Dilation)?FMath::Clamp(Dilation,0.f,20.f):1;
    OreContactFX->SetPaused(bPaused);OreContactFX->SetCustomTimeDilation(PlaybackRate);
    if(bPaused && GetWorld())LastWorldTime=GetWorld()->GetTimeSeconds();
}
void AMiningBeamActor::SetBeamPoints(FVector Start,FVector End)
{
    if(GetWorld())
    {
        const double Now=GetWorld()->GetTimeSeconds();
        if(!bPresentationPaused && LastWorldTime>=0)EffectTime+=FMath::Max(0.0,Now-LastWorldTime)*PlaybackRate;
        LastWorldTime=Now;
    }
    Super::SetBeamPoints(Start,End);
    UpdateMiningVisual();
}
void AMiningBeamActor::SetMiningContact(bool Contact,FName Mineral,FName Crystal)
{
    bOreContact=Contact && !Mineral.IsNone();ContactMineral=Mineral;FocusCrystal=Crystal;
    UpdateMiningVisual();
}
void AMiningBeamActor::SetBeamActive(bool Active)
{
    if(!Active)
    {
        bOreContact=false;OreContactFX->DeactivateImmediate();OreContactGlow->SetVisibility(false);OreContactGlow->SetIntensity(0);
    }
    Super::SetBeamActive(Active);
}
void AMiningBeamActor::UpdateMiningVisual()
{
    const auto& Catalog=FMiningCrystalCatalog::Get();
    const auto Mineral=FMiningCrystalCatalog::MineralFromName(ContactMineral);
    const auto* Focus=Catalog.Appearance(FocusCrystal,Mineral);
    const float Pulse=FMiningCrystalCatalog::Pulse(EffectTime,Focus?Focus->PulseHz:1.2f);
    BeamColor=(Focus?Focus->Color:FLinearColor(.35f,3.2f,2.4f,1))*Pulse;
    BeamWidth=NominalWidth*(Focus?Focus->WidthScale:1)*(.96f+.04f*Pulse);
    ApplyVisual();
    const bool Contact=HasVisibleOreContact() && !BeamStart.Equals(BeamEnd,.001) && Mineral!=ESSWMineral::None;
    OreContactGlow->SetVisibility(Contact);
    if(Contact)
    {
        const FVector Outward=(BeamStart-BeamEnd).GetSafeNormal();
        const FVector Surface=BeamEnd+Outward*2;
        OreContactFX->SetWorldLocationAndRotation(Surface,Outward.Rotation());
        OreContactGlow->SetWorldLocation(Surface+Outward*6);
        OreContactGlow->SetLightColor(BeamColor.GetClamped(0,1));OreContactGlow->SetIntensity(1800*Pulse);
        OreContactFX->SetVariableLinearColor(TEXT("User.Color"),BeamColor);
        OreContactFX->SetVariableVec3(TEXT("User.color"),FVector(BeamColor.R,BeamColor.G,BeamColor.B));
        if(!bPresentationPaused && (!bLastContact || EffectTime-LastSparkTime>=.5f))
        { OreContactFX->Activate(true);LastSparkTime=EffectTime; }
    }
    else
    { OreContactFX->DeactivateImmediate();OreContactGlow->SetIntensity(0); }
    bLastContact=Contact;
}
void AMiningBeamActor::EndPlay(const EEndPlayReason::Type Reason)
{
    OreContactFX->DeactivateImmediate();OreContactGlow->SetIntensity(0);OreContactGlow->SetVisibility(false);
    Super::EndPlay(Reason);
}
