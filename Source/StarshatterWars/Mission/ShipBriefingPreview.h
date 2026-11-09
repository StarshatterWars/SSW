#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShipBriefingPreview.generated.h"

class AActor;
class Mission;
class UWorld;
class UStaticMesh;
class UMaterialInterface;
class UViewport;
class UTextBlock;
class UVerticalBox;
class UBorder;
class UHorizontalBox;
struct FShipDesign;
struct FWeaponDesign;

// Asset-only snapshot: no legacy Ship/Mission pointers and no gameplay actors.
USTRUCT()
struct FBriefingShipPreviewPart
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY() TObjectPtr<UMaterialInterface> OverlayMaterial;
    UPROPERTY() FTransform Transform = FTransform::Identity;
};

namespace BriefingShipPreview
{
    STARSHATTERWARS_API bool ResolveParts(const FShipDesign& Design, TArray<FBriefingShipPreviewPart>& OutParts,
        const TArray<const FWeaponDesign*>& Stores = {});
    STARSHATTERWARS_API FBox CalculateBounds(const TArray<FBriefingShipPreviewPart>& Parts);
    STARSHATTERWARS_API float FitDistance(float Radius, const FVector2D& ViewSize);
    // Rejects main Game/PIE worlds. Only isolated preview worlds are accepted.
    STARSHATTERWARS_API AActor* CreateVisual(UWorld* PreviewWorld, const TArray<FBriefingShipPreviewPart>& Parts, bool bAddLighting = true);
}

UCLASS()
class STARSHATTERWARS_API UShipBriefingPreview : public UUserWidget
{
    GENERATED_BODY()
public:
    bool SetShipDesign(const FShipDesign* Design, const TArray<const FWeaponDesign*>& Stores = {});
    bool SetMissionScene(Mission* MissionDef);
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
    UFUNCTION() void PlayRehearsal();
    UFUNCTION() void RewindRehearsal();
    UFUNCTION() void PauseRehearsal();
    UFUNCTION() void StopRehearsal();
    void UpdateRehearsal();
    void UpdateVisual();
    void UpdateCamera();
    void ClearVisual();
    UPROPERTY(Transient) TObjectPtr<UViewport> PreviewViewport;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UnavailableText;
    UPROPERTY(Transient) TArray<FBriefingShipPreviewPart> Parts;
    UPROPERTY(Transient) TArray<FBriefingShipPreviewPart> RouteShipParts;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RouteCaption;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> RouteChecklist;
    UPROPERTY(Transient) TObjectPtr<UBorder> EventPopup;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EventPopupText;
    TArray<FVector> Route;
    TArray<FString> RouteLabels;
    TArray<double> RouteArrivals;
    TArray<double> RouteDepartures;
    struct FTimedCue { double Time; FString Text; };
    TArray<FTimedCue> TimedCues;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PlaybackControls;
    double RehearsalDuration = 0.0;
    int32 LastCaptionSecond = INDEX_NONE;
    bool bRehearsalPlaying = false;
    TWeakObjectPtr<AActor> RouteShip;
    TWeakObjectPtr<AActor> PreviewVisual;
    FBox MissionBounds = FBox(ForceInit);
    double RouteTime = 0.0;
    bool bMissionScene = false;
    FString DesignKey;
    FVector2D LastViewSize = FVector2D::ZeroVector;
    float OrbitYaw = -130.f;
    float OrbitPitch = 20.f;
    float Zoom = 1.f;
    bool bPreviewActive = true;
    bool bDragging = false;
};
