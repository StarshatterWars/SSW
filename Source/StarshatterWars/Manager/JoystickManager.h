/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    StarshatterWars (Unreal)
	FILE:         JoystickManager.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Unreal-facing joystick manager that bridges Starshatter's plain-C++ Joystick
	class to Unreal input state (PlayerController analog + digital keys).
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputCoreTypes.h"
#include "JoystickManager.generated.h"

class APlayerController;

/*
 * Plain struct (no F prefix) representing the latest joystick sample.
 */
struct JoystickState
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;

	double Pitch = 0.0;
	double Roll = 0.0;
	double Yaw = 0.0;

	double Throttle = 0.0;

	bool   Actions[32] = { false };
};

UCLASS()
class UJoystickManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Static accessor usable from plain C++ callers
	static UJoystickManager* Get(const UObject* WorldContext);

	// Subsystem lifecycle
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Poll physical input (called once per frame)
	void Poll(APlayerController* PC);

	// Access last sampled state
	const JoystickState& GetState() const { return State; }

	// Digital + analog queries
	bool  IsKeyDown(APlayerController* PC, const FKey& Key) const;
	float GetAnalog(APlayerController* PC, const FKey& AnalogKey) const;

	// Starshatter -> Unreal mappings
	void RegisterStarKey(int StarKey, const FKey& UEKey);
	void RegisterStarAxis(int AxisIndex, const FKey& AnalogKey, bool bInvert);

	FKey GetMappedKey(int StarKey) const;
	FKey GetMappedAxisKey(int AxisIndex) const;
	bool GetAxisInverted(int AxisIndex) const;

	// Default joystick/gamepad bindings
	void LoadDefaultMappingsIfEmpty();

private:
	JoystickState State;

	TMap<int, FKey> StarKeyToUEKey;

	FKey AxisKeys[4];   // 0=X, 1=Y, 2=R/Yaw, 3=Throttle
	bool AxisInvert[4];

	double Clamp01(double v) const { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
	double Clamp11(double v) const { return v < -1.0 ? -1.0 : (v > 1.0 ? 1.0 : v); }
};
