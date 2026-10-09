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
    enum EMode { Cockpit, Virtual, Chase, Drop, Orbit, Target, Threat };
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
        const bool bDock = Deck && (Phase < EOPSMode::LOCKED ||
            (Player->IsAirborne() ? Phase >= EOPSMode::DOCKING : Phase >= EOPSMode::RECOVERY));
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
            const FVector Offset = Deck->CamLoc() - Player->GetLocation();
            const auto& Basis = Player->GetCam();
            Eye = Pose.TransformPosition(FVector(FVector::DotProduct(Offset, Basis.vpn()),
                FVector::DotProduct(Offset, Basis.vrt()), FVector::DotProduct(Offset, Basis.vup())));
            if (UGameInstance* GI=PC->GetGameInstance()) {
                if (USSWRuntimeSubsystem* Runtime=GI->GetSubsystem<USSWRuntimeSubsystem>())
                    Runtime->GetCarrierVisualPoint(Player->GetCarrier(), Deck->CamLoc(), Eye);
            }
            Rotation = Aim(Bridge - Eye, Up, Rotation);
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
        else if (Mode == Orbit)
        {
            const double Desired = FMath::Max(Radius * 1.25, Radius * 4.0 * Zoom);
            Distance = Distance <= 0.0 ? Desired : FMath::Lerp(Distance, Desired, 1.0 - FMath::Exp(-8.0 * Dt));
            Eye = Center + FRotator(Elevation, Azimuth, 0.0).Vector() * Distance;
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
            // Standard third person. Start behind Stormhawk along its local -X.
            // Retain legacy's small velocity contribution without lagging the ship position.
            FVector Direction = Forward;
            const FVector Velocity = Player->GetVelocity();
            if (Velocity.SizeSquared() > 100.0)
            {
                const auto& Basis = Player->GetCam();
                const FVector LocalVelocity(FVector::DotProduct(Velocity, Basis.vpn()),
                    FVector::DotProduct(Velocity, Basis.vrt()), FVector::DotProduct(Velocity, Basis.vup()));
                Direction = (Forward * 2.0 + Pose.TransformVectorNoScale(LocalVelocity.GetSafeNormal()) * 0.25).GetSafeNormal();
            }
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
        if (Eye.ContainsNaN() || Rotation.ContainsNaN()) return;
        CameraActor->SetActorLocationAndRotation(Eye, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
        const double Fov = (bWide ? 90.0 : 60.0) * (bInternal ? Zoom : 1.0);
        CameraActor->GetCameraComponent()->SetFieldOfView(float(FMath::Clamp(Fov, 25.0, 110.0)));
        if (PC->GetViewTarget() != CameraActor) PC->SetViewTarget(CameraActor);
    }
};
