#include "AsteroidBaseActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FlightDeck.h"
#include "Ship.h"

void AAsteroidBaseActor::BeginPlay()
{
    Super::BeginPlay();

    for (UStaticMeshComponent* Part : TInlineComponentArray<UStaticMeshComponent*>(this))
    {
        const UStaticMesh* Mesh = Part->GetStaticMesh();
        if (!Mesh) continue;
        if (Mesh->GetFName() == TEXT("SM_Spaceport_Door_Top"))
        {
            Part->SetMobility(EComponentMobility::Movable);
            UpperDoor = Part;
            UpperOpenLocation = Part->GetRelativeLocation();
        }
        else if (Mesh->GetFName() == TEXT("SM_Spaceport_Door_Bot"))
        {
            Part->SetMobility(EComponentMobility::Movable);
            LowerDoor = Part;
            LowerOpenLocation = Part->GetRelativeLocation();
        }
    }
    UpdateDoorPositions();
}

void AAsteroidBaseActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    bool bOpen = false;
    const Ship* Station = GetRuntimeShip();
    if (Station && !Station->IsDead() && !Station->IsDying())
    {
        for (int Index = 0; Index < Station->NumFlightDecks() && !bOpen; ++Index)
        {
            FlightDeck* Deck = Station->GetFlightDeck(Index);
            if (!Deck) continue;
            for (int Slot = 0; Slot < Deck->NumSlots(); ++Slot)
            {
                const int State = Deck->State(Slot);
                if (State == FlightDeck::DOCKING || State == FlightDeck::QUEUED ||
                    State == FlightDeck::LOCKED || State == FlightDeck::LAUNCH)
                {
                    bOpen = true;
                    break;
                }
            }
            if (!bOpen && Deck->IsRecoveryDeck())
            {
                ListIter<InboundSlot> Inbound = Deck->GetRecoveryQueue();
                while (++Inbound)
                {
                    if (Inbound->Cleared() && Inbound->Final())
                    {
                        bOpen = true;
                        break;
                    }
                }
            }
        }
    }

    bBayDoorsOpen = bOpen;
    const float Target = bOpen ? 1.0f : 0.0f;
    if (DoorOpening != Target)
    {
        DoorOpening = BayDoorTravelSeconds > 0.0f
            ? FMath::FInterpConstantTo(DoorOpening, Target, DeltaTime, 1.0f / BayDoorTravelSeconds)
            : Target;
        UpdateDoorPositions();
    }
}

void AAsteroidBaseActor::UpdateDoorPositions()
{
    const float Remaining = 1.0f - DoorOpening;
    if (auto* Door = UpperDoor.Get())
        Door->SetRelativeLocation(UpperOpenLocation - FVector::UpVector * (UpperDoorTravel * Remaining));
    if (auto* Door = LowerDoor.Get())
        Door->SetRelativeLocation(LowerOpenLocation + FVector::UpVector * (LowerDoorTravel * Remaining));
}
