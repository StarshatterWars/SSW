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

#include "FadeSubsystem.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogFadeSubsystem, Log, All);

void UFadeSubsystem::SetFade(double InFade, const FColor& InColor, bool bBuildShade)
{
	const double Clamped = FMath::Clamp(InFade, 0.0, 1.0);

	// Avoid churn if nothing changed:
	if (Fade == Clamped && FadeColor == InColor && bShadeBuilt == bBuildShade)
	{
		return;
	}

	Fade = Clamped;
	FadeColor = InColor;
	bShadeBuilt = bBuildShade;

	UE_LOG(LogFadeSubsystem, VeryVerbose,
		TEXT("UFadeSubsystem::SetFade fade=%.3f color=(%d,%d,%d,%d) shade=%d"),
		Fade, FadeColor.R, FadeColor.G, FadeColor.B, FadeColor.A,
		bShadeBuilt ? 1 : 0);
}
