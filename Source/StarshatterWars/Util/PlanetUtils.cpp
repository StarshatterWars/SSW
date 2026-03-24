// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "PlanetUtils.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/FileHelper.h"
#include "Engine/Texture2D.h"

TMap<FString, UTexture2D*> PlanetUtils::LoadedTextureCache;

UTexture2D* PlanetUtils::LoadPlanetTexture(const FString& TextureName)
{
	// Check cache first
	if (LoadedTextureCache.Contains(TextureName))
	{
		return LoadedTextureCache[TextureName];
	}

	// Build full path
	const FString FilePath = FPaths::ProjectContentDir() / "GameData/Galaxy/PlanetMaterials/" + TextureName + ".png";

	TArray<uint8> RawData;
	if (!FFileHelper::LoadFileToArray(RawData, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load planet texture file: %s"), *FilePath);
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	EImageFormat Format = ImageWrapperModule.DetectImageFormat(RawData.GetData(), RawData.Num());
	if (Format == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Error, TEXT("Unsupported image format for: %s"), *FilePath);
		return nullptr;
	}

	TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(Format);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(RawData.GetData(), RawData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to decompress image: %s"), *FilePath);
		return nullptr;
	}

	TArray<uint8> Uncompressed;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Uncompressed))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract RGBA pixels from: %s"), *FilePath);
		return nullptr;
	}

	int32 Width = Wrapper->GetWidth();
	int32 Height = Wrapper->GetHeight();

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Texture) return nullptr;

	void* Data = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Uncompressed.GetData(), Uncompressed.Num());
	if (Texture)
	{
		#define UpdateResource UpdateResource
		Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
		Texture->UpdateResource();
	}
	// Safe unique name
	int32 Suffix = FMath::RandRange(1000, 9999);
	FString UniqueName = FString::Printf(TEXT("Tex_%s_%d"), *TextureName, Suffix);
	Texture->Rename(*UniqueName, GetTransientPackage());

	// Cache and return
	Texture->AddToRoot(); // Prevent GC
	Texture->SetFlags(RF_Public | RF_Standalone);
	LoadedTextureCache.Add(TextureName, Texture);

	UE_LOG(LogTemp, Log, TEXT("Planet texture loaded and cached: % s(% dx % d)"), *UniqueName, Width, Height);
	return Texture;
}

UTextureRenderTarget2D* PlanetUtils::CreatePlanetRenderTarget(const FString& Name, UObject* Outer, int32 Resolution)
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

FRotator PlanetUtils::GetPlanetAxisTilt(float TiltDegrees)
{
	// Clamp tilt to reasonable real-world range (-90 to +90)
	float ClampedTilt = FMath::Clamp(TiltDegrees, -90.0f, 90.0f);

	// Apply tilt to pitch (or roll if rotating sideways)
	// This assumes planet spins around Y-axis (roll)
	return FRotator(0.0f, 0.0f, ClampedTilt);
}

float PlanetUtils::GetNormalizedPlanetUIScale(double RadiusKm)
{
	// Based on actual Galaxy.def data
	constexpr double MinRadius = 3.15e6;    // Relay
	constexpr double MaxRadius = 38.2e6;    // Tal Amin

	// Clamp input radius to valid bounds
	double Clamped = FMath::Clamp(RadiusKm, MinRadius, MaxRadius);

	// Normalize to 0.0 - 1.0 range
	float Normalized = static_cast<float>((Clamped - MinRadius) / (MaxRadius - MinRadius));

	// Optional: Map to a display-friendly scale range
	constexpr float MinScale = 1.0f;
	constexpr float MaxScale = 4.0f;
	return FMath::Lerp(MinScale, MaxScale, Normalized);
}