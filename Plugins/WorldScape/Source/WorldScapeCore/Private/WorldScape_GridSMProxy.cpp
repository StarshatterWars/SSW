// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_GridSMProxy.h"

#include "WorldScape_GridStruct.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"


// ------------------------------------------------
AWorldScape_GridSMProxy::AWorldScape_GridSMProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::BeginPlay()
{
	Super::BeginPlay();
	if ( UWorld* World = GetWorld() )
	{
		if ( World->GetNetMode() == NM_DedicatedServer )
		{
			IsDedicated = true;
		}
	}
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if ( IsFoliage )
	{
		if ( AActor* LocalOwner = GetOwner() )
		{
			for ( auto& Comp : StaticMeshComponent )
			{
				if ( IsValid(Comp.Key) )
				{
					Comp.Value->UnregisterComponent();
					Comp.Value->DestroyComponent();
				}
			}
			StaticMeshComponent.Empty();
		}
	}
	Super::EndPlay(EndPlayReason);
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::BeginDestroy()
{
	if ( IsFoliage )
	{
		if ( AActor* LocalOwner = GetOwner() )
		{
			if ( IsValid(LocalOwner) )
			{
				for ( auto& Comp : StaticMeshComponent )
				{
					if ( IsValid(Comp.Key) )
					{
						Comp.Value->UnregisterComponent();
						Comp.Value->DestroyComponent();
					}
				}
				StaticMeshComponent.Empty();
			}
		}
	}
	Super::BeginDestroy();
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::Set_ProxyMesh( UStaticMesh* Proxy , const bool SetVisible )
{
	if ( Proxy && !IsDedicated )
	{
		if ( !ProxyMeshComponent )
		{
			ProxyMeshComponent =  NewObject<UStaticMeshComponent>( this , UStaticMeshComponent::StaticClass() , TEXT("WorldScape_GridSMProxy"));
			//ProxyMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>( FName("ProxyMesh") );
			ProxyMeshComponent->RegisterComponent();
			ProxyMeshComponent->SetComponentTickEnabled(false);
			ProxyMeshComponent->SetGenerateOverlapEvents(false);
			ProxyMeshComponent->SetStaticMesh(Proxy);
			ProxyMeshComponent->SetVisibility( SetVisible );
			ProxyMeshComponent->AttachToComponent(RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
			ProxyMeshComponent->SetMobility(EComponentMobility::Static);
			ProxyMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else
		{
			ProxyMeshComponent->SetStaticMesh(Proxy);
			ProxyMeshComponent->SetVisibility( SetVisible );
		}
	}
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::Visible_Proxy( const bool bIsVisible ) const
{
	if ( ProxyMeshComponent && !IsDedicated )
	{
		if ( ProxyMeshComponent->IsVisible() != bIsVisible )
		{
			ProxyMeshComponent->SetVisibility( bIsVisible );
		}
	}
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::Make_StaticMeshList( const TArray<FWS_Grid_Static_SpawnData>& StaticMeshData )
{
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
					if ( IsFoliage )
					{
						if ( AActor* LocalOwner = GetOwner() )
						{
							const FName ComputeName = FName(FString::Printf(TEXT("%s_%s"), *this->GetName(), *StaticMesh->GetName()));
							UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>( LocalOwner , UInstancedStaticMeshComponent::StaticClass() , ComputeName);
							//UInstancedStaticMeshComponent* NewISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>( Data.SpawnableStaticMesh->GetFName() );
							//UStaticMesh* LoadedMesh = Data.SoftStaticMesh.LoadSynchronous();
							NewISM->SetWorldLocation(GetActorLocation());
							NewISM->SetStaticMesh(StaticMesh);
							NewISM->WorldPositionOffsetDisableDistance = 22000;
							//NewISM->SetStaticMesh(Data.SpawnableStaticMesh);
							NewISM->SetCollisionEnabled(Data.Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
							NewISM->SetGenerateOverlapEvents(false);
							NewISM->SetComponentTickEnabled(false);
							NewISM->CastShadow = Data.CastShadow;
							NewISM->RegisterComponent();
							NewISM->SetVisibility(true);
							NewISM->AttachToComponent(LocalOwner->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
							NewISM->SetMobility(EComponentMobility::Static);
							NewISM->AddInstance(Data.SpawnableTransform,true);
							StaticMeshComponent.Add(StaticMesh,NewISM);
							continue;
						}
					}
					UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>( this , UInstancedStaticMeshComponent::StaticClass() , StaticMesh->GetFName());
					//UInstancedStaticMeshComponent* NewISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>( Data.SpawnableStaticMesh->GetFName() );
					//UStaticMesh* LoadedMesh = Data.SoftStaticMesh.LoadSynchronous();
					NewISM->SetStaticMesh(StaticMesh);
					NewISM->WorldPositionOffsetDisableDistance = 22000;
					//NewISM->SetStaticMesh(Data.SpawnableStaticMesh);
					NewISM->SetCollisionEnabled(Data.Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
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
			}
		}
	}
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::Clear_StaticMeshList()
{
	if ( !IsDedicated )
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
}
// ------------------------------------------------
void AWorldScape_GridSMProxy::SetFoliageMode()
{
	IsFoliage = true;
}

