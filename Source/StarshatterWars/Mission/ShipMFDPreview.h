#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
class FPreviewScene;
class AActor;
class ASceneCapture2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FShipDesign;
// Asset-only, isolated rendering. Never retains a simulation Ship or game actor.
class FShipMFDPreview
{
public:
    explicit FShipMFDPreview(bool bInPlayerPanel = false);
    ~FShipMFDPreview();
    bool SetDesign(const FShipDesign* Design);
    bool SetWaypoint();
    bool SetStation(const AActor& Station);
    bool SetAsteroid(const UStaticMeshComponent* Source);
    void SetView(const FQuat& TargetRotation,const FVector& ToObserver,const FVector& ObserverUp);
    bool IsReady() const { return Visual.IsValid() && Target.IsValid(); }
    const FSlateBrush& GetBrush() const { return Brush; }
private:
    friend class FPlayerMFDRenderTest;
    bool bPlayerPanel = false;
    bool bHasLiveView = false;
    double NextStaticCaptureAt = 0.0;
    double NextLiveCaptureAt = 0.0;
    FQuat LastTargetRotation = FQuat::Identity;
    FVector LastObserverDirection = FVector::ZeroVector;
    FVector LastObserverUp = FVector::ZeroVector;
    int32 StaticWarmupCaptures = 0;
    void Clear();
    void RefreshStaticCapture();
    bool BeginCapture(const FString& NextKey, const FBox& Bounds);
    FString Key;
    TUniquePtr<FPreviewScene> Scene;
    TWeakObjectPtr<AActor> Visual;
    TWeakObjectPtr<ASceneCapture2D> Capture;
    TStrongObjectPtr<UTextureRenderTarget2D> Target;
    TStrongObjectPtr<UMaterialInstanceDynamic> Material;
    FSlateBrush Brush;
};
