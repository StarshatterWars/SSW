// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScapeNoiseClass.h"
#include "HeightMapBased.generated.h"

/**
 * 
 */

UCLASS()
class WORLDSCAPENOISE_API UHeightMap : public UNoiseParameter
{
	GENERATED_BODY()

public:
	UHeightMap();



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|HeightMap")
	float HeightOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|HeightMap")
	float MultiplyHeightAboveSeaLevel = 1.75;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|HeightMap")
	float MultiplyHeightBelowSeaLevel = 1.8;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|HeightMap")
	float ExponetialFilterValue = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|HeightMap")
	float LandSeaLevelCorrection = 1;




	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarp2Scale = 0.003;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarpScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarpIntensity = 500;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float ContentialNoiseScale = 0.0015;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float MicroWarpScale  = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float IceSheetThickness = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float IceSheetSoftness = 0.35;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float IceSheetTansitionOveride = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float MicroHillsIntensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float MicroHillsWarpIntensity = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float MicroHillsScale = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float PuffyHillsIntensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float PuffyHillsScale = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills|masks")
	float SharpHillMaskStart = 0.09;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills|masks")
	float SharpHillMaskEnd = 0.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills|masks")
	float PuffyHillMaskStart = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills|masks")
	float PuffyHillMaskEnd = 0.3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	bool GenHotspots = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotWeightStart = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotWeightRange = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotHeight = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotNoiseScale = 0.03;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Mountains")
	float MountainsIntensity = 0.11;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Mountains")
	float MountainsBaseStart = 0.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Mountains")
	float NoVienMaskAlpha = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperatureOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperatureAysemetry = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperatureEquatorHeightCap = 0.52;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperaturePolarHeightCap = 0.11;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature|Advanced")
	float TemperatureNoiseAmount = 0.7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature|Advanced")
	float PolarSmoothStepBeginning = -0.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature|Advanced")
	float PolarSmoothStepend = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	bool UseProceduralClimate = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float ElevationClimateShadow = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float HadleyCellsAmount = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float HadleyCellsTurbulance = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float TurbulantCellsScale = 0.0045;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float humidityMultiplier = 1.4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float EquatorBiasAlpha = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity")
	float BlendDataMap = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity|CellProperties")
	float Cell1Power = 4;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity|CellProperties")
	float Cell1Intensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity|CellProperties")
	float Cell2Power = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Humidity|CellProperties")
	float Cell2Intensity = 1.3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters")
	bool GenCraters = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float HeightOffsetCraters = -0.17;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterFrequency = 1.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	int CraterOctave = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterLacunarity = 5.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterPersistence = 0.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterHeightMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterSize = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterRimHeight = 14;
	//Number of crater Octave that possess crater rim
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterRimOctaveLimit = 3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	int CraterRimPower = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterWarp = 5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters", meta = (EditCondition = "GenCraters", EditConditionHides))
	float CraterNoiseSize = 100;


};

UCLASS()
class WORLDSCAPENOISE_API UHeightMapBased : public UWorldScapeNoiseClass
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	UHeightMap* NoiseParameters;

	FNoiseData GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;
	FNoiseData GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;

	

	FNoiseData GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise);
	UHeightMapBased();

};
