// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#include "WorldScapeRoot.h"
#include "Async/Async.h"


// ----------------------------------------------------------------
void AWorldScapeRoot::AllFoliageToOverrideList()
{
	for ( auto& Fol : Foliages )
	{
		if ( IsValid(Fol) )
		{
			for (auto& Sub : Fol->FoliageList )
			{
				if ( IsValid(Sub) )
				{
					if ( UWorldScapeFoliagesAsset* FoliageAsset = Cast<UWorldScapeFoliagesAsset>(Sub) )
					{
						if ( IsValid(FoliageAsset->StaticMesh) )
						{
							if ( FCP_ComputeNearFar_Asset( FoliageAsset ) )
							{
								FCP_FarData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
							}
							else
							{
								FCP_NearData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
							}
						}
					}
					else if ( UWorldScapeFoliagesCluster* FoliageCluster = Cast<UWorldScapeFoliagesCluster>(Sub) )
					{
						for ( auto& SubCluster : FoliageCluster->FoliagesClusterUnitList )
						{
							if ( IsValid(SubCluster.StaticMesh) )
							{
								if ( FCP_ComputeNearFar_Cluster( SubCluster , FoliageCluster->FoliageSectorSize ) )
								{
									FCP_FarData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
								}
								else
								{
									FCP_NearData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
								}
							}
						}
					}
				}
			}
		}
	}
}
// ----------------------------------------------------------------
void AWorldScapeRoot::AllFoliageWithCollisionToOverrideList()
{
	for ( auto& Fol : Foliages )
	{
		if ( IsValid(Fol) )
		{
			for (auto& Sub : Fol->FoliageList )
			{
				if ( IsValid(Sub) )
				{
					if ( UWorldScapeFoliagesAsset* FoliageAsset = Cast<UWorldScapeFoliagesAsset>(Sub) )
					{
						if ( IsValid(FoliageAsset->StaticMesh) && FoliageAsset->bCollision )
						{
							if ( FCP_ComputeNearFar_Asset( FoliageAsset ) )
							{
								FCP_FarData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
							}
							else
							{
								FCP_NearData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
							}
						}
					}
					else if ( UWorldScapeFoliagesCluster* FoliageCluster = Cast<UWorldScapeFoliagesCluster>(Sub) )
					{
						for ( auto& SubCluster : FoliageCluster->FoliagesClusterUnitList )
						{
							if ( IsValid(SubCluster.StaticMesh) && SubCluster.bCollision )
							{
								if ( FCP_ComputeNearFar_Cluster( SubCluster , FoliageCluster->FoliageSectorSize ) )
								{
									FCP_FarData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
								}
								else
								{
									FCP_NearData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
								}
							}
						}
					}
				}
			}
		}
	}
}
// ----------------------------------------------------------------
void AWorldScapeRoot::WS_FCP_Init()
{
	if ( FoliageCollisionPooling_Enabled )
	{
		if ( UWorld* World = GetWorld() )
		{
			if ( World->GetNetMode() == NM_Client || World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_ListenServer )
			{
				FCP_Build_InternalData();
				FCP_Internal_ValidSquaredDistance = FMath::Square(FCP_ActiveDistance);
				WS_FCP_IsInit = true;
			}
		}
	}
}
// ----------------------------------------------------------------
void AWorldScapeRoot::WS_FCP_Clear()
{
	FCP_StopSystem();
	FCP_RemovePooling();
	FCP_ClearInternal_Data();
	WS_FCP_IsInit = false;
}
// ----------------------------------------------------------------
void AWorldScapeRoot::WS_FCP_TickUpdate( const float DeltaSeconds )
{
	if ( FoliageCollisionPooling_Enabled && WS_FCP_IsInit )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_TickUpdate);
		FCP_Calculate_Velocity( DeltaSeconds );
		WS_FCP_ComputeDistance();
		if ( WS_FCP_OnValidDistance )
		{
			if ( !WS_FCP_IsRunning )
			{
				FCP_InitPooling();
				FCP_StartSystem();
				FCP_Internal_TimeToClearCount = 0.0f;
				WS_FCP_IsRunning = true;
			}
			if ( FCP_Update_MultiFrame && IsValid(this))
			{
				FCP_Update_PoolOnTick();
			}
		}
		else
		{
			if ( WS_FCP_IsRunning )
			{
				FCP_Internal_TimeToClearCount += DeltaSeconds;
				if ( FCP_Internal_TimeToClearCount > FCP_AutoClear_Time )
				{
					FCP_StopSystem();
					FCP_RemovePooling();
					FCP_Internal_TimeToClearCount = 0.0f;
					WS_FCP_IsRunning = false;
				}
			}
		}
	}
}
// ----------------------------------------------------------------
void AWorldScapeRoot::WS_FCP_ComputeDistance()
{
	FCP_Internal_Distance = GetPawnDistanceFromGround(FCP_CameraLocation);
	WS_FCP_OnValidDistance = FCP_Internal_Distance < FCP_ActiveDistance;
}
// ----------------------------------------------------------------
void AWorldScapeRoot::FCP_ClearInternal_Data()
{
	FCP_Internal_NearData.Empty();
	FCP_Internal_FarData.Empty();
}
// ----------------------------------------------------------------
void AWorldScapeRoot::FCP_AddDataFromFoliage()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_AddDataFromFoliage);
	for ( auto& Fol : Foliages )
	{
		if ( IsValid(Fol) )
		{
			for (auto& Sub : Fol->FoliageList )
			{
				if ( IsValid(Sub) )
				{
					if ( UWorldScapeFoliagesAsset* FoliageAsset = Cast<UWorldScapeFoliagesAsset>(Sub) )
					{
						if ( IsValid(FoliageAsset->StaticMesh) && FoliageAsset->bCollision )
						{
							if ( !FCP_Internal_NearData.Contains(FoliageAsset->StaticMesh) && !FCP_Internal_FarData.Contains(FoliageAsset->StaticMesh) )
							{
								if ( FCP_ComputeNearFar_Asset( FoliageAsset ) )
								{
									FCP_Internal_FarData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
								}
								else
								{
									FCP_Internal_NearData.Add(FoliageAsset->StaticMesh,FoliageAsset->CollisionPooling_Data);
								}
							}
						}
					}
					else if ( UWorldScapeFoliagesCluster* FoliageCluster = Cast<UWorldScapeFoliagesCluster>(Sub) )
					{
						for ( auto& SubCluster : FoliageCluster->FoliagesClusterUnitList )
						{
							if ( IsValid(SubCluster.StaticMesh) && SubCluster.bCollision )
							{
								if ( !FCP_Internal_NearData.Contains(SubCluster.StaticMesh) && !FCP_Internal_FarData.Contains(SubCluster.StaticMesh) )
								{
									if ( FCP_ComputeNearFar_Cluster( SubCluster , FoliageCluster->FoliageSectorSize ) )
									{
										FCP_Internal_FarData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
									}
									else
									{
										FCP_Internal_NearData.Add(SubCluster.StaticMesh,SubCluster.CollisionPooling_Data);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
// ----------------------------------------------------------------
bool AWorldScapeRoot::FCP_ComputeNearFar_Asset( const UWorldScapeFoliagesAsset* FoliageAsset )
{
	if ( IsValid( FoliageAsset ) )
	{
		switch ( FoliageAsset->CollisionPooling_ComputeType )
		{
		case EWS_FCP_TypeCompute::Auto:
			{
				return FoliageAsset->FoliageSectorSize > FCP_AutoComputeFar_SectorSizeLimite;
			}
		case EWS_FCP_TypeCompute::Near:
			{
				return false;
			}
		case EWS_FCP_TypeCompute::Far:
			{
				return true;
			}
		}
	}
	return false;
}
// ----------------------------------------------------------------
bool AWorldScapeRoot::FCP_ComputeNearFar_Cluster( const FWorldScapeFoliagesClusterUnit& FoliageClusterUnit , double SectorSize )
{
	switch ( FoliageClusterUnit.CollisionPooling_ComputeType )
	{
	case EWS_FCP_TypeCompute::Auto:
		{
			return SectorSize > FCP_AutoComputeFar_SectorSizeLimite;
		}
	case EWS_FCP_TypeCompute::Near:
		{
			return false;
		}
	case EWS_FCP_TypeCompute::Far:
		{
			return true;
		}
	}
	return false;
}
// ----------------------------------------------------------------
void AWorldScapeRoot::FCP_Build_InternalData()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Build_InternalData);
	FCP_ClearInternal_Data();
	// Add default data
	for ( auto& AdditionalNear : FCP_NearData )
	{
		if ( IsValid(AdditionalNear.Key) )
		{
			FCP_Internal_NearData.Add(AdditionalNear.Key,AdditionalNear.Value);
		}
	}
	for ( auto& AdditionalFar : FCP_FarData )
	{
		if ( IsValid(AdditionalFar.Key) )
		{
			FCP_Internal_FarData.Add(AdditionalFar.Key,AdditionalFar.Value);
		}
	}
	// Add Data from foliage list
	FCP_AddDataFromFoliage();
}
// ----------------------------------------------------------------

// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_PoolOnTick()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_PoolOnTick);
	const FVector PredicateLocation = FCP_CameraLocation + (FCP_Interp_Velocity * FCP_PredicateVelocityScale);
	// Near
	TArray<UStaticMesh*> NearRemove;
	int Near_Index = 0;
	const double Near_StartTime = FPlatformTime::Seconds();
	for ( const auto& Near : FCP_NearCollection )
	{
		FCP_Update_Collision_SingleMeshCollection( Near.Key , Near.Value.ISM , PredicateLocation , FCP_Near_CheckSize , FCP_NearComponents );
		//QDC_Update_Collision( Near.Value.ISM , PredicateLocation , Near_CheckSize , NearComponents );
		NearRemove.Add(Near.Key);
		Near_Index++;
		const double Near_CurrentTime = FPlatformTime::Seconds();
		if ( Near_CurrentTime - Near_StartTime >= FCP_Near_UpdateTimeLimit )
		{
			break;
		}
		if ( Near_Index > FCP_Near_UpdateCollectionPerFrame )
		{
			break;
		}
	}
	for ( const auto& Rnear : NearRemove )
	{
		FCP_NearCollection.Remove( Rnear );
	}
	// Far
	TArray<UStaticMesh*> FarRemove;
	int Far_Index = 0;
	const double Far_StartTime = FPlatformTime::Seconds();
	for ( const auto& Far : FCP_FarCollection )
	{
		FCP_Update_Collision_SingleMeshCollection( Far.Key , Far.Value.ISM , PredicateLocation , FCP_Near_CheckSize , FCP_FarComponents );
		//QDC_Update_Collision( Far.Value.ISM , PredicateLocation , Far_CheckSize , FarComponents );
		FarRemove.Add(Far.Key);
		Far_Index++;
		const double Far_CurrentTime = FPlatformTime::Seconds();
		if ( Far_CurrentTime - Far_StartTime >= FCP_Far_UpdateTimeLimit )
		{
			break;
		}
		if ( Far_Index > FCP_Far_UpdateCollectionPerFrame )
		{
			break;
		}
	}
	for ( const auto& Rfar : FarRemove )
	{
		FCP_FarCollection.Remove( Rfar );
	}
}
// ---------------------------------------------------
bool AWorldScapeRoot::FCP_InitPooling()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_InitPooling);
	if ( !IsValid(FCP_CollisionActor) )
	{
		FCP_CollisionActor = FCP_CreateActor();
		if ( IsValid(FCP_CollisionActor) )
		{
			// Make Near
			for ( auto& Data : FCP_Internal_NearData )
			{
				FWS_FCP_PoolingData CollisionSettings = Data.Value;
				if ( FCP_Override_CollisionSetup )
				{
					CollisionSettings.BodyInstance = FCP_CollisionSetup;
				}
				if ( FCP_OverridePoolSize )
				{
					CollisionSettings.BasePoolSize = FCP_PoolSize;
				}
				if ( FCP_OverrideMinimalScale )
				{
					CollisionSettings.MinimalScale = FCP_MinimalScale;
				}
				if ( UWorldScape_FCP_CollisionComponent* Comp = FCP_CollisionActor->FCP_CollisionComponent_Create(Data.Key,CollisionSettings,FCP_Near_PoolSize_Scale) )
				{
					if ( Comp->FCP_IsReady() )
					{
						FCP_ComponentReady( Comp );
					}
					else
					{
						Comp->CompReady.AddDynamic(this , &AWorldScapeRoot::FCP_ComponentReady );
					}
				}
			}
			FCP_Near_Valid = true;
			// Make Far
			for ( auto& Data : FCP_Internal_FarData )
			{
				FWS_FCP_PoolingData CollisionSettings = Data.Value;
				if ( FCP_Override_CollisionSetup )
				{
					CollisionSettings.BodyInstance = FCP_CollisionSetup;
				}
				if ( FCP_OverridePoolSize )
				{
					CollisionSettings.BasePoolSize = FCP_PoolSize;
				}
				if ( FCP_OverrideMinimalScale )
				{
					CollisionSettings.MinimalScale = FCP_MinimalScale;
				}
				if ( UWorldScape_FCP_CollisionComponent* Comp = FCP_CollisionActor->FCP_CollisionComponent_Create(Data.Key,CollisionSettings,FCP_Far_PoolSize_Scale) )
				{
					if ( Comp->FCP_IsReady() )
					{
						FCP_ComponentReady( Comp );
					}
					else
					{
						Comp->CompReady.AddDynamic(this , &AWorldScapeRoot::FCP_ComponentReady );
					}
				}
			}
			FCP_Far_Valid = true;
		}
		return true;
	}
	return false;
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_ComponentReady( UWorldScape_FCP_CollisionComponent* Comp )
{
	if ( IsValid(Comp) )
	{
		if ( FCP_Internal_NearData.Contains(Comp->StaticMeshBase) )
		{
			FCP_NearComponents.Add(Comp->GetStaticMesh(),Comp);
			return;
		}
		if ( FCP_Internal_FarData.Contains(Comp->StaticMeshBase) )
		{
			FCP_FarComponents.Add(Comp->GetStaticMesh(),Comp);
		}
	}
}
// ---------------------------------------------------
bool AWorldScapeRoot::FCP_RemovePooling()
{
	if ( IsValid(FCP_CollisionActor) )
	{
		FCP_StopSystem();
		FCP_CollisionActor->FCP_CollisionComponent_Destroy();
		FCP_CollisionActor->Destroy(false,false);
		FCP_NearComponents.Empty();
		FCP_FarComponents.Empty();
		return true;
	}
	FCP_NearCollection.Empty();
	FCP_FarCollection.Empty();
	return false;
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_StartSystem()
{
	if ( FCP_Near_Valid )
	{
		FCP_Start_Near_Timer();
		FCP_Start_CheckUpdate_Timer_Near();
	}
	if ( FCP_Far_Valid )
	{
		FCP_Start_Far_Timer();
		FCP_Start_CheckUpdate_Timer_Far();
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_StopSystem()
{
	FCP_Stop_Near_Timer();
	FCP_Stop_Far_Timer();
	FCP_Stop_CheckUpdate_Timer_Near();
	FCP_Stop_CheckUpdate_Timer_Far();
}
// ---------------------------------------------------
AWorldScape_FCP_CollisionActor* AWorldScapeRoot::FCP_CreateActor()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_CreateActor);
	if ( !IsValid(FCP_CollisionActor) )
	{
		FVector LocalLocation = GetActorLocation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this; 
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags = RF_Transient;
		
		if ( UWorld* World = GetWorld() )
		{
			FCP_CollisionActor = World->SpawnActor<AWorldScape_FCP_CollisionActor>(AWorldScape_FCP_CollisionActor::StaticClass(), LocalLocation, FRotator::ZeroRotator, SpawnParams);
			if (IsValid(FCP_CollisionActor))
			{
				FCP_CollisionActor->SetFlags(RF_Transient);
				FCP_CollisionActor->Tags = FCP_TagsCollisionActor; // Add Tags in collision actor.
				return FCP_CollisionActor;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("QDC : Failed to spawn Collision Actor"));
				return nullptr;
			}
		}
	}
	return nullptr;
}
// ---------------------------------------------------
FVector AWorldScapeRoot::FCP_GetLocalPlayerCameraLocation()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		return PlayerController->PlayerCameraManager->GetCameraLocation();
	}
	return FVector::ZeroVector;
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Calculate_Velocity( const double DeltaTime )
{
	FCP_CameraLocation = FCP_GetLocalPlayerCameraLocation();
	FCP_RawCalculateVelocity = (FCP_CameraLocation - FCP_LastLocationFrame) / DeltaTime;
	FCP_Interp_Velocity = FMath::VInterpTo(FCP_Interp_Velocity, FCP_RawCalculateVelocity, DeltaTime, FCP_VelocityInterpSpeed);
	FCP_LastLocationFrame = FCP_CameraLocation;
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Start_Near_Timer()
{
	FCP_Stop_Near_Timer();
	GetWorld()->GetTimerManager().SetTimer(FCP_Near_TimerHandle, this,&AWorldScapeRoot::FCP_Update_Near,FCP_Near_Minimal_AutoUpdateTime, true);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Stop_Near_Timer()
{
	if ( FCP_Near_TimerHandle.IsValid() )
	{
		GetWorld()->GetTimerManager().ClearTimer(FCP_Near_TimerHandle);
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Start_Far_Timer()
{
	FCP_Stop_Far_Timer();
	GetWorld()->GetTimerManager().SetTimer(FCP_Far_TimerHandle, this,&AWorldScapeRoot::FCP_Update_Far,FCP_Far_Minimal_AutoUpdateTime, true);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Stop_Far_Timer()
{
	if ( FCP_Far_TimerHandle.IsValid() )
	{
		GetWorld()->GetTimerManager().ClearTimer(FCP_Far_TimerHandle);
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Start_CheckUpdate_Timer_Near()
{
	FCP_Stop_CheckUpdate_Timer_Near();
	GetWorld()->GetTimerManager().SetTimer(FCP_CheckNeedUpdate_Near, this,&AWorldScapeRoot::FCP_GetUpdate_Near,FCP_Near_CheckUpdateTime, true);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Stop_CheckUpdate_Timer_Near()
{
	if ( FCP_CheckNeedUpdate_Near.IsValid() )
	{
		GetWorld()->GetTimerManager().ClearTimer(FCP_CheckNeedUpdate_Near);
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Start_CheckUpdate_Timer_Far()
{
	FCP_Stop_CheckUpdate_Timer_Far();
	GetWorld()->GetTimerManager().SetTimer(FCP_CheckNeedUpdate_Far, this,&AWorldScapeRoot::FCP_GetUpdate_Far,FCP_Far_CheckUpdateTime, true);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Stop_CheckUpdate_Timer_Far()
{
	if ( FCP_CheckNeedUpdate_Far.IsValid() )
	{
		GetWorld()->GetTimerManager().ClearTimer(FCP_CheckNeedUpdate_Far);
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_Near()
{
	const FVector PredicateLocation = FCP_CameraLocation + (FCP_Interp_Velocity * FCP_PredicateVelocityScale);
	FCP_LastNearUpdateLocation = PredicateLocation;
	FCP_Update(PredicateLocation,FCP_Near_CheckSize,FCP_NearComponents , FCP_NearCollection);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_Far()
{
	const FVector PredicateLocation = FCP_CameraLocation + (FCP_Interp_Velocity * FCP_PredicateVelocityScale);
	FCP_LastFarUpdateLocation = PredicateLocation;
	FCP_Update(PredicateLocation,FCP_Far_CheckSize,FCP_FarComponents , FCP_FarCollection);
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_GetUpdate_Near()
{
	const FVector PredicateLocation = FCP_CameraLocation + (FCP_Interp_Velocity * FCP_PredicateVelocityScale);
	if ( FCP_GetNeedUpdate(PredicateLocation,FCP_LastNearUpdateLocation,FCP_Near_CheckSize,FCP_Near_ForceUpdate_SizeScale) )
	{
		FCP_LastNearUpdateLocation = PredicateLocation;
		FCP_Update(PredicateLocation,FCP_Near_CheckSize,FCP_NearComponents , FCP_NearCollection);
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_GetUpdate_Far()
{
	const FVector PredicateLocation = FCP_CameraLocation + (FCP_Interp_Velocity * FCP_PredicateVelocityScale);
	if ( FCP_GetNeedUpdate(PredicateLocation,FCP_LastFarUpdateLocation,FCP_Far_CheckSize,FCP_Far_ForceUpdate_SizeScale) )
	{
		FCP_LastFarUpdateLocation = PredicateLocation;
		FCP_Update(PredicateLocation,FCP_Far_CheckSize,FCP_FarComponents , FCP_FarCollection);
	}
}
// ---------------------------------------------------
bool AWorldScapeRoot::FCP_GetNeedUpdate( const FVector& NewLocation, const FVector& LastLocation , const double Radius , const double ForceUpdateScale )
{
	if ( (LastLocation-NewLocation).SquaredLength() > FMath::Square( Radius*ForceUpdateScale ) )
	{
		return true;
	}
	return false;
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update( const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling , TMap<UStaticMesh*,FWSFCP_ISM_Collection>& Collection)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Update);
	if ( !Pooling.IsEmpty() )
	{
		TArray<UInstancedStaticMeshComponent*> ISM_Components = FCP_GetISMComponents_InActor( Location ,Radius , Pooling , Collection );
		if ( !ISM_Components.IsEmpty() )
		{
			for ( const auto& ISM : Pooling )
			{
				ISM.Value->Reset_Work_Index();
			}
			if ( !FCP_Update_MultiFrame )
			{
				FCP_Update_Collision( ISM_Components , Location , Radius , Pooling );
			}
		}
	}
}
// ---------------------------------------------------
TArray<UInstancedStaticMeshComponent*> AWorldScapeRoot::FCP_GetISMComponents_InActor( const FVector& Location ,const double GetRadius , const TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling , TMap<UStaticMesh*,FWSFCP_ISM_Collection>& Collection )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_GetISMComponents_InActor);
	if ( IsValid(this) && IsValid(FCP_CollisionActor) && !Pooling.IsEmpty() )
	{
		TArray<UInstancedStaticMeshComponent*> ISM_Components_Out;
		TArray<UInstancedStaticMeshComponent*> ISM_Components;
		GetComponents(UInstancedStaticMeshComponent::StaticClass(), ISM_Components);
		if ( !ISM_Components.IsEmpty() )
		{
			for ( const auto& ISM : ISM_Components )
			{
				if ( IsValid(ISM) && IsValid(ISM->GetStaticMesh()) )
				{
					if ( Pooling.Contains(ISM->GetStaticMesh()) ) // Get is valid SM for pool
					{
						if ( (Location-ISM->Bounds.Origin-GetRadius).SquaredLength() < FMath::Square(ISM->Bounds.SphereRadius*FCP_BoundScale) ) // Filter by bound distance intersect
						{
							ISM_Components_Out.Add(ISM);
							if ( FCP_Update_MultiFrame )
							{
								if ( FWSFCP_ISM_Collection* Found = Collection.Find(ISM->GetStaticMesh()) )
								{
									Found->ISM.Add(ISM);
								}
								else
								{
									Collection.Add(ISM->GetStaticMesh(),FWSFCP_ISM_Collection(ISM));
								}
							}
						}
					}
				}
			}
		}
		
		return ISM_Components_Out;
	}
	return TArray<UInstancedStaticMeshComponent*>();
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_Collision( const TArray<UInstancedStaticMeshComponent*>& Instances , const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Update_Collision);
	if ( IsValid(this) && IsValid(FCP_CollisionActor) && !Pooling.IsEmpty() && !Instances.IsEmpty() )
	{
		for ( const auto& Instance : Instances )
		{
			if ( UWorldScape_FCP_CollisionComponent** CollisionComponent = Pooling.Find(Instance->GetStaticMesh()) )
			{
				if ( IsValid(*CollisionComponent) )
				{
					if ( (*CollisionComponent)->Get_WorkTO() )
					{
						const float SM_Bound = Instance->GetStaticMesh()->GetBounds().SphereRadius;
						for ( const auto& ISMData : Instance->PerInstanceSMData )
						{
							const FTransform WorldTransform = FTransform(ISMData.Transform) * Instance->GetComponentTransform();
							if ( !WorldTransform.GetScale3D().IsNearlyZero() || !(WorldTransform.GetScale3D().GetAbsMin() < (*CollisionComponent)->MinimalScale) )
							{
								if ( ((Location-WorldTransform.GetLocation()).SquaredLength() - FMath::Square((WorldTransform.GetScale3D().GetAbsMax() * SM_Bound) + Radius)) < 0.0 )
								{
									if ( !(*CollisionComponent)->Update_InstanceTransform(WorldTransform) )
									{
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_Collision_SingleMeshCollection( const UStaticMesh* StaticMesh , const TArray<UInstancedStaticMeshComponent*>& Instances , const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Update_Collision_SingleMeshCollection);
	if ( IsValid(this) && IsValid(FCP_CollisionActor) && !Pooling.IsEmpty() && !Instances.IsEmpty() )
	{
		if ( UWorldScape_FCP_CollisionComponent** CollisionComponent = Pooling.Find(StaticMesh) )
		{
			if ( IsValid(*CollisionComponent) )
			{
				if ( (*CollisionComponent)->Get_WorkTO() )
				{
					if ( bFCP_Update_Async )
					{
						if ( !FCP_Pool_InAsync.Contains(StaticMesh) )
						{
							FCP_Update_Async( Instances , *CollisionComponent , Location , Radius );
						}
						return;
					}
					const float SM_Bound = StaticMesh->GetBounds().SphereRadius;
					for ( const auto& Instance : Instances )
					{
						for ( const auto& ISMData : Instance->PerInstanceSMData )
						{
							const FTransform WorldTransform = FTransform(ISMData.Transform) * Instance->GetComponentTransform();
							if ( !WorldTransform.GetScale3D().IsNearlyZero() || !(WorldTransform.GetScale3D().GetAbsMin() < (*CollisionComponent)->MinimalScale) )
							{
								if ( ((Location-WorldTransform.GetLocation()).SquaredLength() - FMath::Square((WorldTransform.GetScale3D().GetAbsMax() * SM_Bound) + Radius)) < 0.0 )
								{
									if ( !(*CollisionComponent)->Update_InstanceTransform(WorldTransform) )
									{
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
// ---------------------------------------------------
void AWorldScapeRoot::FCP_Update_Async( const TArray<UInstancedStaticMeshComponent*>& Instances , UWorldScape_FCP_CollisionComponent* PoolISM , const FVector& Location , const double Radius )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Update_Async);
	if ( IsValid(this) && IsValid(FCP_CollisionActor) && !Instances.IsEmpty() )
	{
		if ( PoolISM->Get_WorkTO() )
		{
			const float SM_Bound = PoolISM->GetStaticMesh()->GetBounds().SphereRadius;
			TArray<FWSFCP_AsyncData> InstanceData;
			for ( const auto& Instance : Instances )
			{
				if ( IsValid(Instance) )
				{
					InstanceData.Add( FWSFCP_AsyncData(Instance->GetComponentTransform(),Instance->PerInstanceSMData) );
				}
			}
			FCP_Pool_InAsync.Add(PoolISM->GetStaticMesh());
			int32 SearchCount = PoolISM->CountWorkAvailable();
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ Data = MoveTemp(InstanceData) , PoolISM , Location , Radius , SM_Bound , SearchCount , this ]
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(WS_FCP_Update_Async);
				int32 Count = 0;
				TArray<FTransform> TransformsInstance;
				for ( const auto& ISM : Data )
				{
					for ( int32 Index = 0 ; Index < ISM.Instances.Num() ; Index++ )
					{
						const FTransform WorldTransform = FTransform(ISM.Instances[Index].Transform) * ISM.Transform;
						if ( !WorldTransform.GetScale3D().IsNearlyZero() )
						{
							if ( ((Location-WorldTransform.GetLocation()).SquaredLength() - FMath::Square((WorldTransform.GetScale3D().GetAbsMax() * SM_Bound) + Radius)) < 0.0 )
							{
								TransformsInstance.Add(WorldTransform);
								Count++;
								if ( Count > SearchCount )
								{
									break;
								}
							}
						}
					}
				}
				AsyncTask(ENamedThreads::GameThread, [ Data = MoveTemp(TransformsInstance) , PoolISM , this ]
				{
					if ( IsValid(this) )
					{
						for ( const auto& Instance : Data )
						{
							if ( !PoolISM->Update_InstanceTransform(Instance) )
							{
								break;
							}
						}
						FCP_Pool_InAsync.Remove(PoolISM->GetStaticMesh());
					}
				});
			});
		}
	}	
}