// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h" 
#include "Kismet/GameplayStatics.h"
#include "GameStructs.h"
#include "Components/CanvasPanel.h" 

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UWorld;
class UUserWidget;
class UImage;

class USystemMap;
class USectorMap;
class UPlanetMarkerWidget;
class UCentralSUnWidget;
class UCentralPlanetWidget;
class UMoonMarkerWidget;
class UOrbitRingWidget;

/**
 * 
 */

struct STARSHATTERWARS_API FPlanetFocusResult
{
	float ScrollOffsetY = 0.f;
	float CanvasOffsetX = 0.f;
	float ZoomLevel = -1.f;
};

class STARSHATTERWARS_API SystemMapUtils
{

public:	
	
	/** Creates a unique 512x512 RGBA8 render target with black clear color */
	static UTextureRenderTarget2D* CreateRenderTarget(const FString& BaseName, int32 Resolution = 256, UObject * Outer = nullptr);

	UFUNCTION()
	static float GetUISizeFromRadiusKm(float RadiusKm, EBodyUISizeClass SizeClass);

	UFUNCTION()
	static float ClampZoomLevel(float ProposedZoom, float MinZoom = 0.5f, float MaxZoom = 3.0f);

	UFUNCTION()
	static float SnapZoomToSteps(float Zoom, float StepSize = 0.1f, float MinZoom = 0.5f, float MaxZoom = 3.0f);

	UFUNCTION()
	static float GetCenteredCanvasPosition(float ContentWidth, float ViewportWidth);

	UFUNCTION()
	static float GetCenteredScrollOffset(float ContentHeight, float ViewportHeight);

	UFUNCTION()
	static float InterpolateScrollOffset(float CurrentOffset, float TargetOffset, float DeltaTime, float Speed = 10.f);

	UFUNCTION()
	static float ClampVerticalPosition(float ProposedY, float ContentHeight, float ViewportHeight, float Margin = 50.f);

	UFUNCTION()
	static float ClampHorizontalPosition(float ProposedY, float ContentHeight, float ViewportHeight, float Margin = 50.f);

	UFUNCTION()
	static float ClampVerticalScroll(float ProposedOffset, float ContentHeight, float ViewportHeight, float Margin = 50.f);

	UFUNCTION()
	static float ClampHorizontalScroll(float ProposedOffset, float ContentWidth, float ViewportWidth, float Margin = 50.f);

	// Applies combined zoom and tilt transform to a target widget
	UFUNCTION()	
	static void ApplyZoomAndTilt(UCanvasPanel* MapCanvas, float Zoom, float Tilt);
	
	UFUNCTION()
	static void ApplyWidgetTilt(UWidget* Widget, float TiltAmount);

	// Schedule a safe CaptureScene() call on the next tick
	static void ScheduleSafeCapture(UObject* WorldContext, USceneCaptureComponent2D* Capture);

	// Cubic smooth step: ease-in/out
	static float EaseInOut(float t);

	UFUNCTION()
	static void ApplyZoomToCanvas(UCanvasPanel* Canvas, float Zoom)
	{
		if (!Canvas) return;
		FWidgetTransform Transform;
		Transform.Scale = FVector2D(Zoom, Zoom);
		Canvas->SetRenderTransform(Transform);
	}

	static UTextureRenderTarget2D* CreateUniqueRenderTargetForActor(
		const FString& Name,
		AActor* OwnerActor,
		int32 Resolution = 512
	);


	/** Computes a moon's 2D orbit offset based on its orbital parameters */
	static FVector2D ComputeMoonOrbitOffset(
		float OrbitKm,
		float OrbitAngleDegrees,
		float Inclination,
		float OrbitToScreen
	);

	UFUNCTION()
	static FPlanetFocusResult CenterOnPlanet(
		const FVector2D& MarkerPosition,
		const FVector2D& MarkerSize,
		const FVector2D& ViewportSize,
		const FVector2D& ContentSize,
		float CurrentZoom,
		float RequestedZoom = -1.f,
		float Margin = 50.f);
	
	// Returns a clamped canvas offset that keeps content partially visible within the viewport
	UFUNCTION()
	static FVector2D ConvertTopLeftToCenterAnchored(const FVector2D& TopLeftPos, const FVector2D& CanvasSize);
	UFUNCTION()
	static FBox2D ComputeContentBounds(const TSet<UWidget*>& ContentWidgets, UCanvasPanel* Canvas);

	// Returns a clamped canvas offset that keeps content partially visible within the viewport
	UFUNCTION()
	static FVector2D ClampCanvasToSafeMargin(
		const FVector2D& ProposedCanvasPos,
		const FVector2D& CanvasSize,
		const FVector2D& ViewportSize,
		const float Margin = 100.f
	)
	{
		// Ensure at least Margin padding from all edges
		const FVector2D Min = ViewportSize - CanvasSize + FVector2D(Margin, Margin);
		const FVector2D Max = FVector2D::ZeroVector - FVector2D(Margin, Margin);

		return FVector2D(
			FMath::Clamp(ProposedCanvasPos.X, Min.X, Max.X),
			FMath::Clamp(ProposedCanvasPos.Y, Min.Y, Max.Y)
		);
	}

	// Converts screen drag positions to local widget delta
	UFUNCTION()
	static FVector2D ComputeLocalDragDelta(const FGeometry& Geometry, const FPointerEvent& Event, const FVector2D& DragStartLocal)
	{
		const FVector2D CurrentLocal = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		return CurrentLocal - DragStartLocal;
	}

	struct FSystemMapDragController
	{
		bool bIsDragging = false;
		FVector2D DragStartLocal = FVector2D::ZeroVector;
		FVector2D InitialCanvasOffset = FVector2D::ZeroVector;

		void BeginDrag(const FGeometry& Geometry, const FPointerEvent& Event, const FVector2D& CurrentCanvasOffset)
		{
			DragStartLocal = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			InitialCanvasOffset = CurrentCanvasOffset;
			bIsDragging = true;
		}

		FVector2D ComputeDragDelta(const FGeometry& Geometry, const FPointerEvent& Event) const
		{
			if (!bIsDragging) return FVector2D::ZeroVector;
			const FVector2D CurrentLocal = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			return CurrentLocal - DragStartLocal;
		}

		void EndDrag()
		{
			bIsDragging = false;
		}
	};

	static UTextureRenderTarget2D* EnsureRenderTarget(UObject* Owner, const FString& Label, int32 Resolution, USceneCaptureComponent2D* Capture, UPrimitiveComponent* ShowOnlyComp);

	static UMaterialInstanceDynamic* CreatePreviewMID(
		UObject* Outer,
		UMaterialInterface* BaseMaterial,
		UTexture* BaseTexture = nullptr,
		const FString& Label = TEXT("Preview")
	);

	static UTextureRenderTarget2D* CreateSystemOverviewRenderTarget(
		UWorld* World,
		FVector CaptureLocation,
		FVector CaptureTarget,
		int32 Resolution = 2048,
		const FString& Name = TEXT("SystemOverview")
	);

	static void ApplyRenderTargetToImage(
		UObject* Outer,
		UImage* Image,
		UMaterialInterface* BaseMaterial,
		UTextureRenderTarget2D* RenderTarget,
		FVector2D BrushSize = FVector2D(128.f, 128.f)
	);

	static FVector ComputePlanetWorldPosition(
		const FVector& StarLocation,
		float OrbitKm,
		float OrbitAngleDeg,
		float InclinationDeg,
		float OrbitToWorldScale = 100000.0f
	);

	static UTextureRenderTarget2D* RenderWidgetToTexture(UUserWidget* Widget, int32 Width, int32 Height, float Scale = 1.0f);

	static void DestroyAllSystemActors(UWorld* World);	
	static void DestroyAllSectorActors(UWorld* World);
	
	template<typename T>
	static void DestroyAllActorsOfType(UWorld* WorldContext)
	{
		if (!WorldContext) return;

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(WorldContext, T::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	}

	static void ClearAllSystemUIElements(USystemMap* Map);
	static void ClearAllSectorUIElements(USectorMap* Map);
	
	// Estimate ideal render target resolution based on planet radius in kilometers
	static int32 GetRenderTargetResolutionForRadius(double RadiusKm);

	static float GetBodyUIScale(double RadiusKm);

	static FRotator GetBodyRotation(float TimeSeconds, float RotationSpeedDegreesPerSec, float TiltDegrees);

	/** Load uasset materials */
	static UTexture2D* LoadBodyAssetTexture(const FString& AssetName, const FString& TextureName);

	/** Gets a UI image scale (in pixels) from radius using log scale */
	static float GetUISizeFromRadius(float Radius, EBodyUISizeClass SizeClass);
	static float GetInitialPhaseDeg(uint64 UniverseSeed, const FString& BodyKey);
};

static float MapLogRadiusToSize(float RadiusKm, float MinRadiusKm, float MaxRadiusKm, float MinPx, float MaxPx, float Power);
