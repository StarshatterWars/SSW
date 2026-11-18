#pragma once

#include "BeamWeaponActor.h"
#include "MiningBeamActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AMiningBeamActor : public ABeamWeaponActor
{
    GENERATED_BODY()

public:
    AMiningBeamActor();
    void BeginSimulationBeam(const FString& Name) override;
    void SetBeamPoints(FVector Start,FVector End) override;
    void SetBeamActive(bool bActive) override;
    void SetPresentationClock(bool bPaused,float Dilation) override;
    void SetMiningContact(bool bContact,FName Mineral,FName Crystal) override;
    bool HasVisibleOreContact() const { return bOreContact && bBeamActive; }
protected:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void UpdateMiningVisual();
    UPROPERTY(VisibleAnywhere,Category="Mining|Presentation") TObjectPtr<UNiagaraComponent> OreContactFX;
    UPROPERTY(VisibleAnywhere,Category="Mining|Presentation") TObjectPtr<class UPointLightComponent> OreContactGlow;
    UPROPERTY() TObjectPtr<UNiagaraSystem> OreContactSystem;
    FName ContactMineral,FocusCrystal;
    bool bOreContact=false,bPresentationPaused=false,bLastContact=false;
    float NominalWidth=5,EffectTime=0,LastSparkTime=-1,PlaybackRate=1;
    double LastWorldTime=-1;
};
