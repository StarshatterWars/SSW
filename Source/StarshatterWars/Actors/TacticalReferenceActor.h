#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalReferenceActor.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

// In-game tactical ship viewer: shows a ship mesh, mouse-drag rotates, wheel zooms.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATacticalReferenceActor : public AActor
{
    GENERATED_BODY()
public:
    ATacticalReferenceActor();

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ShowShip(const FString& DesignName);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void SetDisplayMesh(UStaticMesh* Mesh);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void AddOrbitDelta(float YawDelta, float PitchDelta);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void AddZoomDelta(float Delta);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ResetView();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShipMesh;

    UPROPERTY(VisibleAnywhere)
    USpringArmComponent* Arm;

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* Camera;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float Yaw = 180.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float Pitch = 15.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float Distance = 800.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float MinDistance = 200.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float MaxDistance = 4000.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float OrbitSpeed = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    float ZoomSpeed = 40.f;

    UPROPERTY(EditAnywhere, Category = "Tactical")
    bool bCaptureMouse = true;

    void ApplyView();
};
