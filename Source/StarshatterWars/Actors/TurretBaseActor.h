#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TurretBaseActor.generated.h"
class UStaticMeshComponent;
/** Visual base only. Weapon simulation remains responsible for aim and firing. */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API ATurretBaseActor : public AActor
{
    GENERATED_BODY()
public:
    ATurretBaseActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") TObjectPtr<USceneComponent> BaseRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") TObjectPtr<USceneComponent> ModelRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") TObjectPtr<UStaticMeshComponent> BaseMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base|Definition") FString LegacyBaseModel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret Base|Definition") float LegacyModelScale=1.f;
    void ApplyDefinition(const FString& Model,float Scale);
};
