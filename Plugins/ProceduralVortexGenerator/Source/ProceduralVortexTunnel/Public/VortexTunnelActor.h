// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PVTLibrary.h"
#include "VortexData.h"
#include "VortexSpline.h"

#if WITH_EDITOR
	#include "Widgets/Notifications/SNotificationList.h"
#endif

#include "VortexTunnelActor.generated.h"




UCLASS()
class PROCEDURALVORTEXTUNNEL_API AVortexTunnelActor : public AActor
{
	GENERATED_BODY()

	static FVector GetMeshDimensions(UStaticMesh* Mesh);
	static FVector GetMeshDimensionsFromScale(FVector MeshScale, FVector meshdimensions);

	void GenerateNextSegment(bool bAsync);
	void FinishGeneration();
	
	

#if WITH_EDITOR
	
	TWeakPtr<SNotificationItem> VortexNotificationWeakPtr;

#endif


	
public:	

#if WITH_EDITOR

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	FDelegateHandle CurrentBPDelegate;
	
	UPROPERTY(BlueprintReadOnly, Category="Vortex")
		TArray<FVortexComponent> VortexComponents;

	
	int32 NumberOfSegments;
	float DistanceCovered;
	int32 CurrentSegment;
	TArray<UVortexData*> ValidVortexData;
	
	bool bIsCompilerBound;
	 FDelegateHandle OnBlueprintCompiledHandle;

	bool bIsBusy;
	
	// Sets default values for this actor's properties
	AVortexTunnelActor();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Billboard")
	UBillboardComponent* RootBillboard;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Spline")
	AVortexSpline* SourceSpline;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	FVortexProperties VortexProperties;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	float MinSegmentScale;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	float MaxSegmentScale;

	// Distance in Percentage from the Source Spline where the Vortex would start from
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex", meta = (UIMin = "0.0", UIMax = "100.0", SliderMin = "0.0", SliderMax = "100.0"))
	float RelativeStartDistance;

	// Distance in Percentage from the Source Spline where the Vortex would end at
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex", meta = (UIMin = "0.0", UIMax = "100.0", SliderMin = "0.0", SliderMax = "100.0"))
	float RelativeEndDistance;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Vortex")
	TArray<UVortexData*> VortexData;

	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void GenerateVortex(bool bAsync = true);

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void UpdateVortex();

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		USplineComponent* GetCurrentSplineComponent();

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void GenerateVortexInRange(float CurrentDistance, float EnterDistance = 10000);

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void DestroyVortexOutOfRange(float CurrentDistance, float ExitDistance = 2000);

	UFUNCTION(BlueprintCallable, Category = "Vortex")
	     void DestroyAllVortex();

	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void UpdateScalarProperty(int32 MaterialIndex, FString PropertyName, float PropertyValue);
 
	UFUNCTION(BlueprintCallable, Category = "Vortex")
		void UpdateColorProperty(int32 MaterialIndex, FString PropertyName, FLinearColor PropertyValue);

};
