#include "CarrierHangarComponent.h"

UCarrierHangarComponent::UCarrierHangarComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCarrierHangarComponent::BindHangar(Hangar* InHangar, int32 InDeckIndex)
{
    RuntimeHangar = InHangar;
    AssociatedDeckIndex = InDeckIndex;
}
