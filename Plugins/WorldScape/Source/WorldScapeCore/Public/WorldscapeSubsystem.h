// QANGA // IOLACORP. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Materials/MaterialParameterCollection.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldscapeSubsystem.generated.h"

//forward declaration
class UWorldscapeCollisionInvoker;
class AWorldScapeRoot;

/**
 * 
 */
UCLASS()
class WORLDSCAPECORE_API UWorldscapeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void BeginDestroy() override;
	
	// =============================================================================
	// Delegate
	// =============================================================================
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWS_OnNearestChanged , AWorldScapeRoot* , Root );
	UPROPERTY(BlueprintAssignable , Category="WS")
	FWS_OnNearestChanged WS_OnNearestChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWS_OnNearestNULL);
	UPROPERTY(BlueprintAssignable , Category="WS")
	FWS_OnNearestNULL WS_OnNearestNULL;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWS_OnWorldScapeRegistry , AWorldScapeRoot* , Root );
	UPROPERTY(BlueprintAssignable , Category="WS")
	FWS_OnWorldScapeRegistry WS_OnWorldScapeRegistry;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWS_OnWorldScapeUnRegistry , AWorldScapeRoot* , Root );
	UPROPERTY(BlueprintAssignable , Category="WS")
	FWS_OnWorldScapeUnRegistry FWS_OnWorldScapeUnRegistry;
	
	// =============================================================================
	// Registry
	// =============================================================================
	UFUNCTION()
	void WS_Registry_WorldScapeRoot( AWorldScapeRoot* Root );
	UFUNCTION()
	void WS_UnRegistry_WorldScapeRoot( AWorldScapeRoot* Root );
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision")
    TSet<UWorldscapeCollisionInvoker*> WorldScapeCollisionInvokers;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Actor" , transient )
    TSet<AWorldScapeRoot*> WorldScapeActors;

	UFUNCTION(BlueprintCallable, Category = "WS")
	AWorldScapeRoot* Get_NearWorldScapeFromLocation( const FVector& Location , const FName& ValidTags , const FName& ExcludeTags );
	// -------------------------------------------------------
	// WS_AutoGetNearest
	// -------------------------------------------------------
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	float AutoGetNearest_Update_Interval = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	bool AutoGetNearest_AutoEnabled = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	double AutoGetNearest_PlanetaryRadiusScaleLimit = 1.5f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	double AutoGetNearest_FlatDistanceToGroundLimit = 20000000;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	bool AutoGetNearest_Use_ValidTag = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	FName AutoGetNearest_ValidTag = TEXT("WS_Valid");
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	bool AutoGetNearest_Use_InvalidTag= false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AutoGetNearest")
	FName AutoGetNearest_invalidTag = TEXT("WS_Invalid");

	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_Start_AutoGetNearest();
	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_Stop_AutoGetNearest();
	UFUNCTION(BlueprintCallable, Category = "WS")
	bool WS_GetValidNearest();
	UFUNCTION(BlueprintCallable, Category = "WS")
	AWorldScapeRoot* WS_GetCurrentNearestRoot();
	UFUNCTION(BlueprintCallable, Category = "WS")
	FVector WS_GetCurrentSurfaceUpVector();

	// -------------------------------------------------------
	// WS_Gravity
	// -------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "WS")
	FVector WS_GetActualGravityFromLocation( const FVector& Location , FVector& GravityDir , double& Force );
	

private:
	
	// -------------------------------------------------------
	// WS_AutoGetNearest
	// -------------------------------------------------------
	UFUNCTION()
	void AutoGetNearest_Start();
	UFUNCTION()
	void AutoGetNearest_End();
	UFUNCTION()
	void AutoGetNearest_Update();
	UPROPERTY( transient )
	FTimerHandle AutoGetNearest_Timer;
	
	UFUNCTION()
    AWorldScapeRoot* AutoGetNearest_WorldScapeFromLocation( const FVector& Location);
	
	UPROPERTY( Transient )
	AWorldScapeRoot* CurrentNearest_WorldScapeRoot = nullptr;
	UPROPERTY( Transient )
	bool IsLocalNearestTo_WorldScapeRoot = false;

	// -------------------------------------------------------
	// WS_Global MPC Data
	// -------------------------------------------------------
	UPROPERTY( Transient )
	FVector Current_SurfaceUpVector = FVector(0, 0, 1);
	UPROPERTY()
	UMaterialParameterCollection* WS_MaterialParameterCollection = nullptr;
	UFUNCTION()
	void Update_SurfaceVector();
	UFUNCTION()
	void Update_MPC_Value();
	
	// -----------------------------------------------------------------
	// Custom Tick Template struct
	// -----------------------------------------------------------------
	struct FCustomTickFunction : public FTickFunction
	{
		UWorldscapeSubsystem* Target;
		void (UWorldscapeSubsystem::*TickFunctionPtr)(float);

		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override
		{
			if (Target && TickFunctionPtr)
			{
				(Target->*TickFunctionPtr)(DeltaTime);
			}
		}
		virtual FString DiagnosticMessage() override
		{
			return Target->GetFullName() + TEXT("[CustomTick]");
		}
	};
	// Register Base
	FCustomTickFunction WS_Post_Tick;
	// Function
	void RegisterSingleTick(FCustomTickFunction& TickFunction, void (UWorldscapeSubsystem::*FuncPtr)(float), ETickingGroup TickGroup, float TickInterval , bool Enabled);
	// Custom Tick Function
	UFUNCTION()
	void Register_CustomTick();
	UFUNCTION()
	void UnRegister_CustomTick();
	UFUNCTION()
	void WS_Post_TickFunction( float DeltaTime );
	
	
};
