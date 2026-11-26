#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "RepairAI.h"
#include "FleetRepairSubsystem.generated.h"

// Adapter only: native RepairAI owns assessment, work orders and supply debit.
// Wakes on fleet/supply changes or the next work milestone, never every frame.
UCLASS()
class STARSHATTERWARS_API UFleetRepairSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase&) override;
 virtual void Deinitialize() override;
 void Request();
private:
 bool Process(float);
 void OnEconomyChanged(const struct FEconomyState&);
 uint32 SupplyFingerprint(const struct FEconomyState&) const;
 FFleetRepairPolicy Policy;
 bool bPolicyValid=false;
 bool bProcessing=false;
 uint32 LastSupplyFingerprint=0;
 FDelegateHandle FleetHandle,EconomyHandle;
 FTSTicker::FDelegateHandle WakeHandle;
};
