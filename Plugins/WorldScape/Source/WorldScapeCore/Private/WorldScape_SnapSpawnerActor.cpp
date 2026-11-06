// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_SnapSpawnerActor.h"
#include "WorldScape_GridDataAsset.h"
#include "WorldScape_SnapDataAsset.h"
#include "Components/BillboardComponent.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/MemoryReader.h"

// ------------------------------------------------------
AWorldScape_SnapSpawnerActor::AWorldScape_SnapSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTickGroup(TG_PostUpdateWork);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->Mobility = EComponentMobility::Static;
	RootComponent->SetComponentTickEnabled(false);
#if WITH_EDITOR
	Billboard_Component = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard_Component->SetFlags(RF_Transient);
	Billboard_Component->SetComponentTickEnabled(false);
	Billboard_Component->bIsEditorOnly = true;
	Billboard_Component->SetGenerateOverlapEvents(false);
	Billboard_Component->SetMobility(EComponentMobility::Static);
	Billboard_Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Billboard_Component->SetCollisionProfileName(TEXT("NoCollision"));
	Billboard_Component->SetupAttachment(RootComponent);
	static const TCHAR* GLevelSpriteAssetName = TEXT("/WorldScape/Ressources/Textures/WorldScape128");
	Billboard_Component->SetSprite(LoadObject<UTexture2D>(this, GLevelSpriteAssetName));
	//Billboard_Component->RegisterComponent();
#endif
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
	if ( UWorld* World = GetWorld() )
	{
		if ( World->GetNetMode() == NM_DedicatedServer )
		{
			IsDedicated = true;
			if ( AutoGet_NearWS && SpawnOnDedicated )
			{
				AutoGetWS();
			}
			if ( AutoStartOnBegin && SpawnOnDedicated )
			{
				if ( AutoStart_Delay > 0.0f )
				{
					World->GetTimerManager().SetTimer(DeferredStartTimer , this , &AWorldScape_SnapSpawnerActor::BuildFromWS , AutoStart_Delay , false);
				}
				else
				{
					BuildFromWS();
				}
			}
		}
		else
		{
			if ( AutoGet_NearWS )
			{
				AutoGetWS();
			}
			if ( AutoStartOnBegin )
			{
				if ( AutoStart_Delay > 0.0f )
				{
					World->GetTimerManager().SetTimer(DeferredStartTimer , this , &AWorldScape_SnapSpawnerActor::BuildFromWS , AutoStart_Delay , false);
				}
				else
				{
					BuildFromWS();
				}
			}
		}
	}
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetPending();
	Clear_ActorList();
	Clear_StaticMeshList();
	Super::EndPlay(EndPlayReason);
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::BeginDestroy()
{
	ResetPending();
	Clear_ActorList();
	Clear_StaticMeshList();
	Super::BeginDestroy();
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	Update_Pending();
}
// ------------------------------------------------------
#if WITH_EDITOR
void AWorldScape_SnapSpawnerActor::ViewFlat()
{
	Clear();
	if ( Get_DataValid() )
	{
		SetSpawnData(ConvertDataToSpawnDirect( Get_SnapData() ));
	}
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::Simulate()
{
	Clear();
	if ( AutoGet_NearWS )
	{
		AutoGetWS();
	}
	if ( Get_DataValid() )
	{
		if ( IsValid(WorldScapeRoot) )
		{
			WorldScapeRoot->Snap_ComputeCollection_ForActor_Async(this);
		}
		else if ( AutoUseFlatviewIfWSFail )
		{
			SetSpawnData(ConvertDataToSpawnDirect( Get_SnapData() ));
		}
	}
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::Clear()
{
	ResetPending();
	Clear_ActorList();
	Clear_StaticMeshList();
}
#endif	
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	// Lock transform actor
	if ( AutoSimulateOnconstruct )
	{
		Simulate();
	}
#endif
}
// ------------------------------------------------------
FWS_SnapData AWorldScape_SnapSpawnerActor::Get_SnapData()
{
	switch ( DataType )
	{
	case EWSSnapDataType::ManualData:
		{
			return FWS_SnapData(Collection_ActorsDatas,Collection_StaticMeshDatas,Extent);
		}
	case EWSSnapDataType::SnapData:
		{
			if ( IsValid(SnapDataAsset) )
			{
				return FWS_SnapData(SnapDataAsset->Collection_ActorsDatas,SnapDataAsset->Collection_StaticMeshDatas,SnapDataAsset->Extent);
			}
			break;
		}
	case EWSSnapDataType::GridData:
		{
			if ( IsValid(GridDataAsset) )
			{
				return FWS_SnapData(GridDataAsset->Collection_ActorsDatas,GridDataAsset->Collection_StaticMeshDatas,GridDataAsset->Extent);
			}
			break;
		}
	}
	return FWS_SnapData();
}
// ------------------------------------------------------
bool AWorldScape_SnapSpawnerActor::Get_DataValid()
{
	switch ( DataType )
	{
	case EWSSnapDataType::ManualData:
		{
			return true;
		}
	case EWSSnapDataType::SnapData:
		{
			return IsValid(SnapDataAsset);
		}
	case EWSSnapDataType::GridData:
		{
			return IsValid(GridDataAsset);
		}
	}
	return false;
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::BuildFromWS()
{
	if ( IsValid(WorldScapeRoot) )
	{
		if ( WorldScapeRoot->CheckIsInitialized() )
		{
			WorldScapeRoot->Snap_ComputeCollection_ForActor_Async(this);
		}
		else
		{
			WorldScapeRoot->InitializedEnd.AddDynamic(this,&AWorldScape_SnapSpawnerActor::BuildFromWS);
		}
	}
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::AutoGetWS()
{
	if ( UWorld* World = GetWorld() )
	{
		if ( UWorldscapeSubsystem* SubSystem = World->GetSubsystem<UWorldscapeSubsystem>() )
		{
			WorldScapeRoot = SubSystem->Get_NearWorldScapeFromLocation(GetActorLocation(),WS_ValidTag,WS_ExcludeTag);
		}
	}
}
// ------------------------------------------------------
FWS_SnapSpawnData AWorldScape_SnapSpawnerActor::ConvertDataToSpawnDirect( const FWS_SnapData& Data )
{
	TArray<FWS_Grid_Actor_SpawnData> ActorData;
	TArray<FWS_Grid_Static_SpawnData> StaticMeshData;
	ActorData.Reserve(Data.Actors.Num());
	StaticMeshData.Reserve(Data.StaticMesh.Num());
	const FTransform LocalTransform = GetActorTransform();
	for ( auto& Actor : Data.Actors )
	{
		FVector NewLoaction = LocalTransform.TransformPosition(Actor.Placement.Transform.GetLocation());
		FRotator NewRotation = LocalTransform.TransformRotation(Actor.Placement.Transform.GetRotation()).Rotator();
		FTransform NewTransform = FTransform(NewRotation,NewLoaction);
		ActorData.Add(FWS_Grid_Actor_SpawnData(Actor.Class,NewTransform,FWS_Grid_Actor_Data()));
	}
	for ( auto& StaticMesh : Data.StaticMesh )
	{
		FVector NewLoaction = LocalTransform.TransformPosition(StaticMesh.Placement.Transform.GetLocation());
		FRotator NewRotation = LocalTransform.TransformRotation(StaticMesh.Placement.Transform.GetRotation()).Rotator();
		FTransform NewTransform = FTransform(NewRotation,NewLoaction);
		StaticMeshData.Add(FWS_Grid_Static_SpawnData(StaticMesh.StaticMesh,NewTransform,false,StaticMesh.CastShadow));
	}
	return FWS_SnapSpawnData(ActorData,StaticMeshData);
}
// ------------------------------------------------------
void AWorldScape_SnapSpawnerActor::SetSpawnData( const FWS_SnapSpawnData& Data , const bool PendingManager )
{
	if ( !IsSpawner || !OnPendingSpawn )
	{
		if ( PendingManager )
		{
			if ( IsValid(WorldScapeRoot) )
			{
				PendingSpawnActorData = Data.Actor;
				PendingSpawnStaticMeshData = Data.StaticMesh;
				OnPendingSpawn = true;
				WorldScapeRoot->SnapActor_AddActorToPending(this);
			}
		}
		else
		{
			if ( Use_PendingSpawn )
			{
				PendingSpawnActorData = Data.Actor;
				PendingSpawnStaticMeshData = Data.StaticMesh;
				OnPendingSpawn = true;
				SetActorTickEnabled(true);
			}
			else
			{
				Make_ActorList(Data.Actor);
				Make_StaticMeshList(Data.StaticMesh);
				if ( ComputeCulling )
				{
					PostCompute();
				}
				IsSpawner = true;
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::ResetPending()
{
	if ( OnPendingSpawn )
	{
		OnPendingSpawn = false;
		SetActorTickEnabled(false);
		PendingSpawnActorData.Empty();
		PendingSpawnStaticMeshData.Empty();
	}
}
// ------------------------------------------------
double AWorldScape_SnapSpawnerActor::PostComputeCulling( const double Size )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_PostComputeCulling);
	double Radius = FMath::Sqrt(Size);
	double HalfAngleRad = FMath::DegreesToRadians(CullingAngleBase / 2.0);
	double CullingDistance = Radius / FMath::Tan(HalfAngleRad);
	//UE_LOG(LogTemp, Warning, TEXT("CullingDistance: %f (Radius: %f | FOV: %f)"), CullingDistance, Radius, CullingAngleBase);
	return CullingDistance;
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::PostCompute()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_PostCompute);
	for ( auto& Data : StaticMeshComponentSizing )
	{
		double Culling = PostComputeCulling(Data.Value);
		if ( UInstancedStaticMeshComponent** ISM = StaticMeshComponent.Find(Data.Key) )
		{
			if ( IsValid(*ISM) )
			{
				(*ISM)->SetCullDistances(Culling,Culling);
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Make_ActorList( const TArray<FWS_Grid_Actor_SpawnData>& ActorData )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Make_ActorList);
	if ( UWorld* World = GetWorld() )
	{
		for ( auto& InstanceActor : ActorData )
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.ObjectFlags = RF_Transient;
			SpawnParams.Owner = this ;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			UClass* LoadedClassActor = InstanceActor.SoftClass.LoadSynchronous();
			
			if ( InstanceActor.UseSerialize && !InstanceActor.SerializedData.IsEmpty() )
			{
				SpawnParams.bDeferConstruction = true;
				//AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(LoadedClassActor, InstanceActor.SpawnableTransform , this , nullptr , ESpawnActorCollisionHandlingMethod::AlwaysSpawn );
				AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
				if ( IsValid(SpawnedActor) )
				{
					SpawnedActor->SetFlags(RF_Transient);
					ReferencedActor.Add(SpawnedActor);
					// Add Deserialize Actor Data
					FMemoryReader Ar(InstanceActor.SerializedData, true);
					FObjectAndNameAsStringProxyArchive Archive(Ar, true);
					Archive.ArIsSaveGame = true;
					Archive.ArIgnoreOuterRef = true;
					SpawnedActor->Serialize(Archive);
					//SpawnedActor->SerializeScriptProperties(Archive);
					SpawnedActor->SetActorTransform(InstanceActor.SpawnableTransform);
					SpawnedActor->FinishSpawning(InstanceActor.SpawnableTransform);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("WS Snap : Fail spawn actor"));
				}
			}
			else
			{
				AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
				//AActor* SpawnedActor = World->SpawnActor<AActor>(InstanceActor.SpawnableClass,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
				if ( IsValid(SpawnedActor) )
				{
					SpawnedActor->RegisterAllComponents();
					ReferencedActor.Add(SpawnedActor);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("WS Snap : Fail spawn actor"));
				}
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Make_Actor( FWS_Grid_Actor_SpawnData& ActorData )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Make_Actor);
	if ( UWorld* World = GetWorld() )
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.Owner = this ;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* LoadedClassActor = ActorData.SoftClass.LoadSynchronous();
			
		if ( ActorData.UseSerialize && !ActorData.SerializedData.IsEmpty() )
		{
			SpawnParams.bDeferConstruction = true;
			//AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(LoadedClassActor, InstanceActor.SpawnableTransform , this , nullptr , ESpawnActorCollisionHandlingMethod::AlwaysSpawn );
			AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,ActorData.SpawnableTransform.GetLocation(),ActorData.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
			if ( IsValid(SpawnedActor) )
			{
				/*
				for (TFieldIterator<FProperty> PropIt(LoadedClassActor); PropIt; ++PropIt)
				{
					
				}
				*/
				SpawnedActor->SetFlags(RF_Transient);
				ReferencedActor.Add(SpawnedActor);
				// Add Deserialize Actor Data
				FMemoryReader Ar(ActorData.SerializedData, true);
				FObjectAndNameAsStringProxyArchive Archive(Ar, true);
				Archive.ArIsSaveGame = true;
				Archive.ArIgnoreOuterRef = true;
				SpawnedActor->Serialize(Archive);
				//SpawnedActor->SerializeScriptProperties(Archive);
				SpawnedActor->SetActorTransform(ActorData.SpawnableTransform);
				SpawnedActor->FinishSpawning(ActorData.SpawnableTransform);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("WS Snap : Fail spawn actor"));
			}
		}
		else
		{
			AActor* SpawnedActor = World->SpawnActor<AActor>(LoadedClassActor,ActorData.SpawnableTransform.GetLocation(),ActorData.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
			//AActor* SpawnedActor = World->SpawnActor<AActor>(InstanceActor.SpawnableClass,InstanceActor.SpawnableTransform.GetLocation(),InstanceActor.SpawnableTransform.GetRotation().Rotator(),SpawnParams);
			if ( IsValid(SpawnedActor) )
			{
				SpawnedActor->RegisterAllComponents();
				ReferencedActor.Add(SpawnedActor);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("WS Snap : Fail spawn actor"));
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Clear_ActorList()
{
	if ( !ReferencedActor.IsEmpty() )
	{
		for ( auto& Actor : ReferencedActor )
		{
			if ( IsValid(Actor) )
			{
				Actor->Destroy();
			}
		}
		ReferencedActor.Empty();
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Make_StaticMeshList( const TArray<FWS_Grid_Static_SpawnData>& StaticMeshData )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Make_StaticMeshList);
	if ( !IsDedicated )
	{
		for ( const auto& Data : StaticMeshData )
		{
			if ( UStaticMesh* StaticMesh = Data.SoftStaticMesh.LoadSynchronous() )
			{
				if ( UInstancedStaticMeshComponent** ISMComp = StaticMeshComponent.Find(StaticMesh) )
				{
					if ( IsValid(*ISMComp) )
					{
						(*ISMComp)->AddInstance(Data.SpawnableTransform,true);
					}
				}
				else
				{
					UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>( this , InstancedClass , StaticMesh->GetFName());
					NewISM->SetStaticMesh(StaticMesh);
					NewISM->WorldPositionOffsetDisableDistance = 22000;
					NewISM->SetCollisionEnabled(Data.Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
					NewISM->SetCollisionProfileName(Data.Collision ? TEXT("BlockAll") : TEXT("NoCollision"));
					NewISM->SetGenerateOverlapEvents(false);
					NewISM->SetComponentTickEnabled(false);
					NewISM->CastShadow = Data.CastShadow;
					NewISM->RegisterComponent();
					NewISM->SetVisibility(true);
					NewISM->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
					NewISM->SetMobility(EComponentMobility::Static);
					NewISM->AddInstance(Data.SpawnableTransform,true);
					StaticMeshComponent.Add(StaticMesh,NewISM);
				}
				if ( ComputeCulling )
				{
					if ( double* Size = StaticMeshComponentSizing.Find(StaticMesh) )
					{
						double SquareSize = (StaticMesh->GetBounds().BoxExtent * Data.SpawnableTransform.GetScale3D()).SquaredLength();
						if ( SquareSize > *Size )
						{
							*Size = SquareSize;
						}
					}
					else
					{
						double SquareSize = (StaticMesh->GetBounds().BoxExtent * Data.SpawnableTransform.GetScale3D()).SquaredLength();
						StaticMeshComponentSizing.Add(StaticMesh,SquareSize);
					}
				}
			}
		}
	}
	
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Make_StaticMesh( FWS_Grid_Static_SpawnData& StaticMeshData )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Make_StaticMesh);
	if ( !IsDedicated )
	{
		if ( UStaticMesh* StaticMesh = StaticMeshData.SoftStaticMesh.LoadSynchronous() )
		{
			if ( UInstancedStaticMeshComponent** ISMComp = StaticMeshComponent.Find(StaticMesh) )
			{
				if ( IsValid(*ISMComp) )
				{
					(*ISMComp)->AddInstance(StaticMeshData.SpawnableTransform,true);
				}
			}
			else
			{
				UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>( this , InstancedClass , StaticMesh->GetFName());
				NewISM->SetStaticMesh(StaticMesh);
				NewISM->WorldPositionOffsetDisableDistance = 22000;
				NewISM->SetCollisionEnabled(StaticMeshData.Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				NewISM->SetCollisionProfileName(StaticMeshData.Collision ? TEXT("BlockAll") : TEXT("NoCollision"));
				NewISM->SetGenerateOverlapEvents(false);
				NewISM->SetComponentTickEnabled(false);
				NewISM->CastShadow = StaticMeshData.CastShadow;
				NewISM->RegisterComponent();
				NewISM->SetVisibility(true);
				NewISM->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
				NewISM->SetMobility(EComponentMobility::Static);
				NewISM->AddInstance(StaticMeshData.SpawnableTransform,true);
				StaticMeshComponent.Add(StaticMesh,NewISM);
			}
			if ( ComputeCulling )
			{
				if ( double* Size = StaticMeshComponentSizing.Find(StaticMesh) )
				{
					double SquareSize = (StaticMesh->GetBounds().BoxExtent * StaticMeshData.SpawnableTransform.GetScale3D()).SquaredLength();
					if ( SquareSize > *Size )
					{
						*Size = SquareSize;
					}
				}
				else
				{
					double SquareSize = (StaticMesh->GetBounds().BoxExtent * StaticMeshData.SpawnableTransform.GetScale3D()).SquaredLength();
					StaticMeshComponentSizing.Add(StaticMesh,SquareSize);
				}
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Clear_StaticMeshList()
{
	if ( !StaticMeshComponent.IsEmpty() )
	{
		for ( auto& Comp : StaticMeshComponent )
		{
			if ( IsValid(Comp.Value) )
			{
				Comp.Value->UnregisterComponent();
				Comp.Value->DestroyComponent();
			}
		}
		StaticMeshComponent.Empty();
	}
	StaticMeshComponentSizing.Empty();
}
// ------------------------------------------------
void AWorldScape_SnapSpawnerActor::Update_Pending()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Update_Pending);
	if ( OnPendingSpawn )
	{
		// Actor
		const int32 LastIndexActor = FMath::Min(PendingSpawnActor_PerTick,PendingSpawnActorData.Num());
		for ( int32 Index = 0 ; Index < LastIndexActor ; Index++ )
		{
			Make_Actor( PendingSpawnActorData[Index] );
		}
		PendingSpawnActorData.RemoveAt( 0 , LastIndexActor );
		// StaticMesh
		const int32 LastIndexStaticMesh = FMath::Min(PendingSpawnStaticMesh_PerTick,PendingSpawnStaticMeshData.Num());
		for ( int32 Index = 0 ; Index < LastIndexStaticMesh ; Index++ )
		{
			Make_StaticMesh( PendingSpawnStaticMeshData[Index] );
		}
		PendingSpawnStaticMeshData.RemoveAt( 0 , LastIndexStaticMesh );
		// Check if end
		if ( PendingSpawnActorData.IsEmpty() && PendingSpawnStaticMeshData.IsEmpty() )
		{
			OnPendingSpawn = false;
			SetActorTickEnabled(false);
			IsSpawner = true;
			if ( ComputeCulling )
			{
				PostCompute();
			}
		}
	}
}
// ------------------------------------------------
// ExternalPending
bool AWorldScape_SnapSpawnerActor::Next_Pending()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WS_SnapActor_Next_Pending);
	if ( OnPendingSpawn )
	{
		if ( !PendingSpawnActorData.IsEmpty() )
		{
			Make_Actor( PendingSpawnActorData[0] );
			PendingSpawnActorData.RemoveAt( 0 );
			return true;
		}
		if ( !PendingSpawnStaticMeshData.IsEmpty() )
		{
			Make_StaticMesh( PendingSpawnStaticMeshData[0] );
			PendingSpawnStaticMeshData.RemoveAt( 0 );
			return true;
		}
		if ( PendingSpawnActorData.IsEmpty() && PendingSpawnStaticMeshData.IsEmpty() )
		{
			OnPendingSpawn = false;
			SetActorTickEnabled(false);
			IsSpawner = true;
			if ( ComputeCulling )
			{
				PostCompute();
			}
			return false;
		}
	}
	return false;
}