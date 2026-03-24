// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         SystemMap.h	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_Galaxy, FS_PlanetMap, etc.
#include "SystemMapUtils.h"
#include "SystemMap.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;
class USizeBox;
class UTextureRenderTarget2D;

class UPlanetMarkerWidget;
class UMoonMarkerWidget;
class USystemOrbitWidget;
class UCentralSunWidget;

class ACentralSunActor;
class APlanetPanelActor;

class UOperationsScreen;

// NEW RENDERER
class ASystemOverview;
struct FOverviewBody;

UCLASS()
class STARSHATTERWARS_API USystemMap : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Bound widgets ----
	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* MapCanvas = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* OuterCanvas = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	USizeBox* MapCanvasSize = nullptr;

	// New renderer output
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* OverviewImage = nullptr;

	// ---- Data / ownership ----
	UPROPERTY(meta = (BindWidgetOptional))
	FS_Galaxy SystemData;

	UPROPERTY()
	UOperationsScreen* OwningOperationsScreen = nullptr;

	void SetOwner(UOperationsScreen* Owner) { OwningOperationsScreen = Owner; }

	// ---- Core entry points ----
	void BuildSystemView(const FS_Galaxy* ActiveSystem);

	UFUNCTION()
	void InitMapCanvas();

	UFUNCTION()
	void HandleCentralSunClicked();

	UFUNCTION()
	void HandlePlanetSelected(FS_PlanetMap Planet);

	void ClearSystemView();

	void EnsureSystemOverviewRT();

	void EnsureSystemOverviewActor();

	void BuildSystemOverviewData(const FS_Galaxy* ActiveSystem, TArray<FOverviewBody>& OutBodies);

	void EnsureOverviewImage();

	// ---- Planet widgets ----
	TMap<FString, UPlanetMarkerWidget*> PlanetMarkers;
	TMap<FString, USystemOrbitWidget*> PlanetOrbitMarkers;

	UPlanetMarkerWidget* LastSelectedMarker = nullptr;
	TMap<FString, float> PlanetOrbitAngles;

	// ---- Overlay (optional) ----
	UPROPERTY()
	UMaterialInterface* OverlayMaterial = nullptr;

	UPROPERTY()
	FVector2D ScreenRenderSize = FVector2D(4096.f, 4096.f);

	void SetOverlay();
	void SetMarkerVisibility(bool bVisible);

	// ---- New renderer (SystemMap owns RT) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Overview")
	TSubclassOf<ASystemOverview> SystemOverviewActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Overview")
	int32 OverviewRTSize = 1024;

	void EnsureOverviewResources();
	void UpdateOverviewBrush();

	void CreateSystemView(const FS_Galaxy* ActiveSystem);

	TArray<FOverviewBody> BuildOverviewBodies(const FS_Galaxy* ActiveSystem);

	void BuildPlanetMarkersOnly();

	// New overview renderer (disabled initially)
	UPROPERTY()
	ASystemOverview* SystemOverviewActor = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "System|Overview")
	UMaterialInterface* OverviewMaterial = nullptr;

	UPROPERTY()
	bool bUseNewRenderer = false;   // OFF BY DEFAULT

protected:
	// UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// ---- Widget classes ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<UMoonMarkerWidget> MoonMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<UPlanetMarkerWidget> PlanetMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<USystemOrbitWidget> OrbitWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<UCentralSunWidget> StarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun")
	TSubclassOf<ACentralSunActor> SunActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<APlanetPanelActor> PlanetActorClass;

	UPROPERTY()
	TArray<APlanetPanelActor*> SpawnedPlanetActors;

private:
	// ---- Old-ish visuals (sun + rings + markers) ----
	UPROPERTY()
	UCentralSunWidget* CentralStarMarker = nullptr;

	UPROPERTY()
	ACentralSunActor* SunActor = nullptr;

	UPROPERTY()
	UCentralSunWidget* StarWidget = nullptr;

	// ---- Planet preview actors (optional / independent of overview) ----
	// NOTE: Do notxreuse a single PlanetActor pointer for all planets.
	// We keep only the spawned list.
	UPROPERTY()
	APlanetPanelActor* PlanetActor_DEPRECATED = nullptr;

	// ---- New renderer instances ----
	UPROPERTY(Transient)
	UTextureRenderTarget2D* SystemOverviewRT = nullptr;

	// ---- Helpers ----
	float GetDynamicOrbitScale(const TArray<FS_PlanetMap>& Planets, float MaxPixelRadius) const;

	UFUNCTION()
	void HandlePlanetClicked(const FString& PlanetName);

	// Centering (FIXED implementation lives in .cpp)
	void CenterOnPlanetWidget(UPlanetMarkerWidget* Marker);

	void AddCentralStar(const FS_Galaxy* Star);
	void AddPlanetOrbitalRing(const FS_PlanetMap& Planet);
	void AddPlanet(const FS_PlanetMap& Planet);

	void HighlightSelectedSystem();

	// ---- Viewport/cache ----
	UPROPERTY()
	FVector2D CachedViewportSize = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D CachedCanvasSize = FVector2D(3000.f, 2000.f);

	UPROPERTY()
	FVector2D CanvasSize = FVector2D(6000.f, 6000.f);

	// ---- Drag/zoom ----
	SystemMapUtils::FSystemMapDragController DragController;

	UPROPERTY()
	bool bIsDragging = false;

	UPROPERTY()
	FVector2D DragStartPos = FVector2D::ZeroVector;

	UPROPERTY()
	float MinZoom = 0.5f;

	UPROPERTY()
	float MaxZoom = 4.0f;

	UPROPERTY()
	float ZoomStep = 0.1f;

	UPROPERTY()
	float ZoomLevel = 1.0f;

	UPROPERTY()
	float TargetTiltAmount = 0.0f;

	// ---- Animation (planet focus) ----
	UPROPERTY()
	bool bIsAnimatingToPlanet = false;

	UPROPERTY()
	float PlanetFocusTime = 0.f;

	UPROPERTY()
	float PlanetFocusDuration = 0.5f;

	FVector2D StartCanvasPos = FVector2D::ZeroVector;
	FVector2D TargetCanvasPos = FVector2D::ZeroVector;

	// ---- Misc ----
	UPROPERTY()
	bool bPendingCanvasCenter = false;

	UPROPERTY()
	float ORBIT_TO_SCREEN = 1.f;

	UPROPERTY()
	float OrbitRadius = 0.f;

	// Tilt-in phase (optional)
	UPROPERTY()
	bool bIsTiltingIn = false;

	UPROPERTY()
	float TiltTime = 0.f;

	UPROPERTY()
	float TiltDuration = 0.6f;

	// Movement delay (optional)
	UPROPERTY()
	bool bIsWaitingToProcessMovement = false;

	UPROPERTY()
	float MovementDelayTime = 0.f;

	UPROPERTY()
	float MovementDelayDuration = 0.3f;
};
