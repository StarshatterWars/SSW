#include "OwnedHullLocation.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Mission.h"
#include "MissionElement.h"
namespace OwnedHullLocation
{
FString Carrier(const FString& Name) { return TEXT("Carrier / ") + Name; }
FString Station(const FString& System, const FString& Name) { return TEXT("Station / ") + System + TEXT(" / ") + Name; }
FString Dock(const Ship& Craft)
{
    const auto* Base = Craft.GetCarrier();
    if (!Base || Base->IsDead()) return FString();
    const FString Name(UTF8_TO_TCHAR(Base->GetName()));
    if ((uint32(Base->GetClassification()) & uint32(CLASSIFICATION::CARRIER)) != 0) return Carrier(Name);
    auto* Region = Craft.GetRegion(); const auto* System = Region ? Region->GetSystem() : nullptr;
    return System ? Station(UTF8_TO_TCHAR(System->GetName()), Name) : FString();
}
FString Current(const Ship& Craft)
{
    if (Craft.GetFlightPhase() <= EOPSMode::ALERT) return Dock(Craft);
    auto* Region = Craft.GetRegion(); const auto* System = Region ? Region->GetSystem() : nullptr;
    return System ? FString::Printf(TEXT("In flight / %s / %s"), UTF8_TO_TCHAR(System->GetName()), UTF8_TO_TCHAR(Region->GetName())) : FString();
}
bool IsAt(const FPlayerHangarShip& Hull, const FString& Place)
{ return !Hull.bLost && !Place.IsEmpty() && Hull.Location.Equals(Place, ESearchCase::IgnoreCase); }
bool RecoverPurchase(const FS_PlayerGameInfo& Profile, FPlayerHangarShip& Hull)
{
    if (Hull.Location.StartsWith(TEXT("Carrier / ")) || Hull.Location.StartsWith(TEXT("Station / ")) || Hull.Location.StartsWith(TEXT("In flight / "))) return false;
    for (const auto& Payment : Profile.Economy.Journal)
        if (Payment.Reference == Hull.HullId && Payment.Category == EWalletTransactionCategory::Shipyard)
        {
            const FString Destination = Payment.Destination.ToString();
            if (Destination.StartsWith(TEXT("Carrier / ")) || Destination.StartsWith(TEXT("Station / ")))
            { Hull.Location = Destination; return true; }
            FString Source = Payment.Source.IsNone() ? Payment.To.ToString() : Payment.Source.ToString();
            if (Payment.Source.IsNone() && !Source.RemoveFromStart(TEXT("NPC/"))) continue;
            TArray<FString> Parts; Source.ParseIntoArray(Parts, TEXT("/"), true);
            if (Parts.Num() >= 2)
            {
                Hull.Location = Parts.Num() >= 3 && Parts[2] == TEXT("Squadron Sales") ? Carrier(Parts[1]) : Station(Parts[0], Parts[1]);
                return true;
            }
        }
    return false; // A legacy region alone does not prove which station holds a craft.
}
TArray<FGuid> StoredAt(const FS_PlayerGameInfo& Profile, const FString& Place)
{
    TArray<FGuid> Result;
    for (const auto& Hull : Profile.HangarShips) if (!Hull.bAssigned && IsAt(Hull, Place)) Result.Add(Hull.HullId);
    return Result;
}
bool CanProvision(const FPlayerHangarShip& Hull, Mission& Entry, bool Active)
{
    auto* Flight = Entry.GetPlayer(); if (!Flight || Hull.bLost) return false;
    FString Base(UTF8_TO_TCHAR(Flight->GetCarrier().data()));
    if (Base.IsEmpty() && Flight->GetSquadron().length())
        if (const auto* Squadron = Entry.FindElement(Flight->GetSquadron()))
            Base = UTF8_TO_TCHAR(Squadron->GetCarrier().data());
    if (IsAt(Hull, Carrier(Base)) || IsAt(Hull, Station(UTF8_TO_TCHAR(Entry.GetSystem()), Base))) return true;
    const FString Prefix = FString::Printf(TEXT("In flight / %s / "), UTF8_TO_TCHAR(Entry.GetSystem()));
    return Active && Hull.Location.StartsWith(Prefix); // Existing live-world checkpoints restore the active craft's exact position.
}
}
