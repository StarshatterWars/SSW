#pragma once
#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "TurretBaseComponent.generated.h"
UCLASS(ClassGroup=(Starshatter), meta=(BlueprintSpawnableComponent), AutoExpandCategories=("Turret Base"))
class STARSHATTERWARS_API UTurretBaseComponent : public UChildActorComponent
{
    GENERATED_BODY()
public:
    UTurretBaseComponent();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") FName WeaponType;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") FString LegacyBaseModel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") float ModelScale=1.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret Base") int32 SourceWeaponIndex=INDEX_NONE;
    void Configure(FName Type);
    void ApplyVisualDefinition();
protected:
    virtual void OnRegister() override;
private:
    UPROPERTY() TSubclassOf<AActor> AutoBaseClass;
};
