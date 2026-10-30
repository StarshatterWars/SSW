#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShipBriefingPreview.generated.h"

class AActor;
class UWorld;
class UStaticMesh;
class UMaterialInterface;
class UViewport;
class UTextBlock;
struct FShipDesign;

// Asset-only snapshot: no legacy Ship/Mission pointers and no gameplay actors.
USTRUCT()
struct FBriefingShipPreviewPart
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY() FTransform Transform = FTransform::Identity;
};

namespace BriefingShipPreview
{
    STARSHATTERWARS_API bool ResolveParts(const FShipDesign& Design, TArray<FBriefingShipPreviewPart>& OutParts);
    STARSHATTERWARS_API FBox CalculateBounds(const TArray<FBriefingShipPreviewPart>& Parts);
    STARSHATTERWARS_API float FitDistance(float Radius, const FVector2D& ViewSize);
    // Rejects main Game/PIE worlds. Only isolated preview worlds are accepted.
    STARSHATTERWARS_API AActor* CreateVisual(UWorld* PreviewWorld, const TArray<FBriefingShipPreviewPart>& Parts);
}

UCLASS()
class STARSHATTERWARS_API UShipBriefingPreview : public UUserWidget
{
    GENERATED_BODY()
public:
    bool SetShipDesign(const FShipDesign* Design);
    void SetPreviewActive(bool bActive);
    void ResetView();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
private:
    void UpdateVisual();
    void UpdateCamera();
    void ClearVisual();
    UPROPERTY(Transient) TObjectPtr<UViewport> PreviewViewport;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UnavailableText;
    UPROPERTY(Transient) TArray<FBriefingShipPreviewPart> Parts;
    TWeakObjectPtr<AActor> PreviewVisual;
    FString DesignKey;
    FVector2D LastViewSize = FVector2D::ZeroVector;
    float OrbitYaw = -130.f;
    float OrbitPitch = 20.f;
    float Zoom = 1.f;
    bool bPreviewActive = true;
    bool bDragging = false;
};
