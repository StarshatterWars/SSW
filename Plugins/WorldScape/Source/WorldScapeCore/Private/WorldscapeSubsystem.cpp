// QANGA // IOLACORP. All Rights Reserved


#include "WorldscapeSubsystem.h"
#include "WorldScapeRoot.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/GameplayStatics.h"

// ----------------------------------------------
void UWorldscapeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WS_MaterialParameterCollection = LoadObject<UMaterialParameterCollection>(nullptr,TEXT("/WorldScape/_Reference/WorldScape_MPC.WorldScape_MPC"));
	if ( !IsValid(WS_MaterialParameterCollection) )
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldScape , MPC base not found"));
	}
}
// ----------------------------------------------
void UWorldscapeSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
// ----------------------------------------------
void UWorldscapeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if ( AutoGetNearest_AutoEnabled )
	{
		AutoGetNearest_Start();
	}
	Register_CustomTick();
}
// ----------------------------------------------
void UWorldscapeSubsystem::BeginDestroy()
{
	AutoGetNearest_End();
	UnRegister_CustomTick();
	Super::BeginDestroy();
}
// =============================================================================
// Registry
// =============================================================================
void UWorldscapeSubsystem::WS_Registry_WorldScapeRoot( AWorldScapeRoot* Root )
{
	if ( IsValid( Root ) )
	{
		WorldScapeActors.Add( Root );
		WS_OnWorldScapeRegistry.Broadcast(Root);
	}
}
// ----------------------------------------------
void UWorldscapeSubsystem::WS_UnRegistry_WorldScapeRoot( AWorldScapeRoot* Root )
{
	if ( IsValid( Root ) )
	{
		WorldScapeActors.Remove( Root );
		FWS_OnWorldScapeUnRegistry.Broadcast(Root);
	}
}
// ----------------------------------------------
AWorldScapeRoot* UWorldscapeSubsystem::Get_NearWorldScapeFromLocation( const FVector& Location , const FName& ValidTags , const FName& ExcludeTags )
{
	AWorldScapeRoot* LocalWS = nullptr;
	float LocalDistance = 0;
	TArray<AWorldScapeRoot*> AllRoots = WorldScapeActors.Array();
	for ( int32 i = 0; i < AllRoots.Num(); i++ )
	{
		if ( IsValid(AllRoots[i]) && AllRoots[i]->ActorHasTag(ValidTags) && !AllRoots[i]->ActorHasTag(ExcludeTags) )
		{
			if ( i == 0 )
			{
				LocalDistance = AllRoots[i]->GetPawnDistanceFromGround(Location);
				LocalWS = AllRoots[i];
			}
			else
			{
				float Distance = AllRoots[i]->GetPawnDistanceFromGround(Location);
				if ( Distance < LocalDistance )
				{
					LocalDistance = Distance;
					LocalWS = AllRoots[i];
				}
			}
		}
	}
	return LocalWS;
}
// -------------------------------------------------------
// WS_AutoGetNearest
// -------------------------------------------------------
void UWorldscapeSubsystem::WS_Start_AutoGetNearest()
{
	AutoGetNearest_Start();
}
// ----------------------------------------------
void UWorldscapeSubsystem::WS_Stop_AutoGetNearest()
{
	AutoGetNearest_End();
}
// ----------------------------------------------
bool UWorldscapeSubsystem::WS_GetValidNearest()
{
	return IsLocalNearestTo_WorldScapeRoot;
}
// ----------------------------------------------
AWorldScapeRoot* UWorldscapeSubsystem::WS_GetCurrentNearestRoot()
{
	return CurrentNearest_WorldScapeRoot;	
}
// ----------------------------------------------
FVector UWorldscapeSubsystem::WS_GetCurrentSurfaceUpVector()
{
	return Current_SurfaceUpVector;
}
// ----------------------------------------------
void UWorldscapeSubsystem::AutoGetNearest_Start()
{
	if ( UWorld* World = GetWorld() )
	{
		if ( World->GetNetMode() == NM_Client || World->GetNetMode() == NM_Standalone )
		{
			if ( !AutoGetNearest_Timer.IsValid() )
			{
				World->GetTimerManager().SetTimer(AutoGetNearest_Timer, this,&UWorldscapeSubsystem::AutoGetNearest_Update,1.0f, true);
			}
		}
	}
}
// ----------------------------------------------
void UWorldscapeSubsystem::AutoGetNearest_End()
{
	if ( AutoGetNearest_Timer.IsValid() )
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoGetNearest_Timer);
	}
}
// ----------------------------------------------
void UWorldscapeSubsystem::AutoGetNearest_Update()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AutoGetNearest_Update);
	if ( UWorld* World = GetWorld() )
	{
		if ( World->GetNetMode() == NM_Client || World->GetNetMode() == NM_Standalone )
		{
			if ( APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0) )
			{
				if ( AWorldScapeRoot* Temp = AutoGetNearest_WorldScapeFromLocation( Pawn->GetActorLocation()) )
				{
					if ( IsValid(CurrentNearest_WorldScapeRoot) )
					{
						if ( Temp != CurrentNearest_WorldScapeRoot )
						{
							CurrentNearest_WorldScapeRoot = Temp;
							IsLocalNearestTo_WorldScapeRoot = true;
							WS_OnNearestChanged.Broadcast(CurrentNearest_WorldScapeRoot);
							Update_MPC_Value();
						}
					}
					else
					{
						CurrentNearest_WorldScapeRoot = Temp;
						IsLocalNearestTo_WorldScapeRoot = true;
						WS_OnNearestChanged.Broadcast(CurrentNearest_WorldScapeRoot);
						Update_MPC_Value();
					}
				}
				else
				{
					if ( IsValid(CurrentNearest_WorldScapeRoot) )
					{
						CurrentNearest_WorldScapeRoot = nullptr;
						IsLocalNearestTo_WorldScapeRoot = false;
						WS_OnNearestNULL.Broadcast();
					}
				}
			}
		}
	}
}
// ----------------------------------------------
AWorldScapeRoot* UWorldscapeSubsystem::AutoGetNearest_WorldScapeFromLocation( const FVector& Location )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AutoGetNearest_WorldScapeFromLocation);
	AWorldScapeRoot* LocalWS = nullptr;
	float LocalDistance = 0;
	TArray<AWorldScapeRoot*> AllRoots = WorldScapeActors.Array();
	for ( int32 i = 0; i < AllRoots.Num(); i++ )
	{
		if ( IsValid(AllRoots[i]) && AllRoots[i]->IsAllowAutoGetNearest && (AutoGetNearest_Use_ValidTag ? AllRoots[i]->ActorHasTag(AutoGetNearest_ValidTag) : true) && (AutoGetNearest_Use_InvalidTag ? !AllRoots[i]->ActorHasTag(AutoGetNearest_invalidTag) : true) )
		{
			float Distance = AllRoots[i]->GetPawnDistanceFromGround(Location);
			if ( AllRoots[i]->GenerationType == EWorldScapeType::Planet )
			{
				if ( (AllRoots[i]->GetActorLocation()-Location).SquaredLength() < FMath::Square(AllRoots[i]->PlanetScale*AutoGetNearest_PlanetaryRadiusScaleLimit) )
				{
					if ( !LocalWS )
					{
						LocalDistance = Distance;
						LocalWS = AllRoots[i];
					}
					else
					{
						if ( Distance < LocalDistance )
						{
							LocalDistance = Distance;
							LocalWS = AllRoots[i];
						}
					}
				}
			}
			else
			{
				if ( Distance < AutoGetNearest_FlatDistanceToGroundLimit )
				{
					if ( LocalWS )
					{
						LocalDistance = Distance;
						LocalWS = AllRoots[i];
					}
					else
					{
						if ( Distance < LocalDistance )
						{
							LocalDistance = Distance;
							LocalWS = AllRoots[i];
						}
					}
				}
			}
		}
	}
	return LocalWS;
}
// ----------------------------------------------
void UWorldscapeSubsystem::RegisterSingleTick(FCustomTickFunction& TickFunction, void (UWorldscapeSubsystem::*FuncPtr)(float), ETickingGroup TickGroup, float TickInterval , bool Enabled)
{
	TickFunction.Target = this;
	TickFunction.TickFunctionPtr = FuncPtr;
	TickFunction.SetTickFunctionEnable(Enabled);
	TickFunction.TickGroup = TickGroup;
	TickFunction.TickInterval = TickInterval;
	if (UWorld* World = GetWorld())
	{
		TickFunction.RegisterTickFunction(World->PersistentLevel);
	}
}
// Custom Tick Function
void UWorldscapeSubsystem::Register_CustomTick()
{
	if (IsTemplate()) return;
	RegisterSingleTick(WS_Post_Tick, &UWorldscapeSubsystem::WS_Post_TickFunction, TG_PostUpdateWork , 0.0f,true);
}
// ----------------------------------------------
void UWorldscapeSubsystem::UnRegister_CustomTick()
{
	WS_Post_Tick.UnRegisterTickFunction();
}
// ----------------------------------------------
void UWorldscapeSubsystem::WS_Post_TickFunction( float DeltaTime )
{
	Update_SurfaceVector();
}
// -----------------------------------------------------
void UWorldscapeSubsystem::Update_SurfaceVector()
{
	if ( UWorld* World = GetWorld() )
	{
		if ( World->GetNetMode() == NM_Client || World->GetNetMode() == NM_Standalone )
		{
			if ( APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0) )
			{
				if ( IsValid(CurrentNearest_WorldScapeRoot) )
				{
					if ( CurrentNearest_WorldScapeRoot->GenerationType == EWorldScapeType::Planet )
					{
						Current_SurfaceUpVector = (CurrentNearest_WorldScapeRoot->GetActorLocation()-Pawn->GetActorLocation()).GetSafeNormal();
					}
					else
					{
						Current_SurfaceUpVector = CurrentNearest_WorldScapeRoot->GetActorUpVector();
					}
					if ( IsValid(WS_MaterialParameterCollection) )
					{
						UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WS_MaterialParameterCollection);
						const bool bFoundParameter = Instance->SetVectorParameterValue(TEXT("WS_Surface_UpVector"), Current_SurfaceUpVector);
					}
				}
				else
				{
					if ( IsValid(WS_MaterialParameterCollection) )
					{
						UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WS_MaterialParameterCollection);
						const bool bFoundParameter = Instance->SetVectorParameterValue(TEXT("WS_Surface_UpVector"), FVector(0,0,1));
					}
				}
			}
		}
	}
}
// -----------------------------------------------------
void UWorldscapeSubsystem::Update_MPC_Value()
{
	if ( UWorld* World = GetWorld() )
	{
		if ( IsValid(WS_MaterialParameterCollection) && IsValid(CurrentNearest_WorldScapeRoot) )
		{
			UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WS_MaterialParameterCollection);
			Instance->SetScalarParameterValue(TEXT("WS_Valid"), 1.0f);
			Update_SurfaceVector();
			if ( CurrentNearest_WorldScapeRoot->GenerationType == EWorldScapeType::Planet )
			{
				Instance->SetVectorParameterValue(TEXT("WS_Planet_Origin"), CurrentNearest_WorldScapeRoot->GetActorLocation());
				Instance->SetScalarParameterValue(TEXT("WS_Planet_Size"), CurrentNearest_WorldScapeRoot->PlanetScale);
				Instance->SetScalarParameterValue(TEXT("WS_Planetary"), 1.0f);
			}
			else
			{
				Instance->SetVectorParameterValue(TEXT("WS_Flat_Origin"), CurrentNearest_WorldScapeRoot->GetActorLocation());
				Instance->SetScalarParameterValue(TEXT("WS_Planetary"), 0.0f);
			}
		}
		else
		{
			UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WS_MaterialParameterCollection);
			Instance->SetScalarParameterValue(TEXT("WS_Valid"), 0.0f);
		}
	}
}
// -------------------------------------------------------
// WS_Gravity
// -------------------------------------------------------
FVector UWorldscapeSubsystem::WS_GetActualGravityFromLocation( const FVector& Location , FVector& GravityDir , double& Force )
{
	if ( IsValid(CurrentNearest_WorldScapeRoot) )
	{
		return CurrentNearest_WorldScapeRoot->WS_GetGravityFromLocation(Location,GravityDir,Force);
	}
	GravityDir = FVector(0,0,-1);
	Force = 0.0f;
	return FVector(0,0,0);
}