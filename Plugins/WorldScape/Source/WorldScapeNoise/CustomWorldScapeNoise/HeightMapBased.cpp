// Fill out your copyright notice in the Description page of Project Settings.


#include "HeightMapBased.h"

UHeightMap::UHeightMap() {
}



FNoiseData UHeightMapBased::GetHeight(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
{
	FNoiseData Data = FNoiseData();

	const double continentalwarp = NoiseClass.Fractal(NoisePosition * 0.002, 3, 2, 0.5);
	const DVector continentalWarpedPosition = NoisePosition * NoiseParameters->continentalwarpScale + FVector(continentalwarp, continentalwarp, continentalwarp) * NoiseParameters->continentalwarpIntensity;
	const double ContentialNoise = NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->ContentialNoiseScale, 4, 2, 0.5);
	const double warp3 = NoiseClass.Fractal(NoisePosition * 6 * NoiseParameters->MicroWarpScale, 5, 2.2, 0.5) / 1.5;

	const double N1 = NoiseClass.Fractal(NoisePosition * 0.008, 7, 2.1, 0.7);
	const double DomainWarped = NoiseClass.Noise_01(NoisePosition * 0.014 + (N1 * 5));
	// - generate features for hills

	const double SmoothHillMask = NoiseMathUtils::SmoothStepUE(0.4, 0.8, NoiseClass.Fractal(continentalWarpedPosition * 0.05, 3, 2, 0.5));
	const double SmoothHillMask2 = NoiseMathUtils::SmoothStepUE(0.4, 1, NoiseClass.Fractal(continentalWarpedPosition * 0.06, 3, 2, 0.5));


	// - A dunes varible used for dry conditions
	const double Dunes = NoiseClass.Ridge(NoisePosition * 60) / 500;

	const double hills = NoiseClass.SharpFractal(NoisePosition * 1, 6, 2, 0.5);
	const double Microhills = NoiseClass.SharpFractal(NoisePosition * 6 * NoiseParameters->MicroHillsScale + (warp3 * NoiseParameters->MicroHillsWarpIntensity), 2, 2, 0.6, 0.35, 1) * NoiseParameters->MicroHillsIntensity;

	const double L1 = NoiseClass.SharpFractal(NoisePosition * NoiseParameters->PuffyHillsScale, 6, 2, 0.5, 0.2) * -1 + 1;
	const double L2 = NoiseClass.SharpFractal(NoisePosition * NoiseParameters->PuffyHillsScale / 4, 6, 2, 0.5, 0.2) * -1 + 1;
	const double PufflyHills2 = NoiseMathUtils::Lerp(L2 * 0.4, L2, L2);
	const double PufflyHills = NoiseMathUtils::Lerp(L1 * 0.4 * NoiseParameters->PuffyHillsIntensity, L1 * NoiseParameters->PuffyHillsIntensity, L1);



	// - generate features for mountains
	const double BigMount = NoiseClass.SharpFractal((NoisePosition + (continentalWarpedPosition / 5)) / 4, 2, 2, 0.2, NoiseParameters->MountainsBaseStart, 1);
	const double Flatten = NoiseMathUtils::Lerp(0, BigMount / 3 + (hills / 9), BigMount);
	const double Flatten2 = NoiseMathUtils::Lerp(0, hills / 9, hills);
	const double veins = NoiseMathUtils::Lerp(NoiseClass.SharpFractal((NoisePosition + (continentalwarp * 500)) / 6, 4, 1.3, 1, 0.6, 1.2), 0, NoiseMathUtils::SmoothStepUE( 0.6, 0.9, ActualData.Temperature));


	Data.HeightNormalize = ActualData.HeightNormalize - NoiseParameters->HeightOffset;

	Data.HeightNormalize = NoiseMathUtils::Lerp(pow(Data.HeightNormalize, 1), pow(Data.HeightNormalize, NoiseParameters->ExponetialFilterValue) + (NoiseParameters->LandSeaLevelCorrection * 0.0002), NoiseMathUtils::SmoothStepUE(0, 0.001, Data.HeightNormalize));

	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize * NoiseParameters->MultiplyHeightBelowSeaLevel, Data.HeightNormalize * NoiseParameters->MultiplyHeightAboveSeaLevel, NoiseMathUtils::SmoothStepUE(-0.00001, 0.00001, Data.HeightNormalize));
	
	const double mask = NoiseMathUtils::Lerp(1, BigMount, NoiseMathUtils::SmoothStepUE(0 ,0.1, Data.HeightNormalize));

	double VeinedHills = NoiseMathUtils::Lerp((hills * mask / 9) - 0.03, -Data.HeightNormalize / 3, NoiseMathUtils::SmoothStepUE(0.05, 0.3, veins + ((hills - 0.5) / 5)));

	VeinedHills = NoiseMathUtils::Lerp(VeinedHills, (hills * mask / 9) - 0.03, NoiseParameters->NoVienMaskAlpha);
	Data.HeightNormalize += NoiseMathUtils::Lerp(0, VeinedHills, NoiseMathUtils::SmoothStepUE(NoiseParameters->SharpHillMaskStart, NoiseParameters->SharpHillMaskEnd, ActualData.Temperature + ((hills - 1) / 5)));

	const double mountainRanges = NoiseMathUtils::Lerp(0, (BigMount * NoiseMathUtils::Lerp(hills, 1, 0.3) * NoiseParameters->MountainsIntensity), NoiseMathUtils::SmoothStepUE(0.09, 0.9, ActualData.Temperature + ((hills - 1) / 5)));

	Data.HeightNormalize += mountainRanges;

	double crater1 = 0;
	if (NoiseParameters->GenCraters) {

		crater1 = NoiseClass.GetCraterFractal(NoiseClass, NoisePosition / NoiseParameters->CraterNoiseSize, NoiseParameters->CraterOctave, NoiseParameters->CraterLacunarity, NoiseParameters->CraterPersistence, NoiseParameters->CraterFrequency, NoiseParameters->CraterWarp, NoiseParameters->CraterSize, NoiseParameters->CraterRimHeight, NoiseParameters->CraterRimOctaveLimit, NoiseParameters->CraterRimPower, NoiseParameters->CraterHeightMultiplier);

	}

	if (NoiseParameters->GenCraters) {

		Data.HeightNormalize += (crater1 / 7) + NoiseParameters->HeightOffsetCraters;

	}

	Data.HeightNormalize += NoiseMathUtils::Lerp(0, (PufflyHills / 20) - 0.003, NoiseMathUtils::SmoothStepUE(NoiseParameters->PuffyHillMaskStart, NoiseParameters->PuffyHillMaskEnd, ActualData.Temperature + ((L1 - 1) / 12)));


	Data.HeightNormalize += NoiseMathUtils::Lerp(0, ((PufflyHills / 30) - 0.003) * SmoothHillMask2, NoiseMathUtils::SmoothStepUE(-0.002, 0.006, Data.HeightNormalize));

	Data.HeightNormalize += NoiseMathUtils::Lerp(0, ((PufflyHills2 / 15) - 0.003) * SmoothHillMask, NoiseMathUtils::SmoothStepUE(-0.0, 0.01, Data.HeightNormalize));
	
    double HotSpotMountains = 0;
	if (NoiseParameters->GenHotspots) {
		const double Hotspotsum = NoiseParameters->HotSpotWeightStart + NoiseParameters->HotSpotWeightRange;
		const double Hotspots = NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->HotSpotNoiseScale, 4, 2, 0.5);
		HotSpotMountains = NoiseMathUtils::Lerp(0, NoiseMathUtils::SmoothStepUE(0, 2, Flatten2 * 2 + (NoiseMathUtils::SmoothStepUE(NoiseParameters->HotSpotWeightStart, Hotspotsum, Hotspots) - 0.2)) * 1.5, NoiseMathUtils::SmoothStepUE(NoiseParameters->HotSpotWeightStart, Hotspotsum, Hotspots));
		Data.HeightNormalize += HotSpotMountains * NoiseParameters->HotSpotHeight;
	}


	Data.HeightNormalize -= NoiseMathUtils::SmoothStepUE(0.05,1,ActualData.Temperature) * 0.01;
	
	const double MicroHillsLayer = NoiseMathUtils::SmoothStepUE(0.0032, 0.004, Data.HeightNormalize + 0.0032) + NoiseMathUtils::SmoothStepUE(1, 1.01, -Data.HeightNormalize + 1);
	const double MicroHillsMask = pow(NoiseClass.Fractal(continentalWarpedPosition * 0.09, 2, 2, 0.5), 2);
	Data.HeightNormalize += (Microhills * MicroHillsMask * MicroHillsLayer / 80);

	const double ClimateShadowLatitude = NoiseMathUtils::Lerp(0, .25, NoiseMathUtils::SmoothStepUE(-0.4 + NoiseParameters->TemperatureOffset, 0.7 + NoiseParameters->TemperatureOffset, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)));

	Data.Humidity = NoiseMathUtils::Lerp(pow(ActualData.Humidity, 1.3), 0, NoiseMathUtils::SmoothStepUE(ClimateShadowLatitude, ClimateShadowLatitude + 0.1, Data.HeightNormalize));
	Data.Humidity *= NoiseMathUtils::Lerp(DomainWarped * 1.6,1,0.55);


	if (NoiseParameters->UseProceduralClimate) {

		// - Calculate turbulence intensity to simulate atmospheric disturbances affecting weather patterns.
		const double Turbulance = NoiseClass.Fractal(NoisePosition * 0.0015 * NoiseParameters->HadleyCellsAmount, 3, 2, .7) * NoiseParameters->HadleyCellsTurbulance;

		// - Determine the influence of Hadley cells based on latitude and turbulence, regulating precipitation distribution.
		const double hadleycells = NoiseMathUtils::SmoothStepUE(-0.5, 1, cos(lattitude * 7 * NoiseParameters->HadleyCellsAmount + Turbulance) * 1);

		const double cell1 = pow(NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->TurbulantCellsScale + (N1 * 2), 2, 2, 0.5), NoiseParameters->Cell1Power) * NoiseParameters->Cell1Intensity;
		const double cell2 = pow(NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->TurbulantCellsScale + 20 + (N1 * 2), 2, 2, 0.5), NoiseParameters->Cell2Power) * NoiseParameters->Cell2Intensity;
		const double Randomcells = NoiseMathUtils::SmoothStepUE(.2, 1, pow(NoiseClass.Fractal(continentalWarpedPosition * 0.006 + (N1 * 2), 3, 2, 0.5), 1.5));
		
		//  - Adjust the precipitation amount based on latitude and additional atmospheric conditions.
		const double parcipitationCells = NoiseMathUtils::Lerp(cell1, cell2, hadleycells) * NoiseMathUtils::Lerp(1, ((NoiseMathUtils::SmoothStepUE(-0.2, 1, cos(lattitude * 2) * 1) + 1) / 1.75), NoiseParameters->EquatorBiasAlpha);

		double climateType = NoiseMathUtils::Lerp(parcipitationCells, Randomcells, NoiseMathUtils::Clamp01((NoiseParameters->HadleyCellsAmount - 1.1) + (abs(NoiseParameters->HadleyCellsTurbulance) - 3.5)));
		climateType = NoiseMathUtils::Lerp(NoiseMathUtils::SmoothStepUE(0.25, 0.6, -ContentialNoise + 1), climateType, NoiseMathUtils::Clamp01(NoiseParameters->HadleyCellsAmount * 2));

		const double Climatevar = NoiseMathUtils::Lerp(climateType, NoiseMathUtils::Clamp01(pow(ActualData.Humidity, 1.3)), NoiseParameters->BlendDataMap);
		Data.Humidity = NoiseMathUtils::Lerp(Climatevar, 0, NoiseMathUtils::SmoothStepUE(ClimateShadowLatitude, ClimateShadowLatitude + 0.1, Data.HeightNormalize));
		Data.Humidity = NoiseMathUtils::Lerp(Data.Humidity, NoiseMathUtils::Lerp(climateType, .4, NoiseMathUtils::SmoothStepUE(0,1,Data.HeightNormalize * DomainWarped)), 1);
		Data.Humidity += NoiseMathUtils::Lerp(mountainRanges / 2, 0, NoiseMathUtils::SmoothStepUE(0.5, 0.63, ContentialNoise));
		Data.Humidity = log((Data.Humidity * NoiseParameters->humidityMultiplier) + 1);

	}
	








	const double temp1 = -0 - NoiseMathUtils::Lerp(0, ((hills + (BigMount / 3)) / 6), NoiseMathUtils::SmoothStepUE(0.06, 0.5, Data.HeightNormalize));
	const double LatitudeVar = ((cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry) * 1) + 1) / 1.75;

	// - formation for icecaps
	const double IceCapDeformation = NoiseClass.Fractal(continentalWarpedPosition * 0.005, 4, 2.2, 0.5);
	Data.HeightNormalize = NoiseMathUtils::Lerp(-0.1 + (ContentialNoise * NoiseParameters->IceSheetThickness) + (Dunes * Flatten2 * 4) + HotSpotMountains, Data.HeightNormalize, NoiseMathUtils::SmoothStepUE(.35, NoiseParameters->IceSheetSoftness + 0.35, (LatitudeVar + 0.06  + NoiseParameters->IceSheetTansitionOveride - (NoiseParameters->TemperatureOffset / 1.05)) - ((IceCapDeformation + (hills / 150)) / 10)));

	// - adds dunes in desurts.
	const double dunesLayer = NoiseMathUtils::Lerp(Dunes, 0, NoiseMathUtils::SmoothStepUE(0, 0.15, Data.Humidity));
	Data.HeightNormalize += NoiseMathUtils::Lerp(dunesLayer, 0, NoiseMathUtils::SmoothStepUE(0.05, 0.1, Data.HeightNormalize)) * MicroHillsLayer;

	Data.Temperature = (NoiseMathUtils::SmoothStepUE(NoiseParameters->PolarSmoothStepBeginning, NoiseParameters->PolarSmoothStepend, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)) * NoiseMathUtils::Lerp(DomainWarped, 1, NoiseParameters->TemperatureNoiseAmount))  - temp1;
	Data.Temperature -= NoiseParameters->TemperatureOffset;

	const double latvar = NoiseMathUtils::Lerp(NoiseParameters->TemperaturePolarHeightCap, NoiseParameters->TemperatureEquatorHeightCap, NoiseMathUtils::SmoothStepUE(-.2 + NoiseParameters->TemperatureOffset, 0.6 + NoiseParameters->TemperatureOffset, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)));

	Data.Temperature = NoiseMathUtils::Lerp(Data.Temperature, 0, NoiseMathUtils::SmoothStepUE(0, latvar,Data.HeightNormalize));

	return Data;
}


FNoiseData UHeightMapBased::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	Data.HeightNormalize -= ActualData.WaterMask;

	return Data;

}


FNoiseData UHeightMapBased::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();

	const double continentalwarp = NoiseClass.Fractal(NoisePosition * 0.002, 3, 2, 0.5);


	Data.Height = Data.HeightNormalize * NoiseIntensity;
	return Data;

}

UHeightMapBased::UHeightMapBased() {
	NoiseParameters = CreateDefaultSubobject<UHeightMap>(TEXT("Noise Parameters"));
}