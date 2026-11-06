// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "WorldScapeCommon/Public/DoublePrecisionUtils.h"
#include "WorldScapeCommon/Public/NoiseMathUtils.h"
#include "HeightMapVolumeData.h"
#include "HeightMapVolumeDataCopy.generated.h"


// ----------------------------------------------------
// Heightmap blend
// ----------------------------------------------------
UENUM(BlueprintType)
enum class EWSHeight_Blend_Type : uint8
{
	ShapeFalloff,
	FullBlend
};
// -----------------------
UENUM(BlueprintType)
enum class EWSHeight_Blend_Shape : uint8
{
	Square,
	Circle
};
// -----------------------
UENUM(BlueprintType)
enum class EWSHeight_Blend_Methode : uint8
{
	Add,
	Sub,
	Min,
	Max,
	Mul,
	Average,
	Difference,
	Overlay,
	Screen,
	SoftLight,
	ClampAdd,
	LerpBlend
};
// -----------------------
UENUM(BlueprintType)
enum class EWSHeight_Blend_Curve : uint8
{
	Linear,
	Quadratic,
	QuadraticInverse,
	Cubic,
	CubicInverse,
	ExpoIn,
	ExpoOut,
	ExpoInOut,
	EaseIn,
	EaseOut
};
// -----------------------
USTRUCT(BlueprintType)
struct WORLDSCAPEVOLUME_API FWSHeight_Blend
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Blend")
	EWSHeight_Blend_Type BlendType = EWSHeight_Blend_Type::ShapeFalloff;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Blend")
	EWSHeight_Blend_Shape ShapeType = EWSHeight_Blend_Shape::Square;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Blend")
	EWSHeight_Blend_Methode BLendMethode = EWSHeight_Blend_Methode::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Blend")
	EWSHeight_Blend_Curve BlendCurve = EWSHeight_Blend_Curve::Quadratic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_Minimal = 0.99f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 12.0f, ClampMax = 12.0f),Category = "Blend")
	float Blend_Exposent = 2.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_Intensity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_FallOff = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_NoiseShapeIntensity = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_NoiseIntensity = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 100.0f, ClampMax = 100.0f),Category = "Blend")
	float Blend_NoiseScale = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite , meta = (UIMin = 0, ClampMin = 0, UIMax = 1.0f, ClampMax = 1.0f),Category = "Blend")
	float Blend_NoiseToAll = 0.0f;
	
	float CircleCompute( const FVector2D& UV , const float Add = 0 ) const
	{
		/*
		FVector2D CenteredUV = UV - FVector2D(0.5f, 0.5f);
		float Distance = CenteredUV.Size();
		float Radius = 0.5f;
		float StartBlend = Radius - Blend_FallOff + Add;
		float Blend = FMath::Clamp((Radius - Distance) / Blend_FallOff, 0.0f, 1.0f);
		return (Distance < StartBlend) ? 1.0f : Blend;
		*/
		FVector2D CenteredUV = UV - FVector2D(0.5f, 0.5f);
		float Distance = CenteredUV.Size();
		Distance += Add * Blend_NoiseShapeIntensity;
		float Radius = 0.5f;
		float StartBlend = Radius - Blend_FallOff + Add;
		float Blend = FMath::Clamp((Radius - Distance) / Blend_FallOff, 0.0f, 1.0f);
		return (Distance < StartBlend) ? 1.0f : Blend;
	}
	float SquareCompute( const FVector2D& UV , const float Add = 0 ) const
	{
		float Out = FMath::Clamp((0.5 / Blend_FallOff) - abs(0.5 - UV.X + Add * Blend_NoiseShapeIntensity) / Blend_FallOff, 0., 1.);
		Out *= FMath::Clamp((0.5 / Blend_FallOff) - abs(0.5 - UV.Y + Add * Blend_NoiseShapeIntensity) / Blend_FallOff, 0., 1.);
		return Out;
	}
	float ApplyBlendMethod(float A, float B, EWSHeight_Blend_Methode Method) const
	{
		float Delta = B - A;

		switch (Method)
		{
		case EWSHeight_Blend_Methode::Add:
			return A + Delta;

		case EWSHeight_Blend_Methode::Sub:
			return A - Delta;

		case EWSHeight_Blend_Methode::Min:
			return A + FMath::Min(0.0f, Delta);

		case EWSHeight_Blend_Methode::Max:
			return A + FMath::Max(0.0f, Delta);

		case EWSHeight_Blend_Methode::Mul:
			return A * (1 + (Delta / A));

		case EWSHeight_Blend_Methode::Average:
			return (A + B) * 0.5f;

		case EWSHeight_Blend_Methode::Difference:
			return FMath::Abs(Delta);

		case EWSHeight_Blend_Methode::Overlay:
			return (A < 0.5f) ? (2.0f * A * B) : (1.0f - 2.0f * (1.0f - A) * (1.0f - B));

		case EWSHeight_Blend_Methode::Screen:
			return 1.0f - (1.0f - A) * (1.0f - B);

		case EWSHeight_Blend_Methode::SoftLight:
			return (1.0f - 2.0f * B) * A * A + 2.0f * B * A;

		case EWSHeight_Blend_Methode::ClampAdd:
			return FMath::Clamp(A + Delta, 0.0f, 1.0f);

		case EWSHeight_Blend_Methode::LerpBlend:
			return FMath::Lerp(A, B, Blend_Intensity);

		default:
			return A;
		}
	}
	float EaseIn(float Alpha, float Exponent) const
	{
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return FMath::Pow(Alpha, Exponent);
	}

	float EaseOut(float Alpha, float Exponent) const
	{
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - Alpha, Exponent);
	}
	float Interpolation(float Start, float End, float Alpha, int32 InterpolationType, float ExpoValue ) const
	{
		// Clamp Alpha entre 0 et 1
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		float Result = Start;

		switch (InterpolationType)
		{
		case 0: // Linear
			Result = FMath::Lerp(Start, End, Alpha);
			break;

		case 1: // Quadratic
			Result = FMath::Lerp(Start, End, Alpha * Alpha);
			break;

		case 2: // QuadraticInverse
			Result = FMath::Lerp(Start, End, FMath::Sqrt(Alpha));
			break;

		case 3: // Cubic
			Result = FMath::Lerp(Start, End, Alpha * Alpha * Alpha);
			break;

		case 4: // CubicInverse
			Result = FMath::Lerp(Start, End, FMath::Pow(Alpha, 1.0f/3.0f));
			break;

		case 5: // ExpoIn
			Result = FMath::Lerp(Start, End, FMath::Pow(Alpha, ExpoValue));
			break;

		case 6: // ExpoOut
			Result = FMath::Lerp(Start, End, 1.0f - FMath::Pow(1.0f - Alpha, ExpoValue));
			break;

		case 7: // ExpoInOut
			if (Alpha < 0.5f)
			{
				Result = FMath::Lerp(Start, (Start + End) * 0.5f, 
					FMath::Pow(Alpha * 2.0f, ExpoValue) * 0.5f);
			}
			else
			{
				Result = FMath::Lerp((Start + End) * 0.5f, End,
					1.0f - FMath::Pow(2.0f * (1.0f - Alpha), ExpoValue) * 0.5f);
			}
			break;
		case 8: // EaseIn
			Result = FMath::Lerp(Start, End, EaseIn(Alpha, ExpoValue));
			break;

		case 9: // EaseOut
			Result = FMath::Lerp(Start, End, EaseOut(Alpha, ExpoValue));
			break;
		}
		return Result;
	}
	float HashNoise(const FVector2D& UV, float Scale = 1.0f) const
	{
		FVector2D ScaledUV = UV * Scale;
		float Dot = FVector2D::DotProduct(ScaledUV, FVector2D(12.9898f, 78.233f));
		float SinVal = FMath::Sin(Dot) * 43758.5453f;
		return FMath::Frac(SinVal);
	}
	float ValueNoise2D(const FVector2D& UV, float Scale = 10.0f) const
	{
		FVector2D ScaledUV = UV * Scale;
		int32 X0 = FMath::FloorToInt(ScaledUV.X);
		int32 Y0 = FMath::FloorToInt(ScaledUV.Y);
		int32 X1 = X0 + 1;
		int32 Y1 = Y0 + 1;
		float Fx = ScaledUV.X - X0;
		float Fy = ScaledUV.Y - Y0;
		auto Hash = [](int32 X, int32 Y) -> float
		{
			int32 N = X + Y * 57;
			N = (N << 13) ^ N;
			return 1.0f - ((N * (N * N * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
		};
		float V00 = Hash(X0, Y0);
		float V10 = Hash(X1, Y0);
		float V01 = Hash(X0, Y1);
		float V11 = Hash(X1, Y1);
		float U = Fx * Fx * (3.0f - 2.0f * Fx);
		float V = Fy * Fy * (3.0f - 2.0f * Fy);
		float I1 = FMath::Lerp(V00, V10, U);
		float I2 = FMath::Lerp(V01, V11, U);
		return FMath::Lerp(I1, I2, V) * 0.5f + 0.5f; // [0,1]
	}
	float GetBlend(float Start, float End, const FVector2D& UV) const
	{
		switch (BlendType)
		{
		case EWSHeight_Blend_Type::ShapeFalloff:
			{
				float Noise = ValueNoise2D(UV, Blend_NoiseScale);
				float NoiseAdd = FMath::Clamp(Noise * Blend_NoiseIntensity - 0.5,-0.5,0.5);
				float ShapeMask = (ShapeType == EWSHeight_Blend_Shape::Circle) ? CircleCompute(UV,NoiseAdd) : SquareCompute(UV,NoiseAdd);
				ShapeMask += (Noise - 0.5f) * Blend_NoiseIntensity * Blend_NoiseToAll;
				ShapeMask = FMath::Clamp(ShapeMask, 0.0f, Blend_Minimal);
				float Interpolated = Interpolation(Start, End, ShapeMask, static_cast<int32>(BlendCurve), Blend_Exposent);
				float Blended = FMath::Lerp(Start, Interpolated, Blend_Intensity);
				return ApplyBlendMethod(Start, Blended, BLendMethode);
			}
		case EWSHeight_Blend_Type::FullBlend:
			return ApplyBlendMethod(Start, End, BLendMethode);
		default:
			return End;
		}
	}
	bool GetEqual( const FWSHeight_Blend& B )
	{
		return
			BlendType     == B.BlendType &&
			ShapeType     == B.ShapeType &&
			BLendMethode  == B.BLendMethode &&
			BlendCurve    == B.BlendCurve &&
			Blend_NoiseIntensity == B.Blend_NoiseIntensity &&
			Blend_NoiseScale == B.Blend_NoiseScale &&
			Blend_Minimal == B.Blend_Minimal &&
			Blend_NoiseToAll == B.Blend_NoiseToAll &&
			Blend_NoiseShapeIntensity == B.Blend_NoiseShapeIntensity &&
			FMath::IsNearlyEqual(Blend_Exposent, B.Blend_Exposent, KINDA_SMALL_NUMBER) &&
			FMath::IsNearlyEqual(Blend_Intensity, B.Blend_Intensity, KINDA_SMALL_NUMBER) &&
			FMath::IsNearlyEqual(Blend_FallOff, B.Blend_FallOff, KINDA_SMALL_NUMBER);
	}
};
// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct WORLDSCAPEVOLUME_API FHeightMapVolumeDataCopy {
	GENERATED_BODY()
public:
	bool IsEnabled;
	float EdgeFalloff;
	bool UseBlending;
	FWSHeight_Blend Blending;
	FTransform VolumeLocation;
	UHeightMapVolumeData* HeightMapData;
	bool bOverrideHeight = true;
	bool bHeight = false;
	bool bTemperature = false;
	bool bHumidity = false;
	bool bHeightAlpha = false;
	bool bOceanHeight = false;
	bool bOceanHeightAlpha = false;
	float MapHeight = 10000;
	float Altitude = 0;
	float MaxHeight = 0;
	float MinHeight = 0;
	EHMSamplingMethod SamplingMethod = EHMSamplingMethod::Bilinear;

	float MaxTemperature = 0;
	float MinTemperature = 0;
	float MaxHumidity = 0;
	float MinHumidity = 0;
	float MaxOceanHeight = 0;
	float MinOceanHeight = 0;

	bool CanSample() ;

	bool CanSample(EWSHeightMask Mask);
	
	bool IsHeightDataValid() const;
	
	double GetData(FDVector2D UV, EWSHeightMask Mask) const;
};