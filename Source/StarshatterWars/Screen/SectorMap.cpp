// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "SectorMap.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"

#include "SystemOrbitWidget.h"
#include "OperationsScreen.h"

#include "GameStructs.h" // FS_Galaxy struct
#include "GalaxyManager.h"

#include "SSWGameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "PlanetOrbitUtils.h"
#include "SystemMapUtils.h"
#include "StarUtils.h"

#include "TimerManager.h"
#include "EngineUtils.h" 


void USectorMap::NativeConstruct()
{
	Super::NativeConstruct();
	InitSectorCanvas();
	
	if (USSWGameInstance* GI = GetGameInstance<USSWGameInstance>())
	{
		FS_PlanetMap SelectedSector = GI->SelectedSector;

		UE_LOG(LogTemp, Log, TEXT("USectorMap::NativeConstruct() Found planet: %s"), *SelectedSector.Name);
		BuildSectorView(GI->SelectedSector);
	}
}

void USectorMap::NativeDestruct()
{
	Super::NativeDestruct();
}

void USectorMap::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!MapCanvas)
		return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot);
	if (!CanvasSlot)
		return;

	// Handle movement delay timer
	if (bIsWaitingToProcessMovement)
	{
		MovementDelayTime += InDeltaTime;

		if (MovementDelayTime >= MovementDelayDuration)
		{
			bIsWaitingToProcessMovement = false;
			MovementDelayTime = 0.0f;

			// Trigger any post-drag action here
			if (LastSelectedMoonMarker)
			{
				CenterOnMoonWidget(LastSelectedMoonMarker, ZoomLevel);
				LastSelectedMoonMarker = nullptr;
			}
		}
	}

	if (bIsAnimatingToMoon)
	{
		MoonFocusTime += InDeltaTime;
		float Alpha = FMath::Clamp(MoonFocusTime / MoonFocusDuration, 0.f, 1.f);
		float Smoothed = SystemMapUtils::EaseInOut(Alpha); // ease = t * t * (3 - 2t)

		const FVector2D InterpPos = FMath::Lerp(StartCanvasPos, TargetCanvasPos, Smoothed);

		if (UCanvasPanelSlot* PlanetSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			PlanetSlot->SetPosition(InterpPos);
		}

		if (Alpha >= 1.f)
		{
			bIsAnimatingToMoon = false;
		}
	}

	// Phase 2: Zoom in/out
	/*if (!FMath::IsNearlyEqual(ZoomLevel, TargetZoom))
	{
		const float DeltaZoom = TargetZoom - ZoomLevel;
		ZoomLevel += DeltaZoom * FMath::Clamp(InDeltaTime * ZoomInterpSpeed, 0.f, 1.f);

		// Zoom at anchor point
		if (UCanvasPanelSlot* ZoomSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			const FVector2D ScaleDelta = FVector2D(ZoomLevel / FMath::Max(0.01f, ZoomLevel - DeltaZoom));
			const FVector2D AnchorOffset = ZoomAnchorLocal - PreZoomCanvasPos;
			const FVector2D ScaledOffset = AnchorOffset * (1.f - ZoomLevel / TargetZoom);

			const FVector2D NewCanvasPos = CanvasSlot->GetPosition() + ScaledOffset;
			ZoomSlot->SetPosition(NewCanvasPos);
		}

		// Apply transform scale
		SystemMapUtils::ApplyZoomToCanvas(MapCanvas, ZoomLevel);
	}*/

	// Phase 3: Tilt on load
	if (bIsTiltingIn)
	{
		TiltTime += InDeltaTime;
		float Progress = FMath::Clamp(TiltTime / TiltDuration, 0.0f, 1.0f);
		float SmoothT = SystemMapUtils::EaseInOut(Progress);

		float CurrentTilt = FMath::Lerp(0.0f, TargetTiltAmount, SmoothT);
		SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, CurrentTilt);

		if (Progress >= 1.0f)
		{
			bIsTiltingIn = false;
			SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, TargetTiltAmount);
		}
	}
}
void USectorMap::AddMoonOrbitalRing(const FS_MoonMap& Moon)
{
	OrbitRadius = Moon.Orbit / ORBIT_TO_SCREEN;

	UE_LOG(LogTemp, Warning, TEXT("Moon: %s -> Orbit radius: %f"), *Moon.Name, OrbitRadius);

	if (OrbitWidgetClass)
	{
		auto* MoonOrbitRing = CreateWidget<USystemOrbitWidget>(this, OrbitWidgetClass);

		// Apply visual configuration
		MoonOrbitRing->SetOrbitRadius(OrbitRadius);
		//const float InclinationVisual = PlanetOrbitUtils::AmplifyInclination(Planet.Inclination);
		//Orbit->SetOrbitInclination(InclinationVisual);

		// Position orbit ring widget
		if (UCanvasPanelSlot* OrbitSlot = MapCanvas->AddChildToCanvas(MoonOrbitRing))
		{
			OrbitSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			OrbitSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			OrbitSlot->SetPosition(FVector2D(0.f, 0.f)); // use panel center
			OrbitSlot->SetZOrder(0);
			OrbitSlot->SetAutoSize(true);

			MoonOrbitMarkers.Add(Moon.Name, MoonOrbitRing);
		}

		MoonOrbitRing->SetVisibility(ESlateVisibility::Visible);
	}
}


void USectorMap::HandleMoonClicked(const FString& MoonName) {
	/*if (UMoonMarkerWidget** Found = MoonMarkers.Find(MoonName))
	{
		LastSelectedMoonMarker = *Found;
		// Optional: center or highlight
		UE_LOG(LogTemp, Log, TEXT("Moon selected: %s"), *MoonName);
		CenterOnMoonWidget(*Found, 1.0f);
	}*/
}

void USectorMap::CenterOnMoonWidget(UMoonMarkerWidget* Marker, float Zoom)
{
	if (!Marker || !MapCanvas) return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot);
	UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot);
	if (!CanvasSlot || !MarkerSlot) return;

	const FVector2D MarkerCenter = MarkerSlot->GetPosition() + MarkerSlot->GetSize() * MarkerSlot->GetAlignment();
	const FVector2D ViewportCenter = CachedViewportSize * 0.5f;

	StartCanvasPos = CanvasSlot->GetPosition();
	TargetCanvasPos = ViewportCenter - MarkerCenter;

	MoonFocusTime = 0.f;
	bIsAnimatingToMoon = true;
}


void USectorMap::BuildSectorView(const FS_PlanetMap& ActivePlanet)
{
	PlanetMarkers.Empty();
	MoonMarkers.Empty();
	MoonOrbitMarkers.Empty();

	AddCentralPlanet(ActivePlanet);

	// Convert to screen scale
	ORBIT_TO_SCREEN = GetDynamicOrbitScale(ActivePlanet.Moon, 480.f);

	// Build moon markers and orbit rings
	for (const FS_MoonMap& Moon : ActivePlanet.Moon)
	{
		AddMoon(Moon);
	}

	HighlightSelectedSystem();
	SetMarkerVisibility(true);
}


void USectorMap::InitSectorCanvas()
{
	SystemMapUtils::ClearAllSectorUIElements(this);
	SystemMapUtils::DestroyAllSectorActors(GetWorld());

	// DO notxdo this here — it destroys SystemMap’s one-shot RT too.
	// UGalaxyManager::Get(this)->ClearAllRenderTargets();

	// This was backwards. If OuterCanvas exists, you were returning early.
	if (!MapCanvas || !OuterCanvas)
	{
		return;
	}
	//UGalaxyManager::Get(this)->ClearAllRenderTargets();
	//SystemMapUtils::ClearAllSectorUIElements(this);
	//SystemMapUtils::DestroyAllSectorActors(GetWorld());

	ZoomLevel = 1.0f;
	TargetZoom = 1.0f;
	TargetTiltAmount = 0.0f;

	bIsCenteringToMoon = false;
	bIsZoomingToMoon = false;
	bIsDragging = false;

	bIsTiltingIn = true;
	TiltTime = 0.0f;

	CurrentDragOffset = FVector2D::ZeroVector;
	bPendingCanvasCenter = true;

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);

		
	if (UCanvasPanelSlot* MainSlot = Cast<UCanvasPanelSlot>(OuterCanvas->Slot))
	{
		MainSlot->SetPosition(FVector2D::ZeroVector);
	}

	// Delay layout-dependent logic like centering
	FTimerHandle LayoutTimer;
	GetWorld()->GetTimerManager().SetTimer(LayoutTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetSize(CanvasSize);
				CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
				CanvasSlot->SetZOrder(3);
				
				//SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, TargetTiltAmount);
			}
		}), 0.05f, false);
}

void USectorMap::HandlePlanetClicked(const FString& PlanetName)
{
	UE_LOG(LogTemp, Warning, TEXT("Central planet %s clicked — requesting system map view"), *PlanetName);

	if (OwningOperationsScreen)

	{
		SetVisibility(ESlateVisibility::Collapsed);
		SetIsEnabled(false);
		ClearSectorView();
		OwningOperationsScreen->ShowSystemMap(); // or equivalent
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No owner set for USectorMap"));
	}
}

void USectorMap::AddMoon(const FS_MoonMap& Moon)
{
	if (!MapCanvas || !MoonActorClass)
		return;

	AddMoonOrbitalRing(Moon);

	// ---------------- ORBIT CONTEXT (FIX) ----------------
	// In sector view, moons orbit the CENTRAL PLANET.
	// Use the planet as orbit authority for time.
	ASystemBodyPanelActor* OrbitAuthority = PlanetActor;

	// Deterministic seed: planet + moon
	const FString Seed = PlanetActor
		? (PlanetActor->BodyName + TEXT("_") + Moon.Name)
		: Moon.Name;
	// ----------------------------------------------------

	// --- Spawn moon actor ---
	const FVector MoonWorldPos = FVector::ZeroVector;

	AMoonPanelActor* MoonActor = AMoonPanelActor::SpawnWithMoonData(
		GetWorld(),
		MoonWorldPos,
		FRotator::ZeroRotator,
		MoonActorClass,
		Moon,
		OrbitAuthority,
		Seed
	);

	if (MoonActor)
	{
		SpawnedMoonActors.Add(MoonActor);

		// Make moon follow moving planet center
		if (PlanetActor)
		{
			MoonActor->SetParentPlanet(PlanetActor);
		}

		UE_LOG(LogTemp, Log, TEXT("Moon Created: %s"), *Moon.Name);
	}

	// --- Spawn moon marker (UI) ---
	if (MoonMarkerClass)
	{
		UMoonMarkerWidget* MoonMarker = CreateWidget<UMoonMarkerWidget>(this, MoonMarkerClass);
		if (MoonMarker)
		{
			// ---------- DETERMINISTIC ORBIT ANGLE ----------
			const uint32 H = FCrc::StrCrc32(*Seed);
			const float Phase0Deg =
				((H & 0x00FFFFFFu) / float(0x01000000u)) * 360.0f;

			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

			// Visual tuning constant
			const float BaseDegPerSec = 10.0f;

			// Outer moons slower
			const float RefOrbit = 100000.f;
			const float OrbitScale =
				FMath::Sqrt(FMath::Max(RefOrbit, 1.f) / FMath::Max(Moon.Orbit, 1.f));

			const float OrbitAngle =
				FMath::Fmod(Phase0Deg + Now * BaseDegPerSec * OrbitScale, 360.0f);
			// ---------------------------------------------

			const float VisualInclination =
				PlanetOrbitUtils::AmplifyInclination(Moon.Inclination, 2.0f);

			OrbitRadius = Moon.Orbit / ORBIT_TO_SCREEN;

			const FVector2D TiltedPos =
				PlanetOrbitUtils::Get2DOrbitPositionWithInclination(
					OrbitRadius,
					OrbitAngle,
					VisualInclination
				);

			if (UCanvasPanelSlot* MoonSlot = MapCanvas->AddChildToCanvas(MoonMarker))
			{
				MoonSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				MoonSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				MoonSlot->SetPosition(TiltedPos);
				MoonSlot->SetAutoSize(true);
				MoonSlot->SetZOrder(10);
			}

			MoonMarker->SetVisibility(ESlateVisibility::Visible);
			MoonMarker->InitFromMoonActor(Moon, MoonActor);

			MoonMarkers.Add(Moon.Name, MoonMarker);
		}
	}
}

void USectorMap::SetMarkerVisibility(bool bVisible)
{
	const ESlateVisibility Vis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	for (auto& Elem : PlanetMarkers)
		if (Elem.Value) Elem.Value->SetVisibility(Vis);

	for (auto& Elem : MoonMarkers)
		if (Elem.Value) Elem.Value->SetVisibility(Vis);
}

void USectorMap::FocusAndZoomToMoon(UMoonMarkerWidget* Marker)
{
	if (!Marker || !MapCanvas) return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot);
	UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot);
	if (!CanvasSlot || !MarkerSlot) return;

	const FVector2D MarkerCenter = MarkerSlot->GetPosition() + MarkerSlot->GetSize() * MarkerSlot->GetAlignment();
	const FVector2D ViewportCenter = CachedViewportSize * 0.5f;

	// Set zoom level instantly or interpolate target
	TargetZoom = FMath::Clamp(1.5f, MinZoom, MaxZoom); // Change 1.5f to desired zoom
	ZoomAnchorLocal = ViewportCenter;
	PreZoomCanvasPos = CanvasSlot->GetPosition();

	// Begin centering animation
	StartCanvasPos = CanvasSlot->GetPosition();
	TargetCanvasPos = ViewportCenter - MarkerCenter;
	MoonFocusTime = 0.f;
	bIsAnimatingToMoon = true;
}


FReply USectorMap::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const float ScrollDelta = InMouseEvent.GetWheelDelta();

	// Store previous zoom
	const float PreviousZoom = ZoomLevel;

	// Apply delta
	ZoomLevel = FMath::Clamp(ZoomLevel + ScrollDelta * ZoomStep, MinZoom, MaxZoom);

	if (ZoomLevel == PreviousZoom)
		return FReply::Handled(); // no change

	// Reapply transform
	SystemMapUtils::ApplyZoomToCanvas(MapCanvas, ZoomLevel);

	// Optional: recenter on last selected planet
	if (LastSelectedMoonMarker)
	{
		CenterOnMoonWidget(LastSelectedMoonMarker, 1.5f); // will animate based on your setup
	}

	return FReply::Handled();
}

FReply USectorMap::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Find planet under cursor (optional — you could just zoom to LastSelectedMarker)
		if (LastSelectedMoonMarker && MapCanvas)
		{
			if (LastSelectedMoonMarker)
			{
				CenterOnMoonWidget(LastSelectedMoonMarker, 1.5f); // will animate based on your setup
				SystemMapUtils::ApplyZoomToCanvas(MapCanvas, 2.0f);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply USectorMap::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;

		// Store the mouse position in screen space at the moment of click
		DragStartPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			DragController.BeginDrag(InGeometry, InMouseEvent, CanvasSlot->GetPosition());
		}

		// Optional: Cancel any ongoing centering/zooming
		bIsCenteringToMoon = false;
		bIsZoomingToMoon = false;
		LastSelectedMoonMarker = nullptr;

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}


FReply USectorMap::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DragController.bIsDragging && MapCanvas)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			FVector2D Delta = DragController.ComputeDragDelta(InGeometry, InMouseEvent);
			FVector2D Proposed = DragController.InitialCanvasOffset + Delta;

			FVector2D Clamped = SystemMapUtils::ClampCanvasToSafeMargin(
				Proposed,
				CachedCanvasSize,
				CachedViewportSize,
				100.f // margin in pixels
			);

			//FVector2D Final = SystemMapUtils::ClampCanvasDragOffset(Clamped, CachedCanvasSize, CachedViewportSize, 50.f);
			CanvasSlot->SetPosition(Clamped);
			//CanvasSlot->SetPosition(Proposed);
		}
	}

	return FReply::Handled();
}

FReply USectorMap::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		DragController.EndDrag();

		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void USectorMap::SetZoomLevel(float NewZoom)
{
	ZoomLevel = FMath::Clamp(NewZoom, 0.25f, 4.0f);

	if (MapCanvas)
	{
		SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, TargetTiltAmount);
	}
}

void USectorMap::HighlightSelectedSystem()
{
	// Highlight current system
	if (const USSWGameInstance* GI = GetWorld()->GetGameInstance<USSWGameInstance>())
	{
		const FString& CurrentSector = GI->SelectedSector.Name;

		if (UMoonMarkerWidget** Found = MoonMarkers.Find(CurrentSector))
		{
			UMoonMarkerWidget* Marker = *Found;
			Marker->SetSelected(true);

			FTimerHandle Delay;
			GetWorld()->GetTimerManager().SetTimer(Delay, FTimerDelegate::CreateWeakLambda(this, [this, Marker]()
				{
					if (!Marker) return;

					FVector2D Center;
					if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Marker->Slot))
					{
						Center = Slot->GetPosition() + Slot->GetSize() * Slot->GetAlignment();
					}

					// Optionally center camera or draw selection lines here
					UE_LOG(LogTemp, Log, TEXT("Selected planet: %s at %s"), *Marker->MoonData.Name, *Center.ToString());

				}), 0.01f, false);
		}
	}
}


void USectorMap::AddCentralPlanet(const FS_PlanetMap& Planet)
{
	// You cannotxadd UI to a null canvas
	if (!MapCanvas)
	{
		UE_LOG(LogTemp, Error, TEXT("USectorMap::AddCentralPlanet() MapCanvas is null"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("USectorMap::AddCentralPlanet() World is null"));
		return;
	}

	// -------------------- CENTRAL PLANET ACTOR --------------------
	if (PlanetActorClass)
	{
		// Destroy previous actor cleanly
		if (PlanetActor)
		{
			PlanetActor->Destroy();
			PlanetActor = nullptr;
		}

		const FVector ActorLocation = FVector::ZeroVector;
		const FRotator ActorRotation = FRotator::ZeroRotator;

		// FIX: call the new signature (OrbitAuthority + SystemSeed)
		PlanetActor = APlanetPanelActor::SpawnWithPlanetData(
			World,
			ActorLocation,
			ActorRotation,
			PlanetActorClass,
			Planet,
			/*OrbitAuthority*/ nullptr,   // sector view: central planet does notxorbit
			/*SystemSeed*/    Planet.Name // stable seed for deterministic UI/orbit math if needed
		);

		if (PlanetActor)
		{
			// In sector view, the central planet should notxorbit.
			PlanetActor->bEnableOrbit = false;
			PlanetActor->OrbitEpochSeconds = World->GetTimeSeconds();
			PlanetActor->OrbitTimeScale = 1.0f;
			PlanetActor->SystemSeed = Planet.Name;

			SpawnedPlanetActors.Add(PlanetActor);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("USectorMap::AddCentralPlanet() Failed to spawn planet actor for %s"), *Planet.Name);
		}
	}

	// -------------------- CENTRAL PLANET MARKER --------------------
	if (!PlanetMarkerClass)
	{
		return;
	}

	// Remove existing marker
	if (PlanetMarker)
	{
		PlanetMarker->RemoveFromParent();
		PlanetMarker = nullptr;
	}

	PlanetMarker = CreateWidget<UPlanetMarkerWidget>(this, PlanetMarkerClass);
	if (!PlanetMarker)
	{
		UE_LOG(LogTemp, Error, TEXT("USectorMap::AddCentralPlanet() Failed to create PlanetMarker widget for %s"), *Planet.Name);
		return;
	}

	UCanvasPanelSlot* PlanetSlot = MapCanvas->AddChildToCanvas(PlanetMarker);
	if (!PlanetSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("USectorMap::AddCentralPlanet() Failed to add PlanetMarker to MapCanvas"));
		PlanetMarker->RemoveFromParent();
		PlanetMarker = nullptr;
		return;
	}

	PlanetSlot->SetAutoSize(true);
	PlanetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PlanetSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PlanetSlot->SetPosition(FVector2D(0.f, 0.f));
	PlanetSlot->SetZOrder(12);
	PlanetSlot->SetSize(FVector2D(64.f, 64.f));

	UE_LOG(LogTemp, Warning, TEXT("Adding Central Planet %s"), *Planet.Name);

	PlanetMarker->InitFromPlanetActor(Planet, PlanetActor);
	PlanetMarker->OnObjectClicked.AddDynamic(this, &USectorMap::HandlePlanetClicked);

	PlanetMarkers.Remove(Planet.Name);
	PlanetMarkers.Add(Planet.Name, PlanetMarker);
}

float USectorMap::GetDynamicOrbitScale(const TArray<FS_MoonMap>& Moons, float MaxPixelRadius) const
{
	float MaxOrbit = 0.f;
	for (const FS_MoonMap& Moon : Moons)
	{
		MaxOrbit = FMath::Max(MaxOrbit, Moon.Orbit);
	}

	if (MaxOrbit <= 0.f)
	{
		return 1.0f; // fallback to avoid divide-by-zero
	}

	return MaxOrbit / MaxPixelRadius; // e.g. 1e9 km mapped to 500 px = 2e6 scale
}

void USectorMap::ClearSectorView()
{
	if (MapCanvas)
	{
		MapCanvas->ClearChildren();
	}

	PlanetMarkers.Empty();
	MoonMarkers.Empty();
	MoonOrbitMarkers.Empty();
	LastSelectedMarker = nullptr;

	SystemMapUtils::DestroyAllSectorActors(GetWorld());

	if (UGalaxyManager* GM = UGalaxyManager::Get(this))
	{
		GM->ClearAllRenderTargets();
	}

	UE_LOG(LogTemp, Warning, TEXT("[SectorMap] Cleared all markers, actors, and render targets"));
}

