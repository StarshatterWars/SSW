#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShipStoreAI.h"
#include "ShipStoreSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UShipStoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    bool RefreshCatalog(FText& Reason);
    const TArray<FShipStoreOffer>& GetOffers() const {return Offers;}
    bool CanPurchase(FName Id,int32 Quantity,FText& Reason) const;
    bool Purchase(FName Id,int32 Quantity,int64 ExpectedTotal,FText& Reason);
    bool RequestPurchase(FName Id,int32 Quantity,int64 ExpectedTotal,TFunction<void(bool,const FText&)>&& Completed,FText& Reason);
    bool IsPurchasePending()const;
    int32 Stored(FName Id) const;
    int64 Balance() const;
    FString Dock() const;
    bool CanTransferMiningDrone(int32 Quantity,FText& Reason) const;
    bool TransferMiningDrone(int32 Quantity,FText& Reason);
private:
    TArray<FShipStoreOffer> Offers;
    bool bPurchasePending=false;
};
