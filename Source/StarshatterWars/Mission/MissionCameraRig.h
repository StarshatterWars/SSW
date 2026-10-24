#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "ShipActor.h"
#include "Ship.h"
#include "FlightDeck.h"
#include "SimRegion.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionTargetOverlay.h"

// Presentation adapter for Stars45/CameraDirector modes. Simulation positions
// cannot be used as UE world positions: region-shell placement moves visuals.
// All close camera offsets use the displayed actor's frame (+X forward, +Z up).
struct FMissionCameraRig
{
    enum EMode { Cockpit, Virtual, Chase, Drop, Orbit, Overview, Target, Threat };
    EMode Mode = Chase;
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
    double HeldLaneOffset = 0.0;
    FQuat HeldDeckRotation = FQuat::Identity;
    bool bHeldDeckPoseValid = false;
    // Deck release is a presentation hand-off, not an instantaneous camera cut.
    bool bLaunchExitBlendActive = false;
    double LaunchExitBlendElapsed = 0.0;
    FVector LaunchExitBlendEye = FVector::ZeroVector;
    FQuat LaunchExitBlendRotation = FQuat::Identity;

    void RestoreVisibility(APlayerController* PC)
    {
        if (PC)
            for (const auto& Entry : HiddenByCamera)
                if (AActor* Actor = Entry.Get()) PC->HiddenActors.Remove(Actor);
        HiddenByCamera.Reset();
    }

    void Reset(APlayerController* PC)
    {
        RestoreVisibility(PC);
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
        if (!PC || !CameraActor || !Visual || !Player || !Region) return;
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
        // Cache the real authored flight deck while the player is staged.
        // The simulation can advance from ALERT to LAUNCH in a single frame;
        // flight phase alone must not release the camera inside the bay.
        if (Deck && Deck->IsLaunchDeck() && Player->GetCarrier() &&
            Phase < EOPSMode::LOCKED)
        {
            if (HeldLaunchDeck != Deck || HeldLaunchPlayer != Player)
                bHeldDeckPoseValid = false;
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
            const FVector End = HeldLaunchDeck->EndPoint();
            const FVector Run = End - Start;
            const double RunLengthSq = Run.SizeSquared();
            const double Progress = RunLengthSq > 1.0
                ? FVector::DotProduct(Player->GetLocation() - Start, Run) / RunLengthSq
                : 1.0;
            bLaunchDeckHold = Progress < 1.0;
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
        const bool bTransition = Player->InTransition();
        if (bTransition != bWasTransition) { bDropCaptured = false; bWasTransition = bTransition; }
        const bool bPadlock = Mode == Target && Selected && Player->GetTarget() == Selected->GetRuntimeShip();
        const bool bInternalPadlock = bPadlock && Player->GetCockpit() != nullptr;
        const bool bInternal = !bDock && !bTransition && (Mode == Cockpit || Mode == Virtual || bInternalPadlock);
        if (!bInternal) RestoreVisibility(PC);
        else
        {
            // Hide only for this player view; do not stop simulation or effects.
            TArray<AActor*> Actors;
            Visual->GetAttachedActors(Actors, false, true);
            Actors.Add(Visual);
            for (AActor* Actor : Actors)
                if (IsValid(Actor) && !PC->HiddenActors.Contains(Actor))
                {
                    PC->HiddenActors.Add(Actor);
                    HiddenByCamera.Add(Actor);
                }
        }

        FVector Eye = Bridge;
        FQuat Rotation = ShipRotation;
        if (bDock)
        {
            // Deck camera is in simulation space; rebase into the displayed ship frame.
            FlightDeck* ActiveDeck = bLaunchDeckHold ? HeldLaunchDeck : Deck;
            if (!ActiveDeck) return;
            FVector CameraSimPoint = ActiveDeck->CamLoc();
            if (ActiveDeck->IsLaunchDeck() && Player->GetCarrier())
            {
                // .def cam is often on the centerline. The two authored launch
                // lanes are displaced sideways (Orion: X=+60 and X=-60).
                // Preserve the authored camera height and fore/aft separation,
                // but move its lateral coordinate into the fighter's actual spot.
                const FVector Run = (ActiveDeck->EndPoint() - ActiveDeck->StartPoint()).GetSafeNormal();
                const FVector CarrierUp = Player->GetCarrier()->GetCam().vup().GetSafeNormal();
                const FVector Right = FVector::CrossProduct(Run, CarrierUp).GetSafeNormal();
                if (!Right.IsNearlyZero())
                {
                    if (!bLaunchDeckHold)
                        HeldLaneOffset = FVector::DotProduct(Player->GetLocation() - CameraSimPoint, Right);
                    CameraSimPoint += Right * HeldLaneOffset;
                }
            }
            if (bLaunchDeckHold)
            {
                // The old implementation held Eye at CamLoc() for the entire
                // launch run, making the flight look like a static tableau.
                // Travel forward along the .def launch segment, carrier-relative.
                // A partial follow factor leaves perceptible motion of both the
                // fighter and the bay, while preserving the authored camera lane.
                const FVector DeckRun = ActiveDeck->EndPoint() - ActiveDeck->StartPoint();
                const double DeckLength = DeckRun.Size();
                if (DeckLength > 1.0)
                {
                    const FVector Along = DeckRun / DeckLength;
                    const double DistanceAlong = FMath::Clamp(
                        FVector::DotProduct(Player->GetLocation() - ActiveDeck->StartPoint(), Along),
                        0.0, DeckLength);
                    CameraSimPoint += Along * (0.70 * DistanceAlong);
                }
            }
            const FVector Offset = CameraSimPoint - Player->GetLocation();
            const auto& Basis = Player->GetCam();
            Eye = Pose.TransformPosition(FVector(FVector::DotProduct(Offset, Basis.vpn()),
                FVector::DotProduct(Offset, Basis.vrt()), FVector::DotProduct(Offset, Basis.vup())));
            if (UGameInstance* GI = PC->GetGameInstance())
                if (USSWRuntimeSubsystem* Runtime = GI->GetSubsystem<USSWRuntimeSubsystem>())
                    Runtime->GetCarrierVisualPoint(Player->GetCarrier(), CameraSimPoint, Eye);

            if (bLaunchDeckHold && bHeldDeckPoseValid)
            {
                // Keep the stationary carrier-relative camera pointing out of
                // the bay instead of looking back toward the moving fighter.
                Rotation = HeldDeckRotation;
            }
            else
            {
                // Confirmed 180-degree pre-launch correction from prior test.
                Rotation = FQuat(Up.GetSafeNormal(), PI) * Aim(Bridge - Eye, Up, Rotation);
                if (ActiveDeck->IsLaunchDeck() && Phase < EOPSMode::LOCKED)
                {
                    HeldDeckRotation = Rotation;
                    bHeldDeckPoseValid = true;
                }
            }
        }
        else if (Mode == Drop || bTransition)
        {
            if (!bDropCaptured)
            {
                DropPosition = CameraActor->GetActorLocation();
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
            Rotation = Aim(Bridge + Forward * Radius - Eye, Up, Rotation);
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
        // The actor updates every rendered frame; the old 60 Hz timer left the camera
        // behind on intervening frames (a large offset at fighter speeds).
        // Attach chase view to the displayed hull so both move together between callbacks.
        const bool FollowHull = Mode == Chase && !bDock && !bTransition && !bLaunchExitBlendActive;
        if (FollowHull) {
            if (CameraActor->GetAttachParentActor() != Visual)
                CameraActor->AttachToActor(Visual, FAttachmentTransformRules::KeepWorldTransform);
        } else if (CameraActor->GetAttachParentActor()) {
            CameraActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
        CameraActor->SetActorLocationAndRotation(Eye, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
        const double Fov = (bWide ? 90.0 : 60.0) * (bInternal ? Zoom : 1.0);
        CameraActor->GetCameraComponent()->SetFieldOfView(float(FMath::Clamp(Fov, 25.0, 110.0)));
        if (PC->GetViewTarget() != CameraActor) PC->SetViewTarget(CameraActor);
    }
};
