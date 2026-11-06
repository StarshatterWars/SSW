// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScapeNoiseClass.h"
#include "EarthNoise.generated.h"

/**
 * 
 */
UCLASS()
class WORLDSCAPENOISE_API UEarthNoise : public UNoiseParameter
{
	GENERATED_BODY()
	
public:

	UEarthNoise();


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float PlanetaryHeight = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float TemperatureNoiseScale = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float TemperatureNoiseAffect = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float TemperatureMask = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float HumidityOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float OverallLandHeight = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float HumidityScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float WarpIntensity = 0.01f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float WarpScale = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float Warp2Scale = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float WarpScale2 = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float Warp2Scale2 = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float WarpIntensity2 = 0.01f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float continentalwarpIntensity = 0.01f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float continentalwarpcale = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		float continentalwarp2Scale = 100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		bool bIgnorePlanetaryHeightmap = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foilage")
		float FoilageValue = 1.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foilage")
		float FoilagetypeMask = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Mountain")
		float PlanetaryMountainInfulnce = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Mountain")
		float PlanetaryMountainInfulnceIntensity = 6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Mountain")
		float PlanetaryMountainInfulnceSharpness = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|ExtreamHills")
		float PlanetaryHillsInfulnce = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|ExtreamHills")
		float PlanetaryHillsInfulnceIntensity = 6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|ExtreamHills")
		float PlanetaryHillsInfulnceSharpness = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Hills")
		float PlanetarySmallHillsInfulnce = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Hills")
		float PlanetarySmallHillsInfulnceIntensity = 6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap|Hills")
		float PlanetarySmallHillsInfulnceSharpness = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap")
		float PlanetaryHeightPower = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap")
		float AvreagerInstensity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachMask = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachMaskOffset = 3.75;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachMaskWeight = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachHeight = 0.001;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Beach")
		float BeachHeight2 = 0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverVienScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverHeightScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverSharpness = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverBumpiness = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverBumpinessIntensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
		float RiverBumpinessOffset = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
		float RiverWarpIntensity = 0.01f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
		float Warp3Scape = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
		float RiverWarpScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverBranches")
		float RiverBranchScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverBranches")
		float RiverBranchSharpness = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorHeight = 0.015;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorBaseHeight = -0.1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		int OceanFloorOctave = 5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorLacunarity = 2.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorPersistence = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorWeight = 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		int OceanFloorWeightOctave = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorWeightScale = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
		float OceanFloorWeightPower = 256.0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainHeight = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainHeightOffset = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		FVector MountainShift;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainHeightPower = 1.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainScale = 0.15;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		int MountainOctave = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainLacunarity = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainPersistence = 0.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainWeight = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainWeightScale = 0.0025;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainWeightPower = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountWarpScale = 0.1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountWarp2Scale = 0.1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float MountainWarpInsesnity = 0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsHeight = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsHeightOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsHeightPower = 1.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsScale = 0.15;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		int ExteameHillsOctave = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsLacunarity = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsPersistence = 0.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsWeight = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsWeightScale = 0.0025;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float ExteameHillsWeightPower = 10;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float ExteameHillsWarpScale = 0.1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float ExteameHillsWarp2Scale = 0.1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
		float ExteameHillsWarpInsesnity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
		float HillHeight = .1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
		float HillScale = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
		int HillOctave = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
		float HillLacunarity = 2.3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
		float HillPersistence = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
		float HillsWeightScale = 0.0025;

};


class Water_Biome1 : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
	static double GetMask(CustomNoise NoiseClass, DVector position, int octave);
};

class IceCap1 : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
	static double GetMask(CustomNoise NoiseClass, DVector position, int octave);
};

class Mountain_Biome1 : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
	static double GetMask(CustomNoise NoiseClass, DVector position);
};

class Hill_Biome1 : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
};



class ExteameHills_Biome1 : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
	static double GetMask(CustomNoise NoiseClass, DVector position);
};

UCLASS()
class WORLDSCAPENOISE_API UEarthNoiseFuntion : public UWorldScapeNoiseClass {

	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
		UEarthNoise* NoiseParameters;

	FNoiseData GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;
	FNoiseData GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;





	FNoiseData GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise);
	UEarthNoiseFuntion();
};

