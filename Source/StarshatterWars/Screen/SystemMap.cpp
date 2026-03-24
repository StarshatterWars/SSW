// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         SystemMap.cpp	AUTHOR:       Carlos Bott*/

#include "SystemMap.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "EngineUtils.h"

#include "PlanetMarkerWidget.h"
#include "MoonMarkerWidget.h"
#include "SystemOrbitWidget.h"
#include "CentralSunWidget.h"
#include "OperationsScreen.h"

#include "GalaxyManager.h"
#include "SSWGameInstance.h"

#include "CentralSunActor.h"
#include "PlanetPanelActor.h"

#include "PlanetOrbitUtils.h"
#include "SystemMapUtils.h"

// NEW RENDERER
#include "SystemOverview.h"   // adjust include path to where ASystemOverview lives

void USystemMap::NativeConstruct()
{
	Super::NativeConstruct();
	InitMapCanvas();
}

void USystemMap::NativeDestruct()
{
	ClearSystemView();

	// Destroy overview actor (SystemMap-owned)
	if (SystemOverviewActor)
	{
		SystemOverviewActor->Destroy();
		SystemOverviewActor = nullptr;
	}

	// Release RT (GC will handle, but null it for safety)
	SystemOverviewRT = nullptr;

	Super::NativeDestruct();
}

float USystemMap::GetDynamicOrbitScale(const TArray<FS_PlanetMap>& Planets, float MaxPixelRadius) const
{
	float MaxOrbit = 0.f;
	for (const FS_PlanetMap& Planet : Planets)
	{
		MaxOrbit = FMath::Max(MaxOrbit, Planet.Orbit);
	}

	if (MaxOrbit <= 0.f)
	{
		return 1.0f;
	}

	return MaxOrbit / MaxPixelRadius;
}

void USystemMap::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Always keep viewport cache current (this was a common cause of “drift” when it was zero)
	CachedViewportSize = MyGeometry.GetLocalSize();

	if (!MapCanvas)
		return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot);
	if (!CanvasSlot)
		return;

	// Movement delay
	if (bIsWaitingToProcessMovement)
	{
		MovementDelayTime += InDeltaTime;

		if (MovementDelayTime >= MovementDelayDuration)
		{
			bIsWaitingToProcessMovement = false;
			MovementDelayTime = 0.0f;

			if (LastSelectedMarker)
			{
				CenterOnPlanetWidget(LastSelectedMarker);
				LastSelectedMarker = nullptr;
			}
		}
	}

	// Focus animation
	if (bIsAnimatingToPlanet)
	{
		PlanetFocusTime += InDeltaTime;
		const float Alpha = FMath::Clamp(PlanetFocusTime / PlanetFocusDuration, 0.f, 1.f);
		const float Smoothed = SystemMapUtils::EaseInOut(Alpha);

		const FVector2D InterpPos = FMath::Lerp(StartCanvasPos, TargetCanvasPos, Smoothed);
		CanvasSlot->SetPosition(InterpPos);

		if (Alpha >= 1.f)
		{
			bIsAnimatingToPlanet = false;
		}
	}

	// Tilt-in phase (optional)
	if (bIsTiltingIn)
	{
		TiltTime += InDeltaTime;
		const float Progress = FMath::Clamp(TiltTime / TiltDuration, 0.0f, 1.0f);
		const float SmoothT = SystemMapUtils::EaseInOut(Progress);

		const float CurrentTilt = FMath::Lerp(0.0f, TargetTiltAmount, SmoothT);
		SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, CurrentTilt);

		if (Progress >= 1.0f)
		{
			bIsTiltingIn = false;
			SystemMapUtils::ApplyZoomAndTilt(MapCanvas, ZoomLevel, TargetTiltAmount);
		}
	}
}

void USystemMap::InitMapCanvas()
{
	ZoomLevel = 1.0f;
	TargetTiltAmount = 0.0f;

	bIsDragging = false;
	bIsAnimatingToPlanet = false;

	bIsTiltingIn = true;
	TiltTime = 0.0f;

	bPendingCanvasCenter = true;

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);

	// Clear old UI + actors
	if (UGalaxyManager* GM = UGalaxyManager::Get(this))
	{
		GM->ClearAllRenderTargets();
	}
	SystemMapUtils::ClearAllSystemUIElements(this);
	SystemMapUtils::DestroyAllSystemActors(GetWorld());

	if (!OuterCanvas || !MapCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("USystemMap::InitMapCanvas(): OuterCanvas/MapCanvas notxbound"));
		return;
	}

	// Layout init (centered canvas)
	FTimerHandle LayoutTimer;
	GetWorld()->GetTimerManager().SetTimer(
		LayoutTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
					CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					CanvasSlot->SetSize(CanvasSize);
					CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
				}
			}),
		0.05f,
		false
	);

	// Ensure new renderer resources exist up front (safe even if no system yet)
	EnsureOverviewResources();
	UpdateOverviewBrush();

	// Build initial system if one is selected
	if (USSWGameInstance* GI = GetGameInstance<USSWGameInstance>())
	{
		const FString& SelectedSystem = GI->SelectedSystem;
		const FS_Galaxy* System = UGalaxyManager::Get(this)->FindSystemByName(SelectedSystem);

		if (System)
		{
			UE_LOG(LogTemp, Log, TEXT("USystemMap::InitMapCanvas() Found system: %s"), *SelectedSystem);
			BuildSystemView(System);
			//CreateSystemView(System);
		}
	}
}

void USystemMap::BuildSystemView(const FS_Galaxy* ActiveSystem)
{
	if (!ActiveSystem || ActiveSystem->Stellar.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("USystemMap::BuildSystemView(): ActiveSystem missing Stellar[0]"));
		return;
	}

	if (!MapCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("USystemMap::BuildSystemView(): Missing MapCanvas"));
		return;
	}

	bPendingCanvasCenter = true;

	MapCanvas->ClearChildren();
	PlanetMarkers.Empty();
	PlanetOrbitMarkers.Empty();

	AddCentralStar(ActiveSystem);

	ORBIT_TO_SCREEN = GetDynamicOrbitScale(ActiveSystem->Stellar[0].Planet, 480.f);

	for (const FS_PlanetMap& Planet : ActiveSystem->Stellar[0].Planet)
	{
		AddPlanet(Planet);
	}

	HighlightSelectedSystem();
	SetMarkerVisibility(true);
}

void USystemMap::EnsureOverviewResources()
{
	// Create RT if needed (SystemMap owns it)
	if (!SystemOverviewRT)
	{
		SystemOverviewRT = NewObject<UTextureRenderTarget2D>(this, TEXT("RT_SystemOverview"), RF_Transient);
		SystemOverviewRT->RenderTargetFormat = RTF_RGBA8;
		SystemOverviewRT->ClearColor = FLinearColor::Black;
		SystemOverviewRT->bAutoGenerateMips = false;
		SystemOverviewRT->InitAutoFormat(OverviewRTSize, OverviewRTSize);
		SystemOverviewRT->UpdateResourceImmediate(true);
	}

	// Ensure actor exists
	if (!SystemOverviewActor)
	{
		UWorld* World = GetWorld();
		if (!World)
			return;

		TSubclassOf<ASystemOverview> UseClass = SystemOverviewActorClass;
		if (!UseClass)
		{
			UseClass = ASystemOverview::StaticClass();
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;

		SystemOverviewActor = World->SpawnActor<ASystemOverview>(UseClass, FVector(500000.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
		if (SystemOverviewActor)
		{
			SystemOverviewActor->SetActorHiddenInGame(true);
			SystemOverviewActor->SetRenderTarget(SystemOverviewRT);
		}
	}

	// If OverviewImage isn't bound in BP, we can't display it (but capture still works)
}

void USystemMap::HandleCentralSunClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Central sun clicked — requesting galaxy map view"));

	if (OwningOperationsScreen)
	{
		OwningOperationsScreen->ShowGalaxyMap();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No owner set for USystemMap"));
	}
}

void USystemMap::HandlePlanetSelected(FS_PlanetMap Planet)
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	UE_LOG(LogTemp, Warning, TEXT("Planet %s clicked — requesting sector map view"), *Planet.Name);

	if (OwningOperationsScreen)
	{
		SSWInstance->SelectedSector = Planet;
		SSWInstance->SelectedSectorName = Planet.Name;
		SetVisibility(ESlateVisibility::Collapsed);
		SetIsEnabled(false);

		ClearSystemView();
		OwningOperationsScreen->ShowSectorMap(Planet);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No owner set for USystemMap"));
	}
}

FReply USystemMap::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const float ScrollDelta = InMouseEvent.GetWheelDelta();
	const float PrevZoom = ZoomLevel;

	ZoomLevel = FMath::Clamp(ZoomLevel + ScrollDelta * ZoomStep, MinZoom, MaxZoom);

	if (FMath::IsNearlyEqual(ZoomLevel, PrevZoom))
		return FReply::Handled();

	SystemMapUtils::ApplyZoomToCanvas(MapCanvas, ZoomLevel);

	if (LastSelectedMarker)
	{
		CenterOnPlanetWidget(LastSelectedMarker);
	}

	return FReply::Handled();
}

FReply USystemMap::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply USystemMap::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		DragStartPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			DragController.BeginDrag(InGeometry, InMouseEvent, CanvasSlot->GetPosition());
		}

		// Cancel any pending post-drag recenters
		bIsWaitingToProcessMovement = false;
		MovementDelayTime = 0.f;

		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USystemMap::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DragController.bIsDragging && MapCanvas)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot))
		{
			const FVector2D Delta = DragController.ComputeDragDelta(InGeometry, InMouseEvent);
			const FVector2D Proposed = DragController.InitialCanvasOffset + Delta;

			// If you want clamping, apply it here (you had Clamped but weren’t using it)
			// const FVector2D Clamped = SystemMapUtils::ClampCanvasToSafeMargin(Proposed, CachedCanvasSize, CachedViewportSize, 100.f);
			CanvasSlot->SetPosition(Proposed);
		}
	}

	return FReply::Handled();
}

FReply USystemMap::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bIsDragging = false;
		DragController.EndDrag();

		// Optional: start delay before recentering to the last selection
		// bIsWaitingToProcessMovement = true;
		// MovementDelayTime = 0.f;

		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void USystemMap::HandlePlanetClicked(const FString& PlanetName)
{
	if (UPlanetMarkerWidget** Found = PlanetMarkers.Find(PlanetName))
	{
		// Second click loads sector map
		if (LastSelectedMarker == *Found)
		{
			UE_LOG(LogTemp, Log, TEXT("Planet already selected: %s, loading sector view"), *PlanetName);
			HandlePlanetSelected(LastSelectedMarker->PlanetData);
			return;
		}

		LastSelectedMarker = *Found;
		UE_LOG(LogTemp, Log, TEXT("Planet selected: %s (centering)"), *PlanetName);

		CenterOnPlanetWidget(*Found);
	}
}

/*
 * CENTERING FIX (IMPORTANT):
 * - Uses OuterCanvas cached geometry to compute delta in the SAME SPACE that the MapCanvas slot uses.
 * - Avoids brittle CanvasPanelSlot Position/Alignment/Size math that breaks once zoom/tilt/render transforms exist.
 * - This addresses the “runs down and to the right after clicking” symptom.
 */
void USystemMap::CenterOnPlanetWidget(UPlanetMarkerWidget* Marker)
{
	if (!Marker || !MapCanvas || !OuterCanvas)
		return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapCanvas->Slot);
	if (!CanvasSlot)
		return;

	const FGeometry OuterGeo = OuterCanvas->GetCachedGeometry();
	const FGeometry MarkerGeo = Marker->GetCachedGeometry();

	// Marker center in absolute space, then convert to OuterCanvas local
	const FVector2D MarkerAbsCenter = MarkerGeo.GetAbsolutePosition() + (MarkerGeo.GetLocalSize() * 0.5f);
	const FVector2D MarkerLocalInOuter = OuterGeo.AbsoluteToLocal(MarkerAbsCenter);

	// Viewport center in OuterCanvas local space
	const FVector2D ViewCenterLocal = OuterGeo.GetLocalSize() * 0.5f;

	// Delta needed to move marker to the center
	const FVector2D DeltaLocal = ViewCenterLocal - MarkerLocalInOuter;

	StartCanvasPos = CanvasSlot->GetPosition();
	TargetCanvasPos = StartCanvasPos + DeltaLocal;

	PlanetFocusTime = 0.f;
	bIsAnimatingToPlanet = true;
}

void USystemMap::AddCentralStar(const FS_Galaxy* Star)
{
	if (!Star || Star->Stellar.Num() == 0)
		return;

	if (SunActorClass)
	{
		if (SunActor)
		{
			SunActor->Destroy();
			SunActor = nullptr;
		}

		if (StarWidget)
		{
			StarWidget->RemoveFromParent();
			StarWidget = nullptr;
		}

		const FVector Location = FVector(-500, 0, 200);
		const FRotator Rotation = FRotator::ZeroRotator;

		SunActor = ACentralSunActor::SpawnWithSpectralClass(
			GetWorld(),
			Location,
			Rotation,
			SunActorClass,
			Star->Stellar[0].Class,
			Star->Stellar[0].Radius,
			Star->Stellar[0].Name
		);
	}

	if (StarWidgetClass)
	{
		StarWidget = CreateWidget<UCentralSunWidget>(this, StarWidgetClass);
		StarWidget->OnSunClicked.AddDynamic(this, &USystemMap::HandleCentralSunClicked);

		if (StarWidget && SunActor)
		{
			StarWidget->InitializeFromSunActor(SunActor);
			if (UCanvasPanelSlot* StarSlot = MapCanvas->AddChildToCanvas(StarWidget))
			{
				StarSlot->SetAutoSize(true);
				StarSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				StarSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				StarSlot->SetPosition(FVector2D(0.f, 0.f));
				StarSlot->SetZOrder(15);
				StarSlot->SetSize(FVector2D(64.f, 64.f));
				CentralStarMarker = StarWidget;
			}
		}
	}
}

void USystemMap::AddPlanetOrbitalRing(const FS_PlanetMap& Planet)
{
	float Perihelion = 0.f;
	float Aphelion = 0.f;

	PlanetOrbitUtils::CalculateOrbitExtremes(Planet.Orbit, Planet.Eccentricity, Perihelion, Aphelion);

	float a = (Perihelion + Aphelion) * 0.5f;
	float b = a * FMath::Sqrt(1.0f - FMath::Square(Planet.Eccentricity));
	a /= ORBIT_TO_SCREEN;
	b /= ORBIT_TO_SCREEN;

	OrbitRadius = Planet.Orbit / ORBIT_TO_SCREEN;

	if (OrbitWidgetClass)
	{
		USystemOrbitWidget* PlanetOrbitRing = CreateWidget<USystemOrbitWidget>(this, OrbitWidgetClass);

		PlanetOrbitRing->SetOrbitRadius(OrbitRadius);
		const float InclinationVisual = PlanetOrbitUtils::AmplifyInclination(Planet.Inclination);
		PlanetOrbitRing->SetOrbitInclination(InclinationVisual);

		if (UCanvasPanelSlot* OrbitSlot = MapCanvas->AddChildToCanvas(PlanetOrbitRing))
		{
			OrbitSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			OrbitSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			OrbitSlot->SetPosition(FVector2D(0.f, 0.f));
			OrbitSlot->SetZOrder(0);
			OrbitSlot->SetAutoSize(true);

			PlanetOrbitMarkers.Add(Planet.Name, PlanetOrbitRing);
		}

		PlanetOrbitRing->SetVisibility(ESlateVisibility::Visible);
	}
}

void USystemMap::AddPlanet(const FS_PlanetMap& Planet)
{
	AddPlanetOrbitalRing(Planet);

	APlanetPanelActor* NewPlanetActor = nullptr;

	if (PlanetActorClass)
	{
		const FVector ActorLocation = FVector(-1000, 0, 200);
		const FRotator ActorRotation = FRotator::ZeroRotator;

		// IMPORTANT:
		// Replace SystemStarActor with YOUR actual pointer to the star/authority actor.
		// It must be a class derived from ASystemBodyPanelActor.
		APlanetPanelActor* OrbitAuthority = NewPlanetActor;     // <-- rename to your real variable
		const FString SystemSeed = OrbitAuthority ? OrbitAuthority->SystemSeed : FString(TEXT("SYS"));

		NewPlanetActor = APlanetPanelActor::SpawnWithPlanetData(
			GetWorld(),
			ActorLocation,
			ActorRotation,
			PlanetActorClass,
			Planet,
			OrbitAuthority,
			SystemSeed
		);

		if (NewPlanetActor)
		{
			SpawnedPlanetActors.Add(NewPlanetActor);
		}
	}

	if (PlanetMarkerClass && MapCanvas)
	{
		UPlanetMarkerWidget* PlanetMarker = CreateWidget<UPlanetMarkerWidget>(this, PlanetMarkerClass);
		if (PlanetMarker)
		{
			// ---------------- DETERMINISTIC ORBIT ANGLE (NO RANDOM) ----------------
			// Stable starting phase from seed:
			const FString PlanetSeed = (TEXT("PLANET_") + Planet.Name);
			const uint32 H = FCrc::StrCrc32(*PlanetSeed);
			const float Phase0Deg = ((H & 0x00FFFFFFu) / float(0x01000000u)) * 360.0f;

			// Advance with time; outer planets slower:
			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

			// Tune this constant for “system map” speed
			const float BaseDegPerSec = 6.0f;

			// Planet.Orbit is your orbital radius (same field you already use below)
			const float RefOrbit = 100000.f; // choose a reference in same units as Planet.Orbit
			const float OrbitScale = FMath::Sqrt(FMath::Max(RefOrbit, 1.f) / FMath::Max(Planet.Orbit, 1.f));

			const float OrbitAngle = FMath::Fmod(Phase0Deg + Now * BaseDegPerSec * OrbitScale, 360.0f);
			// ----------------------------------------------------------------------

			const float VisualInclination = PlanetOrbitUtils::AmplifyInclination(Planet.Inclination, 2.0f);
			OrbitRadius = Planet.Orbit / ORBIT_TO_SCREEN;

			const FVector2D TiltedPos = PlanetOrbitUtils::Get2DOrbitPositionWithInclination(
				OrbitRadius,
				OrbitAngle,
				VisualInclination
			);

			if (UCanvasPanelSlot* PlanetSlot = MapCanvas->AddChildToCanvas(PlanetMarker))
			{
				PlanetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				PlanetSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				PlanetSlot->SetPosition(TiltedPos);
				PlanetSlot->SetAutoSize(true);
				PlanetSlot->SetZOrder(10);
			}

			PlanetMarker->InitFromPlanetActor(Planet, NewPlanetActor);
			PlanetMarker->OnObjectClicked.AddDynamic(this, &USystemMap::HandlePlanetClicked);

			PlanetMarkers.Add(Planet.Name, PlanetMarker);
		}
	}
}

void USystemMap::HighlightSelectedSystem()
{
	if (const USSWGameInstance* GI = GetWorld()->GetGameInstance<USSWGameInstance>())
	{
		const FString& CurrentSystem = GI->SelectedSystem;

		if (UPlanetMarkerWidget** Found = PlanetMarkers.Find(CurrentSystem))
		{
			UPlanetMarkerWidget* Marker = *Found;
			Marker->SetSelected(true);
		}
	}
}

void USystemMap::SetOverlay()
{
	// Optional: keep your overlay code here if you still want it.
}

void USystemMap::SetMarkerVisibility(bool bVisible)
{
	const ESlateVisibility Vis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	if (CentralStarMarker)
		CentralStarMarker->SetVisibility(Vis);

	for (auto& Elem : PlanetMarkers)
	{
		if (Elem.Value)
			Elem.Value->SetVisibility(Vis);
	}
}

void USystemMap::ClearSystemView()
{
	// 1) UI
	if (MapCanvas)
	{
		MapCanvas->ClearChildren();
	}

	PlanetMarkers.Empty();
	PlanetOrbitMarkers.Empty();
	LastSelectedMarker = nullptr;

	// 2) Destroy per-planet preview actors we spawned (best: destroy only what WE spawned)
	for (APlanetPanelActor* Actor : SpawnedPlanetActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedPlanetActors.Empty();

	// If you also spawned a SunActor inside SystemMap:
	if (IsValid(SunActor))
	{
		SunActor->Destroy();
		SunActor = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SystemMap] Cleared markers, actors, and map-owned render targets"));
}

void USystemMap::EnsureSystemOverviewRT()
{
	if (SystemOverviewRT)
		return;

	SystemOverviewRT = NewObject<UTextureRenderTarget2D>(this);
	SystemOverviewRT->RenderTargetFormat = RTF_RGBA16f;
	SystemOverviewRT->ClearColor = FLinearColor::Black;
	SystemOverviewRT->bAutoGenerateMips = false;
	SystemOverviewRT->InitAutoFormat(2048, 2048);
	SystemOverviewRT->UpdateResourceImmediate(true);
}

void USystemMap::EnsureSystemOverviewActor()
{
	if (SystemOverviewActor && IsValid(SystemOverviewActor))
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	SystemOverviewActor =
		World->SpawnActor<ASystemOverview>(ASystemOverview::StaticClass());

	check(SystemOverviewActor);

	// Create RT ONCE
	SystemOverviewRT = NewObject<UTextureRenderTarget2D>(
		this,
		TEXT("SystemOverviewRT"),
		RF_Transient
	);

	SystemOverviewRT->RenderTargetFormat = RTF_RGBA16f;
	SystemOverviewRT->InitAutoFormat(2048, 2048);
	SystemOverviewRT->ClearColor = FLinearColor::Black;
	SystemOverviewRT->UpdateResourceImmediate(true);

	SystemOverviewActor->SetRenderTarget(SystemOverviewRT);
}

void USystemMap::BuildSystemOverviewData(
	const FS_Galaxy* ActiveSystem,
	TArray<FOverviewBody>& OutBodies
)
{
	OutBodies.Reset();

	// Star
	const auto& Star = ActiveSystem->Stellar[0];
	OutBodies.Add({
		Star.Name,
		0.f,
		(float) Star.Radius,
		0.f,
		0.f,
		INDEX_NONE
		});

	// Planets
	for (int32 i = 0; i < Star.Planet.Num(); ++i)
	{
		const FS_PlanetMap& Planet = Star.Planet[i];

		OutBodies.Add({
			Planet.Name,
			(float) Planet.Orbit,
			(float) Planet.Radius,
			FMath::FRandRange(0.f, 360.f),
			(float) Planet.Inclination,
			0 // parent = star
			});
	}
}
void USystemMap::EnsureOverviewImage()
{
	if (!MapCanvas || OverviewImage)
		return;

	OverviewImage = NewObject<UImage>(this);
	MapCanvas->AddChild(OverviewImage);

	if (UCanvasPanelSlot* OverviewSlot =
		Cast<UCanvasPanelSlot>(OverviewImage->Slot))
	{
		OverviewSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		OverviewSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		OverviewSlot->SetSize(CanvasSize);
		OverviewSlot->SetZOrder(-10); // behind markers
	}

	OverviewImage->SetVisibility(ESlateVisibility::Hidden);
}

void USystemMap::UpdateOverviewBrush()
{
	if (!OverviewImage || !SystemOverviewRT || !OverlayMaterial)
		return;

	UMaterialInstanceDynamic* MID =
		UMaterialInstanceDynamic::Create(OverlayMaterial, this);

	MID->SetTextureParameterValue(
		TEXT("InputTexture"),
		SystemOverviewRT
	);

	OverviewImage->SetBrushFromMaterial(MID);
}

void USystemMap::CreateSystemView(const FS_Galaxy* ActiveSystem)
{
	//ClearSystemView();

	// STEP 1: Build overview bodies
	TArray<FOverviewBody> Bodies = BuildOverviewBodies(ActiveSystem);

	UE_LOG(LogTemp, Log, TEXT("Overview bodies: %d"), Bodies.Num());

	// STEP 2: Spawn overview actor (ONCE)
	EnsureSystemOverviewActor();

	UE_LOG(LogTemp, Log, TEXT("SystemOverviewActor: %s"), *GetNameSafe(SystemOverviewActor));

	// STEP 3: Send data to new renderer
	SystemOverviewActor->BuildDiorama(Bodies);

	// STEP 4: Capture
	SystemOverviewActor->CaptureOnce();

	// STEP 5: Apply RT to OverviewImage
	UpdateOverviewBrush();

	// STEP 6: Build UI markers ONLY (no planet actors)
	BuildPlanetMarkersOnly();
}

TArray<FOverviewBody> USystemMap::BuildOverviewBodies(const FS_Galaxy* ActiveSystem)
{
	TArray<FOverviewBody> Bodies;

	if (!ActiveSystem || ActiveSystem->Stellar.Num() == 0)
		return Bodies;

	const auto& Star = ActiveSystem->Stellar[0];

	// ---- STAR (index 0) ----
	{
		FOverviewBody StarBody;
		StarBody.Name = Star.Name;
		StarBody.RadiusKm = Star.Radius;
		StarBody.OrbitKm = 0.f;
		StarBody.OrbitAngleDeg = 0.f;
		StarBody.InclinationDeg = 0.f;
		StarBody.ParentIndex = INDEX_NONE;
		Bodies.Add(StarBody);
	}

	// ---- PLANETS ----
	for (const FS_PlanetMap& Planet : Star.Planet)
	{
		FOverviewBody PlanetBody;
		PlanetBody.Name = Planet.Name;
		PlanetBody.OrbitKm = Planet.Orbit;
		PlanetBody.RadiusKm = Planet.Radius;
		PlanetBody.OrbitAngleDeg = PlanetOrbitAngles.FindOrAdd(Planet.Name);
		PlanetBody.InclinationDeg = Planet.Inclination;
		PlanetBody.ParentIndex = 0; // star

		Bodies.Add(PlanetBody);
	}

	return Bodies;
}

void USystemMap::BuildPlanetMarkersOnly()
{
	PlanetMarkers.Empty();
	PlanetOrbitMarkers.Empty();

	if (!SystemData.Stellar.Num())
		return;

	const auto& Planets = SystemData.Stellar[0].Planet;

	ORBIT_TO_SCREEN = GetDynamicOrbitScale(Planets, 480.f);

	for (const FS_PlanetMap& Planet : Planets)
	{
		AddPlanetOrbitalRing(Planet);

		if (!PlanetMarkerClass)
			continue;

		UPlanetMarkerWidget* Marker =
			CreateWidget<UPlanetMarkerWidget>(this, PlanetMarkerClass);

		float& Angle = PlanetOrbitAngles.FindOrAdd(Planet.Name);
		if (FMath::IsNearlyZero(Angle))
			Angle = FMath::FRandRange(0.f, 360.f);

		const float Radius = Planet.Orbit / ORBIT_TO_SCREEN;

		const FVector2D Pos =
			PlanetOrbitUtils::Get2DOrbitPositionWithInclination(
				Radius,
				Angle,
				PlanetOrbitUtils::AmplifyInclination(Planet.Inclination)
			);

		if (UCanvasPanelSlot* PlanetSlot = MapCanvas->AddChildToCanvas(Marker))
		{
			PlanetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PlanetSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PlanetSlot->SetPosition(Pos);
			PlanetSlot->SetAutoSize(true);
			PlanetSlot->SetZOrder(10);
		}

		//Marker->InitFromPlanetDataOnly(Planet);
		Marker->OnObjectClicked.AddDynamic(
			this, &USystemMap::HandlePlanetClicked
		);

		PlanetMarkers.Add(Planet.Name, Marker);
	}
}