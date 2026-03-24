/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         Joystick.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Joystick Input class (Unreal-native polling)

	- Plain C++ class; Unreal-facing work is delegated to UJoystickManager.
	- KeyDown/KeyDownMap route through JoystickManager’s key registry.
*/

#include "Joystick.h"

// Unreal includes are confined to the .cpp:
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#include "JoystickManager.h"  // UJoystickManager + JoystickState

static Joystick* joystick_instance = nullptr;

// ---------------------------
// Utility
// ---------------------------

static double Clamp11(double v)
{
	if (v < -1.0) return -1.0;
	if (v > 1.0) return  1.0;
	return v;
}

static double Clamp01(double v)
{
	if (v < 0.0) return 0.0;
	if (v > 1.0) return 1.0;
	return v;
}

const UObject* Joystick::FindWorldContextObject()
{
	// Prefer a real game/PIE world.
	if (!GEngine)
		return nullptr;

	const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Ctx : Contexts) {
		UWorld* World = Ctx.World();
		if (!World)
			continue;

		if (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE) {
			return World;
		}
	}

	// Fallback: whatever the engine considers the current play world.
	if (UWorld* PlayWorld = GEngine->GetCurrentPlayWorld())
		return PlayWorld;

	return nullptr;
}

// ---------------------------
// Joystick
// ---------------------------

Joystick::Joystick()
{
	joystick_instance = this;

	ResetHat();

	// Defaults: identity mapping for digital keys (Starshatter code -> Starshatter code).
	for (int i = 0; i < KEY_MAP_SIZE; ++i)
		map[i] = i;

	// Axis maps are left as-is; JoystickManager provides default axis keys if unset.
	for (int a = 0; a < 4; ++a) {
		map_axis[a] = a;
		inv_axis[a] = false;
	}
}

Joystick::~Joystick()
{
	if (joystick_instance == this)
		joystick_instance = nullptr;
}

Joystick* Joystick::GetInstance()
{
	if (!joystick_instance)
		joystick_instance = new Joystick();

	return joystick_instance;
}

void Joystick::MapKeys(KeyMapEntry* mapping, int nkeys)
{
	if (!mapping || nkeys <= 0)
		return;

	// Get UE-side manager so we can register "Starshatter action -> UE key" bindings:
	const UObject* WorldContext = FindWorldContextObject();
	UJoystickManager* Manager = WorldContext ? UJoystickManager::Get(WorldContext) : nullptr;

	for (int i = 0; i < nkeys; ++i) {
		const int joy_btn = mapping[i].joy;  // physical joystick button index (Starshatter)
		const int act = mapping[i].act;  // Starshatter action index

		if (joy_btn < 0 || joy_btn >= KEY_MAP_SIZE)
			continue;

		if (act < 0 || act >= MotionController::MaxActions)
			continue;

		// Starshatter action plumbing: joy button -> action
		map[joy_btn] = act;

		// UE internal mapping: register "action" as the key id queried by KeyDown(action).
		// We bind that action to a concrete Unreal key representing the physical joystick button.
		if (Manager) {
			const FKey UEButtonKey = FKey(*FString::Printf(TEXT("Joystick_Button%d"), joy_btn));

			// Some platforms expose buttons as "Gamepad_FaceButton_*" etc. If the constructed key
			// doesn't exist on the running platform, it will be invalid and safely ignored.
			if (UEButtonKey.IsValid()) {
				Manager->RegisterStarKey(act, UEButtonKey);
			}
		}
	}
}


void Joystick::Acquire()
{
	const UObject* WorldContext = FindWorldContextObject();
	if (!WorldContext)
		return;

	UWorld* World = WorldContext->GetWorld();
	if (!World)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
		return;

	UJoystickManager* Manager = UJoystickManager::Get(WorldContext);
	if (!Manager)
		return;

	// Poll Unreal input state once per frame:
	Manager->Poll(PC);

	const JoystickState& S = Manager->GetState();

	// Feed Starshatter’s axis pipeline:
	ProcessAxes(S.X, S.Y, S.Yaw, S.Throttle);

	// Populate Starshatter action[] using mapped keys.
	for (int i = 0; i < MotionController::MaxActions; ++i) {
		action[i] = KeyDownMap(i);
	}
}

void Joystick::ProcessAxes(double joy_x, double joy_y, double joy_r, double joy_t)
{
	// Minimal shaping; keep Starshatter’s API stable.
	// If you later want Starshatter-accurate deadzones/curves, implement them here.

	const double xx = Clamp11(inv_axis[0] ? -joy_x : joy_x);
	const double yy = Clamp11(inv_axis[1] ? -joy_y : joy_y);
	const double rr = Clamp11(inv_axis[2] ? -joy_r : joy_r);

	double tt = joy_t;
	if (inv_axis[3])
		tt = 1.0 - tt;
	tt = Clamp01(tt);

	x = xx;
	y = yy;

	// Keep legacy fields consistent with flight usage:
	r = x;       // roll
	p = -y;      // pitch (invert typical)
	w = rr;      // yaw/twist
	t = tt;      // throttle [0..1]

	z = 0.0;
}

void Joystick::ResetHat()
{
	for (int a = 0; a < 4; ++a)
		for (int b = 0; b < 4; ++b)
			hat[a][b] = false;
}

// ---------------------------
// Static digital queries
// ---------------------------

bool Joystick::KeyDown(int key)
{
	const UObject* WorldContext = FindWorldContextObject();
	if (!WorldContext)
		return false;

	UWorld* World = WorldContext->GetWorld();
	if (!World)
		return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
		return false;

	UJoystickManager* Manager = UJoystickManager::Get(WorldContext);
	if (!Manager)
		return false;

	// Starshatter key -> Unreal key is maintained by JoystickManager’s registry.
	const FKey UEKey = Manager->GetMappedKey(key);
	if (!UEKey.IsValid())
		return false;

	return Manager->IsKeyDown(PC, UEKey);
}

bool Joystick::KeyDownMap(int key)
{
	Joystick* js = GetInstance();
	if (!js)
		return false;

	if (key < 0 || key >= KEY_MAP_SIZE)
		return false;

	const int mapped = js->map[key];
	return KeyDown(mapped);
}

// ---------------------------
// Legacy device API stubs
// ---------------------------

void Joystick::EnumerateDevices()
{
	// Unreal-native. Enumeration is intentionally a no-op.
}

int Joystick::NumDevices()
{
	// Unreal-native. If you later add platform-specific device discovery, update here.
	return 1;
}

const char* Joystick::GetDeviceName(int i)
{
	// Keep stable return values for legacy UI.
	if (i == 0)
		return "Unreal Joystick";
	return "";
}

// ---------------------------
// Optional debug helpers
// ---------------------------

int Joystick::ReadRawAxis(int axis)
{
	// Derived from last sampled values (scaled to signed 16-bit).
	Joystick* js = GetInstance();
	if (!js)
		return 0;

	double v = 0.0;
	switch (axis) {
	case 0: v = js->x; break;
	case 1: v = js->y; break;
	case 2: v = js->w; break;
	case 3: v = (js->t * 2.0) - 1.0; break; // convert [0..1] to [-1..1]
	default: return 0;
	}

	v = Clamp11(v);
	return (int)(v * 32767.0);
}

int Joystick::GetAxisMap(int n)
{
	Joystick* js = GetInstance();
	if (!js || n < 0 || n > 3)
		return 0;
	return js->map_axis[n];
}

int Joystick::GetAxisInv(int n)
{
	Joystick* js = GetInstance();
	if (!js || n < 0 || n > 3)
		return 0;
	return js->inv_axis[n] ? 1 : 0;
}
