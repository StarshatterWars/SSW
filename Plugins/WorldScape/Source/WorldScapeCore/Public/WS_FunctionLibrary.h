// QANGA IOLACORP. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldScapeRoot.h"
#include "WorldScape_GridStruct.h"
#include "WS_FunctionLibrary.generated.h"

// ------------------------------------
// Delegate
// ------------------------------------
DECLARE_DYNAMIC_DELEGATE_TwoParams(FWS_SampleReturn , int32 , Index , const TArray<FVector>& , Samples );
/**
 * 
 */
UCLASS()
class WORLDSCAPECORE_API UWS_FunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "WS")
	static bool WS_SampleSurface( AWorldScapeRoot* Root , const FVector Location, const double Size, const double StepSize, const double HeighTreshold, TArray<FVector>& DrawDebug);

	UFUNCTION(BlueprintCallable, Category = "WS")
	static bool WS_SampleSlopeSurface(AWorldScapeRoot* Root, const FVector Location, const double Size, const int32 Step, const double SlopeMin, const double SlopeMax , FVector& RelativeNormal);

	UFUNCTION(BlueprintCallable, Category = "WS")
	static void WS_SampleSurfaceH(AWorldScapeRoot* Root, const FVector CenterLocation, const FVector Forward, const FVector Right, const FVector Up, const double Size, const double StepSize, TArray<FVector>& DrawDebugPoints, double& HDifference, double& Median, double& moyenne);

	UFUNCTION(BlueprintCallable, Category = "WS")
	static void WS_SampleSlope(AWorldScapeRoot* Root, const FVector CenterLocation, const FVector Forward, const FVector Right, const FVector Up, const double Size, TArray<FVector>& DrawDebugPoints , FVector& Normal1 , FVector& Normal2 );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<FVector> WS_Sample_2DGrid( AWorldScapeRoot* Root , const FVector Center , const FRotator Rotation , const FVector Extend , const int32 Step );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static void WS_Sample_2DGrid_Async( const int32 Index ,FWS_SampleReturn Out , AWorldScapeRoot* Root , const FVector Center , const FRotator Rotation , const FVector Extend , const int32 Step );


	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_SphereToCube(const FVector& Location, const FVector& CenterLocation, const double MinRadius);

	UFUNCTION(BlueprintCallable, Category = "WS")
	static FWS_Grid WS_GetGridCoord( const FVector& Location,const FVector& BaseLocation , const int32 Depth, const double Radius);
	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<FWS_Grid> WS_GetAdjacentGridCoord( const FWS_Grid& Coord ,const FVector& BaseLocation , const double Radius);


	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<FTransform> WS_ProjectGroundLocation_Multi(AWorldScapeRoot* Root, const TArray<FVector>& Locations );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_GetTerrainNormalTriTest(AWorldScapeRoot* Root, const FVector& Location , const float Offset = 100.0 );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<FTransform> WS_ProjectGroundLocationWithNormal_Multi(AWorldScapeRoot* Root, const TArray<FVector>& Locations , const float Offset = 100.0 );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static void WS_ProjectActorToGround(AWorldScapeRoot* Root, AActor* Actor , const double Offset = 0 );

	// Using Scale for Actor Bound Extend
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FTransform WS_ProjectTransformToGround(AWorldScapeRoot* Root, const FTransform& Transform, const double Offset = 0 );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_SingleProject(AWorldScapeRoot* Root, const FVector& Location );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FTransform WS_SingleProject_Transform(AWorldScapeRoot* Root, const FVector& Location , const double Size , const double YawBase , const double Z_Offset );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static double WS_GetHeightValueAtLocation(AWorldScapeRoot* Root, const FVector& Location );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static double WS_GetTemperatureValueAtLocation(AWorldScapeRoot* Root, const FVector& Location );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static double WS_GetHumidityValueAtLocation(AWorldScapeRoot* Root, const FVector& Location );

	UFUNCTION(BlueprintCallable, Category = "WS")
	static bool WS_GetGroundInWater(AWorldScapeRoot* Root, const FVector& Location);


	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<uint8> Grid_SerializeActor_Direct( AActor* Actor );


	// Auto Compute H
	UFUNCTION(BlueprintCallable, Category = "WS")
	static double WS_FindMinimalGroundH( AWorldScapeRoot* Root , const TArray<FVector>& Points );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_OverrideLocationH( AWorldScapeRoot* Root , const FVector& Location , const double NewH );

	// Spline Compute
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_CubicBezierY(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T);
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FVector WS_GetSplineOffset(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T);

	// Utils for Volume
	UFUNCTION(BlueprintCallable, Category = "WS")
	static FTransform WS_RelativeToWorld( const FTransform& WorldTransform , const FVector& RelativeLocation , const FRotator& RelativeRotation );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static double WS_GetAltitudeInLocation( AWorldScapeRoot* Root , const FVector& Location , const double Offset );
	UFUNCTION(BlueprintCallable, Category = "WS")
	static TArray<int> WS_GetAllFoliageIndexMask( AWorldScapeRoot* Root );
	
};
