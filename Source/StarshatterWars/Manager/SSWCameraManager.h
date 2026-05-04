#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSWCameraManager.generated.h"

class UCameraComponent;

UENUM()
enum class ESSWCameraMode : uint8
{
    None,
    Static,
    BodyOrbit,
    ActorFollow,
    GroupFollow
};

UCLASS()
class STARSHATTERWARS_API ASSWCameraManager : public AActor
{
    GENERATED_BODY()

public:
    ASSWCameraManager();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // ----------------------------------------------------
    // COMPONENTS
    // ----------------------------------------------------
public:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* Camera;

    // ----------------------------------------------------
    // CAMERA CONTROL
    // ----------------------------------------------------
public:

    void ActivateCamera(float BlendTime = 0.0f);

    void SetBodyOrbitView(AActor* Target, const FVector& OrbitPoint);
    void SetOrbitRates(const FVector& Rates);

    void SetActorFollowView(
        AActor* Target,
        const FVector& Offset,
        const FRotator& Rotator);

    void SetStaticView(const FVector& Location, const FRotator& Rotation);

    void ClearCamera();

    FVector ComputeTightFollowOffset(AActor* Target) const;

public:
    void SetGroupFollowView(
        const TArray<AActor*>& InTargets,
        const FVector& Offset,
        const FVector& InVelocityDir,
        float InLookAhead);

protected:

    void UpdateOrbit(float DeltaTime);
    void UpdateActorFollow(float DeltaTime);

    void LookAt(const FVector& Target);

protected:

    UPROPERTY()
    ESSWCameraMode Mode = ESSWCameraMode::None;

    UPROPERTY()
    TObjectPtr<AActor> TargetActor;

    // ----------------------------------------------------
    // ORBIT (LEGACY CORE)
    // ----------------------------------------------------
    double Azimuth = 0.0;
    double Elevation = 0.0;
    double Range = 10000.0;

    double AzRate = 0.0;
    double ElRate = 0.0;
    double RangeRate = 0.0;

    // ----------------------------------------------------
    // ACTOR FOLLOW
    // ----------------------------------------------------
    FVector FollowOffset = FVector::ZeroVector;
    FRotator FollowRotator = FRotator::ZeroRotator;

    // ----------------------------------------------------
    // SETTINGS
    // ----------------------------------------------------
    UPROPERTY(EditAnywhere)
    float MinRange = 100.0f;

    UPROPERTY(EditAnywhere)
    float MaxRange = 1e9f;

    UPROPERTY(EditAnywhere)
    bool bLegacyAxisMapping = true;

private:
    void UpdateGroupFollow(float DeltaTime);

private:
    UPROPERTY()
    TArray<TObjectPtr<AActor>> GroupTargets;

    FVector GroupFollowOffset = FVector(-3000.0f, 1200.0f, 800.0f);
    FVector GroupVelocityDir = FVector::ForwardVector;
    float GroupLookAhead = 0.0f;
    float GroupLagSpeed = 5.0f;
};