// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldScape_GridSMProxy.generated.h"

class UInstancedStaticMeshComponent;
struct FWS_Grid_StaticMesh_Data;

UCLASS()
class WORLDSCAPECORE_API AWorldScape_GridSMProxy : public AActor
{
	GENERATED_BODY()

public:
	
	AWorldScape_GridSMProxy();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	
	UPROPERTY()
	UStaticMesh* ProxyMesh = nullptr;

	UFUNCTION()
	void Set_ProxyMesh( UStaticMesh* Proxy , const bool SetVisible = true );
	UFUNCTION()
	void Visible_Proxy( const bool bIsVisible ) const;
	UFUNCTION()
	void Make_StaticMeshList( const TArray<FWS_Grid_Static_SpawnData>& StaticMeshData );
	UFUNCTION()
	void Clear_StaticMeshList();
	UFUNCTION()
	void SetFoliageMode();

	UPROPERTY()
	UStaticMeshComponent* ProxyMeshComponent = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Component")
	TMap<UStaticMesh*,UInstancedStaticMeshComponent*> StaticMeshComponent;

	
	UPROPERTY()
	bool IsFoliage = false;

private:
	
	UPROPERTY()
	bool ProxyVisible = false;
	UPROPERTY()
	bool IsDedicated = false;
	
};
