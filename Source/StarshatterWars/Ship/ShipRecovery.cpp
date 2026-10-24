#include "Ship.h"
#include "FlightDeck.h"
#include "Drive.h"
#include "NavSystem.h"
#include "Sim.h"
#include "SteerAI.h"
#include "RadioView.h"

bool Ship::CanAutoLand() const
{
    if (!sim || sim->GetPlayerShip() != this || !IsDropship() || IsDead() || IsDying() ||
        InTransition() || net_control || !inbound || !inbound->Cleared() ||
        (flight_phase != EOPSMode::APPROACH && flight_phase != EOPSMode::RECOVERY)) return false;
    FlightDeck* Deck = inbound->GetDeck();
    const Ship* Carrier = Deck ? Deck->GetCarrier() : nullptr;
    return Carrier && !Carrier->IsDead() && !Carrier->IsDying() && GetRegion() &&
        Carrier->GetRegion() == GetRegion() && Deck->IsRecoveryDeck() &&
        main_drive && main_drive->IsPowerOn() && flcs;
}

bool Ship::SetAutoLand(bool engage)
{
    if (engage)
    {
        if (bAutoLand) return true;
        if (!CanAutoLand())
        {
            RadioView::Message("Autoland unavailable. Call inbound and obtain landing clearance first.");
            return false;
        }
        SimDirector* LandingDirector = SteerAI::Create(this, ESteerAIType::FIGHTER);
        if (!LandingDirector) return false;
        if (navsys) navsys->DisengageAutoNav();
        ClearManualRotationInput();
        bManualRotation = false;
        bManualThrottle = false;
        SetAugmenter(false);
        delete dir;
        dir = LandingDirector;
        bAutoLand = true;
        director_info = "Autoland";
        LowerGear();
        RadioView::Message("Autoland engaged. Following assigned recovery approach.");
        UE_LOG(LogTemp, Display, TEXT("[AutoLand] Engaged ship='%hs' deck='%hs'"),
            GetName(), inbound->GetDeck()->GetName());
        return true;
    }
    // Once the deck has captured the aircraft, it owns the docking motion.
    if (flight_phase >= EOPSMode::DOCKING) return false;
    if (bAutoLand)
    {
        bAutoLand = false;
        if (inbound) inbound->SetFinal(false);
        EnableManualFlight();
        RadioView::Message("Autoland disengaged. You have control.");
        UE_LOG(LogTemp, Display, TEXT("[AutoLand] Disengaged ship='%hs'"), GetName());
    }
    return true;
}
