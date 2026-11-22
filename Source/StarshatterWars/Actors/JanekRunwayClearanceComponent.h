#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InstancedStaticMeshDelegates.h"
#include "JanekRunwayClearanceComponent.generated.h"
class UInstancedStaticMeshComponent;

// Protect the runway from asynchronously generated rocks and vegetation.
UCLASS()
class STARSHATTERWARS_API UJanekRunwayClearanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UJanekRunwayClearanceComponent();
    void Protect(const FVector& Site, double HalfWidth);
    void StopProtection();
    static int32 SuppressIntrudingInstances(UInstancedStaticMeshComponent& Mesh,const FVector& Site,double HalfWidth);
protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
private:
    void InstancesChanged(UInstancedStaticMeshComponent* Mesh,TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Changes);
    void FlushPending();
    FDelegateHandle ChangeHandle;
    TSet<TWeakObjectPtr<UInstancedStaticMeshComponent>> Pending;
    FVector ProtectedSite=FVector::ZeroVector;
    double ProtectedHalfWidth=0;
    bool bActive=false;
    bool bClearing=false;
    bool bScheduled=false;
};
