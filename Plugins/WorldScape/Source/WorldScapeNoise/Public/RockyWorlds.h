// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScapeNoiseClass.h"
#include "RockyWorlds.generated.h"

/**
 * 
 */




UCLASS()
class WORLDSCAPENOISE_API URockyWorlds : public UNoiseParameter
{
	GENERATED_BODY()

	public:
	URockyWorlds();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarp2Scale = 0.003;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|Noise Settings")
	int ContinentOctave = 7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|Noise Settings")
	float continentLacunarity = 2.3;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|Noise Settings")
	float continentPersistence = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarpScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float continentalwarpIntensity = 500;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float IslandArc = 14.2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics")
	float IslandArcSharpness = 30;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|orogeny")
	float Mask1 = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|orogeny")
	float Mask2 = 0.78;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Tectonics|orogeny")
	float OragneyVeinScale = 0.004;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float ConentialWeightPercentage = 0.55;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float ConentialMaskSharpness = 0.05;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential")
	float ContentialNoiseScale = 0.0015;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float MicroHillsIntensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float BigHillsScale = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float BigHillsSoftness = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float MicroHillsScale = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float PuffyHillsIntensity = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hills")
	float PuffyHillsScale = 1;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Orogeny")
	float OrogenyWeightStart = 1.46;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Orogeny")
	float OrogenyWeightRange = 0.17;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotWeightStart = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotWeightRange = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotHeight = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Conential|Hotsots")
	float HotSpotNoiseScale = 0.03;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Oceanic")
	float OceanFloorHeight = -0.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Oceanic")
	float SubductionIntensity = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Crust Properties|Oceanic")
	float SeaFloorSpreadingIntensity = 1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperatureOffset = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Climate|Temperature")
	float TemperatureAysemetry = 0;
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


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TestViews")
	float HeightTester = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TestViews")
	bool ShowOceanElvation = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General|Craters")
	bool GenCraters = false;
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
class WORLDSCAPENOISE_API URockyWorldsNoise : public UWorldScapeNoiseClass {

	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "General")
	URockyWorlds* NoiseParameters;

	FNoiseData GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;
	FNoiseData GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary) override;



	FNoiseData GetHeight(CustomNoise NoiseClass, DVector position, DVector PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise);
	URockyWorldsNoise();
};
