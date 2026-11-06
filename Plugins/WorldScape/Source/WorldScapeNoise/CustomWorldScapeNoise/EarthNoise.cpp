// Fill out your copyright notice in the Description page of Project Settings.


#include "EarthNoise.h"






UEarthNoise::UEarthNoise() {
}

double Water_Biome1::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.FractalRidge(position, octave, lacunarity, persistence);
}
double Water_Biome1::GetMask(CustomNoise NoiseClass, DVector position, int octave) {
	return NoiseClass.Fractal(position, octave);
}
double IceCap1::GetMask(CustomNoise NoiseClass, DVector position, int octave) {
	return NoiseClass.Fractal(position, octave);
}
double IceCap1::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}
double Mountain_Biome1::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double Mountain_Biome1::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 6);
}

double Hill_Biome1::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}



double ExteameHills_Biome1::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double ExteameHills_Biome1::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 5);
}




FNoiseData UEarthNoiseFuntion::GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
{






	double warp = NoiseClass.Noise_01(NoisePosition * NoiseParameters->Warp2Scale);
	DVector WarpedPosition = NoisePosition * NoiseParameters->WarpScale + FVector(warp, warp, warp) * NoiseParameters->WarpIntensity;

	double continentalwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->continentalwarp2Scale, 3);
	DVector continentalWarpedPosition = NoisePosition * NoiseParameters->continentalwarpcale + FVector(continentalwarp, continentalwarp, continentalwarp) * NoiseParameters->continentalwarpIntensity;


	double Mountwarp = NoiseClass.Noise_01(NoisePosition * NoiseParameters->MountWarp2Scale);
	DVector MountainWarpedPosition = NoisePosition * NoiseParameters->MountWarpScale + FVector(Mountwarp, Mountwarp, Mountwarp) * NoiseParameters->MountainWarpInsesnity;

	double ExteameHillswarp = NoiseClass.Noise_01(NoisePosition * NoiseParameters->ExteameHillsWarp2Scale);
	DVector ExteameHillswarpPosition = NoisePosition * NoiseParameters->ExteameHillsWarpScale + FVector(ExteameHillswarp, ExteameHillswarp, ExteameHillswarp) * NoiseParameters->ExteameHillsWarpInsesnity;

	double warp2 = NoiseClass.Noise_01(NoisePosition * NoiseParameters->Warp2Scale2);
	DVector WarpedPosition2 = NoisePosition * NoiseParameters->WarpScale2 + FVector(warp2, warp2, warp2) * NoiseParameters->WarpIntensity2;

	double warp3 = NoiseClass.Fractal(NoisePosition * NoiseParameters->Warp3Scape, 5);
	DVector RiverWarpedPosition1 = NoisePosition * NoiseParameters->RiverWarpScale + FVector(warp3, warp3, warp3) * NoiseParameters->RiverWarpIntensity;

	double GeneralNoise = NoiseClass.Noise_01(NoisePosition * 0.005f);
	double TempNoise = (NoiseClass.Fractal(WarpedPosition2 * 1.5f * NoiseParameters->TemperatureNoiseScale, 2));

	/*
	**	FNoiseData, is a struct to register every data you want from the noise generation, you can edit them as you wish and use the param here,
	*
	**	the only parametter the PlanetRoot is using are :
	**	HeightNormalize		- Height value in a range of 0 to 1, used to register the height value in the Red Vertex Color.
	**	Height;				- Height value in Unreal Unit, allow to get the real height of the ground.
	**	Temperature;		- Temperature from a range of 0 to 1.
	**	Humidity;			- Humidity from a range of 0 to 1.
	**	FoliageMask			- Used for foliage densitty (not used right now but will be is near future).
	*/
	FNoiseData Data = FNoiseData();
	if (FlatWorld) {
		Data.Temperature = NoiseClass.Fractal(WarpedPosition * 0.028143, 6);
	}
	else {
		float LattitudeRemapped = 1 - abs(lattitude);

		double OceanTempMask = (pow(Water_Biome1::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave) * NoiseParameters->OceanFloorWeight, 2) - 0.05f);


		//Humidity and temperature based on planet latitude, you can use these value for terrain generation (using them as mask for biome) 
		//and for texture (they are saved as Green (temperature) and Blue (Humidity) in the vertex Color of the terrain Mesh;
		//Data.Temperature = (TempNoise * NoiseParameters->TemperatureNoiseAffect) + pow(1 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 2, 2) + NoiseParameters->TemperatureOffset;

		//Data.Temperature = NoiseMathUtils::Lerp(Data.Temperature, (TempNoise * NoiseParameters->TemperatureNoiseAffect), NoiseParameters->TemperatureMask + (NoiseParameters->TemperatureOffset2 + pow(1 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 2, 2)));
		



		
		Data.Humidity = NoiseMathUtils::Clamp01(ActualData.Humidity * 1.1 - (NoiseClass.Fractal(WarpedPosition * 0.004443, 6 , 1.5 , 1.1) * 0.2));
	}

	//ExtreamHills Gen
	double ExteameHillsNoisemesking2 = NoiseMathUtils::Clamp01(pow(ExteameHills_Biome1::GetNoise(NoiseClass, ExteameHillswarpPosition * 310, 3, 1.5, 0.8) * 1 , 2) * 2 - .1);

	 double ExteameHillsNoisemesking = NoiseMathUtils::Clamp01(pow(ExteameHills_Biome1::GetNoise(NoiseClass, ExteameHillswarpPosition * 320, NoiseParameters->ExteameHillsOctave, 2.2, 0.3) * NoiseParameters->ExteameHillsHeight, NoiseParameters->ExteameHillsHeightPower) * 0.8 + 0.2);

	 double ExtreamHillsVartiation = NoiseMathUtils::Clamp01(pow(NoiseClass.Fractal( ExteameHillswarpPosition * 2, 1, 1.5, 0.8) * 1, 3) * 2 + .2);
	//Generating Mountain Mask and noise
	
	
	double MountainMask = NoiseMathUtils::Clamp01( pow(ActualData.HeightNormalize * 2, NoiseParameters->PlanetaryMountainInfulnceSharpness) * NoiseParameters->PlanetaryMountainInfulnceIntensity - NoiseParameters->PlanetaryMountainInfulnce);
	
	const double MountainNoise = pow(Mountain_Biome1::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, NoiseParameters->MountainOctave, NoiseParameters->MountainLacunarity, NoiseParameters->MountainPersistence) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	//RiverVienMaskGen

	double RiverEdge = (NoiseClass.FractalRidge(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 5, 2.0, 0.4) * (NoiseParameters->RiverBumpinessIntensity - (ExteameHillsNoisemesking * 0.7))) + NoiseParameters->RiverBumpinessOffset;
	double RiverVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), NoiseParameters->RiverSharpness - (ExteameHillsNoisemesking * 0.7)) * RiverEdge);
	double RiverVienMaskArea = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1  * 1.5f * NoiseParameters->RiverVienScale, 1), 10) * 2);
	double RiverBranchVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.1f * NoiseParameters->RiverBranchScale, 1), NoiseParameters->RiverBranchSharpness - (ExteameHillsNoisemesking * 0.7)) * RiverEdge);

	
	
	


	double hillmask2 = NoiseMathUtils::Clamp01(NoiseClass.Fractal(continentalWarpedPosition * 1.5f * NoiseParameters->HillsWeightScale, 4));
	double hillmask = NoiseMathUtils::Clamp01(pow(ActualData.HeightNormalize * 2, NoiseParameters->PlanetarySmallHillsInfulnceSharpness) * NoiseParameters->PlanetarySmallHillsInfulnceIntensity - NoiseParameters->PlanetarySmallHillsInfulnce ) ;

	
	//ExtreamHills GenMask
		//ExtreamHills Gen
	const double ExteameHillsNoise = pow(ExteameHills_Biome1::GetNoise(NoiseClass, ExteameHillswarpPosition * NoiseParameters->ExteameHillsScale, NoiseParameters->ExteameHillsOctave, NoiseParameters->ExteameHillsLacunarity, NoiseParameters->ExteameHillsPersistence) * NoiseParameters->ExteameHillsHeight , NoiseParameters->ExteameHillsHeightPower);

	

	double ExteameHillsMask = NoiseMathUtils::Clamp01(pow(ActualData.HeightNormalize * 2 , NoiseParameters->PlanetaryHillsInfulnceSharpness) * NoiseParameters->PlanetaryHillsInfulnceIntensity - NoiseParameters->PlanetaryHillsInfulnce );
	
	//Generating Hill, which serve as a default Noise
	const double HillNoise = Hill_Biome1::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->HillScale, NoiseParameters->HillOctave, NoiseParameters->HillLacunarity, NoiseParameters->HillPersistence) * NoiseParameters->HillHeight * hillmask2;



	double Icecapemask = NoiseMathUtils::Clamp01(pow(21.2 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1, 120) );

	double OceanFloorMask;
	double ExteameHillsMasks;

	double BeachMaskValue = NoiseParameters->BeachMask;
	double BeachMaskOffset = NoiseParameters->BeachMaskOffset;
	double BeachMaskWeight = NoiseParameters->BeachMaskWeight;



	//Making the Noise of the ocean floor
	double OceanFloorNoise = NoiseParameters->OceanFloorBaseHeight +
		Water_Biome1::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->OceanFloorScale, NoiseParameters->OceanFloorOctave, NoiseParameters->OceanFloorLacunarity, NoiseParameters->OceanFloorPersistence) * NoiseParameters->OceanFloorHeight;
	;


	//Generating Ocean Floor Mask if there is no Planetary heightmap, otherwise, simply make it based on the heightmap;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		//Needed to make sure the default value work both if you use an height map or no
		BeachMaskValue /= 5;
		BeachMaskOffset /= 15;
		BeachMaskWeight /= 2;

		

		OceanFloorMask = NoiseMathUtils::Clamp01(pow(ActualData.HeightNormalize * 2, NoiseParameters->OceanFloorWeightPower) * NoiseParameters->OceanFloorWeight - NoiseParameters->OceanFloorWeightScale);
		ExteameHillsMasks = (1 - NoiseMathUtils::Clamp01(pow(LandMassNoise * (1 / NoiseParameters->ExteameHillsWeight) * 50, NoiseParameters->ExteameHillsWeightPower * 2 / 800)));
		OceanFloorNoise *= 0.3;
	}
	else
	{
		OceanFloorMask = NoiseMathUtils::Clamp01(pow(Water_Biome1::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower) - 0.05f);
		ExteameHillsMasks = NoiseMathUtils::Clamp01(pow(ExteameHills_Biome1::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->ExteameHillsWeightScale * 0.001) * NoiseParameters->ExteameHillsWeight, 2) - 0.05f);

	}

	//Making Mask and noise for beach
	const double BeachMask = NoiseMathUtils::Clamp01((pow(ActualData.HeightNormalize * 2, NoiseParameters->BeachMask) * NoiseParameters->BeachMaskWeight - NoiseParameters->BeachMaskOffset) * ExteameHillsNoisemesking);
	const double BeachNoise = pow(NoiseClass.FractalRidge(WarpedPosition * 1.5f * NoiseParameters->BeachScale, 6), 2) * NoiseParameters->BeachHeight - NoiseParameters->BeachHeight2;

	
	const double PlanetaryTest = ActualData.HeightNormalize;
	const double SurfaceTemp = NoiseMathUtils::Clamp01(pow(ActualData.Temperature * 1.7, 10));
	const double SurfaceTemp3 = NoiseMathUtils::Clamp01(pow(ActualData.Temperature * 3.3, 10));
	const double SurfaceTemp2 = NoiseMathUtils::Clamp01(pow(ActualData.Humidity * 30, 5));
	const double SurfaceAvereager = ActualData.HeightNormalize;

	//Lerping together the differents noise made based on their mask (Mountain on Hill, Ocean on previous result, and beach on top)
	Data.HeightNormalize = NoiseMathUtils::Lerp(HillNoise * hillmask, ExteameHillsNoise * ExtreamHillsVartiation - (NoiseParameters->ExteameHillsHeightOffset * ExtreamHillsVartiation), NoiseMathUtils::Clamp01((ExteameHillsMask * (SurfaceTemp + 0.1) ) * 6 + ExteameHillsNoisemesking2 / 6));
	Data.HeightNormalize = NoiseMathUtils::Lerp(OceanFloorNoise, Data.HeightNormalize, SurfaceTemp2);
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, MountainNoise - NoiseParameters->MountainHeightOffset, MountainMask * (SurfaceTemp3 + 0.03));
	double Mountains = NoiseMathUtils::Lerp(0, MountainNoise, MountainMask);
	
	Data.HeightNormalize = Data.HeightNormalize - NoiseParameters->OverallLandHeight;
	Data.HeightNormalize = Data.HeightNormalize + (pow(PlanetaryTest, NoiseParameters->PlanetaryHeightPower) * NoiseParameters->PlanetaryHeight - (pow(SurfaceAvereager, 6) * NoiseParameters->AvreagerInstensity));
	Data.HeightNormalize = NoiseMathUtils::Lerp(NoiseParameters->BeachHeight + HillNoise * hillmask , Data.HeightNormalize, BeachMask);
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, PlanetaryTest / 1.06  + (HillNoise * 0.32), NoiseMathUtils::Lerp(NoiseMathUtils::Min(RiverVienMask + (RiverBranchVienMask * RiverVienMaskArea), 1.4), 0, NoiseMathUtils::Clamp01(MountainMask * 5)));

	Data.HeightNormalize = NoiseMathUtils::Lerp(PlanetaryTest - 0.005 , Data.HeightNormalize, OceanFloorMask);
	 Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize - (MountainNoise * hillmask2 * 0.2) , Data.HeightNormalize, NoiseMathUtils::Clamp01(pow(OceanFloorMask * 4, 2)));

	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, (Mountains * 0) + PlanetaryTest, NoiseMathUtils::Lerp(NoiseMathUtils::Clamp01(pow(PlanetaryTest, 3) * 3), 0, Icecapemask));
	//Reducing the temperature based on the altitude;
	
	
	Data.Temperature = pow(ActualData.Temperature, 0.6);
	Data.Temperature *= 0.8f + 0.2f * NoiseClass.Fractal(WarpedPosition * 0.001443, 6);
	Data.Temperature = NoiseMathUtils::Clamp(Data.Temperature - ((pow((Data.HeightNormalize * 1.12), 4) - 0.2)), 0.f, 1);

	Data.HeightNormalize = (Data.HeightNormalize * 2200000.0) / NoiseIntensity;




	//Random noise for foliage mask
	Data.FoliageMask = NoiseMathUtils::Lerp(GeneralNoise * 0.5 + 0.5, NoiseParameters->FoilageValue, NoiseParameters->FoilagetypeMask);


	return Data;
}

FNoiseData UEarthNoiseFuntion::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}


FNoiseData UEarthNoiseFuntion::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();
	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}

UEarthNoiseFuntion::UEarthNoiseFuntion() {
	NoiseParameters = CreateDefaultSubobject<UEarthNoise>(TEXT("Noise Parameters"));
}