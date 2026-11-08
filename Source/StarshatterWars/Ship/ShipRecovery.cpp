#include "Ship.h"
#include "FlightDeck.h"
#include "Drive.h"
#include "NavSystem.h"
#include "Sim.h"
#include "SimElement.h"
#include "SteerAI.h"
#include "RadioView.h"
#include "Instruction.h"

bool Ship::IsAutoLandEngaged() const
{
    return bAutoLand || (navsys && navsys->IsAutoLandRTB() && navsys->AutoNavEngaged());
}

bool Ship::CanAutoLand() const
{
    if (!sim || sim->GetPlayerShip() != this || !IsDropship() || IsDead() || IsDying() ||
        InTransition() || net_control || !inbound || !inbound->Cleared() ||
        (flight_phase != EOPSMode::APPROACH && flight_phase != EOPSMode::RECOVERY &&
         !(flight_phase == EOPSMode::ACTIVE && (bAutoLand || (navsys && navsys->HasReachedRTB()))))) return false;
    FlightDeck* Deck = inbound->GetDeck();
    const Ship* Carrier = Deck ? Deck->GetCarrier() : nullptr;
    return Carrier && !Carrier->IsDead() && !Carrier->IsDying() && GetRegion() &&
        Carrier->GetRegion() == GetRegion() && Deck->IsRecoveryDeck() &&
        main_drive && main_drive->IsPowerOn() && flcs;
}

Instruction* Ship::GetPendingRTBWaypoint() const
{
    if (!element) return nullptr;
    ListIter<Instruction> Route = element->GetFlightPlan();
    while (++Route)
        if (Route->GetAction() == EInstruction::RTB && Route->GetStatus() <= INSTRUCTION_STATUS::ACTIVE)
            return Route.value();
    return nullptr;
}

bool Ship::CanRequestAutoLand() const
{
    if (CanAutoLand()) return true;
    return sim && sim->GetPlayerShip() == this && IsDropship() && !IsDead() && !IsDying() &&
        !InTransition() && !net_control && navsys && navsys->IsPowerOn() && GetPendingRTBWaypoint() &&
        (flight_phase == EOPSMode::ACTIVE || flight_phase == EOPSMode::APPROACH);
}

bool Ship::SetAutoLand(bool engage)
{
    if (engage)
    {
        if (bAutoLand || (navsys && navsys->IsAutoLandRTB() && !navsys->HasReachedRTB()))
            return true;

        // Nearby arrivals keep the assigned approach. Distant arrivals fly the RTB leg first.
        constexpr double DirectRecoveryRange = 20000.0;
        const FlightDeck* Deck = inbound ? inbound->GetDeck() : nullptr;
        const Ship* Home = Deck ? Deck->GetCarrier() : GetController();
        const bool bDistant = !Home || Home->GetRegion() != GetRegion() ||
            FVector::DistSquared(Home->GetLocation(), GetLocation()) > FMath::Square(DirectRecoveryRange);
        if (navsys && navsys->IsPowerOn() && !navsys->HasReachedRTB() &&
            (bDistant || !CanAutoLand()) && CanRequestAutoLand() && GetPendingRTBWaypoint())
        {
            navsys->SelectRTBForAutoLand(true);
            if (GetNextNavPoint())
            {
                navsys->EngageAutoNav();
                bAutoTrack = false;
                bManualThrottle = false;
                SetControls(sim->GetControls());
                RadioView::Message("Autoland engaged. Flying to the return waypoint.");
                return true;
            }
            navsys->SelectRTBForAutoLand(false);
        }
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
        bAutoTrack = false;
        bAutoLand = true;
        director_info = "Autoland";
        RadioView::Message("Autoland engaged. Following assigned recovery approach.");
        UE_LOG(LogTemp, Display, TEXT("[AutoLand] Engaged ship='%hs' deck='%hs'"),
            GetName(), inbound->GetDeck()->GetName());
        return true;
    }
    // Once the deck has captured the aircraft, it owns the docking motion.
    if (flight_phase >= EOPSMode::DOCKING) return false;
    if (IsAutoLandEngaged())
    {
        if (navsys && navsys->IsAutoLandRTB()) navsys->DisengageAutoNav();
        bAutoLand = false;
        if (inbound) inbound->SetFinal(false);
        EnableManualFlight();
        RadioView::Message("Autoland disengaged. You have control.");
        UE_LOG(LogTemp, Display, TEXT("[AutoLand] Disengaged ship='%hs'"), GetName());
    }
    return true;
}
