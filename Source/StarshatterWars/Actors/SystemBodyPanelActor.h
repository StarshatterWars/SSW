#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "Misc/Crc.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "SystemBodyPanelActor.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * Base class for all system bodies (star, planet, moon) rendered
 * in the system view. Supports:
 *  - axial spin + tilt
 *  - optional deterministic orbital motion (seed + time based)
 *
 * Star bodies typically:
 *  - provide orbit authority (time + center)
 *  - do notxenable orbit movement
 *
 * Planets / moons:
 *  - enable orbit
 *  - reference an orbit authority (usually the star)
 */
UCLASS(Abstract)
class STARSHATTERWARS_API ASystemBodyPanelActor : public AActor
{
	GENERATED_BODY()

public:
	ASystemBodyPanelActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ------------------------------------------------------------
	// UI / Rendering access
	// ------------------------------------------------------------

	float ComputeUIScale(float Radius) const;

	UFUNCTION(BlueprintCallable, Category = "Render")
	UTextureRenderTarget2D* GetRenderTarget() const { return BodyRenderTarget; }

	UFUNCTION(BlueprintCallable, Category = "Render")
	UMaterialInstanceDynamic* GetMaterialInstance() const { return BodyMaterialInstance; }

	// Call after derived data is set (radius, tilt, etc.)
	UFUNCTION(BlueprintCallable, Category = "Body")
	void InitializeBody();

	FRotator ComputeSpinRotation(float WorldTimeSeconds) const;
	UTexture2D* LoadBodyTexture(FString AssetType);

	// ------------------------------------------------------------
	// ORBIT AUTHORITY (time + center)
	// ------------------------------------------------------------

	/** Stable epoch for deterministic orbital phase */
	UPROPERTY(EditAnywhere, Category = "Orbit")
	double OrbitEpochSeconds = 0.0;

	/** Orbit speed multiplier for the whole system (1 = real time) */
	UPROPERTY(EditAnywhere, Category = "Orbit")
	float OrbitTimeScale = 1.0f;

	/** Stable system seed (usually system name) */
	UPROPERTY(EditAnywhere, Category = "Orbit")
	FString SystemSeed;

	/** World-space center for system orbits (star location) */
	FORCEINLINE FVector GetOrbitCenter() const
	{
		return GetActorLocation();
	}

	/** Scaled orbit time used by orbiting bodies */
	double GetOrbitTimeSeconds() const;

	// ------------------------------------------------------------
	// ORBIT (BASE SUPPORT FOR PLANETS / MOONS)
	// ------------------------------------------------------------

	/** Enable deterministic orbit movement */
	UPROPERTY(EditAnywhere, Category = "Orbit")
	bool bEnableOrbit = false;

	/** Authority providing orbit center + time (usually the star) */
	UPROPERTY()
	TObjectPtr<ASystemBodyPanelActor> OrbitAuthority = nullptr;

	/** Orbit radius in kilometers */
	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit"))
	float OrbitRadiusKm = 0.f;

	/** Deterministic seed for phase (SystemName_BodyName) */
	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit"))
	FString OrbitSeed;

	/** If true, orbit around a custom center (e.g. moon around planet) */
	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit"))
	bool bUseCustomOrbitCenter = false;

	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit && bUseCustomOrbitCenter"))
	FVector CustomOrbitCenter = FVector::ZeroVector;

	/** Game-scaled orbital speed (cm/s) at ReferenceRadiusKm */
	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit"))
	float BaseSpeedCmPerSec = 40000.f;

	/** Reference radius (km) used for sqrt(Ref / r) scaling */
	UPROPERTY(EditAnywhere, Category = "Orbit", meta = (EditCondition = "bEnableOrbit"))
	float ReferenceRadiusKm = 1.0e6f;

	/** Initialize deterministic orbit parameters */
	UFUNCTION(BlueprintCallable, Category = "Orbit")
	void InitializeOrbit(
		ASystemBodyPanelActor* InAuthority,
		float InOrbitRadiusKm,
		const FString& InOrbitSeed
	);

	// ------------------------------------------------------------
	// BODY / SPIN DATA (existing)
	// ------------------------------------------------------------

	bool bApplyAxisTilt = true;
	float RotationSpeed = 20.f;
	float BodyTilt = 0.f;
	double BodyRadius = 0.0;
	FString BodyName;
	EBodyUISizeClass BodyType;

protected:
	// ------------------------------------------------------------
	// One-time setup
	// ------------------------------------------------------------

	void InitBody(FString AssetType);

	// ------------------------------------------------------------
	// Derived class hooks
	// ------------------------------------------------------------

	virtual FString GetBodyName() const PURE_VIRTUAL(ASystemBodyPanelActor::GetBodyName, return TEXT("Body"););
	virtual float GetBodyRadius() const PURE_VIRTUAL(ASystemBodyPanelActor::GetBodyRadius, return 1.0f;);
	virtual float GetBodyTiltDegrees() const PURE_VIRTUAL(ASystemBodyPanelActor::GetBodyTiltDegrees, return 0.0f;);
	virtual UMaterialInterface* GetBaseMaterial() const PURE_VIRTUAL(ASystemBodyPanelActor::GetBaseMaterial, return nullptr;);

	void EndPlay(const EEndPlayReason::Type EndPlayReason);

	// ------------------------------------------------------------
	// Render helpers
	// ------------------------------------------------------------

	int32 ComputeRenderTargetResolution(float Radius) const;

	UPROPERTY(EditAnywhere, Category = "Body")
	bool bApplyUIScale = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Body")
	USceneComponent* RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Body")
	UStaticMeshComponent* BodyMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Body")
	USceneCaptureComponent2D* SceneCapture = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* BodyMaterialInstance = nullptr;

	UPROPERTY()
	UTexture2D* BodyTexture = nullptr;

	UPROPERTY()
	FString TextureName;

private:
	// ------------------------------------------------------------
	// Orbit internals
	// ------------------------------------------------------------

	static float HashTo01(const FString& Key);
	void UpdateOrbitLocation(double NowSeconds);
	FVector GetOrbitCenterForThisBody() const;

	// ------------------------------------------------------------
	// Render target
	// ------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Render", meta = (AllowPrivateAccess = true))
	UTextureRenderTarget2D* BodyRenderTarget = nullptr;
};