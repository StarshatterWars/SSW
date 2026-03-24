/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         FadeSubsystem.h
    AUTHOR:       Carlos Bott

    ORIGINAL DESIGN CONTEXT
    =======================
    Legacy Starshatter used global, renderer-owned fade state via
    Color::SetFade() and palette shade tables. That approach relied on
    single-threaded, paletted video hardware and static globals.

    UNREAL PORT DESIGN
    ==================
    UFadeSubsystem replaces legacy global fade state with an
    engine-owned, deterministic container:

      • Fade state lives in the GameInstance (one per game session)
      • FadeView drives fade timing and writes values
      • HUD, Camera, Screen, or PostProcess code can read values
      • No static globals
      • No render-thread ownership assumptions
      • Safe for PIE, multiplayer, and future threading

    RESPONSIBILITIES
    ================
      • Store current fade amount (0–1)
      • Store fade color (typically black)
      • Preserve legacy "shade built" semantic for callers
      • Provide a single authoritative fade source for the engine

    NON-RESPONSIBILITIES
    ====================
      • Does NOT render anything
      • Does NOT own timing or transitions
      • Does NOT apply post-processing
      • Does NOT manage palettes or shade tables

    OWNERSHIP & LIFETIME
    ====================
      • Owned by UGameInstance
      • Created automatically by Unreal
      • Destroyed on game shutdown / world teardown

    RELATED CLASSES
    ===============
      • FadeView        – Legacy fade controller logic
      • Screen / HUD    – Consumers of fade state
      • CameraView      – Optional consumer for full-screen fades

*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FadeSubsystem.generated.h"

/**
 * Global fade state store (engine-owned).
 * FadeView writes; HUD/Renderer reads.
 */
UCLASS()
class STARSHATTERWARS_API UFadeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Read API ---
	UFUNCTION(BlueprintCallable, Category = "Fade")
	double GetFade() const { return Fade; }

	UFUNCTION(BlueprintCallable, Category = "Fade")
	FColor GetFadeColor() const { return FadeColor; }

	UFUNCTION(BlueprintCallable, Category = "Fade")
	bool IsShadeBuilt() const { return bShadeBuilt; }

	// --- Write API ---
	// Clamp enforced in implementation.
	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetFade(double InFade, const FColor& InColor, bool bBuildShade = false);

	// Optional convenience:
	UFUNCTION(BlueprintCallable, Category = "Fade")
	void ResetFade()
	{
		Fade = 1.0;
		FadeColor = FColor::Black;
		bShadeBuilt = false;
	}

private:
	// 0..1
	UPROPERTY()
	double Fade = 1.0;

	UPROPERTY()
	FColor FadeColor = FColor::Black;

	UPROPERTY()
	bool bShadeBuilt = false;
};
#pragma once
