// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_FCP_CollisionComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"


// ---------------------------------------------------
UWorldScape_FCP_CollisionComponent::UWorldScape_FCP_CollisionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	SetVisibleFlag(false);
	SetGenerateOverlapEvents(false);
	bWorldPositionOffsetWritesVelocity = false;
	bVisibleInReflectionCaptures = false;
	bVisibleInSceneCaptureOnly = false;
	bVisibleInRealTimeSkyCaptures = false;
	bReceivesDecals = false;
	bRenderInMainPass = false;
	bRenderInDepthPass = false;
	bUseAsOccluder = false;
	bCastDistanceFieldIndirectShadow = false;
	SetCastShadow(false);
}
// ---------------------------------------------------
bool UWorldScape_FCP_CollisionComponent::FCP_IsReady()
{
	return IsReady;
}
// ---------------------------------------------------
void UWorldScape_FCP_CollisionComponent::FCP_MakeCollisionPooling( UStaticMesh* inStaticMesh , const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale )
{
	IsReady = true;
	StaticMeshBase = inStaticMesh; // Store Base StaticMesh
	if ( IsValid(StaticMeshBase) )
	{
		Compute_Internal_Data(StaticMeshBase, CollisionSettings, PoolScale);
	}
}
// ---------------------------------------------------
void UWorldScape_FCP_CollisionComponent::FCP_RemovePool()
{
	for ( int32 Index = 0 ; Index < PerInstanceSMData.Num() ; Index++ )
	{
		RemoveInstance(Index);
	}
}
// ---------------------------------------------------
int32 UWorldScape_FCP_CollisionComponent::CountWorkAvailable()
{
	return PerInstanceSMData.Num() - Work_Index;
}
// ---------------------------------------------------
bool UWorldScape_FCP_CollisionComponent::Update_InstanceTransform( const FTransform& NewTransform )
{
	if ( PerInstanceSMData.IsValidIndex(Work_Index) )
	{
		if ( IsValidInstance(Work_Index) )
		{
			UpdateInstanceTransform(Work_Index,NewTransform,true,false,true);
			Work_Index += 1;
			return Work_Index < PerInstanceSMData.Num();
		}
	}
	return Work_Index < PerInstanceSMData.Num();
}
// ---------------------------------------------------
void UWorldScape_FCP_CollisionComponent::Reset_Work_Index()
{
	Work_Index = 0;
}
// ---------------------------------------------------
bool UWorldScape_FCP_CollisionComponent::Get_WorkTO()
{
	return Work_Index < PerInstanceSMData.Num();
}
// ---------------------------------------------------
void UWorldScape_FCP_CollisionComponent::Compute_Internal_Data( UStaticMesh*& inStaticMesh ,const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale)
{
	if ( IsValid(this) )
	{
		if ( IsValid(inStaticMesh) )
		{
			SetStaticMesh(inStaticMesh);
			BodySetup = inStaticMesh->GetBodySetup();
			BasePoolSize = CollisionSettings.BasePoolSize * PoolScale;
			
			BodyInstance = CollisionSettings.BodyInstance; // Store in PrimitiveComponentClass
			QDC_BodyInstance = CollisionSettings.BodyInstance;
			QDC_BodyInstance.ActorHandle = DefaultPhysicsActorHandle();
			
			MinimalScale = CollisionSettings.MinimalScale;
			StaticMeshBoundsRadius = inStaticMesh->GetBounds().SphereRadius;
			Internal_Make_Pool();
		}
	}
}
// ---------------------------------------------------
void UWorldScape_FCP_CollisionComponent::Internal_Make_Pool()
{
	for ( int i = 0 ; i < BasePoolSize ; i++ )
	{
		AddInstance(GetComponentTransform(),true);
	}
	IsReady = true;
	CompReady.Broadcast(this);
}