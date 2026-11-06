// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScapeNoiseClass.h"
#include "EarthTerraMix.generated.h"

/**
 * 
 */
UCLASS()
class WORLDSCAPENOISE_API UEarthTerraMix : public UNoiseParameter
{
	GENERATED_BODY()
	
public:

	UEarthTerraMix();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureOffset = -0.013251;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureLandintensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureLandPower = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureWeight = -0.136;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureNoiseScale1 = 0.001;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureNoiseOffset = 0.588;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperatureOffset2 = -0.090962;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float TemperaturePolarSharpness = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Temperature")
	float HumidityInflunce = 1;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float PlanetaryHeight = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float TestOcean = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float TemperatureNoiseScale = 0.002;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float TemperatureNoiseAffect = 0.152381;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float TemperatureMask = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float HumidityOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float OverallLandHeight = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float WarpIntensity = 0.01f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float WarpScale = 6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float Warp2Scale = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float WarpScale2 = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float Warp2Scale2 = 0.01;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	float WarpIntensity2 = 900.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	bool bIgnorePlanetaryHeightmap = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	bool bTextureTempratureChannelAsTypography = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	bool IsTidalLocked = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float HumidityScale = 0.0015;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float CloudLayerRunoffsharpness = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float CloudLayerRunoff = 0.06;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float CloudLayerMaxheight = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float CloudPercipitationIntensity = 0.175;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Humidity")
	float DeformationRunoffIntensity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	float continentalwarpIntensity = 1200.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	float continentalwarpScale = 4.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	float continentalwarp2Scale = 0.007;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	int ContinentOctave = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	float continentLacunarity = 2.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Continents")
	float continentPersistence = 0.4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float PlanetaryhumidMask = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float PlanetaryPostAffect = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Deformation")
	float DeformationAreaContrast = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Deformation")
	float DefomationCutoff = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Deformation")
	float DefomationIntesity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Hills")
	float hillsAreaContrast = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Hills")
	float hillsCutoff = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Hills")
	float hillsIntesity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Mountains")
	int PlanetaryMountainMaskPower = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Mountains")
	float PlanetaryMountainInfluence = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Mountains")
	float PlanetaryMountainOffset = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Humidity")
	float PlanetarHumidityPower = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|Humidity")
	float PlanetarHumidityIntensity = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|ExtremeHills")
	float PlanetaryHillsInfluence = 0.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|ExtremeHills")
	float PlanetaryHillsPower = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional|ExtremeHills")
	float PlanetaryHillsOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float PlanetaryHeightPower = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float MountainyHeightInfluncence = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float HeightCutOff = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|PlanetaryHeightmap-Optional")
	float HeightSharpness = 1;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapHeight = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float Aysemtrical = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapTansitionShaprness = 30;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapTansition = 1.0652;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapTansitionNoiseScale = 0.008;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapNoiseTansition = 0.106;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapNoiseTansition2 = 0.17;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapNoiseLacunarity = 2.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float IceCapNoisePersistence = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|IceCap")
	float MountainInflunce = 0.125;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverVienScale = 0.029047;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverHeightScale = -0.004382;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverSharpness = 156.947586;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverBumpiness = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverBumpinessIntensity = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2")
	float RiverBumpinessOffset = 1.070637;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
	float RiverWarpIntensity = 2.091608f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
	float Warp3Scape = 1.406435;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverWarp")
	float RiverWarpScale = 0.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverBranches")
	float RiverBranchScale = 0.309524;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|River2|RiverBranches")
	float RiverBranchSharpness = 15.854459;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	float OceanFloorWeight = 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	int OceanFloorWeightOctave = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	float OceanFloorWeightScale = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	float OceanFloorWeightPower = 256.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	float OceanLacunarity = 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Ocean")
	float OceanPersistence = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainHeight = 1.15;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainHeightOffset = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	FVector MountainShift;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainHeightPower = 1.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainScale = 0.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainDetailMethodAlpha = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainLacunarity = 2.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainPersistence = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainPersistence1 = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float Erosion = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float Erosion1 = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float Damp = 0.8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float Damp2 = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float DampScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainWeight = 1.45;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainWeightScale = 0.0025;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainWeightPower = 12;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountWarpScale = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountWarp2Scale = 0.05;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain")
	float MountainWarpIntensity = 50;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	float MountainDetailScale = 0.494;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	float MountainDetailHeight = 1.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	float MountainHeighDetailPower = 1.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	int MountainOctave = 8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	float MountainDetailLacunarity = 2.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Mountain|Detail")
	float MountainDetailPersistence = 0.35;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsHeight = 0.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsDetailHeight = 0.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsHeightOffset = 0.3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsHeightPower = 0.76;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWeight = 1.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWeightScale = 0.0025;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWeightPower = 3.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWarpScale = 3.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWarp2Scale = 0.02;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills")
	float ExteameHillsWarpInsesnity = 200.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	int ExteameHillsOctave = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsLacunarity = 2.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsPersistence = 0.36;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsPersistence1 = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsErosion = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsErosion1 = 0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsDamp = 0.8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsDamp2 = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|ExteameHills|NoiseProperties")
	float ExteameHillsDampScale = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float HillHeight = -0.02;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float HillScale = 0.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	int HillOctave = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float HillLacunarity = 2.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float HillPersistence = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float MicroHillsMask = 200.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Hill")
	float HeightOffset = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Coastline")
	float CoastlineShift = 0.125;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Coastline")
	float CoastlineShift2 = 0;
};


class Water_BiomeMix : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
	static double GetMask(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
};

class Mountain_BiomeMix : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence, float persistence1, float warp, float warp0, float damp, float damp0, float dampScale);
	static double GetMask(CustomNoise NoiseClass, DVector position);
};

class Hill_BiomeMix : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence);
};

class ExteameHills_BiomeMix : BiomeNoise {
public:
	static double GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence, float persistence1, float warp, float warp0, float damp, float damp0, float dampScale);
	static double GetMask(CustomNoise NoiseClass, DVector position);
};

UCLASS()
class WORLDSCAPENOISE_API UearthTerra : public UWorldScapeNoiseClass {

	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	UEarthTerraMix* NoiseParameters;

	FNoiseData GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;
	FNoiseData GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;





	FNoiseData GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise);
	UearthTerra();
};
