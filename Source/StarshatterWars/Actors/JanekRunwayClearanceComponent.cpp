#include "JanekRunwayClearanceComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Async/Async.h"

UJanekRunwayClearanceComponent::UJanekRunwayClearanceComponent()
{ PrimaryComponentTick.bCanEverTick=false; }
void UJanekRunwayClearanceComponent::OnRegister()
{
    Super::OnRegister();
    ChangeHandle=FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddUObject(this,&UJanekRunwayClearanceComponent::InstancesChanged);
}
void UJanekRunwayClearanceComponent::OnUnregister()
{
    FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(ChangeHandle);
    Pending.Reset();bActive=false;
    Super::OnUnregister();
}
void UJanekRunwayClearanceComponent::Protect(const FVector& Site,double HalfWidth)
{
    ProtectedSite=Site;ProtectedHalfWidth=HalfWidth;bActive=true;
    TArray<UInstancedStaticMeshComponent*> Meshes;
    if(GetOwner())GetOwner()->GetComponents<UInstancedStaticMeshComponent>(Meshes);
    for(auto* Mesh:Meshes)Pending.Add(Mesh);
    FlushPending();
}
void UJanekRunwayClearanceComponent::StopProtection(){bActive=false;Pending.Reset();}
int32 UJanekRunwayClearanceComponent::SuppressIntrudingInstances(UInstancedStaticMeshComponent& Mesh,const FVector& Site,double HalfWidth)
{
    if(!Mesh.GetStaticMesh())return 0;
    TArray<int32> Remove;
    const FBox Local=Mesh.GetStaticMesh()->GetBoundingBox();
    for(int32 Index=0;Index<Mesh.GetInstanceCount();++Index)
    {
        FTransform Pose;
        if(!Mesh.GetInstanceTransform(Index,Pose,true) || Pose.GetScale3D().IsNearlyZero())continue;
        const FBox Bounds=Local.TransformBy(Pose);
        // Check the entire rock/tree, not just its origin. Large instances can
        // originate outside a spawn mask and still cover the runway.
        if(Bounds.Min.X<=Site.X+HalfWidth && Bounds.Max.X>=Site.X-HalfWidth &&
           Bounds.Min.Y<=Site.Y+HalfWidth && Bounds.Max.Y>=Site.Y-HalfWidth)
            Remove.Add(Index);
    }
    // Keep the generator's instance indices stable. Suppression avoids
    // invalidating WorldScape's own cached sector-to-instance mappings.
    for(int32 Index:Remove)
    {
        FTransform Pose;
        Mesh.GetInstanceTransform(Index,Pose,true);
        Pose.SetScale3D(FVector::ZeroVector);
        Mesh.UpdateInstanceTransform(Index,Pose,true,false,true);
    }
    if(!Remove.IsEmpty())Mesh.MarkRenderStateDirty();
    return Remove.Num();
}
void UJanekRunwayClearanceComponent::InstancesChanged(UInstancedStaticMeshComponent* Mesh,TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData>)
{
    if(!bActive || bClearing || !IsValid(Mesh) || Mesh->GetOwner()!=GetOwner())return;
    Pending.Add(Mesh);
    if(bScheduled)return;
    bScheduled=true;
    const TWeakObjectPtr<UJanekRunwayClearanceComponent> WeakThis(this);
    // Wait until the generator finishes publishing its instance indices.
    AsyncTask(ENamedThreads::GameThread,[WeakThis](){if(auto* Self=WeakThis.Get())Self->FlushPending();});
}
void UJanekRunwayClearanceComponent::FlushPending()
{
    bScheduled=false;
    if(!bActive){Pending.Reset();return;}
    bClearing=true;
    int32 Removed=0;
    for(const auto& WeakMesh:Pending)
        if(auto* Mesh=WeakMesh.Get())Removed+=SuppressIntrudingInstances(*Mesh,ProtectedSite,ProtectedHalfWidth);
    Pending.Reset();bClearing=false;
    if(Removed)UE_LOG(LogTemp,Display,TEXT("[JanekRunwayClearance] Suppressed %d overlapping rock/vegetation instances"),Removed);
}
