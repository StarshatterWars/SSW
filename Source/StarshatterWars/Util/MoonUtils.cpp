// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "MoonUtils.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/FileHelper.h"
#include "Engine/Texture2D.h"

float MoonUtils::GetUISizeFromRadius(float Radius, float MinSize, float MaxSize)
{
	constexpr float MinRadiusKm = 0.25e6;
	constexpr float MaxRadiusKm = 6e6;

	float LogRadius = FMath::LogX(10.f, FMath::Max(Radius, 1.f));
	float MinLog = FMath::LogX(10.f, MinRadiusKm);
	float MaxLog = FMath::LogX(10.f, MaxRadiusKm);

	float Normalized = FMath::Clamp((LogRadius - MinLog) / (MaxLog - MinLog), 0.f, 1.f);
	//return FMath::Lerp(MinSize, MaxSize, Normalized);

	// UI scaling range — safe for mesh scale and texture logic
	constexpr float MinUIScale = 16.0f;
	constexpr float MaxUIScale = 64.0f;

	// Final safe UI scale
	float UIScale = FMath::Lerp(MinUIScale, MaxUIScale, static_cast<float>(Normalized));

	// Clamp final value to hard max
	return FMath::Clamp(UIScale, MinUIScale, MaxUIScale);
}

UTexture2D* MoonUtils::LoadMoonAssetTexture(const FString& TextureName)
{
	FString AssetPath = FString::Printf(TEXT("/Game/GameData/Galaxy/MoonMaterials/%s.%s"), *TextureName, *TextureName);

	UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath));
	if (!Texture)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load moon texture asset: %s"), *AssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded moon texture asset: %s"), *Texture->GetName());
	}

	return Texture;
}

float MoonUtils::GetMoonUIScale(double RadiusKm)
{
	// Min and max expected radius (from Galaxy.def or design)
	constexpr double MinRadius = 0.25e6;     // Small moons
	constexpr double MaxRadius = 6e6;   // Large gas giants

	// Get normalized [0.0, 1.0] range
	double Normalized = FMath::Clamp((RadiusKm - MinRadius) / (MaxRadius - MinRadius), 0.0, 1.0);

	// UI scaling range — safe for mesh scale and texture logic
	constexpr float MinUIScale = 0.5f;
	constexpr float MaxUIScale = 4.0f;

	// Final safe UI scale
	float UIScale = FMath::Lerp(MinUIScale, MaxUIScale, static_cast<float>(Normalized));

	// Clamp final value to hard max
	return FMath::Clamp(UIScale, MinUIScale, MaxUIScale);
}

FRotator MoonUtils::GetMoonRotation(float TimeSeconds, float RotationSpeedDegreesPerSec, float TiltDegrees)
{
	// Clamp tilt for safety
	float ClampedTilt = FMath::Clamp(TiltDegrees, -90.0f, 90.0f);

	// Compute current Yaw based on speed and time
	float CurrentYaw = FMath::Fmod(TimeSeconds * RotationSpeedDegreesPerSec, 360.0f);

	// Rotation = spin (Yaw) combined with axis tilt (Roll or Pitch)
	return FRotator(0.0f, CurrentYaw, ClampedTilt); // (Pitch, Yaw, Roll)
}

float MoonUtils::GetNormalizedMoonUIScale(double RadiusKm)
{
	// Based on actual Galaxy.def data
	constexpr double MinRadius = 0.25e6;    // Relay
	constexpr double MaxRadius = 6.0e6;    // Tal Amin

	// Clamp input radius to valid bounds
	double Clamped = FMath::Clamp(RadiusKm, MinRadius, MaxRadius);

	// Normalize to 0.0 - 1.0 range
	float Normalized = static_cast<float>((Clamped - MinRadius) / (MaxRadius - MinRadius));

	// Optional: Map to a display-friendly scale range
	constexpr float MinScale = 1.0f;
	constexpr float MaxScale = 4.0f;
	return FMath::Lerp(MinScale, MaxScale, Normalized);
}

UTextureRenderTarget2D* MoonUtils::CreateMoonRenderTarget(const FString& Name, UObject* Outer, int32 Resolution)
{
	if (!Outer)
	{
		Outer = GetTransientPackage(); // fallback
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(Outer, *Name);
	if (!RenderTarget)
	{
		return nullptr;
	}

	RenderTarget->RenderTargetFormat = RTF_RGBA16f; // good for lit previews
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->InitAutoFormat(Resolution, Resolution);
	RenderTarget->UpdateResourceImmediate(true);

	return RenderTarget;
}

FRotator MoonUtils::GetMoonAxisTilt(float TiltDegrees)
{
	// Clamp tilt to reasonable real-world range (-90 to +90)
	float ClampedTilt = FMath::Clamp(TiltDegrees, -90.0f, 90.0f);

	// Apply tilt to pitch (or roll if rotating sideways)
	// This assumes planet spins around Y-axis (roll)
	return FRotator(0.0f, 0.0f, ClampedTilt);
}

