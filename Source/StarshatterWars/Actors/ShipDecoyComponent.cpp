#include "ShipDecoyComponent.h"
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
UShipDecoyComponent::UShipDecoyComponent() { PrimaryComponentTick.bCanEverTick=false; }
bool UShipDecoyComponent::IsAvailable() const {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetDecoy():nullptr;
    return W && W->Enabled() && W->Ammo()!=0;
}
int32 UShipDecoyComponent::GetAmmo() const {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetDecoy():nullptr;
    return W?W->Ammo():0;
}
bool UShipDecoyComponent::Deploy() {
    Ship* S=BoundShip(this); Weapon* W=S?S->GetDecoy():nullptr;
    if(!S || !W || !S->GetRegion() || S->IsDead() || S->IsDying()) return false;
    return S->FireDecoy();
}
int32 UShipDecoyComponent::GetActiveCount() const {
    Ship* S=BoundShip(this); if(!S) return 0;
    int32 Count=0;
    auto& Active=S->GetActiveDecoys();
    for(int I=0;I<Active.size();++I) if(Active[I] && Active[I]->GetLife()>0) ++Count;
    return Count;
}
