#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShieldComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

UCLASS(ClassGroup=(Ship), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UShieldComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShieldComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shield")
    TObjectPtr<UTexture2D> ShieldTexture;

    void ShowImpact(const FVector& LocalImpact, const FVector& LocalNormal, double WorldSize);

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ImpactMesh;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ImpactMaterial;
    float RemainingTime = 0;
};
