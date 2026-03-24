// /*  Project nGenEx	Fractal Dev Games
//	Copyright (C) 2024. All Rights Reserved.
//	SUBSYSTEM:    SSW
//	FILE:         SectorMap.h
//	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "GameStructs.h"
#include "SSWGameInstance.h"
#include "SystemMapUtils.h"
#include "PlanetPanelActor.h"
#include "PlanetMarkerWidget.h"
#include "MoonMarkerWidget.h"
#include "SectorMap.generated.h"

class UCanvasPanel;
class USizeBox;

class USystemOrbitWidget;

class AMoonPanelActor;
class UOperationsScreen;

UCLASS()
class STARSHATTERWARS_API USectorMap : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UOperationsScreen* OwningOperationsScreen = nullptr;

	void SetOwner(UOperationsScreen* Owner) { OwningOperationsScreen = Owner; }

	// Called to draw all moons for the current planet/sector
	void BuildSectorView(const FS_PlanetMap& ActivePlanet);

	void InitSectorCanvas();

	UFUNCTION()
	void HighlightSelectedSystem();

	void AddCentralPlanet(const FS_PlanetMap& Planet);

	// Single orbit scale helper (keep one name)
	float GetDynamicOrbitScale(const TArray<FS_MoonMap>& Moons, float MaxPixelRadius) const;

	void ClearSectorView();

	UFUNCTION()
	void HandlePlanetClicked(const FString& PlanetName);

	void FocusAndZoomToMoon(UMoonMarkerWidget* Marker);

	// UI lookup maps
	TMap<FString, UPlanetMarkerWidget*> PlanetMarkers;
	TMap<FString, UMoonMarkerWidget*> MoonMarkers;
	TMap<FString, USystemOrbitWidget*> MoonOrbitMarkers;

	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* MapCanvas = nullptr;

	UMoonMarkerWidget* LastSelectedMarker = nullptr;
	// Holds per-moon orbit angle (randomized once per moon per session)
	TMap<FString, float> MoonOrbitAngles;

	// Stores the most recently selected moon marker
	UPROPERTY()
	UMoonMarkerWidget* LastSelectedMoonMarker = nullptr;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* OuterCanvas = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	USizeBox* MapCanvasSize = nullptr;

	// This is data, notxa widget binding
	UPROPERTY()
	FS_PlanetMap PlanetData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<APlanetPanelActor> PlanetActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<AMoonPanelActor> MoonActorClass;

	UPROPERTY()
	TArray<APlanetPanelActor*> SpawnedPlanetActors;

	UPROPERTY()
	TArray<AMoonPanelActor*> SpawnedMoonActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<UPlanetMarkerWidget> PlanetMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<UMoonMarkerWidget> MoonMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
	TSubclassOf<USystemOrbitWidget> OrbitWidgetClass;

	UPROPERTY()
	APlanetPanelActor* PlanetActor = nullptr;

	UPROPERTY()
	UPlanetMarkerWidget* PlanetMarker = nullptr;

	UFUNCTION()
	void SetZoomLevel(float NewZoom);

private:
	UFUNCTION()
	void HandleMoonClicked(const FString& MoonName);

	UFUNCTION()
	void CenterOnMoonWidget(UMoonMarkerWidget* Marker, float Zoom);

	void AddMoonOrbitalRing(const FS_MoonMap& Moon);
	void AddMoon(const FS_MoonMap& Moon);

	void SetMarkerVisibility(bool bVisible);

	// Drag controller (utility struct)
	SystemMapUtils::FSystemMapDragController DragController;

	UPROPERTY()
	FString SelectedMoonName;

	// Zoom + tilt state
	UPROPERTY()
	float StartZoomLevel = 1.0f;

	UPROPERTY()
	float TargetZoom = 1.0f;

	UPROPERTY()
	float ZoomInterpSpeed = 5.f;

	UPROPERTY()
	float MinZoom = 0.5f;

	UPROPERTY()
	float MaxZoom = 4.0f;

	UPROPERTY()
	float ZoomStep = 0.1f;

	UPROPERTY()
	float ZoomLevel = 1.0f;

	// Animation state
	UPROPERTY()
	bool bIsAnimatingToMoon = false;

	UPROPERTY()
	bool bIsCenteringToMoon = false;

	UPROPERTY()
	bool bIsZoomingToMoon = false;

	UPROPERTY()
	float TargetTiltAmount = 0.0f;

	UPROPERTY()
	float TiltDuration = 0.6f;

	UPROPERTY()
	bool bIsTiltingIn = false;

	UPROPERTY()
	float TiltTime = 0.0f;

	UPROPERTY()
	float MovementDelayTime = 0.0f;

	UPROPERTY()
	bool bIsWaitingToProcessMovement = false;

	UPROPERTY()
	float MovementDelayDuration = 0.3f;

	UPROPERTY()
	bool bPendingCanvasCenter = false;

	UPROPERTY()
	float MoonFocusTime = 0.f;

	UPROPERTY()
	float MoonFocusDuration = 0.5f;

	UPROPERTY()
	float ORBIT_TO_SCREEN = 1.0f;

	UPROPERTY()
	float OrbitRadius = 0.f;

	UPROPERTY()
	bool bIsDragging = false;

	UPROPERTY()
	bool bIsDraggingConfirmed = false;

	// Positions
	FVector2D StartCanvasPos = FVector2D::ZeroVector;
	FVector2D TargetCanvasPos = FVector2D::ZeroVector;

	FVector2D CachedViewportSize = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D CachedViewportCenter = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D DragStartPos = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D ZoomAnchorLocal = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D PreZoomCanvasPos = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D CachedCanvasSize = FVector2D(3000.f, 2000.f);

	UPROPERTY()
	FVector2D CanvasSize = FVector2D(6000.f, 6000.f);

	UPROPERTY()
	FVector2D CurrentDragOffset = FVector2D::ZeroVector;
};