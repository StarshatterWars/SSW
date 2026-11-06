// Fill out your copyright notice in the Description page of Project Settings.


#include "EarthTerraMix.h"


UEarthTerraMix::UEarthTerraMix() {
}



double Water_BiomeMix::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.FractalRidge(position, octave, lacunarity, persistence);
}
double Water_BiomeMix::GetMask(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.Fractal(position, octave, lacunarity, persistence);
}
double Mountain_BiomeMix::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence, float persistence1, float warp, float warp0, float damp, float damp0, float dampScale) {
	return pow(NoiseClass.BillowFractal(position * 2, 6, 2.1, 0.43), 1.5) * 0.65 + 0.45 * NoiseClass.JprdanTurbulance(position, octave, lacunarity, persistence, persistence1, warp, warp0, damp, damp0, dampScale);
}
double Mountain_BiomeMix::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 6);
}

double Hill_BiomeMix::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence) {
	return NoiseClass.BillowFractal(position, octave, lacunarity, persistence) * -1 + 1;
}

double ExteameHills_BiomeMix::GetNoise(CustomNoise NoiseClass, DVector position, int octave, float lacunarity, float persistence, float persistence1, float warp, float warp0, float damp, float damp0, float dampScale) {
	return NoiseClass.JprdanTurbulance(position * 0.15, octave, lacunarity, persistence, persistence1, warp, warp0, damp, damp0, dampScale);
}
double ExteameHills_BiomeMix::GetMask(CustomNoise NoiseClass, DVector position) {
	return NoiseClass.Fractal(position, 5);
}




FNoiseData UearthTerra::GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
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
			const double HumidMap1 = ((pow(ActualData.Humidity * NoiseParameters->PlanetarHumidityIntensity, NoiseParameters->PlanetarHumidityPower) * 1.1) + (NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->HumidityScale, 5, 2, .65) * (0.33 + ActualData.Humidity) - 0.3));
			Data.Humidity = HumidMap1;
		}


	}


	double MountainNoise = pow(Mountain_BiomeMix::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, 6, NoiseParameters->MountainLacunarity, NoiseParameters->MountainPersistence, NoiseParameters->MountainPersistence1, NoiseParameters->Erosion, NoiseParameters->Erosion1, NoiseParameters->Damp, NoiseParameters->Damp2, NoiseParameters->DampScale) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{

		MountainNoise = pow(Mountain_BiomeMix::GetNoise(NoiseClass, MountainWarpedPosition * NoiseParameters->MountainScale, 6, NoiseParameters->MountainLacunarity, NoiseParameters->MountainPersistence, NoiseParameters->MountainPersistence1, NoiseParameters->Erosion, NoiseParameters->Erosion1, NoiseParameters->Damp, NoiseParameters->Damp2, NoiseParameters->DampScale) * NoiseParameters->MountainHeight, NoiseParameters->MountainHeightPower);
	}

	const double MountainDetailNoise = 1;



	//Generating Mountain Mask and noise
	double MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeMix::GetMask(NoiseClass, ((continentalWarpedPosition * 1.6) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeMix::GetMask(NoiseClass, (((continentalWarpedPosition * 1.6) + (pow(ActualData.HeightNormalize, 0.5) * 100)) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));

		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			MountainMask = NoiseMathUtils::Clamp01(pow(Mountain_BiomeMix::GetMask(NoiseClass, (((continentalWarpedPosition * 1.6) + (pow(ActualData.Temperature, 0.5) * 100)) - (NoiseParameters->continentalwarpIntensity * 0.6)) * NoiseParameters->MountainWeightScale + NoiseParameters->MountainShift) * NoiseParameters->MountainWeight, NoiseParameters->MountainWeightPower));
		}
		MountainMask = NoiseMathUtils::Clamp01(MountainMask + (pow(ActualData.HeightNormalize * NoiseParameters->PlanetaryMountainInfluence, NoiseParameters->PlanetaryMountainMaskPower) / 0.25 - NoiseParameters->PlanetaryMountainOffset));
		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			MountainMask = NoiseMathUtils::Clamp01(MountainMask + (pow(ActualData.Temperature * NoiseParameters->PlanetaryMountainInfluence, NoiseParameters->PlanetaryMountainMaskPower) / 0.25 - NoiseParameters->PlanetaryMountainOffset));
		}
	}

	double const Mask1 = NoiseMathUtils::Clamp01((ActualData.HeightNormalize - 0.68) / 0.15);

	double OceanTempMask = Water_BiomeMix::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, NoiseParameters->OceanLacunarity, NoiseParameters->OceanPersistence) * NoiseParameters->OceanFloorWeight;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{



		OceanTempMask = pow(Water_BiomeMix::GetMask(NoiseClass, (continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.0035) + (pow(ActualData.HeightNormalize, 0.5) / 62) * 19, NoiseParameters->OceanFloorWeightOctave, NoiseParameters->OceanLacunarity, NoiseParameters->OceanPersistence), 0.1) * NoiseParameters->OceanFloorWeight / 1.72;

	}
	if (NoiseParameters->IsTidalLocked)
	{
		OceanTempMask = Water_BiomeMix::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, NoiseParameters->OceanLacunarity, NoiseParameters->OceanPersistence) * NoiseParameters->OceanFloorWeight - (LatitudeVar / 5);
	}





	//Flatlang Gen
	const double ExteameHillsNoise = pow(ExteameHills_BiomeMix::GetNoise(NoiseClass, ExteameHillswarpPosition * NoiseParameters->ExteameHillsScale, 3, NoiseParameters->ExteameHillsLacunarity, NoiseParameters->ExteameHillsPersistence, NoiseParameters->ExteameHillsPersistence1, NoiseParameters->ExteameHillsErosion, NoiseParameters->ExteameHillsErosion1, NoiseParameters->ExteameHillsDamp, NoiseParameters->ExteameHillsDamp2, NoiseParameters->ExteameHillsDampScale) * NoiseParameters->ExteameHillsHeight, NoiseParameters->ExteameHillsHeightPower);

	double ExteameHillsMask = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeMix::GetMask(NoiseClass, WarpedPosition * NoiseParameters->ExteameHillsWeightScale + NoiseParameters->MountainShift) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		double ExteameHillsMask2 = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeMix::GetMask(NoiseClass, (WarpedPosition * NoiseParameters->ExteameHillsWeightScale) + (pow(ActualData.HeightNormalize, 0.5) / 6) + NoiseParameters->MountainShift) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));


		ExteameHillsMask = NoiseMathUtils::Clamp01(ExteameHillsMask2 * (ActualData.HeightNormalize - NoiseParameters->PlanetaryHillsInfluence) / 0.25);
		if (NoiseParameters->bTextureTempratureChannelAsTypography)
		{
			ExteameHillsMask = NoiseMathUtils::SmoothStep(NoiseMathUtils::Clamp01(ExteameHillsMask2 * ((pow(ActualData.Temperature * NoiseParameters->PlanetaryHillsInfluence, NoiseParameters->PlanetaryHillsPower) - NoiseParameters->PlanetaryHillsOffset) * (ExteameHillsNoise + 1 / 2))));
		}
	}
	double ExteameHillsMask3 = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeMix::GetMask(NoiseClass, (WarpedPosition * NoiseParameters->ExteameHillsWeightScale)) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower));






	double IceNoiseMask = (NoiseClass.Fractal(continentalWarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale, 8, NoiseParameters->IceCapNoiseLacunarity, NoiseParameters->IceCapNoisePersistence) * NoiseParameters->IceCapNoiseTansition) + (NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->IceCapTansitionNoiseScale / 4, 2) * NoiseParameters->IceCapNoiseTansition2);
	double Icecapemask = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(NoiseParameters->IceCapTansitionShaprness, IceNoiseMask + (NoiseParameters->IceCapTansition + pow(NoiseParameters->TemperatureOffset, 3)) - abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset + (lattitude * NoiseParameters->Aysemtrical));
	double Icecapemask2 = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(1, abs(asin(lattitude) / 3.1415926535897932384626433832795) * 1.8) + NoiseParameters->TemperatureOffset);

	if (NoiseParameters->IsTidalLocked)
	{
		Icecapemask = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(NoiseParameters->IceCapTansitionShaprness, IceNoiseMask + (NoiseParameters->IceCapTansition + pow(NoiseParameters->TemperatureOffset, 3)) - lattitude / 3.1415926535897932384626433832795 * 1.8) + NoiseParameters->TemperatureOffset);
		Icecapemask2 = NoiseMathUtils::Clamp01(NoiseMathUtils::Contrast(1, lattitude / 3.1415926535897932384626433832795) * 1.8 + NoiseParameters->TemperatureOffset);
	}

	double OceanFloorMask;
	double ExteameHillsMasks;




	//Making the Noise of the ocean floor
	const double MicroDunes = pow(NoiseClass.FractalRidge(WarpedPosition * 1.8, 3, 2, 0.9), 3) * 0.0008;


	const double PlanetaryTest = ActualData.HeightNormalize - (pow(ActualData.HeightNormalize, 11) * 0.08);
	const double PlanetaryTest2 = ActualData.HeightNormalize - (pow(ActualData.HeightNormalize, 11) * 0.08) * NoiseParameters->PlanetaryHeight;
	const double PlanetaryDeformation = pow(ActualData.HeightNormalize, NoiseParameters->DeformationAreaContrast) - NoiseParameters->DefomationCutoff;

	const double MountMask2 = MountainMask * NoiseMathUtils::Clamp01(pow(((MountainNoise / 1.1) + 2) / 3, 2) - 0.1);
	double Mountains = NoiseMathUtils::Lerp(0, MountainNoise / 1.1, MountainMask * NoiseMathUtils::Clamp01(pow((MountainNoise / 1.1), 2) - 0.1));


	const double Bumbiness = (pow(OceanTempMask / 4 + 0.4, 24) * 10) * NoiseMathUtils::Clamp01((PlanetaryTest - (NoiseParameters->OverallLandHeight + 0.035)) * 3);
	if (NoiseParameters->IsTidalLocked)
	{
		LatitudeVar = pow((lattitude * -1) + 0.8 / 5, 3) / 2;
	}

	//Generating Ocean Floor Mask if there is no Planetary heightmap, otherwise, simply make it based on the heightmap;
	if (UsePlanetary && !NoiseParameters->bIgnorePlanetaryHeightmap)
	{
		OceanFloorMask = (NoiseMathUtils::Clamp01(pow((1 - PlanetaryTest2) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower)));
	}
	else
	{
		OceanFloorMask = NoiseMathUtils::Clamp01(pow(Water_BiomeMix::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, 2.0, 0.5) * NoiseParameters->OceanFloorWeight, NoiseParameters->OceanFloorWeightPower) - 0.05f);
		ExteameHillsMasks = NoiseMathUtils::Clamp01(pow(ExteameHills_BiomeMix::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->ExteameHillsWeightScale * 0.001) * NoiseParameters->ExteameHillsWeight, NoiseParameters->ExteameHillsWeightPower) - 0.05f);

	}
	if (NoiseParameters->IsTidalLocked)
	{
		OceanFloorMask = NoiseMathUtils::Clamp01(pow(Water_BiomeMix::GetMask(NoiseClass, continentalWarpedPosition * NoiseParameters->OceanFloorWeightScale * 0.001, NoiseParameters->OceanFloorWeightOctave, 2.0, 0.5) * NoiseParameters->OceanFloorWeight - (LatitudeVar / 5), NoiseParameters->OceanFloorWeightPower) - 0.05f);;
	}


	/// - Heavy const used to keep cache down, and read only for these varibles do not need double checking.
	if (UsePlanetary)
	{
		/// - For flexibility with terrain noise, this preforms post-processing on the elevation map data if necessary
		const double PlanetDeformationPost = NoiseMathUtils::Clamp(PlanetaryDeformation * NoiseParameters->DefomationIntesity, 0, 500);
		Data.HeightNormalize = (NoiseMathUtils::Clamp(PlanetaryTest2, 0, NoiseParameters->HeightCutOff) - NoiseParameters->OverallLandHeight) / NoiseParameters->PlanetaryPostAffect + (Bumbiness * 2);

		/// - stores height information after noise is applied
		const double BeachlineTest2 = NoiseMathUtils::Clamp01((Data.HeightNormalize - .01) * 200);
		const double HeightInfo1 = NoiseMathUtils::Clamp01(Data.HeightNormalize * 20);


		/// - Generate and process basic terrain noise elements. Initialize and process from heightmap data
		const double HillNoise = NoiseClass.Fractal(WarpedPosition * 0.05, 7, 2.5, 2) * 0.005 * NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(NoisePosition * .3) * NoiseParameters->MicroHillsMask, 2));
		const double HillNoise2 = pow(Hill_BiomeMix::GetNoise(NoiseClass, WarpedPosition * NoiseParameters->HillScale, NoiseParameters->HillOctave, NoiseParameters->HillLacunarity, NoiseParameters->HillPersistence), 2) * NoiseParameters->HillHeight;
		const double HillNoiseMask = NoiseMathUtils::Clamp01(pow(NoiseMathUtils::Clamp01(ActualData.HeightNormalize - NoiseParameters->hillsCutoff) * NoiseParameters->hillsIntesity, NoiseParameters->hillsAreaContrast) * NoiseMathUtils::Clamp01(pow(NoiseClass.Noise_01(ExteameHillswarpPosition / 35) * NoiseParameters->MicroHillsMask / 2, 5)));
		const double Hills = ((ExteameHillsNoise)*ExteameHillsMask);

		const double TestMountains2 = Mountains * NoiseMathUtils::Clamp01(ActualData.HeightNormalize * 2);
		const double MountainsVar = TestMountains2 / (1 + (PlanetDeformationPost * 0.5));
		const double HillSection = HillNoise + NoiseParameters->HeightOffset + (pow(MountMask2, 8) * 3.2);

		const double ridgeDetail = pow(NoiseClass.FractalRidge(WarpedPosition * 0.40, 3, 2.2, 0.8) * 1, 0.5);
		const double hilllayer = HillNoise2 * (pow(ridgeDetail, 1) * 1.5) * (HillNoiseMask - MountainsVar) * BeachlineTest2;


		/// - blends all basic terrian commonents together in one varible
		const double AboveSeaLevel = NoiseMathUtils::Lerp(hilllayer + ((Hills * (Mountains + (HillNoise2 / 2) + 0.012) * ridgeDetail - (NoiseParameters->ExteameHillsHeightOffset * ExteameHillsMask))), MountainsVar, MountainMask);

		Data.HeightNormalize += AboveSeaLevel;

		/// - adds micro hill noise and flattens it near coastline areas
		const double BeachlineTest = NoiseMathUtils::Clamp01(pow(((Data.HeightNormalize) - NoiseParameters->CoastlineShift2) * 1730, 3) * 0.04);
		Data.HeightNormalize += HillNoise * BeachlineTest;


		const double heightvar = (pow(Data.HeightNormalize * -1 + 1, 70) * 7);
		const double heightvar2 = (pow(Data.HeightNormalize * -1 + 0.95, 40) * 1.5);

		/// - simple river algorithm (-outdated-)

		const double RiverVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), NoiseParameters->RiverSharpness * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity * NoiseMathUtils::Clamp(pow(heightvar, 7), .02, 1)) + NoiseParameters->RiverBumpinessOffset * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)));
		const double RiverVienMaskArea = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.5f * NoiseParameters->RiverVienScale, 1), 10) * 2);
		const double RiverBranchVienMask = NoiseMathUtils::Clamp01(pow(NoiseClass.FractalRidge(RiverWarpedPosition1 * 1.1f * NoiseParameters->RiverBranchScale, 1), NoiseParameters->RiverBranchSharpness * NoiseMathUtils::Clamp(pow(heightvar, 5), .2, 1)) * ((NoiseClass.Fractal(WarpedPosition * 1.5f * NoiseParameters->RiverBumpiness, 4) * NoiseParameters->RiverBumpinessIntensity * NoiseMathUtils::Clamp(pow(heightvar, 7), .02, 1)) + NoiseParameters->RiverBumpinessOffset));
		const double rivermask = NoiseMathUtils::Lerp(RiverVienMask + (RiverBranchVienMask * RiverVienMaskArea), 0, NoiseMathUtils::Clamp01(MountainMask * 5)) * (OceanFloorMask * -1 + 1);


		/// - Generation for underwater surface detail by utilizing the already given noise and inverting it to create crevasses 
		const double underwater = NoiseMathUtils::Lerp(0, -HillNoise + -HillNoise2, NoiseMathUtils::Clamp01((.0002 + Data.HeightNormalize) * -150));
		Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, (Data.HeightNormalize * 4.5), OceanFloorMask);
		Data.HeightNormalize += underwater;
		Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, Data.HeightNormalize - (pow(NoiseMathUtils::Clamp(PlanetaryTest2, 0, 0.75) * 1.1, 30) * 1) - Bumbiness + NoiseParameters->RiverHeightScale, rivermask);

		/// - generation for icecaps
		Data.HeightNormalize = NoiseMathUtils::Lerp((Mountains * 2) + NoiseParameters->IceCapHeight, Data.HeightNormalize, Icecapemask);




		/// - initializes temperature from latitude and terrain topography with the added affect with humidity influnce from greenhouse like affects
		const double Tempvar1 = (pow(LatitudeVar, 2) * 2.5) + NoiseParameters->TemperatureOffset;
		Data.Temperature = Tempvar1 - (Icecapemask2 * 0.1);
		Data.Temperature = Data.Temperature - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 1, 1.2);
		Data.Temperature *= NoiseClass.Fractal((WarpedPosition + (pow(ActualData.HeightNormalize, 0.5) * 3000)) * 0.002443, 6) + .4;
		Data.Temperature *= pow(OceanTempMask, 1.2) * 1.4;
		Data.Temperature += NoiseMathUtils::Lerp(((ActualData.Humidity * 1.8) - 1) / 4, ((ActualData.Humidity * -1 * NoiseParameters->HumidityInflunce) + 1) / 4, NoiseMathUtils::Clamp01(pow(LatitudeVar + 0.7, 60)));

		/// - makes heigher alltitude more humid due to humidity build up in the layer, still needs futher work in the future 
		const double CloudLayer = NoiseMathUtils::Clamp01(pow((pow(Data.HeightNormalize / NoiseParameters->CloudLayerRunoff, NoiseParameters->CloudLayerRunoffsharpness)) * (-Data.HeightNormalize + NoiseParameters->CloudLayerMaxheight) * 6, 1) * 1200);
		Data.Humidity *= NoiseMathUtils::Clamp01((-Data.HeightNormalize + 0.4 + (CloudLayer * NoiseParameters->CloudPercipitationIntensity - (PlanetDeformationPost * NoiseParameters->DeformationRunoffIntensity))) - (MountainMask * 0.175) + NoiseMathUtils::Clamp(((pow(RiverVienMaskArea, 2) * 4) - powf(NoiseMathUtils::Clamp01(Data.HeightNormalize - 0.01) * 10, 1.2)) * (heightvar2 * 2) - (Data.Humidity) * 3, 0, 5)) * Icecapemask;


		/// - Generatues sand dune patterns at near 0 humidity
		Data.HeightNormalize = NoiseMathUtils::Lerp(NoiseMathUtils::Lerp(Data.HeightNormalize + MicroDunes * NoiseMathUtils::Clamp01(Icecapemask + 0.24), Data.HeightNormalize, NoiseMathUtils::Clamp01(MountainMask * 6)), Data.HeightNormalize, pow(NoiseMathUtils::Clamp01(Data.Humidity + 0.993), 500) + MountainMask);

		/// - creates a gradient from dry to humid from deepest to shallowest (-unfinished:notes; add variations for potential coral reefs-)
		Data.Humidity = NoiseMathUtils::Lerp(Data.HeightNormalize + 0.78, Data.Humidity, NoiseMathUtils::Clamp01((Data.HeightNormalize - NoiseParameters->TestOcean) * 2000000));

		Data.Temperature = NoiseMathUtils::Lerp((pow(NoiseMathUtils::Clamp01(LatitudeVar - 0.2), 2) * 6) * (OceanTempMask + 0.1), Data.Temperature, NoiseMathUtils::Clamp01((Data.HeightNormalize + 0.01) * 2000));
		Data.Temperature /= 1.8;
	}
	else
	{

		Data.HeightNormalize = 0;
		
	}
	//Random noise for foliage mask
	Data.FoliageMask = GeneralNoise * 0.5 + 0.5;


	return Data;
}

FNoiseData UearthTerra::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}


FNoiseData UearthTerra::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();

	Data.Height = Data.HeightNormalize * NoiseIntensity;
	return Data;

}

UearthTerra::UearthTerra() {
	NoiseParameters = CreateDefaultSubobject<UEarthTerraMix>(TEXT("Noise Parameters"));
}