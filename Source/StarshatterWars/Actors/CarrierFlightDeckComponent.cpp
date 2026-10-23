#include "CarrierFlightDeckComponent.h"
#include "FlightDeck.h"

UCarrierFlightDeckComponent::UCarrierFlightDeckComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCarrierFlightDeckComponent::BindFlightDeck(FlightDeck* InDeck, int32 InIndex)
{
    RuntimeDeck = InDeck;
    DeckIndex = InIndex;
}

bool UCarrierFlightDeckComponent::IsLaunchDeck() const
{
    return RuntimeDeck && RuntimeDeck->IsLaunchDeck();
}

bool UCarrierFlightDeckComponent::IsRecoveryDeck() const
{
    return RuntimeDeck && RuntimeDeck->IsRecoveryDeck();
}

int32 UCarrierFlightDeckComponent::GetSlotCount() const
{
    return RuntimeDeck ? RuntimeDeck->NumSlots() : 0;
}
