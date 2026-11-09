#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CentralSun.generated.h"

class UChildActorComponent;
class UStaticMeshComponent;

UCLASS()
class STARSHATTERWARS_API ACentralSun : public AActor
{
    GENERATED_BODY()

public:
    ACentralSun();

    UFUNCTION(BlueprintCallable, Category = "System|Sun")
    void ShowSun();

    UFUNCTION(BlueprintCallable, Category = "System|Sun")
    void HideSun();

    UFUNCTION(BlueprintCallable, Category = "System|Sun")
    void SetSunVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category = "System|Sun")
    AActor* GetSunActor() const;

protected:
    virtual void BeginPlay() override;

private:
    // Legacy central-star Blueprints bind material controls to this component.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Sun", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> SunMesh;

    UPROPERTY(VisibleAnywhere, Category = "System|Sun")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "System|Sun")
    TObjectPtr<UChildActorComponent> SunComponent;
};