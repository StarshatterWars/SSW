#include "ShipProbeComponent.h"
#include "ShipActor.h"
#include "Ship.h"
#include "Weapon.h"
#include "Drone.h"
namespace {
Ship* BoundShip(const UActorComponent* Component) {
    auto* Actor=Cast<AShipActor>(Component->GetOwner());
    return Actor ? Actor->GetRuntimeShip() : nullptr;
}
}
UShipProbeComponent::UShipProbeComponent() { PrimaryComponentTick.bCanEverTick=false; }
bool UShipProbeComponent::IsAvailable() const {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetProbeLauncher():nullptr;
    return W && W->Enabled() && W->Ammo()!=0;
}
int32 UShipProbeComponent::GetAmmo() const {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetProbeLauncher():nullptr;
    return W?W->Ammo():0;
}
bool UShipProbeComponent::Deploy() {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetProbeLauncher():nullptr;
    if(!S || !W || !S->GetRegion() || S->IsDead() || S->IsDying()) return false;
    Drone* Previous=S->GetProbe();
    const int Before=W->Ammo();
    S->LaunchProbe();
    return S->GetProbe() && (S->GetProbe()!=Previous || W->Ammo()!=Before);
}
int32 UShipProbeComponent::GetActiveCount() const {
    Ship* S=BoundShip(this); if(!S) return 0;
    return S->GetProbe() && S->GetProbe()->GetLife()>0 ? 1 : 0;
}
