// Fill out your copyright notice in the Description page of Project Settings.


#include "EarthNoise2.h"


UEarthNoise2::UEarthNoise2() {
}

double Water_BiomeER::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.FractalRidge(position, octave, lacunarity, persistence);
}
double Water_BiomeER::GetMask(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}
double IceCapER::GetMask(CustomNoise NoiseClass, DVector position, int octave) {
	return NoiseClass.Fractal(position, octave);
}
double IceCapER::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}
double Mountain_BiomeER::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double Mountain_BiomeER::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 6);
}

double Hill_BiomeER::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}

double ExteameHills_BiomeER::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, 5) * 0.15 + 0.85 * NoiseClass.FractalRidge(position * 0.15, octave, lacunarity, persistence);
}
double ExteameHills_BiomeER::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 5);
}




FNoiseData UEarthNoise2Example::GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
{






	double warp = NoiseClass.Noise_01(NoisePosition * NoiseParameters->Warp2Scale);
	DVector WarpedPosition = NoisePosition * NoiseParameters->WarpScale + FVector(warp, warp, warp) * NoiseParameters->WarpIntensity;

	double continentalwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->continentalwarp2Scale, NoiseParameters->ContinentOctave, NoiseParameters->continentLacunarity, NoiseParameters->continentPersistence);
	DVector continentalWarpedPosition = NoisePosition * NoiseParameters->continentalwarpScale + FVector(continentalwarp, continentalwarp, continentalwarp) * NoiseParameters->continentalwarpIntensity;


	double Mountwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->MountWarp2Scale, 4, 2.4);
	DVector MountainWarpedPosition = NoisePosition * NoiseParameters->MountWarpScale + FVector(Mountwarp, Mountwarp, Mountwarp) * NoiseParameters->MountainWarpIntensity;

	double ExteameHillswarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->ExteameHillsWarp2Scale, 4, 3);
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
	double LatitudeVar = (pow(1 - (abs(asin(lattitude) / 3.1415926535897932384626433832795)) * 2, 1) / 1.3);

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

		Data.Humidity = 0.01 + NoiseClass.Noise(continentalWarpedPosition * 1 * NoiseParameters->HumidityScale) * 0.02;
		const double HumidBase = 0.5 + NoiseClass.Noise(continentalWarpedPosition * .5 * NoiseParameters->HumidityScale);
		const double HumidLevel1 = 0.1 + (((HumidBase) * 2)) / 2.5;
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
			//double drytoHumid = NoiseMathUtils::Clamp01(pow((ActualData.Humidity - NoiseParameters->PlanetaryhumidMask), 4) / 0.25);
			//const double HumidMap1 = NoiseMathUtils::Clamp01(ActualData.Humidity - (NoiseClass.Fractal(NoiseMathUtils::Lerp2(WarpedPosition * .002, continentalWarpedPosition, .5) * 0.002443, 5, 1.8, .75) * 0.25));
			//const double HumidMap2 = NoiseMathUtils::Clamp01(pow(ActualData.Humidity, 2) * 0.7 + (pow((NoiseClass.Fractal(NoiseMathUtils::Lerp2(WarpedPosition * .0005, continentalWarpedPosition, 8) * 0.001743, 5, 1.9, .8)), 2.9) * 2));
			//Data.Humidity = NoiseMathUtils::Lerp(HumidMap2 + humid / 2, HumidMap1, drytoHumid);

			const double HumidMap1 = ((pow(ActualData.Humidity * NoiseParameters->PlanetarHumidityIntensity, NoiseParameters->PlanetarHumidityPower) * 1.1) + (NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->HumidityScale, 5, 2, .65) * (0.33 + ActualData.Humidity) - 0.3));
			Data.Humidity = HumidMap1;
		}


	}


	double MountainNoise = pow(Mountain_BiomeER::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, 3, NoiseParameters->MountainLacunarity, NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * 3) * NoiseParameters->MicroHillsMask, 2) + 0.4) * NoiseParameters->MountainPersistence) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{

		MountainNoise = pow(Mountain_BiomeER::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, 3, NoiseParameters->MountainLacunarity, NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * 3) * NoiseParameters->MicroHillsMask, 2) + 0.4) * NoiseParameters->MountainPersistence) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	}

	const double MountainDetailNoise = pow(Mountain_BiomeER::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->MountainDetailScale, NoiseParameters->MountainOctave, NoiseParameters->MountainDetailLacunarity, NoiseParameters->MountainDetailPersistence) * NoiseParameters->MountainDetailHeight, NoiseParameters->MountainHeighDetailPower) * MountainNoise / 2;



	//Generating Mountain Mask and noise
	double MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeER::GetMask(NoiseClass, ((continentalWarpedPosition * 1.6) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeER::GetMask(NoiseClass, (((continentalWarpedPosition * 1.6) + (pow(ActualData.Temperature, 0.5) * 100)) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));
		}
		MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeER::GetMask(NoiseClass, (((continentalWarpedPosition * 1.6) + (pow(ActualData.HeightNormalize, 0.5) * 100)) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));
	}

	double const Mask1 = NoiseMathUtils::Clamp01((ActualData.HeightNormalize - 0.68) / 0.15);

	double OceanTempMask = Water_BiomeER::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, NoiseParameters->OceanLacunarity, NoiseParameters->OceanPersistence) * NoiseParameters->OceanFloorWeight;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{

		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			MountainMask = NoiseMathUtils::Clamp01(MountainMask + (pow(ActualData.Temperature * NoiseParameters->PlanetaryMountainInfluence, NoiseParameters->PlanetaryMountainMaskPower) / 0.25 - NoiseParameters->PlanetaryMountainOffset));
		}
		MountainMask = NoiseMathUtils::Clamp01(MountainMask + (pow(ActualData.HeightNormalize * NoiseParameters->PlanetaryMountainInfluence, NoiseParameters->PlanetaryMountainMaskPower) / 0.25 - NoiseParameters->PlanetaryMountainOffset));
		OceanTempMask = pow(Water_BiomeER::GetMask(NoiseClass, (continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.0035) + (pow(ActualData.HeightNormalize, 0.5) / 62) * 19, NoiseParameters->OceanFloorWeightOctave, NoiseParameters->OceanLacunarity, NoiseParameters->OceanPersistence), 0.1) * NoiseParameters->OceanFloorWeight / 1.72;

	}
	





	//Flatlang Gen

	double ExteameHillsMask = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeER::GetMask(NoiseClass, WarpedPosition * NoiseParameters->ExteameHillsWeightScale + NoiseParameters->MountainShift) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		double ExteameHillsMask2 = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeER::GetMask(NoiseClass, (WarpedPosition * NoiseParameters->ExteameHillsWeightScale) + (pow(ActualData.HeightNormalize, 0.5) / 6) + NoiseParameters->MountainShift) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));


		ExteameHillsMask = NoiseMathUtils::Clamp01(ExteameHillsMask2 * (ActualData.HeightNormalize - NoiseParameters->PlanetaryHillsInfluence) / 0.25);
		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			ExteameHillsMask = NoiseMathUtils::Clamp01(ExteameHillsMask2 * (pow(ActualData.Temperature * NoiseParameters->PlanetaryHillsInfluence, NoiseParameters->PlanetaryHillsPower) - NoiseParameters->PlanetaryMountainOffset));
		}
	}
	const double ExteameHillsNoise = pow(ExteameHills_BiomeER::GetNoise(NoiseClass, ExteameHillswarpPosition * NoiseParameters->ExteameHillsScale, NoiseParameters->ExteameHillsOctave, NoiseParameters->ExteameHillsLacunarity, NoiseParameters->ExteameHillsPersistence) * NoiseParameters->ExteameHillsHeight, NoiseParameters->ExteameHillsHeightPower) - 0.2;




	// Section for Icecap masking
	double IceNoiseMask = (NoiseClass.Fractal(continentalWarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale, 8, NoiseParameters->IceCapNoiseLacunarity, NoiseParameters->IceCapNoisePersistence) * NoiseParameters->IceCapNoiseTansition) + (NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale / 4, 2) * NoiseParameters->IceCapNoiseTansition2);
	double Icecapemask = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(NoiseParameters->IceCapTansitionShaprness, IceNoiseMask + (NoiseParameters->IceCapTansition + pow(NoiseParameters->TemperatureOffset, 3)) - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset + (lattitude * NoiseParameters->Aysemtrical));
	double Icecapemask2 = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(1, abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset);

	
    double OceanFloorMask;
	double ExteameHillsMasks;

	double BeachMaskValue = NoiseParameters->BeachMask;
	double BeachMaskOffset = NoiseParameters->BeachMaskOffset;
	double BeachMaskWeight = NoiseParameters->BeachMaskWeight;



	//Making the Noise of the ocean floor
	double OceanFloorNoise =
		pow(Water_BiomeER::GetNoise(NoiseClass, warp3 * NoiseParameters->OceanFloorScale, NoiseParameters->OceanFloorOctave, NoiseParameters->OceanFloorLacunarity, NoiseParameters->OceanFloorPersistence), 4) * NoiseParameters->OceanFloorHeight;
	;


	//Call for dune noise
	const double MicroDunes = pow(NoiseClass.FractalRidge(WarpedPosition * 0.2, 3, 5.5, 0.6), 3) * 0.009;




	//Making Mask and noise for beach
	
	const double BeachNoise = pow(NoiseClass.FractalRidge(WarpedPosition * 1.5f * NoiseParameters->BeachScale, 6), 2) * NoiseParameters->BeachHeight - NoiseParameters->BeachHeight2;


	const double PlanetaryTest = ActualData.HeightNormalize - (pow(ActualData.HeightNormalize, 11) * 0.08);
	const double PlanetaryTest2 = ActualData.HeightNormalize - (pow(ActualData.HeightNormalize, 11) * 0.08) * NoiseParameters->PlanetaryHeight;
	const double PlanetaryDeformation = pow(ActualData.HeightNormalize, NoiseParameters->DeformationAreaContrast) - NoiseParameters->DefomationCutoff;

	const double MountMask2 = MountainMask * NoiseMathUtils::Clamp01(pow(((MountainNoise / 1.1 + MountainDetailNoise) + 2) / 3, 2) - 0.1);
	double Mountains = NoiseMathUtils::Lerp(0, MountainNoise / 1.1 + MountainDetailNoise, MountainMask * NoiseMathUtils::Clamp01(pow((MountainNoise / 1.1 + MountainDetailNoise), 2) - 0.1));


	const double Bumbiness = (pow(OceanTempMask / 4 + 0.4, 24) * 10) * NoiseMathUtils::Clamp01((PlanetaryTest - (NoiseParameters->OverallLandHeight + 0.035)) * 3);
	

	//Generating Ocean Floor Mask if there is no Planetary heightmap, otherwise, simply make it based on the heightmap;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		//Needed to make sure the default value work both if you use an height map or no
		BeachMaskValue /= 5;
		BeachMaskOffset /= 15;
		BeachMaskWeight /= 2;



		OceanFloorMask = (NoiseMathUtils::Clamp01(pow((1 - PlanetaryTest2) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower)));


		OceanFloorNoise *= 0.3;
	}
	else
	{
		OceanFloorMask = NoiseMathUtils::Clamp01(pow(Water_BiomeER::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, 2.0, 0.5) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower) - 0.05f);
		ExteameHillsMasks = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeER::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->ExteameHillsWeightScale * 0.001) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower) - 0.05f);

	}

	

		// *** Section to wrap up the surface generation ***
		const double OceanFloorMask2 = (NoiseMathUtils::Clamp01((pow((1 - PlanetaryTest2) * NoiseParameters->OceanFloorWeight, 30) * -1 + 1)));

		// postprocessing of planetary heightmap
		Data.HeightNormalize = (NoiseMathUtils::Clamp(PlanetaryTest2, 0, NoiseParameters->HeightCutOff) - NoiseParameters->OverallLandHeight) / NoiseParameters->PlanetaryPostAffect + (Bumbiness * 2);

		// Masking for of extreame hills to make less extreme peristance on overlapping noise
		const double Hills = ((ExteameHillsNoise * -1 + 1) * ExteameHillsMask);
		const double HillNoiseMask = NoiseMathUtils::Clamp01(pow(NoiseMathUtils::Clamp01(ActualData.HeightNormalize - NoiseParameters->hillsCutoff) * NoiseParameters->hillsIntesity, NoiseParameters->hillsAreaContrast) * NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * .4) * NoiseParameters->MicroHillsMask, 2)));


		// masking for coastal areas
		double BeachlineTest = NoiseMathUtils::Clamp01(pow(((Data.HeightNormalize * -1) - NoiseParameters->CoastlineShift2) * 1730, 3) * 0.04) + NoiseMathUtils::Clamp01(pow(((Data.HeightNormalize - (Hills * 0.02)) - NoiseParameters->CoastlineShift) * 1730, 3) * 0.04);

		//Generating Hill of hills
		const double HillNoise = Hill_BiomeER::GetNoise(NoiseClass, WarpedPosition * 0.080, 8, 2.1, NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * .3) * NoiseParameters->MicroHillsMask, 2) + 0.1) * 1.2 * ((ExteameHillsNoise * ExteameHillsMask) * -1 + 1)) * -0.01 * NoiseMathUtils::Clamp01(BeachlineTest + 0.43);
		const double HillNoise2 = Hill_BiomeER::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->HillScale, NoiseParameters->HillOctave, NoiseParameters->HillLacunarity, NoiseParameters->HillPersistence) * NoiseParameters->HillHeight * NoiseMathUtils::Clamp01(BeachlineTest + 0.43);


		Data.HeightNormalize += NoiseMathUtils::Lerp(HillNoise + NoiseParameters->HeightOffset + (pow(MountMask2, 8) * 3.2) + (HillNoise2 * HillNoiseMask), (ExteameHillsNoise * (MountainDetailNoise + 0.5) / 1.5) - NoiseParameters->ExteameHillsHeightOffset - (HillNoise * 10), ExteameHillsMask * NoiseMathUtils::Clamp01((pow(ExteameHillsNoise, 1.5))) * OceanFloorMask2);
		Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, NoiseMathUtils::Lerp(MountainNoise / 1.1 + MountainDetailNoise, MountainNoise * MountainDetailNoise, NoiseParameters->MountainDetailMethodAlpha) - (NoiseParameters->MountainHeightOffset), MountainMask * NoiseMathUtils::Clamp01(pow((MountainNoise / 1.1 + MountainDetailNoise), 1.75) - 0.1));

		const double heightvar = (pow(Data.HeightNormalize * -1 + 1, 70) * 7);
		Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, NoiseParameters->OceanFloorBaseHeight - (PlanetaryTest2 * 0.3), OceanFloorMask);
		Data.HeightNormalize += NoiseMathUtils::Clamp(PlanetaryDeformation * NoiseParameters->DefomationIntesity, 0, 200);

		const double heightvar2 = (pow(Data.HeightNormalize * -1 + 0.95, 40) * 1.5);
		double RiverVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), NoiseParameters->RiverSharpness * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity * NoiseMathUtils::Clamp(pow(heightvar, 7), .02, 1)) + NoiseParameters->RiverBumpinessOffset * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)));
		double RiverVienMaskArea = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), 10) * 2);
		double RiverBranchVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.1f * NoiseParameters->RiverBranchScale, 1), NoiseParameters->RiverBranchSharpness * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity * NoiseMathUtils::Clamp(pow(heightvar, 7), .02, 1)) + NoiseParameters->RiverBumpinessOffset));

		Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, Data.HeightNormalize - (pow(NoiseMathUtils::Clamp(PlanetaryTest2, 0, 0.75) * 1.1, 30) * 1) - Bumbiness + NoiseParameters->RiverHeightScale, NoiseMathUtils::Lerp(RiverVienMask + (RiverBranchVienMask * RiverVienMaskArea), 0, NoiseMathUtils::Clamp01(MountainMask * 5)) * (OceanFloorMask * -1 + 1));
		Data.HeightNormalize = NoiseMathUtils::Lerp((Mountains * 0.3) + NoiseParameters->IceCapHeight, Data.HeightNormalize, Icecapemask);

		// *** Postprocessing for temperature and humidity for both procedural and baked ***
		Data.Temperature = LatitudeVar - (Icecapemask2 * 0.5);
		Data.Temperature = Data.Temperature - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 2, 1.45);
		Data.Temperature *= NoiseClass.Fractal((WarpedPosition + (pow(ActualData.HeightNormalize, 0.5) * 3000)) * 0.002443, 6) + .4;
		Data.Temperature *= pow(OceanTempMask, 1.2) * 1.4;
		Data.Temperature += ((ActualData.Humidity * -1) + 1) / 4;
		Data.Humidity *= NoiseMathUtils::Clamp01((-Data.HeightNormalize + 0.4) - (MountainMask * 0.175) + NoiseMathUtils::Clamp(((pow(RiverVienMaskArea, 2) * 4) - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 10, 1.2)) * (heightvar2 * 2) - (Data.Humidity) * 3, 0, 5)) * Icecapemask;
		Data.HeightNormalize = NoiseMathUtils::Lerp(NoiseMathUtils::Lerp(Data.HeightNormalize + ((OceanFloorNoise * NoiseMathUtils::Clamp01(Icecapemask + 0.15)) + MicroDunes * NoiseMathUtils::Clamp01(Icecapemask + 0.24)), Data.HeightNormalize, NoiseMathUtils::Clamp01(MountainMask * 6)), Data.HeightNormalize, pow(NoiseMathUtils::Clamp01(Data.Humidity + 0.993), 500) + MountainMask);
		Data.Temperature = NoiseMathUtils::Lerp(LatitudeVar * (OceanTempMask + 0.1), Data.Temperature, NoiseMathUtils::Clamp01((Data.HeightNormalize + 0.01) * 2000));
		


	//Random noise for foliage mask
	Data.FoliageMask = GeneralNoise * 0.5 + 0.5;


	return Data;
}

FNoiseData UEarthNoise2Example::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}


FNoiseData UEarthNoise2Example::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();

	Data.Height = Data.HeightNormalize * NoiseIntensity;
	return Data;

}

UEarthNoise2Example::UEarthNoise2Example() {
	NoiseParameters = CreateDefaultSubobject<UEarthNoise2>(TEXT("Noise Parameters"));
}