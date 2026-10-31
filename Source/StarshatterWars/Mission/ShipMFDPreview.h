#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
class FPreviewScene;
class AActor;
class ASceneCapture2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
struct FShipDesign;
// Asset-only, isolated rendering. Never retains a simulation Ship or game actor.
class FShipMFDPreview
{
public:
    explicit FShipMFDPreview(bool bInPlayerPanel = false);
    ~FShipMFDPreview();
    bool SetDesign(const FShipDesign* Design);
    void SetView(const FQuat& TargetRotation,const FVector& ToObserver,const FVector& ObserverUp);
    bool IsReady() const { return Visual.IsValid() && Target.IsValid(); }
    const FSlateBrush& GetBrush() const { return Brush; }
private:
    friend class FPlayerMFDRenderTest;
    bool bPlayerPanel = false;
    bool bHasLiveView = false;
    void Clear();
    FString Key;
    TUniquePtr<FPreviewScene> Scene;
    TWeakObjectPtr<AActor> Visual;
    TWeakObjectPtr<ASceneCapture2D> Capture;
    TStrongObjectPtr<UTextureRenderTarget2D> Target;
    TStrongObjectPtr<UMaterialInstanceDynamic> Material;
    FSlateBrush Brush;
};
