/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    StarshatterWars (Unreal)
	FILE:         JoystickManager.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC
*/

#include "JoystickManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

UJoystickManager* UJoystickManager::Get(const UObject* WorldContext)
{
	if (!WorldContext)
		return nullptr;

	const UWorld* World = WorldContext->GetWorld();
	if (!World)
		return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UJoystickManager>() : nullptr;
}

void UJoystickManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	for (int i = 0; i < 4; ++i) {
		AxisKeys[i] = FKey();
		AxisInvert[i] = false;
	}

	LoadDefaultMappingsIfEmpty();
}

void UJoystickManager::Deinitialize()
{
	StarKeyToUEKey.Empty();
	Super::Deinitialize();
}

void UJoystickManager::RegisterStarKey(int StarKey, const FKey& UEKey)
{
	if (UEKey.IsValid())
		StarKeyToUEKey.Add(StarKey, UEKey);
}

void UJoystickManager::RegisterStarAxis(int AxisIndex, const FKey& AnalogKey, bool bInvert)
{
	if (AxisIndex < 0 || AxisIndex > 3)
		return;

	AxisKeys[AxisIndex] = AnalogKey;
	AxisInvert[AxisIndex] = bInvert;
}

FKey UJoystickManager::GetMappedKey(int StarKey) const
{
	if (const FKey* Found = StarKeyToUEKey.Find(StarKey))
		return *Found;

	return FKey();
}

FKey UJoystickManager::GetMappedAxisKey(int AxisIndex) const
{
	return (AxisIndex >= 0 && AxisIndex <= 3) ? AxisKeys[AxisIndex] : FKey();
}

bool UJoystickManager::GetAxisInverted(int AxisIndex) const
{
	return (AxisIndex >= 0 && AxisIndex <= 3) ? AxisInvert[AxisIndex] : false;
}

bool UJoystickManager::IsKeyDown(APlayerController* PC, const FKey& Key) const
{
	return PC && Key.IsValid() ? PC->IsInputKeyDown(Key) : false;
}

float UJoystickManager::GetAnalog(APlayerController* PC, const FKey& AnalogKey) const
{
	return (PC && AnalogKey.IsValid())
		? PC->GetInputAnalogKeyState(AnalogKey)
		: 0.0f;
}

void UJoystickManager::Poll(APlayerController* PC)
{
	if (!PC)
		return;

	LoadDefaultMappingsIfEmpty();

	float ax = GetAnalog(PC, AxisKeys[0]);
	float ay = GetAnalog(PC, AxisKeys[1]);
	float ar = GetAnalog(PC, AxisKeys[2]);
	float at = GetAnalog(PC, AxisKeys[3]);

	if (AxisInvert[0]) ax = -ax;
	if (AxisInvert[1]) ay = -ay;
	if (AxisInvert[2]) ar = -ar;
	if (AxisInvert[3]) at = -at;

	State.X = Clamp11(ax);
	State.Y = Clamp11(ay);

	State.Roll = State.X;
	State.Pitch = -State.Y;
	State.Yaw = Clamp11(ar);
	State.Z = 0.0;

	double t = at;
	if (t < 0.0)
		t = (t + 1.0) * 0.5;

	State.Throttle = Clamp01(t);
}

void UJoystickManager::LoadDefaultMappingsIfEmpty()
{
	// If already configured, do nothing.
	if (AxisKeys[0].IsValid() || AxisKeys[1].IsValid() || AxisKeys[2].IsValid() || AxisKeys[3].IsValid())
		return;

	// Many UE builds do NOT expose JoystickAxis1..n in EKeys.
	// Gamepad axes are consistently available across platforms/versions.
	AxisKeys[0] = EKeys::Gamepad_LeftX;             // X
	AxisKeys[1] = EKeys::Gamepad_LeftY;             // Y
	AxisKeys[2] = EKeys::Gamepad_RightX;            // R/Yaw
	AxisKeys[3] = EKeys::Gamepad_RightTriggerAxis;  // Throttle-like axis

	AxisInvert[0] = false;
	AxisInvert[1] = true;   // flight-stick convention (pull back = pitch up)
	AxisInvert[2] = false;
	AxisInvert[3] = false;
}