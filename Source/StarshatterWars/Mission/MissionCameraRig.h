#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "ShipActor.h"
#include "FighterActor.h"
#include "Components/ChildActorComponent.h"
#include "Ship.h"
#include "FlightDeck.h"
#include "SimRegion.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionTargetOverlay.h"
#include "MissionViewCamera.h"
#include "Components/LineBatchComponent.h"
#include "Components/PointLightComponent.h"
#include "SceneTypes.h"
#include "OrbitalRegion.h"
#include "PlayerCharacter.h"

// Presentation adapter for Stars45/CameraDirector modes. Simulation positions
// cannot be used as UE world positions: region-shell placement moves visuals.
// All close camera offsets use the displayed actor's frame (+X forward, +Z up).
struct FMissionCameraRig
{
    enum EMode { Cockpit, Virtual, Chase, Drop, Orbit, Overview, Target, Threat };
    enum { PanelCommand = 13, OverviewCommand, FirstLookCommand };
    static constexpr double LookYaw[] = {0, -60, 60, -120, 120, 0, 0};
    static constexpr double LookPitch[] = {0, 0, 0, 0, 0, 30, -22.5};
    const TCHAR* ModeLabel() const
    {
        switch (Mode)
        {
        case Cockpit: return TEXT("COCKPIT");
        case Virtual:
            if (HeadYaw < -100) return TEXT("LEFT AFT");
            if (HeadYaw < -30) return TEXT("LEFT");
            if (HeadYaw > 100) return TEXT("RIGHT AFT");
            if (HeadYaw > 30) return TEXT("RIGHT");
            if (HeadPitch > 15) return TEXT("UP");
            if (HeadPitch < -15) return TEXT("DOWN");
            return TEXT("VIRTUAL COCKPIT");
        case Chase: return TEXT("CHASE");
        case Drop: return TEXT("DROP");
        case Orbit: return TEXT("ORBIT");
        case Overview: return TEXT("OVERVIEW");
        case Target: return TEXT("PADLOCK TARGET");
        case Threat: return TEXT("PADLOCK THREAT");
        }
        return TEXT("");
    }
    EMode Mode = Chase;
    bool bThirdPersonView = false;
    TWeakObjectPtr<AShipActor> ViewObject;
    TWeakObjectPtr<AShipActor> BoundsActor;
    TWeakObjectPtr<AShipActor> PlayerActor;
    FBox LocalBounds = FBox(ForceInit);
    TArray<TWeakObjectPtr<AActor>> HiddenByCamera;
    double Azimuth = 180.0, Elevation = 15.0;
    double LookX = 0.0, LookY = 0.0, RangeInput = 0.0;
    double HeadYaw = 0.0, HeadPitch = 0.0;
    double Distance = 0.0;
    bool bWide = false, bDropCaptured = false, bWasTransition = false;
    FVector DropPosition = FVector::ZeroVector;
    // Launch presentation only: retain the authored deck while the fighter exits.
    FlightDeck* HeldLaunchDeck = nullptr;
    Ship* HeldLaunchPlayer = nullptr;
    FVector HeldLaunchEyeLocal = FVector::ZeroVector;
    FQuat HeldLaunchRotationLocal = FQuat::Identity;
    bool bHeldDeckPoseValid = false;
    // Deck release is a presentation hand-off, not an instantaneous camera cut.
    bool bLaunchExitBlendActive = false;
    double LaunchExitBlendElapsed = 0.0;
    FVector LaunchExitBlendEye = FVector::ZeroVector;
    FQuat LaunchExitBlendRotation = FQuat::Identity;

    TWeakObjectPtr<UPointLightComponent> LaunchFill;
    TWeakObjectPtr<ULineBatchComponent> TacticalGrid;
    FVector TacticalGridCenter = FVector::ZeroVector;
    double TacticalGridSpacing = 0.0;

    void UpdateTacticalGrid(ACameraActor* Camera, Ship* Player, SimRegion* Region, const FVector& Focus)
    {
        const auto* Profile = PlayerCharacter::GetCurrentPlayer();
        const auto* OrbitalRegion = Region->GetOrbitalRegion();
        if ((Mode != Orbit && Mode != Overview) || Player->IsAirborne() ||
            Player->InTransition() || (Profile && Profile->GridMode() == 0) || !OrbitalRegion)
        {
            if (auto* Lines = TacticalGrid.Get()) Lines->SetVisibility(false);
            return;
        }

        const auto* Instance = Camera->GetGameInstance();
        const auto* Runtime = Instance ? Instance->GetSubsystem<USSWRuntimeSubsystem>() : nullptr;
        if (!Runtime) return;
        const FVector Origin = Runtime->GetVisualLocationForRegion(Region, FVector::ZeroVector);
        double Spacing = OrbitalRegion->GetGridSpace();
        if (!FMath::IsFinite(Spacing) || Spacing <= 0.0) return;
        const double ViewScale = FMath::Max(Distance, FMath::Abs(Focus.Z - Origin.Z));
        while (Spacing > FMath::Max(10.0, ViewScale * 0.4)) Spacing *= 0.25;
        const FVector Center = Origin + FVector(
            FMath::GridSnap(Focus.X - Origin.X, Spacing),
            FMath::GridSnap(Focus.Y - Origin.Y, Spacing), 0.0);

        auto* Lines = TacticalGrid.Get();
        if (Lines && Lines->GetOwner() != Camera)
        {
            Lines->DestroyComponent();
            Lines = nullptr;
        }
        if (!Lines)
        {
            Lines = NewObject<ULineBatchComponent>(Camera);
            Camera->AddInstanceComponent(Lines);
            Lines->RegisterComponent();
            TacticalGrid = Lines;
            TacticalGridSpacing = 0.0;
        }
        Lines->SetVisibility(true);
        if (TacticalGridSpacing == Spacing && TacticalGridCenter.Equals(Center)) return;
        TacticalGridSpacing = Spacing;
        TacticalGridCenter = Center;
        Lines->Flush();

        constexpr int32 HalfCount = 12;
        const double Extent = HalfCount * Spacing;
        for (int32 Index = -HalfCount; Index <= HalfCount; ++Index)
        {
            const double Offset = Index * Spacing;
            const int64 XIndex = FMath::RoundToInt64((Center.X - Origin.X + Offset) / Spacing);
            const int64 YIndex = FMath::RoundToInt64((Center.Y - Origin.Y + Offset) / Spacing);
            const FLinearColor Major(0.14f, 0.26f, 0.36f);
            const FLinearColor Minor(0.06f, 0.12f, 0.18f);
            Lines->DrawLine(Center + FVector(Offset, -Extent, 0), Center + FVector(Offset, Extent, 0),
                XIndex % 4 == 0 ? Major : Minor, SDPG_World, 0.0f, -1.0f);
            Lines->DrawLine(Center + FVector(-Extent, Offset, 0), Center + FVector(Extent, Offset, 0),
                YIndex % 4 == 0 ? Major : Minor, SDPG_World, 0.0f, -1.0f);
        }
    }

    void RestoreVisibility(APlayerController* PC)
    {
        if (PC)
            for (const auto& Entry : HiddenByCamera)
                if (AActor* Actor = Entry.Get()) PC->HiddenActors.Remove(Actor);
        HiddenByCamera.Reset();
        if (auto* Fighter = Cast<AFighterActor>(PlayerActor.Get())) Fighter->SetCockpitViewActive(false);
    }

    bool bRecoveryViewHeld = false;
    FTransform RecoveryViewPose = FTransform::Identity;
    void HoldRecoveryView(APlayerController* PC, ACameraActor* Camera)
    {
        if (!Camera) return;
        if (auto* Lines = TacticalGrid.Get()) Lines->SetVisibility(false);
        if (auto* Visual=PlayerActor.Get()) Visual->HoldRecoveryPresentation();
        if (!bRecoveryViewHeld) { RecoveryViewPose=Camera->GetActorTransform(); bRecoveryViewHeld=true; }
        Camera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Camera->SetActorTransform(RecoveryViewPose,false,nullptr,ETeleportType::TeleportPhysics);
        RestoreVisibility(PC);
        if (PC && PC->GetViewTarget()!=Camera) PC->SetViewTarget(Camera);
    }

    void Reset(APlayerController* PC)
    {
        RestoreVisibility(PC);
        if (auto* Visual=PlayerActor.Get()) Visual->ReleaseRecoveryPresentation();
        if (auto* Lines = TacticalGrid.Get()) Lines->DestroyComponent();
        if (auto* Fill = LaunchFill.Get()) Fill->DestroyComponent();
        *this = FMissionCameraRig();
    }

    void SetMode(EMode NewMode)
    {
        if (Mode == NewMode) return;
        Mode = NewMode;
        HeadYaw = HeadPitch = 0.0;
        Distance = 0.0;
        bDropCaptured = false;
        if (Mode != Orbit) ViewObject.Reset();
    }

    // Keep roll: FVector::Rotation() alone would discard the ship's up axis.
    static FQuat Aim(const FVector& Direction, const FVector& Up, const FQuat& Fallback)
    {
        if (Direction.IsNearlyZero()) return Fallback;
        return FRotationMatrix::MakeFromXZ(Direction.GetSafeNormal(), Up).ToQuat();
    }

    void Tick(APlayerController* PC, ACameraActor* CameraActor, AShipActor* Visual,
              Ship* Player, SimRegion* Region, AShipActor* Selected, double Dt, double& Zoom)
    {
        if (!PC || !CameraActor || !Visual || !Player || !Region)
        {
            if (auto* Lines = TacticalGrid.Get()) Lines->SetVisibility(false);
            return;
        }
        Dt = FMath::Clamp(Dt, 0.0, 0.1);
        if (PlayerActor.Get() != Visual)
        {
            RestoreVisibility(PC);
            PlayerActor = Visual;
            Distance = 0.0;
            UE_LOG(LogTemp, Log, TEXT("[MissionCamera] Player=%s Forward=%s BridgeLocal=%s ChaseLocal=%s VisualRotation=%s"),
                *Visual->GetName(), *Visual->GetActorForwardVector().ToString(),
                *Visual->BridgePointOffset.ToString(), *Visual->ChasePointOffset.ToString(),
                IsValid(Visual->VisualActor) && Visual->VisualActor->GetRootComponent() ? *Visual->VisualActor->GetRootComponent()->GetRelativeRotation().ToString() : TEXT("native mesh"));
        }
        auto Alive = [Region](AShipActor* Actor)
        {
            Ship* Data = IsValid(Actor) && !Actor->IsActorBeingDestroyed() ? Actor->GetRuntimeShip() : nullptr;
            return Data && Region->GetShips().contains(Data) && !Data->IsDead() && !Data->IsDying();
        };
        if (!Alive(ViewObject.Get())) ViewObject.Reset();
        if (!Alive(Selected)) Selected = nullptr;
        AShipActor* FocusActor = Mode == Orbit && ViewObject.IsValid() ? ViewObject.Get() : Visual;
        if (BoundsActor.Get() != FocusActor)
        {
            BoundsActor = FocusActor;
            LocalBounds = MissionTargetLocalBounds(FocusActor);
            Distance = 0.0;
        }
        const FTransform Pose = Visual->GetActorTransform();
        const FQuat ShipRotation = Visual->GetActorQuat();
        const FVector Forward = Visual->GetActorForwardVector();
        const FVector Up = Visual->GetActorUpVector();
        const FBox Bounds = LocalBounds.TransformBy(FocusActor->GetActorTransform());
        const double Radius = FMath::Max(10.0, LocalBounds.GetExtent().Size() * FocusActor->GetActorScale3D().GetAbsMax());
        const FVector Center = Bounds.GetCenter();
        const FVector Bridge = Visual->BridgePoint ? Visual->BridgePoint->GetComponentLocation()
            : Pose.TransformPosition(FVector(Player->GetBridgeLocation().Y, Player->GetBridgeLocation().X, Player->GetBridgeLocation().Z));
        Zoom = FMath::Clamp(Zoom * FMath::Exp(-RangeInput * Dt), 0.5, 3.0);
        if (Mode == Virtual || Mode == Cockpit)
        {
            HeadYaw = FMath::Clamp(HeadYaw + LookX * 70.0 * Dt, -135.0, 135.0);
            HeadPitch = FMath::Clamp(HeadPitch + LookY * 70.0 * Dt, -22.5, 60.0);
        }
        else if (Mode == Orbit)
        {
            Azimuth = FMath::UnwindDegrees(Azimuth + LookX * 70.0 * Dt);
            Elevation = FMath::Clamp(Elevation + LookY * 70.0 * Dt, -77.4, 77.4);
        }

        const EOPSMode Phase = Player->GetFlightPhase();
        FlightDeck* Deck = Player->GetDock();
        // Once caught by a recovery deck, keep the last stable external pose.
        // Neither hull taxi/spotting nor a near-coincident deck look-at can shake it.
        if (bRecoveryViewHeld || (Deck && Deck->IsRecoveryDeck() && Phase >= EOPSMode::DOCKING))
        {
            HoldRecoveryView(PC,CameraActor);
            return;
        }
        // Cache the real authored flight deck while the player is staged.
        // The simulation can advance from ALERT to LAUNCH in a single frame;
        // flight phase alone must not release the camera inside the bay.
        if (Deck && Deck->IsLaunchDeck() && Player->GetCarrier() &&
            Phase < EOPSMode::LOCKED)
        {
            if (HeldLaunchDeck != Deck || HeldLaunchPlayer != Player)
            {
                bHeldDeckPoseValid = false;
            }
            HeldLaunchDeck = Deck;
            HeldLaunchPlayer = Player;
            bLaunchExitBlendActive = false;
            LaunchExitBlendElapsed = 0.0;
        }
        else if (HeldLaunchPlayer != Player)
        {
            HeldLaunchDeck = nullptr;
            HeldLaunchPlayer = nullptr;
            bHeldDeckPoseValid = false;
        }

        bool bLaunchDeckHold = false;
        if (HeldLaunchDeck && HeldLaunchPlayer == Player &&
            Player->GetCarrier() && Phase >= EOPSMode::LOCKED &&
            Phase <= EOPSMode::ACTIVE)
        {
            // Compare actual simulation position with the authored launch segment;
            // carrier motion is already incorporated by FlightDeck::Orient().
            const FVector Start = HeldLaunchDeck->StartPoint();
            const FVector TubeForward = Player->GetCarrier()->GetCam().vpn().GetSafeNormal();
            const double ExitClearance = FMath::Max(1.0, double(Player->GetRadius()) * 1.5);
            constexpr double GroundCameraHandoffMSL = 1000.0;
            bLaunchDeckHold = Player->IsAirborne()
                ? Phase < EOPSMode::ACTIVE && Player->GetAltitudeMSL() < GroundCameraHandoffMSL
                : Phase < EOPSMode::ACTIVE ||
                    FVector::DotProduct(Player->GetLocation() - Start, TubeForward) < ExitClearance;
            if (!bLaunchDeckHold)
            {
                // Start from the *last displayed* camera pose for a seamless
                // transition, even if the authored end is passed in one frame.
                bLaunchExitBlendActive = true;
                LaunchExitBlendElapsed = 0.0;
                LaunchExitBlendEye = CameraActor->GetActorLocation();
                LaunchExitBlendRotation = CameraActor->GetActorQuat();
                HeldLaunchDeck = nullptr;
                HeldLaunchPlayer = nullptr;
                bHeldDeckPoseValid = false;
            }
        }
        else if (HeldLaunchDeck && Phase > EOPSMode::ACTIVE)
        {
            // The simulation can skip directly beyond ACTIVE in one tick.
            bLaunchExitBlendActive = true;
            LaunchExitBlendElapsed = 0.0;
            LaunchExitBlendEye = CameraActor->GetActorLocation();
            LaunchExitBlendRotation = CameraActor->GetActorQuat();
            HeldLaunchDeck = nullptr;
            HeldLaunchPlayer = nullptr;
            bHeldDeckPoseValid = false;
        }

        const bool bDock = (Deck && (Phase < EOPSMode::LOCKED ||
            (Player->IsAirborne() ? Phase >= EOPSMode::DOCKING : Phase >= EOPSMode::RECOVERY))) ||
            bLaunchDeckHold;
        // Planetary handoffs keep the selected flight view tracking the hull.
        // The stationary drop camera is for flybys and other transitions.
        const bool bTransition = Player->InTransition() && !Player->IsAttaining() && !Player->IsDropping();
        if (bTransition != bWasTransition) { bDropCaptured = false; bWasTransition = bTransition; }
        const bool bPadlock = Mode == Target && Selected && Player->GetTarget() == Selected->GetRuntimeShip();
        const bool bInternalPadlock = bPadlock && Player->GetCockpit() != nullptr;
        const bool bInternal = !bDock && !bTransition && (Mode == Cockpit || Mode == Virtual || bInternalPadlock);
        bThirdPersonView = !bInternal && !bDock && !bTransition;
        RestoreVisibility(PC);
        if (auto* Fighter = Cast<AFighterActor>(Visual)) Fighter->SetCockpitViewActive(!bDock && !bTransition && Mode == Cockpit);
        if (bInternal)
        {
            // Hide only for this player view; do not stop simulation or effects.
            TArray<AActor*> Actors;
            Visual->GetAttachedActors(Actors, false, true);
            Actors.Add(Visual);
            AActor* CockpitActor = nullptr;
            if (auto* Fighter = Cast<AFighterActor>(Visual))
                if (Fighter->IsCockpitViewActive() && Fighter->CockpitComponent) CockpitActor = Fighter->CockpitComponent->GetChildActor();
            for (AActor* Actor : Actors)
                if (IsValid(Actor) && Actor != CameraActor && Actor != CockpitActor &&
                    !(CockpitActor && Actor->IsAttachedTo(CockpitActor)) && !PC->HiddenActors.Contains(Actor))
                {
                    PC->HiddenActors.Add(Actor);
                    HiddenByCamera.Add(Actor);
                }
        }

        if (auto* ViewCamera = Cast<UMissionViewCameraComponent>(CameraActor->GetCameraComponent()))
            ViewCamera->bCockpitView = !bDock && !bTransition && Mode == Cockpit;
        FVector Eye = Bridge;
        // FalconPit is authored around the pilot eye; its visual-origin correction
        // must also move the cockpit camera, never the legacy simulation bridge.
        if (auto* Fighter = Cast<AFighterActor>(Visual))
            if (Fighter->IsCockpitViewActive() && Fighter->CockpitComponent &&
                Fighter->CockpitComponent->GetChildActor())
                Eye = Fighter->CockpitComponent->GetComponentLocation();
        FQuat Rotation = ShipRotation;
        if (bDock)
        {
            // Deck camera is in simulation space; rebase into the displayed ship frame.
            FlightDeck* ActiveDeck = bLaunchDeckHold ? HeldLaunchDeck : Deck;
            if (!ActiveDeck) return;
            const Ship* Carrier = ActiveDeck->GetCarrier();
            USSWRuntimeSubsystem* Runtime = PC->GetGameInstance()
                ? PC->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>() : nullptr;
            if (ActiveDeck->IsLaunchDeck() && Carrier && Carrier->GetRegion() && Runtime)
            {
                // Use the launch-bay chase pose in the same rendered frame as the fighter.
                const auto& CarrierBasis = Carrier->GetCam();
                const FVector CarrierPosition = Carrier->GetLocation();
                FVector Origin, ForwardPoint, UpPoint;
                Runtime->GetCarrierVisualPoint(Carrier, CarrierPosition, Origin);
                Runtime->GetCarrierVisualPoint(Carrier, CarrierPosition + CarrierBasis.vpn(), ForwardPoint);
                Runtime->GetCarrierVisualPoint(Carrier, CarrierPosition + CarrierBasis.vup(), UpPoint);
                const FVector CarrierUp = (UpPoint - Origin).GetSafeNormal();
                const FQuat CarrierRotation = Aim(ForwardPoint - Origin, CarrierUp, ShipRotation);
                const FTransform CarrierFrame(CarrierRotation, Origin);
                if (!bLaunchDeckHold || !bHeldDeckPoseValid)
                {
                    const FBox FighterBounds = FocusActor == Visual ? LocalBounds : MissionTargetLocalBounds(Visual);
                    const double FighterRadius = FMath::Max(10.0, FighterBounds.GetExtent().Size() * Pose.GetScale3D().GetAbsMax());
                    const FVector Target = FighterBounds.IsValid ? Pose.TransformPosition(FighterBounds.GetCenter()) : Pose.GetLocation();
                    const FVector Chase = Visual->ChasePoint
                        ? Visual->ChasePoint->GetComponentLocation() - Pose.GetLocation() : FVector::ZeroVector;
                    const double Back = FMath::Max(FMath::Abs(FVector::DotProduct(Chase, Forward)), FighterRadius * 3.0);
                    const double Height = FMath::Max(FMath::Abs(FVector::DotProduct(Chase, Up)), FighterRadius * 0.65);
                    Eye = Target - Forward * Back + Up * Height;
                    Rotation = Aim(Target - Eye, CarrierUp, ShipRotation);
                    HeldLaunchEyeLocal = CarrierFrame.InverseTransformPositionNoScale(Eye);
                    HeldLaunchRotationLocal = CarrierRotation.Inverse() * Rotation;
                    bHeldDeckPoseValid = true;
                }
                else
                {
                    Eye = CarrierFrame.TransformPositionNoScale(HeldLaunchEyeLocal);
                    Rotation = CarrierRotation * HeldLaunchRotationLocal;
                }
            }
            else
            {
                const FVector CameraSimPoint = ActiveDeck->CamLoc();
                const FVector Offset = CameraSimPoint - Player->GetLocation();
                const auto& Basis = Player->GetCam();
                Eye = Pose.TransformPositionNoScale(FVector(FVector::DotProduct(Offset, Basis.vpn()),
                    FVector::DotProduct(Offset, Basis.vrt()), FVector::DotProduct(Offset, Basis.vup())));
                if (Carrier && Runtime) Runtime->GetCarrierVisualPoint(Carrier, CameraSimPoint, Eye);
                Rotation = Aim(Bridge - Eye, Up, Rotation);
            }
        }
        else if (Mode == Drop || bTransition)
        {
            if (!bDropCaptured)
            {
                DropPosition = CameraActor->GetActorLocation();
                if (Player->IsDying())
                {
                    const Camera& Frame = Player->GetCam();
                    const FVector Delta = Player->GetTransitionLocation() - Player->GetLocation();
                    const FVector Local(FVector::DotProduct(Delta, Frame.vpn()), FVector::DotProduct(Delta, Frame.vrt()),
                        FVector::DotProduct(Delta, Frame.vup()));
                    DropPosition = Pose.TransformPositionNoScale(Local);
                }
                bDropCaptured = true;
            }
            Eye = DropPosition;
            Rotation = Aim(Visual->GetActorLocation() - Eye, FVector::UpVector, Rotation);
        }
        else if (bInternal)
        {
            if (bInternalPadlock)
            {
                const FVector LocalDirection = ShipRotation.UnrotateVector(Selected->GetActorLocation() - Eye);
                const FRotator Bearing = LocalDirection.Rotation();
                HeadYaw = FMath::Clamp(double(Bearing.Yaw), -135.0, 135.0);
                HeadPitch = FMath::Clamp(double(Bearing.Pitch), -22.5, 60.0);
            }
            Rotation = ShipRotation * FRotator(HeadPitch, HeadYaw, 0.0).Quaternion();
        }
        else if (Mode == Orbit || Mode == Overview)
        {
            const double Desired = FMath::Max(Radius * 1.25, Radius * 4.0 * Zoom);
            Distance = Distance <= 0.0 ? Desired : FMath::Lerp(Distance, Desired, 1.0 - FMath::Exp(-8.0 * Dt));
            const double El = (Mode == Overview) ? 80.0 : Elevation;
            Eye = Center + FRotator(El, Azimuth, 0.0).Vector() * Distance;
            Rotation = Aim(Center - Eye, FVector::UpVector, Rotation);
        }
        else if (bPadlock)
        {
            const FVector TargetLocation = Selected->GetActorLocation();
            const FVector Direction = (TargetLocation - Visual->GetActorLocation()).GetSafeNormal();
            // Legacy external padlock, used when there is no rendered cockpit.
            Eye = Visual->GetActorLocation() - Direction * (5.0 * Radius * Zoom) + Up * Radius;
            Rotation = Aim(TargetLocation - Eye, Up, Rotation);
        }
        else
        {
            // Stable third-person frame: use the displayed hull basis for both eye and aim.
            // Mixing raw simulation velocity with the interpolated actor pose made the
            // ship slide sideways then recenter as velocity caught up after a turn.
            const FVector Direction = Forward;
            const FVector AuthoredChase = Visual->ChasePoint
                ? Pose.InverseTransformPosition(Visual->ChasePoint->GetComponentLocation())
                : FVector(Player->GetChaseLocation().Y, Player->GetChaseLocation().X, Player->GetChaseLocation().Z);
            const FVector ScaledChase = AuthoredChase * Pose.GetScale3D();
            const double BaseDistance = FMath::Max(FMath::Abs(ScaledChase.X), Radius * 3.0);
            const double Desired = FMath::Max(Radius * 1.25, BaseDistance * Zoom);
            Distance = Distance <= 0.0 ? Desired : FMath::Lerp(Distance, Desired, 1.0 - FMath::Exp(-8.0 * Dt));
            Eye = Center - Direction * Distance + Up * FMath::Max(FMath::Abs(ScaledChase.Z), Radius * 0.45);
            Rotation = Aim(Direction, Up, Rotation);
        }
        // Blend the final deck frame into the normal flight camera rather than
        // snapping between two unrelated world-space eye positions.
        if (bLaunchExitBlendActive)
        {
            LaunchExitBlendElapsed += Dt;
            const double Alpha = FMath::Clamp(LaunchExitBlendElapsed / 0.85, 0.0, 1.0);
            const double Smooth = Alpha * Alpha * (3.0 - 2.0 * Alpha);
            Eye = FMath::Lerp(LaunchExitBlendEye, Eye, Smooth);
            Rotation = FQuat::Slerp(LaunchExitBlendRotation, Rotation, Smooth).GetNormalized();
            if (Alpha >= 1.0)
                bLaunchExitBlendActive = false;
        }
        if (Eye.ContainsNaN() || Rotation.ContainsNaN()) return;
        const bool bLightDeck = bDock && (bLaunchDeckHold || (Deck && Deck->IsLaunchDeck())) && !Player->IsAirborne();
        if (bLightDeck && !LaunchFill.IsValid())
        {
            auto* Fill = NewObject<UPointLightComponent>(CameraActor);
            CameraActor->AddInstanceComponent(Fill);
            Fill->SetupAttachment(CameraActor->GetRootComponent());
            Fill->SetMobility(EComponentMobility::Movable);
            Fill->SetRelativeLocation(FVector(90, 35, 45));
            Fill->SetAttenuationRadius(1200.0f);
            Fill->SetUseInverseSquaredFalloff(false);
            Fill->SetLightFalloffExponent(2.0f);
            Fill->SetIntensity(4.0f);
            Fill->SetInverseExposureBlend(0.85f);
            Fill->SetCastShadows(false);
            Fill->RegisterComponent();
            LaunchFill = Fill;
        }
        if (auto* Fill = LaunchFill.Get()) Fill->SetVisibility(bLightDeck);

        // The actor updates every rendered frame; the old 60 Hz timer left the camera
        // behind on intervening frames (a large offset at fighter speeds).
        // The cockpit and chase cameras must follow the rendered hull between updates.
        const bool FollowHull = (Mode == Chase || Mode == Cockpit) &&
            !bDock && !bTransition && !bLaunchExitBlendActive;
        if (FollowHull) {
            if (CameraActor->GetAttachParentActor() != Visual)
                CameraActor->AttachToActor(Visual, FAttachmentTransformRules::KeepWorldTransform);
        } else if (CameraActor->GetAttachParentActor()) {
            CameraActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
        CameraActor->SetActorLocationAndRotation(Eye, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
        const double Fov = bInternal ? (bWide ? 90.0 : 60.0) * Zoom : (bWide ? 95.0 : 65.0);
        CameraActor->GetCameraComponent()->SetFieldOfView(float(FMath::Clamp(Fov, 25.0, 110.0)));
        UpdateTacticalGrid(CameraActor, Player, Region, Center);
        if (PC->GetViewTarget() != CameraActor) PC->SetViewTarget(CameraActor);
    }
};
