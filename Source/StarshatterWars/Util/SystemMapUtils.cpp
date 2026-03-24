// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "SystemMapUtils.h"

#include "Components/Widget.h"
#include "Components/CanvasPanel.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetComponent.h"

#include "PlanetOrbitUtils.h"
#include "TimerManager.h"

#include "Materials/MaterialInstanceDynamic.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/World.h"

#include "GalaxyManager.h"
#include "SystemOrbitWidget.h"

#include "CentralSunActor.h"
#include "PlanetPanelActor.h"
#include "MoonPanelActor.h"

#include "SystemMap.h"
#include "SectorMap.h"

#include "CentralSunWidget.h"
#include "PlanetMarkerWidget.h"
#include "MoonMarkerWidget.h"

#include "Slate/WidgetRenderer.h"
#include "Widgets/SViewport.h"
#include "Kismet/KismetRenderingLibrary.h"


UTextureRenderTarget2D* SystemMapUtils::CreateUniqueRenderTargetForActor(
	const FString& Name,
	AActor* OwnerActor,
	int32 Resolution
)
{
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateUniqueRenderTargetForActor failed: null owner for %s"), *Name);
		return nullptr;
	}

	const FName UniqueRTName = FName(*FString::Printf(TEXT("RT_%s_%s"), *OwnerActor->GetName(), *Name));

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(
		OwnerActor,
		UTextureRenderTarget2D::StaticClass(),
		UniqueRTName,
		RF_Transient
	);

	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create render target for %s"), *Name);
		return nullptr;
	}

	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->InitAutoFormat(Resolution, Resolution);
	RenderTarget->UpdateResourceImmediate(true);

	UE_LOG(LogTemp, Log, TEXT("Created unique render target: %s [Addr: %p] for actor %s"),
		*UniqueRTName.ToString(), RenderTarget, *OwnerActor->GetName());

	return RenderTarget;
}


float SystemMapUtils::ClampZoomLevel(float ProposedZoom, float MinZoom, float MaxZoom)
{
	return FMath::Clamp(ProposedZoom, MinZoom, MaxZoom);
}

float SystemMapUtils::GetCenteredCanvasPosition(float ContentWidth, float ViewportWidth)
{
	return (ViewportWidth - ContentWidth) * 0.5f;
}

float SystemMapUtils::GetCenteredScrollOffset(float ContentHeight, float ViewportHeight)
{
	const float MaxScrollY = FMath::Max(ContentHeight - ViewportHeight, 0.f);
	return MaxScrollY * 0.5f;
}

float SystemMapUtils::InterpolateScrollOffset(float CurrentOffset, float TargetOffset, float DeltaTime, float Speed)
{
	return FMath::FInterpTo(CurrentOffset, TargetOffset, DeltaTime, Speed);
}

float SystemMapUtils::SnapZoomToSteps(float Zoom, float StepSize, float MinZoom, float MaxZoom)
{
	float Clamped = FMath::Clamp(Zoom, MinZoom, MaxZoom);
	float Steps = FMath::RoundToFloat(Clamped / StepSize);
	return FMath::Clamp(Steps * StepSize, MinZoom, MaxZoom);
}

float SystemMapUtils::ClampHorizontalPosition(float ProposedX, float ContentWidth, float ViewportWidth, float Margin)
{
	const float MinVisible = 32.f; // ensure part of the central sun stays on-screen

	// Prevent the left edge from moving too far right (star disappearing)
	const float MaxX = Margin;

	// Prevent the right edge from dragging too far left (entire canvas offscreen)
	const float MinX = FMath::Min(ViewportWidth - ContentWidth + Margin, Margin - MinVisible);
	//const float MinX = ViewportWidth - ContentWidth - Margin;

	return FMath::Clamp(ProposedX, MinX, MaxX);
}

float SystemMapUtils::ClampVerticalPosition(float ProposedY, float ContentHeight, float ViewportHeight, float Margin)
{
	const float MinY = Margin; // Prevents top from going offscreen
	const float MaxY = FMath::Max(ContentHeight - ViewportHeight - Margin, 0.f); // Prevents bottom overscroll

	return FMath::Clamp(ProposedY, MinY, MaxY);
}

float SystemMapUtils::ClampVerticalScroll(float ProposedOffset, float ContentHeight, float ViewportHeight, float Margin)
{
	const float MaxScrollY = FMath::Max(ContentHeight - ViewportHeight - Margin, 0.f);
	const float MinScrollY = Margin;

	return FMath::Clamp(ProposedOffset, MinScrollY, MaxScrollY);
}

float SystemMapUtils::ClampHorizontalScroll(float ProposedOffset, float ContentWidth, float ViewportWidth, float Margin)
{
	const float MaxScrollX = FMath::Max(ContentWidth - ViewportWidth - Margin, 0.f);
	const float MinScrollX = Margin;

	return FMath::Clamp(ProposedOffset, MinScrollX, MaxScrollX);
}

float SystemMapUtils::EaseInOut(float t)
{
	return t * t * (3.f - 2.f * t);
}

void SystemMapUtils::ApplyZoomAndTilt(UCanvasPanel* MapCanvas, float Zoom, float Tilt)
{
	if (!MapCanvas) return;

	// Apply zoom and tilt to the canvas itself
	FWidgetTransform CanvasTransform;
	CanvasTransform.Scale = FVector2D(Zoom, Zoom);
	CanvasTransform.Shear = FVector2D(0.0f, -Tilt); // Vertical tilt on canvas
	MapCanvas->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	MapCanvas->SetRenderTransform(CanvasTransform);

	// Apply tilt/parallax to children individually (for visual depth)
	for (UWidget* Child : MapCanvas->GetAllChildren())
	{
		if (!Child) continue;

		// Optional: only apply to specific types
		if (Child->IsA(UPlanetMarkerWidget::StaticClass()) ||
			Child->IsA(USystemOrbitWidget::StaticClass()))
		{
			FWidgetTransform ChildTransform;
			ChildTransform.Shear = FVector2D(0.0f, -Tilt * 0.5f); // Shallower parallax
			Child->SetRenderTransform(ChildTransform);
		}
	}
}

FPlanetFocusResult SystemMapUtils::CenterOnPlanet(
	const FVector2D& MarkerPosition,
	const FVector2D& MarkerSize,
	const FVector2D& ViewportSize,
	const FVector2D& ContentSize,
	float CurrentZoom,
	float RequestedZoom,
	float Margin)
{
	FPlanetFocusResult Result;

	const FVector2D MarkerCenter = MarkerPosition + MarkerSize * 0.5f;

	UE_LOG(LogTemp, Log, TEXT("CenterOnPlanet:"));
	UE_LOG(LogTemp, Log, TEXT("  MarkerPos: %s, MarkerSize: %s, MarkerCenter: %s"), *MarkerPosition.ToString(), *MarkerSize.ToString(), *MarkerCenter.ToString());
	UE_LOG(LogTemp, Log, TEXT("  ViewportSize: %s, ContentSize: %s, CurrentZoom: %.2f, RequestedZoom: %.2f"), *ViewportSize.ToString(), *ContentSize.ToString(), CurrentZoom, RequestedZoom);

	// Scroll Y clamp
	const float ProposedScrollY = MarkerCenter.Y - ViewportSize.Y * 0.5f;
	const float MaxScrollY = FMath::Max(ContentSize.Y - ViewportSize.Y, 0.f);
	const float MinScrollY = 0.f;
	Result.ScrollOffsetY = FMath::Clamp(ProposedScrollY, MinScrollY, MaxScrollY);

	// Canvas X clamp
	const float ProposedCanvasX = ViewportSize.X * 0.5f - MarkerCenter.X;
	const float MaxCanvasX = Margin;
	const float MinCanvasX = ViewportSize.X - ContentSize.X;
	Result.CanvasOffsetX = FMath::Clamp(ProposedCanvasX, MinCanvasX, MaxCanvasX);

	UE_LOG(LogTemp, Log, TEXT("  ProposedCanvasX: %.1f, ClampedCanvasX: %.1f (MinX: %.1f, MaxX: %.1f)"), ProposedCanvasX, Result.CanvasOffsetX, MinCanvasX, MaxCanvasX);

	// Zoom level
	Result.ZoomLevel = (RequestedZoom > 0.f)
		? ClampZoomLevel(RequestedZoom)
		: CurrentZoom;

	UE_LOG(LogTemp, Log, TEXT("  Final ZoomLevel: %.2f"), Result.ZoomLevel);

	return Result;
}

FVector2D SystemMapUtils::ConvertTopLeftToCenterAnchored(const FVector2D& TopLeftPos, const FVector2D& CanvasSize)
{
	return TopLeftPos - (CanvasSize * 0.5f);
}

UTextureRenderTarget2D* SystemMapUtils::CreateRenderTarget(const FString& Name, int32 Resolution, UObject* Outer)
{
	if (!Outer)
	{
		Outer = GetTransientPackage(); // fallback
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(Outer, *Name);
	if (!RenderTarget)
	{
		return nullptr;
	}

	Resolution = FMath::Clamp(Resolution, 256, 2048);
	RenderTarget->RenderTargetFormat = RTF_RGBA16f; // good for lit previews
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->InitAutoFormat(Resolution, Resolution);
	RenderTarget->UpdateResourceImmediate(true);

	return RenderTarget;
}

static float MapLogRadiusToSize(float RadiusKm, float MinRadiusKm, float MaxRadiusKm, float MinPx, float MaxPx, float Power)
{
	RadiusKm = FMath::Clamp(RadiusKm, MinRadiusKm, MaxRadiusKm);

	const float LogR = FMath::LogX(10.f, RadiusKm);
	const float LogMin = FMath::LogX(10.f, MinRadiusKm);
	const float LogMax = FMath::LogX(10.f, MaxRadiusKm);

	float T = (LogR - LogMin) / (LogMax - LogMin);
	T = FMath::Clamp(T, 0.f, 1.f);

	// Perceptual curve: <1 boosts large end; >1 boosts small end
	T = FMath::Pow(T, Power);

	return FMath::Lerp(MinPx, MaxPx, T);
}

float SystemMapUtils::GetUISizeFromRadiusKm(float RadiusKm, EBodyUISizeClass SizeClass)
{
	switch (SizeClass)
	{
		case EBodyUISizeClass::Moon:
		{
			// Example moon radii range: ~300 km to ~3000 km
			// (tune for your data)
			const float Size = MapLogRadiusToSize(RadiusKm,
				300.f, 3500.f,
				18.f, 44.f,
				0.85f); // keeps differences but prevents tiny moons

			return FMath::Max(Size, 18.f); // hard minimum for clickability
		}

		case EBodyUISizeClass::Planet:
		{
			// Example planet radii range: ~2500 km to ~70000 km
			const float Size = MapLogRadiusToSize(RadiusKm,
				2000.f, 80000.f,
				28.f, 92.f,
				0.75f);

			return Size;
		}

		case EBodyUISizeClass::Star:
		default:
		{
			// Your star ranges are much larger; use your existing StarUtils if you prefer.
			const float Size = MapLogRadiusToSize(RadiusKm,
				1.2e9f, 2.2e9f,
				64.f, 160.f,
				0.70f);

			return Size;
		}
	}
}

FBox2D SystemMapUtils::ComputeContentBounds(const TSet<UWidget*>& ContentWidgets, UCanvasPanel* Canvas)
{
	FBox2D Bounds(EForceInit::ForceInit);

	if (!Canvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("ComputeContentBounds: Canvas is null"));
		return Bounds;
	}

	int32 Count = 0;

	for (UWidget* Widget : ContentWidgets)
	{
		if (!Widget)
		{
			UE_LOG(LogTemp, Warning, TEXT("ComputeContentBounds: Null widget skipped"));
			continue;
		}

		if (!Canvas->GetAllChildren().Contains(Widget))
		{
			UE_LOG(LogTemp, Warning, TEXT("ComputeContentBounds: Widget %s notxfound in canvas"), *Widget->GetName());
			continue;
		}

		if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			const FVector2D Pos = Slot->GetPosition();
			const FVector2D Size = Slot->GetSize();
			const FVector2D Align = Slot->GetAlignment();

			const FVector2D TopLeft = Pos - Size * Align;
			const FVector2D BottomRight = TopLeft + Size;

			UE_LOG(LogTemp, Warning,
				TEXT("[Bounds] Widget=%s | Pos=%s | Size=%s | Align=%s | TL=%s | BR=%s"),
				*Widget->GetName(),
				*Pos.ToString(),
				*Size.ToString(),
				*Align.ToString(),
				*TopLeft.ToString(),
				*BottomRight.ToString());

			Bounds += TopLeft;
			Bounds += BottomRight;
			Count++;
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ComputeContentBounds: Widget %s is notxin a CanvasPanelSlot"),
				*Widget->GetName());
		}
	}
	Bounds = Bounds.ExpandBy(FVector2D(400.f, 300.f)); // pad outward

	UE_LOG(LogTemp, Warning,
		TEXT("[ComputeContentBounds] %d valid widgets | Final Bounds: Min=%s Max=%s Size=%s"),
		Count,
		*Bounds.Min.ToString(),
		*Bounds.Max.ToString(),
		*(Bounds.GetSize().ToString()));

	return Bounds;
}

FVector2D SystemMapUtils::ComputeMoonOrbitOffset(
	float OrbitKm,
	float OrbitAngleDegrees,
	float Inclination,
	float OrbitToScreen
)
{
	const float AngleRad = FMath::DegreesToRadians(FMath::Fmod(OrbitAngleDegrees, 360.0f));
	const float Inclined = PlanetOrbitUtils::AmplifyInclination(Inclination, 2.0f);
	const float Radius = OrbitKm / OrbitToScreen;

	return PlanetOrbitUtils::Get2DOrbitPositionWithInclination(Radius, AngleRad, Inclined);
}

void SystemMapUtils::ScheduleSafeCapture(UObject* WorldContext, USceneCaptureComponent2D* Capture)
{
	if (!WorldContext || !Capture)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	// Weak refs prevent use-after-free when actors/components are destroyed
	TWeakObjectPtr<USceneCaptureComponent2D> WeakCapture(Capture);

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakCapture]()
		{
			if (!WeakCapture.IsValid())
			{
				return;
			}

			USceneCaptureComponent2D* Cap = WeakCapture.Get();

			// Additional guards: during teardown these can be false/invalid
			if (!IsValid(Cap) || Cap->IsUnreachable() || Cap->HasAnyFlags(RF_BeginDestroyed))
			{
				return;
			}

			// Must have a target and be registered
			if (!Cap->IsRegistered() || Cap->TextureTarget == nullptr)
			{
				return;
			}

			Cap->CaptureScene();
		}));
}


UTextureRenderTarget2D* SystemMapUtils::EnsureRenderTarget(
	UObject* Owner,
	const FString& Label,
	int32 Resolution,
	USceneCaptureComponent2D* Capture,
	UPrimitiveComponent* ShowOnlyComp)
{
	if (!Owner || !Capture) return nullptr;

	// Unique RT object name per actor instance
	const FName UniqueRTName = MakeUniqueObjectName(
		Owner,
		UTextureRenderTarget2D::StaticClass(),
		FName(*FString::Printf(TEXT("RT_%s"), *Label))
	);

	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Owner, UniqueRTName);
	RT->RenderTargetFormat = RTF_RGBA16f;
	RT->ClearColor = FLinearColor::Black;
	RT->bAutoGenerateMips = false;
	RT->InitAutoFormat(Resolution, Resolution);
	RT->UpdateResourceImmediate(true);

	// Configure capture to only this component
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->ShowOnlyComponents.Empty();
	if (ShowOnlyComp)
	{
		Capture->ShowOnlyComponents.Add(ShowOnlyComp);
	}

	Capture->TextureTarget = RT;

	UE_LOG(LogTemp, Warning, TEXT("RT CREATED: %s  PTR=%p  Outer=%s"),
		*RT->GetName(), RT, *GetNameSafe(Owner));

	return RT;
}

UMaterialInstanceDynamic* SystemMapUtils::CreatePreviewMID(
	UObject* Outer,
	UMaterialInterface* BaseMaterial,
	UTexture* BaseTexture,
	const FString& Label)
{
	if (!BaseMaterial || !Outer)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewMID failed: Missing base material or outer"));
		return nullptr;
	}

	// Create dynamic instance
	UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, Outer);
	if (!DynMat)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewMID failed: UMaterialInstanceDynamic::Create returned null"));
		return nullptr;
	}

	// Give it a unique name for debugging
	const FString MIDName = FString::Printf(TEXT("MID_%s"), *Label);
	DynMat->Rename(*MIDName);

	if (BaseTexture && DynMat)
	{
		// Only needed if texture contents were changed at runtime
		// BaseTexture->UpdateResource();

		DynMat->SetTextureParameterValue(
			FName(TEXT("BaseTexture")),
			BaseTexture
		);
	}

	UE_LOG(LogTemp, Log, TEXT("Created PreviewMID: %s (Label: %s, Texture: %s)"),
		*MIDName,
		*Label,
		*GetNameSafe(BaseTexture));

	return DynMat;
}

UTextureRenderTarget2D* SystemMapUtils::CreateSystemOverviewRenderTarget(
	UWorld* World,
	FVector CaptureLocation,
	FVector CaptureTarget,
	int32 Resolution,
	const FString& Name)
{
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSystemOverviewRenderTarget: World is null"));
		return nullptr;
	}

	// Create the render target
	UTextureRenderTarget2D* RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		World,
		Resolution,
		Resolution,
		RTF_RGBA16f
	);

	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create system render target"));
		return nullptr;
	}

	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->UpdateResourceImmediate(true);

	// Spawn a temporary SceneCapture actor
	ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), CaptureLocation, FRotator::ZeroRotator);
	if (!CaptureActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn SceneCapture2D"));
		return nullptr;
	}

	USceneCaptureComponent2D* Capture = CaptureActor->GetCaptureComponent2D();
	Capture->SetWorldLocation(CaptureLocation);
	Capture->SetWorldRotation((CaptureTarget - CaptureLocation).Rotation());
	Capture->FOVAngle = 30.f;
	Capture->TextureTarget = RenderTarget;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;

	// Optional: remove unnecessary render features
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetSkyLighting(false);
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetPostProcessing(false);
	
	UE_LOG(LogTemp, Warning, TEXT("SceneCapture using RT: %s"), *GetNameSafe(Capture->TextureTarget));


	// Trigger the capture
	Capture->CaptureScene();

	UE_LOG(LogTemp, Log, TEXT("System overview captured to %s (Size: %dx%d)"), *Name, Resolution, Resolution);
	return RenderTarget;
}

void SystemMapUtils::ApplyRenderTargetToImage(
	UObject* Outer,
	UImage* Image,
	UMaterialInterface* BaseMaterial,
	UTextureRenderTarget2D* RenderTarget,
	FVector2D BrushSize)
{
	if (!Image || !BaseMaterial || !RenderTarget || !Outer)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyRenderTargetToImage: Invalid input"));
		return;
	}

	UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, Outer);
	if (!DynMat)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material"));
		return;
	}

	DynMat->SetTextureParameterValue("InputTexture", RenderTarget);
	Image->SetBrushFromMaterial(DynMat);
	Image->SetBrushSize(BrushSize);

	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot))
	{
		Slot->SetSize(BrushSize);
	}

	UE_LOG(LogTemp, Log, TEXT("RenderTarget applied to image: %s"), *RenderTarget->GetName());
}

FVector SystemMapUtils::ComputePlanetWorldPosition(
	const FVector& StarLocation,
	float OrbitKm,
	float OrbitAngleDeg,
	float InclinationDeg,
	float OrbitToWorldScale
)
{
	const float OrbitRadius = OrbitKm / OrbitToWorldScale;
	const float AngleRad = FMath::DegreesToRadians(OrbitAngleDeg);
	const float InclinationRad = FMath::DegreesToRadians(InclinationDeg);

	const float X = FMath::Cos(AngleRad) * OrbitRadius;
	const float Y = FMath::Sin(AngleRad) * OrbitRadius;
	const float Z = OrbitRadius * FMath::Sin(InclinationRad); // simple vertical tilt

	return StarLocation + FVector(X, Y, Z);
}

UTextureRenderTarget2D* SystemMapUtils::RenderWidgetToTexture(UUserWidget* Widget, int32 Width, int32 Height, float Scale)
{
	if (!Widget)
	{
		UE_LOG(LogTemp, Error, TEXT("RenderWidgetToTexture: Widget is null"));
		return nullptr;
	}

	// Create temporary shared renderer
	TSharedRef<FWidgetRenderer> Renderer = MakeShared<FWidgetRenderer>(true);

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->InitAutoFormat(Width, Height);
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->UpdateResourceImmediate();

	// Draw the widget directly into the RT
	Renderer->DrawWidget(
		RenderTarget,
		Widget->TakeWidget(),
		FVector2D(Width, Height),
		Scale
	);

	UE_LOG(LogTemp, Log, TEXT("Widget rendered to RenderTarget: %dx%d"), Width, Height);
	return RenderTarget;
}

void SystemMapUtils::DestroyAllSystemActors(UWorld* World)
{
	DestroyAllActorsOfType<APlanetPanelActor>(World);
	DestroyAllActorsOfType<ACentralSunActor>(World);
}

void SystemMapUtils::DestroyAllSectorActors(UWorld* World)
{
	DestroyAllActorsOfType<APlanetPanelActor>(World);
	DestroyAllActorsOfType<AMoonPanelActor>(World);
}

void SystemMapUtils::ClearAllSystemUIElements(USystemMap* Map)
{
	if (!Map || !Map->MapCanvas)
		return;

	// Remove all children from MapCanvas
	Map->MapCanvas->ClearChildren();

	// Destroy planet markers
	for (auto& Pair : Map->PlanetMarkers)
	{
		if (UPlanetMarkerWidget* Marker = Pair.Value)
		{
			Marker->RemoveFromParent();
			Marker->MarkAsGarbage();
		}
	}
	Map->PlanetMarkers.Empty();

	// Destroy orbit rings
	for (auto& Pair : Map->PlanetOrbitMarkers)
	{
		if (USystemOrbitWidget* Ring = Pair.Value)
		{
			Ring->RemoveFromParent();
			Ring->MarkAsGarbage();
		}
	}
	Map->PlanetOrbitMarkers.Empty();

	// Clear selection
	Map->LastSelectedMarker = nullptr;
	//Map->PlanetOrbitAngles.Empty();

	UE_LOG(LogTemp, Warning, TEXT("[SystemMapUtils] Cleared all UI system elements (markers + rings)"));
}

void SystemMapUtils::ClearAllSectorUIElements(USectorMap* Map)
{
	if (!Map || !Map->MapCanvas)
		return;

	// Remove all children from MapCanvas
	Map->MapCanvas->ClearChildren();

	// Destroy planet markers
	for (auto& Pair : Map->PlanetMarkers)
	{
		if (UPlanetMarkerWidget* Marker = Pair.Value)
		{
			Marker->RemoveFromParent();
			Marker->MarkAsGarbage();
		}
	}
	Map->PlanetMarkers.Empty();

	// Destroy orbit rings
	for (auto& Pair : Map->MoonOrbitMarkers)
	{
		if (USystemOrbitWidget* Ring = Pair.Value)
		{
			Ring->RemoveFromParent();
			Ring->MarkAsGarbage();
		}
	}
	Map->MoonOrbitMarkers.Empty();

	// Clear selection
	Map->LastSelectedMoonMarker = nullptr;
	Map->MoonOrbitAngles.Empty();

	UE_LOG(LogTemp, Warning, TEXT("[SystemMapUtils] Cleared all Sector UI elements (markers + rings)"));
}

int32 SystemMapUtils::GetRenderTargetResolutionForRadius(double RadiusKm)
{
	double MinRadius = 0.25e6;
	double MaxRadius = 38.2e6;

	// Normalize with optional logarithmic scaling
	RadiusKm = FMath::Clamp(RadiusKm, MinRadius, MaxRadius);

	const double LogMin = FMath::LogX(10.0, MinRadius);
	const double LogMax = FMath::LogX(10.0, MaxRadius);
	const double LogVal = FMath::LogX(10.0, RadiusKm);
	double T = (LogVal - LogMin) / (LogMax - LogMin); // 0.0 - 1.0

	// Map T to a range of powers of 2: [128, 256, 512, 1024]
	T = FMath::Clamp(T, 0.0, 1.0);
	const TArray<int32> ResOptions = { 128, 256, 512, 1024 };

	int32 Index = FMath::FloorToInt(T * (ResOptions.Num() - 1));
	return ResOptions[Index];
}

float SystemMapUtils::GetBodyUIScale(double RadiusKm)
{
	double MinRadius = 0.25e6;
	double MaxRadius = 38.2e6; 
	// Get normalized [0.0, 1.0] range
	double Normalized = FMath::Clamp((RadiusKm - MinRadius) / (MaxRadius - MinRadius), 0.0, 1.0);

	// UI scaling range — safe for mesh scale and texture logic
	constexpr float MinUIScale = 0.5f;
	constexpr float MaxUIScale = 4.0f;

	// Final safe UI scale
	float UIScale = FMath::Lerp(MinUIScale, MaxUIScale, static_cast<float>(Normalized));

	// Clamp final value to hard max
	return FMath::Clamp(UIScale, MinUIScale, MaxUIScale);
}

FRotator SystemMapUtils::GetBodyRotation(float TimeSeconds, float RotationSpeedDegreesPerSec, float TiltDegrees)
{
	// Clamp tilt for safety
	float ClampedTilt = FMath::Clamp(TiltDegrees, -90.0f, 90.0f);

	// Compute current Yaw based on speed and time
	float CurrentYaw = FMath::Fmod(TimeSeconds * RotationSpeedDegreesPerSec, 360.0f);

	// Rotation = spin (Yaw) combined with axis tilt (Roll or Pitch)
	return FRotator(0.0f, CurrentYaw, ClampedTilt); // (Pitch, Yaw, Roll)
}

UTexture2D* SystemMapUtils::LoadBodyAssetTexture(const FString& AssetName, const FString& TextureName)
{
	FString AssetPath = FString::Printf(TEXT("/Game/GameData/Galaxy/%sMaterials/%s.%s"), *AssetName, *TextureName, *TextureName);

	UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath));
	if (!Texture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load body texture asset: %s"), *AssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded body texture asset: %s"), *Texture->GetName());
	}

	return Texture;
}

float SystemMapUtils::GetUISizeFromRadius(float RadiusKm, EBodyUISizeClass SizeClass)
{
	double MinRadiusKm;
	double MaxRadiusKm;
	
	switch (SizeClass)
		{
		case EBodyUISizeClass::Moon:
		{
			MinRadiusKm = 0.25e6;  // smallest moon
			MaxRadiusKm = 6.1e6;  // largest moon
			break;
		}

		case EBodyUISizeClass::Planet:
		{
			// Dataset-derived bounds (km)
			MinRadiusKm = 2.15e6;  // smallest planet
			MaxRadiusKm = 38.2e6;  // largest planet
			break;
		}

		case EBodyUISizeClass::Star:
		default:
		{
			MinRadiusKm = 1.2e9;  // smallest moon
			MaxRadiusKm = 2.2e9;  // largest planet
			break;
		}
	}

	const double R = FMath::Clamp<double>(RadiusKm, MinRadiusKm, MaxRadiusKm);

	// Log normalization: compresses dynamic range while preserving ordering
	const double LogR = FMath::LogX(10.0, R);
	const double LogMin = FMath::LogX(10.0, MinRadiusKm);
	const double LogMax = FMath::LogX(10.0, MaxRadiusKm);

	double T = (LogR - LogMin) / (LogMax - LogMin);
	T = FMath::Clamp(T, 0.0, 1.0);

	// Perceptual shaping:
	// - Lower than 1.0 makes big bodies grow faster (more readable size differences)
	// - 0.60–0.75 is a practical range; 0.65 is a good default
	constexpr double PerceptualPower = 0.65;
	T = FMath::Pow(T, PerceptualPower);

	// Optional: top-end boost so the largest planets separate more clearly.
	// This only kicks in above ~70% of the range.
	// Comment out if you want pure pow() only.
	if (T > 0.70)
	{
		const double U = (T - 0.70) / 0.30;      // remap to 0..1
		const double Boost = FMath::Pow(U, 0.55); // emphasize the top end
		T = 0.70 + (0.30 * Boost);
	}

	// UI pixel range (tune to taste)
	constexpr float MinPx = 16.f;   // smallest moon still visible
	constexpr float MaxPx = 128.f;  // allow giants to feel giant

	const float SizePx = FMath::Lerp(MinPx, MaxPx, (float)T);
	return FMath::Clamp(SizePx, MinPx, MaxPx);
}

static uint32 HashStringStable(const FString& S)
{
	// Stable across runs for same string
	return FCrc::StrCrc32(*S);
}

static uint32 HashCombine32(uint32 A, uint32 B)
{
	return HashCombineFast(A, B);
}

float SystemMapUtils::GetInitialPhaseDeg(uint64 UniverseSeed, const FString& BodyKey)
{
	const uint32 SeedLo = uint32(UniverseSeed & 0xFFFFFFFFull);
	const uint32 SeedHi = uint32((UniverseSeed >> 32) & 0xFFFFFFFFull);

	uint32 H = HashStringStable(BodyKey);
	H = HashCombine32(H, SeedLo);
	H = HashCombine32(H, SeedHi);

	// Map to [0,360)
	const float U = (H / 4294967296.0f); // [0,1)
	return U * 360.0f;
}










