// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetTerraNoise.h"






UPlanetTerraNoise::UPlanetTerraNoise() {
}

double Water_Biome10::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.FractalRidge(position, octave, lacunarity, persistence);
}
double Water_Biome10::GetMask(CustomNoise NoiseClass, DVector position, int octave) {
	return NoiseClass.Fractal(position, octave);
}
double IceCap10::GetMask(CustomNoise NoiseClass, DVector position, int octave) {
	return NoiseClass.Fractal(position, octave);
}
double IceCap10::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}
double Mountain_Biome10::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double Mountain_Biome10::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 6);
}

double Hill_Biome10::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}

double ExteameHills_Biome10::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double ExteameHills_Biome10::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 5);
}




FNoiseData UTerraNoiseExample::GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
{




	
	
	double warp = NoiseClass.Noise_01(NoisePosition * NoiseParameters->Warp2Scale);
	DVector WarpedPosition = NoisePosition * NoiseParameters->WarpScale + FVector(warp, warp, warp) * NoiseParameters->WarpIntensity;

	double continentalwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->continentalwarp2Scale, NoiseParameters->ContinentOctave, NoiseParameters->continentLacunarity, NoiseParameters->continentPersistence);
	DVector continentalWarpedPosition = NoisePosition * NoiseParameters->continentalwarpScale + FVector(continentalwarp, continentalwarp, continentalwarp) * NoiseParameters->continentalwarpIntensity;


	double Mountwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->MountWarp2Scale, 4, 2.4);
	DVector MountainWarpedPosition = NoisePosition * NoiseParameters->MountWarpScale + FVector(Mountwarp, Mountwarp, Mountwarp) * NoiseParameters->MountainWarpIntensity;

	double ExteameHillswarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->ExteameHillsWarp2Scale, 4 , 3);
	DVector ExteameHillswarpPosition = NoisePosition * NoiseParameters->ExteameHillsWarpScale + FVector(ExteameHillswarp, ExteameHillswarp, ExteameHillswarp) * NoiseParameters->ExteameHillsWarpInsesnity;

	double warp2 = NoiseClass.Noise_01(NoisePosition * NoiseParameters->Warp2Scale2);
	DVector WarpedPosition2 = NoisePosition * NoiseParameters->WarpScale2 + FVector(warp2, warp2, warp2) * NoiseParameters->WarpIntensity2;

	double warp3 = NoiseClass.Fractal(NoisePosition * NoiseParameters->Warp3Scape, 6) + (continentalwarp * 40);
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

	const double NoiseDefualt = NoiseClass.Fractal(WarpedPosition * 0.015537, 6);
	if (FlatWorld) {
		Data.Temperature = NoiseClass.Fractal(WarpedPosition * 0.028143, 6);
		Data.Humidity = NoiseDefualt;
	}
	else {
		float LattitudeRemapped = 1 - abs(lattitude);


		//Humidity and temperature based on planet latitude, you can use these value for terrain generation (using them as mask for biome) 
		//and for texture (they are saved as Green (temperature) and Blue (Humidity) in the vertex Color of the terrain Mesh;
		//Data.Temperature = (TempNoise * NoiseParameters->TemperatureNoiseAffect) + pow(1 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 2, 2) + NoiseParameters->TemperatureOffset;

		//Data.Temperature = NoiseMathUtils::Lerp(Data.Temperature, (TempNoise * NoiseParameters->TemperatureNoiseAffect), NoiseParameters->TemperatureMask + (NoiseParameters->TemperatureOffset2 + pow(1 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 2, 2)));
	


		const double humid = NoiseMathUtils::Clamp01(pow(NoiseClass.Fractal(continentalWarpedPosition * 0.25 * NoiseParameters->HumidityScale, 4, 1.5, 10) * 1.4, 3) * 1.5);
		const double TempNoist = NoiseMathUtils::Clamp01(pow(NoiseClass.Fractal(continentalWarpedPosition * 0.25 * NoiseParameters->TemperatureNoiseScale1, 5, 2.3, .7) * 1.4, 3) + pow(ActualData.Temperature, 2) + 0.2);
		double polarAndEquatorial = pow(NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(1, 1 - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 2)), 2) * pow(NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(.8, abs(asin(lattitude) / 3.1415926535897932384626433832795) * 3)), 2);

		Data.Humidity =  0.01 + NoiseClass.Noise(continentalWarpedPosition * 1 * NoiseParameters->HumidityScale) * 0.02;
		const double HumidBase = 0.5 + NoiseClass.Noise(continentalWarpedPosition * .5 * NoiseParameters->HumidityScale);
		const double HumidLevel1 = 0.1 + (((HumidBase) * 2)) / 2.5 ;
		const double HumidLevel2 = (((HumidBase) * 2) - 0.5);
		const double HumidLevel3 = (((HumidBase) * 4) - 2.7);
		Data.Humidity += NoiseMathUtils::SmoothStep(NoiseMathUtils::Max(0, HumidLevel1) + (polarAndEquatorial * 0.3) * (HumidLevel1 + 1));
		Data.Humidity += NoiseMathUtils::Clamp01(HumidLevel2) + (NoiseMathUtils::Clamp01(HumidLevel3));
		Data.Humidity *= ((NoiseDefualt + 0.5) / 1.5);
		
	
		

		if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
		{

			if (ActualData.Temperature >= 0)
			{
				Data.Temperature = ActualData.Temperature + NoiseParameters->TemperatureOffset;
				Data.Temperature *= TempNoist;
			}
			double drytoHumid = NoiseMathUtils::Clamp01(pow((ActualData.Humidity - NoiseParameters->PlanetaryhumidMask), 4) / 0.25);
			const double HumidMap1 = NoiseMathUtils::Clamp01(ActualData.Humidity - (NoiseClass.Fractal(NoiseMathUtils::Lerp2(WarpedPosition * .002, continentalWarpedPosition, .5) * 0.002443, 5, 1.8, .75) * 0.25));
			const double HumidMap2 = NoiseMathUtils::Clamp01(pow(ActualData.Humidity, 2) * 0.7 + (pow((NoiseClass.Fractal(NoiseMathUtils::Lerp2(WarpedPosition * .0005, continentalWarpedPosition, 8) * 0.001743, 5, 1.9, .8)), 2.9) * 2));
			Data.Humidity = NoiseMathUtils::Lerp(HumidMap2 + humid / 2, HumidMap1, drytoHumid);
		}


	}


	

	//Generating Mountain Mask and noise
	double MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_Biome10::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));


	double const Mask1 = NoiseMathUtils::Clamp01((ActualData.HeightNormalize - 0.68) / 0.15);


	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		MountainMask = NoiseMathUtils::Clamp01(MountainMask + (ActualData.HeightNormalize - NoiseParameters->PlanetaryMountainInfluence) / 0.25);
		
	}
	
	
	const double MountainNoise = pow(Mountain_Biome10::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, 2, NoiseParameters->MountainLacunarity, NoiseParameters->MountainPersistence) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	const double MountainDetailNoise = pow(Mountain_Biome10::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->MountainDetailScale, NoiseParameters->MountainOctave, NoiseParameters->MountainLacunarity, NoiseParameters->MountainPersistence) * NoiseParameters->MountainDetailHeight, NoiseParameters->MountainHeighDetailPower) * MountainNoise / 2;

	
	//RiverVienMaskGen

	double RiverVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), NoiseParameters->RiverSharpness) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity) + NoiseParameters->RiverBumpinessOffset));
	double RiverVienMaskArea = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), 10) * 2);
	double RiverBranchVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.1f * NoiseParameters->RiverBranchScale, 1), NoiseParameters->RiverBranchSharpness) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity) + NoiseParameters->RiverBumpinessOffset));


	//Flatlang Gen

	double ExteameHillsMask = NoiseMathUtils::Clamp01(pow(ExteameHills_Biome10::GetMask(NoiseClass, WarpedPosition * NoiseParameters->ExteameHillsWeightScale + NoiseParameters->MountainShift) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		ExteameHillsMask = NoiseMathUtils::Clamp01(ExteameHillsMask + (ActualData.HeightNormalize - NoiseParameters->PlanetaryHillsInfluence) / 0.25);
	}
	const double ExteameHillsNoise = pow(ExteameHills_Biome10::GetNoise(NoiseClass, ExteameHillswarpPosition * NoiseParameters->ExteameHillsScale, NoiseParameters->ExteameHillsOctave, NoiseParameters->ExteameHillsLacunarity, NoiseParameters->ExteameHillsPersistence) * NoiseParameters->ExteameHillsHeight, NoiseParameters->ExteameHillsHeightPower);

	//Generating Hill, which serve as a default Noise
	const double HillNoise = Hill_Biome10::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->HillScale, NoiseParameters->HillOctave, NoiseParameters->HillLacunarity, NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * .3) * NoiseParameters->MicroHillsMask, 2) + 0.1) * NoiseParameters->HillPersistence) * NoiseParameters->HillHeight;



	double IceNoiseMask = (NoiseClass.Fractal(continentalWarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale, 8, NoiseParameters->IceCapNoiseLacunarity, NoiseParameters->IceCapNoisePersistence) * NoiseParameters->IceCapNoiseTansition) + (NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale / 4, 2) * NoiseParameters->IceCapNoiseTansition2);
	double Icecapemask = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(NoiseParameters->IceCapTansitionShaprness, IceNoiseMask + (NoiseParameters->IceCapTansition + pow(NoiseParameters->TemperatureOffset, 3)) - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset) ;
	double Icecapemask2 = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(1, abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset);

	double OceanFloorMask;
	double ExteameHillsMasks;

	double BeachMaskValue = NoiseParameters->BeachMask;
	double BeachMaskOffset = NoiseParameters->BeachMaskOffset;
	double BeachMaskWeight = NoiseParameters->BeachMaskWeight;

	

	//Making the Noise of the ocean floor
	double OceanFloorNoise = NoiseParameters->OceanFloorBaseHeight +
		Water_Biome10::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->OceanFloorScale, NoiseParameters->OceanFloorOctave, NoiseParameters->OceanFloorLacunarity, NoiseParameters->OceanFloorPersistence) * NoiseParameters->OceanFloorHeight;
	;


	//Generating Ocean Floor Mask if there is no Planetary heightmap, otherwise, simply make it based on the heightmap;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		//Needed to make sure the default value work both if you use an height map or no
		BeachMaskValue /= 5;
		BeachMaskOffset /= 15;
		BeachMaskWeight /= 2;

		

		OceanFloorMask = (1 - NoiseMathUtils::Clamp01(pow(LandMassNoise * (1 / NoiseParameters->OceanFloorWeight) * 50, NoiseParameters->OceanFloorWeightPower / 800)));

		OceanFloorNoise *= 0.3;
	}
	else
	{
		OceanFloorMask = NoiseMathUtils::Clamp01(pow(Water_Biome10::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower) - 0.05f);
		ExteameHillsMasks = NoiseMathUtils::Clamp01(pow(ExteameHills_Biome10::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->ExteameHillsWeightScale * 0.001) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower) - 0.05f);

	}

	//Making Mask and noise for beach
	const double BeachMask = NoiseMathUtils::Clamp01(exp(-pow(OceanFloorMask * BeachMaskValue - BeachMaskOffset, 2)) * BeachMaskWeight);
	const double BeachNoise = pow(NoiseClass.FractalRidge(WarpedPosition * 1.5f * NoiseParameters->BeachScale, 6), 2) * NoiseParameters->BeachHeight - NoiseParameters->BeachHeight2;


	const double PlanetaryTest = ActualData.HeightNormalize - (pow(ActualData.HeightNormalize, 11) * 0.08);


	double OceanTempMask = Water_Biome10::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave) * NoiseParameters->OceanFloorWeight;


	const double MountMask2 = MountainMask * NoiseMathUtils::Clamp01(pow(((MountainNoise / 1.1 + MountainDetailNoise) + 2) /3, 2) - 0.1);
	//Lerping together the differents noise made based on their mask (Mountain on Hill, Ocean on previous result, and beach on top)
	Data.HeightNormalize = NoiseMathUtils::Lerp(HillNoise + NoiseParameters->HeightOffset + (pow(MountMask2, 8) * 3.2), ExteameHillsNoise - NoiseParameters->ExteameHillsHeightOffset - (HillNoise * 10), ExteameHillsMask * NoiseMathUtils::Clamp01((pow(ExteameHillsNoise, 1.5))));
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, MountainNoise / 1.1 + MountainDetailNoise - (NoiseParameters->MountainHeightOffset), MountainMask * NoiseMathUtils::Clamp01(pow((MountainNoise / 1.1 + MountainDetailNoise), 2) - 0.1));
	double Mountains = NoiseMathUtils::Lerp(0, MountainNoise / 1.1 + MountainDetailNoise, MountainMask * NoiseMathUtils::Clamp01(pow((MountainNoise / 1.1 + MountainDetailNoise), 2) - 0.1));
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize + (Mask1 * -0.15), ((Data.HeightNormalize * -1.1) - 0.04) + OceanFloorNoise, OceanFloorMask);
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, BeachNoise, BeachMask);
	Data.HeightNormalize = Data.HeightNormalize - NoiseParameters->OverallLandHeight;
	Data.HeightNormalize = Data.HeightNormalize + (pow(PlanetaryTest, NoiseParameters->PlanetaryHeightPower) * NoiseParameters->PlanetaryHeight);
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, NoiseMathUtils::Lerp((PlanetaryTest / 2 - NoiseParameters->OverallLandHeight) + NoiseParameters->RiverHeightScale , Data.HeightNormalize + NoiseParameters->RiverHeightScale, OceanFloorMask), NoiseMathUtils::Lerp(RiverVienMask + (RiverBranchVienMask * RiverVienMaskArea), 0, NoiseMathUtils::Clamp01(MountainMask * 5)));
	Data.HeightNormalize = NoiseMathUtils::Lerp((Mountains * 0.6) + NoiseParameters->IceCapHeight, Data.HeightNormalize, Icecapemask);
	
	
	//Reducing the temperature based on the altitude;
	const double LatitudeVar = (pow(1 - (abs(asin(lattitude) / 3.1415926535897932384626433832795)) * 2 - 0.14, 1) ) + NoiseParameters->TemperatureOffset;
	Data.Temperature = LatitudeVar - (Icecapemask2 * 0.1);
	Data.Temperature = Data.Temperature - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 1, 1.2);
	Data.Temperature *= 0.8f + 0.2f * NoiseClass.Fractal(WarpedPosition * 0.001443, 6);
	Data.Temperature *= pow(OceanTempMask, 1.2) * 1.4;
	Data.Humidity = NoiseMathUtils::Lerp(Data.Humidity * pow(OceanTempMask / 1.2, 14), Data.Humidity, NoiseMathUtils::Clamp01(pow((NoiseParameters->OceanFloorWeight - 1.6) * 1.9, 3)));
	Data.Humidity *= (- Data.HeightNormalize + 0.4) - (MountainMask * 0.175) + NoiseMathUtils::Clamp((((pow(RiverVienMaskArea, 2) * 4) - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 10, 1.2)) * pow(OceanTempMask, 50)) - (Data.Humidity) * 3, 0, 5) * Icecapemask;
	
	Data.Temperature = NoiseMathUtils::Lerp(LatitudeVar * (OceanTempMask + 0.1), Data.Temperature, NoiseMathUtils::Clamp01((Data.HeightNormalize + 0.01) * 2000));





	//Random noise for foliage mask
	Data.FoliageMask = GeneralNoise * 0.5 + 0.5;


	return Data;
}

FNoiseData UTerraNoiseExample::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}


FNoiseData UTerraNoiseExample::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();
	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}

UTerraNoiseExample::UTerraNoiseExample() {
	NoiseParameters = CreateDefaultSubobject<UPlanetTerraNoise>(TEXT("Noise Parameters"));
}