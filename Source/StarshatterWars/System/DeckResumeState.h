#pragma once
#include "FlightDeck.h"
#include "Ship.h"

namespace DeckResumeState
{
// Mission provisioning seeds deck slots before the saved world is applied.
// Those slots must stop owning a craft that has already departed in the save.
inline void Release(FlightDeck& Deck,Ship& Craft)
{
    for(int32 I=0;I<Deck.NumSlots();++I)if(Deck.GetShip(I)==&Craft)Deck.Clear(I);
}
inline bool Park(FlightDeck& Deck,Ship& Craft)
{
    Release(Deck,Craft);
    int32 Index=INDEX_NONE;
    const bool Result=Deck.Spot(&Craft,Index);
    if(Result){Craft.SetThrottle(0);if(Deck.GetCarrier())Craft.SetVelocity(Deck.GetCarrier()->GetVelocity());}
    return Result;
}
}
