// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "Engine/TextureRenderTarget2D.h" 
#include "CentralSunActor.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;

UCLASS()
class STARSHATTERWARS_API ACentralSunActor : public AActor
{
	GENERATED_BODY()
	
public:
	ACentralSunActor();

	static ACentralSunActor* SpawnWithSpectralClass(
		UWorld* World,
		const FVector& Location,
		const FRotator& Rotation,
		TSubclassOf<ACentralSunActor> ActorClass,
		ESPECTRAL_CLASS InSpectralClass,
		float InRadius,
		FString InName
	);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION()
	void ApplyStarVisuals(ESPECTRAL_CLASS Class);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Render")
	USceneCaptureComponent2D* SceneCapture = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stellar")
	UStaticMeshComponent* SunMesh;

	UFUNCTION(BlueprintCallable, Category = "Render")
	UTextureRenderTarget2D* GetRenderTarget() const { return SunRenderTarget; }

	UPROPERTY(EditDefaultsOnly, Category = "Stellar" )
	UMaterialInterface* StarBaseMaterial;
	
	UPROPERTY()
	USceneComponent* RootScene;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stellar")
	ESPECTRAL_CLASS SpectralClass = ESPECTRAL_CLASS::G;

	// Star material texture (sunspot mask)
	UPROPERTY(EditDefaultsOnly, Category = "Materials")
	UTexture2D* SunspotTexture;

	UPROPERTY()
	UMaterialInstanceDynamic* StarMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Sun")
	float RotationSpeed = 20.0f;
	
	UFUNCTION(BlueprintCallable, Category = "Stellar")
	float GetRadius() const { return Radius; }
	
	UPROPERTY()
	FString StarName;

	UPROPERTY(EditAnywhere, Category = "Sun")
	UTexture2D* StarTexture;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stellar")
	float Radius = 1.6e9f; // Default to sun size

private:
	UPROPERTY()
	FRotator CurrentRotation;
	UPROPERTY()
	FLinearColor StarColor;

	UPROPERTY(VisibleAnywhere, Category="Render", meta=(AllowPrivateAccess=true))
	UTextureRenderTarget2D* SunRenderTarget;
};