// Fill out your copyright notice in the Description page of Project Settings.


#include "RockyWorlds.h"

URockyWorlds::URockyWorlds() {
}



FNoiseData URockyWorldsNoise::GetHeight(CustomNoise NoiseClass, DVector position, DVector PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary, double& LandMassNoise)
{
	FNoiseData Data = FNoiseData();


	const double LatitudeVar = ((cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry) * 1) + 1) / 1.75;
	
	// - Set up for local varibles
	const double Hotspotsum = NoiseParameters->HotSpotWeightStart + NoiseParameters->HotSpotWeightRange;
	const double OreganysSum = NoiseParameters->OrogenyWeightStart + NoiseParameters->OrogenyWeightRange;

	const double weight0 = NoiseParameters->ConentialWeightPercentage - 0.55;
	const double weight1 = NoiseParameters->OrogenyWeightStart - 1.46;
	
	const double N1 = NoiseClass.Fractal(NoisePosition * 0.008, 7, 2.1, 0.7);
	const double DomainWarped = NoiseClass.Noise_01(NoisePosition * 0.014 + (N1 * 5));


	// - Generation for terrian warping
	const double continentalwarp = NoiseClass.Fractal(NoisePosition * NoiseParameters->continentalwarp2Scale, NoiseParameters->ContinentOctave, NoiseParameters->continentLacunarity, NoiseParameters->continentPersistence);
	const DVector continentalWarpedPosition = NoisePosition * NoiseParameters->continentalwarpScale + FVector(continentalwarp, continentalwarp, continentalwarp) * NoiseParameters->continentalwarpIntensity;
	const double warp2 = NoiseClass.Fractal(NoisePosition * 0.6, 5, 2.2, 0.5) / 4;


	const double ContentialNoise = NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->ContentialNoiseScale, 4, 2, 0.5);
	
	// Generate river noise add characteristics of rivers on the terrain
	const double RiverNoise = NoiseClass.Ridge(continentalWarpedPosition * 0.02 + warp2);


	const double SmoothHillMask = NoiseMathUtils::SmoothStepUE(0.4, 0.8, NoiseClass.Fractal(continentalWarpedPosition * 0.05, 3, 2, 0.5));
	
	/// - Calculate the intensity of orogeny (mountain-building) based on ridge noise function values,
	const double orogeny = NoiseMathUtils::SmoothStepUE(0.3,1,NoiseClass.Ridge(continentalWarpedPosition * NoiseParameters->OragneyVeinScale));

	/// - Calculate the presence of a seafloor rift based on ridge noise function values,
	const double SeaFloorRift = (NoiseMathUtils::SmoothStepUE(.8, 1, NoiseClass.Ridge(continentalWarpedPosition * 0.0013 + 2)) - (NoiseMathUtils::SmoothStepUE(.995, .999, NoiseClass.Ridge(continentalWarpedPosition * 0.0013 + 2)) / 3)) * NoiseParameters->SeaFloorSpreadingIntensity;
	
	/// - Varible that holds contential components
	
	const double arcs = NoiseMathUtils::Lerp((NoiseMathUtils::SmoothStepUE(0.4, 0.5, orogeny) / 4), 0, NoiseMathUtils::SmoothStepUE(.55, .6, ContentialNoise));
	const double contential = -.005 +(NoiseMathUtils::Clamp01(ContentialNoise - 0.6)) / 1.5;
	
	/// - Variable that holds Oceanic Components
	const double Oceananic = NoiseParameters->OceanFloorHeight - ((pow(orogeny, 7) * NoiseParameters->SubductionIntensity) * NoiseMathUtils::SmoothStepUE(0.5, 1, NoiseMathUtils::Lerp(0, orogeny, NoiseMathUtils::SmoothStepUE(0.39 + weight0, .6 + weight0,ContentialNoise + (orogeny  / 1.5) - 0.65)) ) / 1.5) + SeaFloorRift / 5 + (ContentialNoise / 3);
	
	/// - Apply a mask to distinguish between oceanic and continental regions. This helps in generating distinct features like islands and continental shelves.
	const double OceanicContentialMask = NoiseMathUtils::Clamp01(NoiseMathUtils::SmoothStepUE(NoiseParameters->ConentialWeightPercentage, NoiseParameters->ConentialWeightPercentage + NoiseParameters->ConentialMaskSharpness, ContentialNoise + (pow(orogeny, NoiseParameters->IslandArcSharpness) / NoiseParameters->IslandArc)));
	const double OceanicContentialMask2 = NoiseMathUtils::Clamp01(NoiseMathUtils::SmoothStepUE(NoiseParameters->Mask1, NoiseParameters->Mask2, ContentialNoise));

	/// - Interpolate between oceanic and continental heights based on a mask value,
	Data.HeightNormalize = NoiseMathUtils::Lerp(Oceananic, contential, OceanicContentialMask);

	// - generate features for hills
	const double hills = NoiseClass.SharpFractal(NoisePosition * NoiseParameters->BigHillsScale, 6, 2, 0.5) / NoiseParameters->BigHillsSoftness;
	const double Microhills = NoiseClass.SharpFractal(NoisePosition * 6 * NoiseParameters->MicroHillsScale, 2, 2, 0.6,0.35, 1) * NoiseParameters->MicroHillsIntensity;
	const double L1 = NoiseClass.SharpFractal(NoisePosition * NoiseParameters->PuffyHillsScale, 6, 2, 0.5, 0.2) * -1 + 1;
	const double PufflyHills = NoiseMathUtils::Lerp(L1 * 0.4 * NoiseParameters->PuffyHillsIntensity, L1 * NoiseParameters->PuffyHillsIntensity, L1);
	const double smoothHills = PufflyHills * SmoothHillMask;


	
	// - generate features for mountains
	const double test12 =  (ContentialNoise + orogeny) * 200;
	const double BigMount = NoiseClass.SharpFractal((NoisePosition + test12 + (continentalWarpedPosition / 5)) / 4, 2, 2, 0.2,0.2 ,1);
	const double Flatten = NoiseMathUtils::Lerp(0, BigMount / 3 + (hills / 9), BigMount);
	const double Flatten2 = NoiseMathUtils::Lerp(0, hills / 9, hills) ;


	// - Simulate geological features such as veins, mountains, plateaus, uplifts, and mountain ranges with various noise functions and interpolations based on continental noise and orogeny intensity.
	const double veins = NoiseMathUtils::Lerp(NoiseClass.SharpFractal((NoisePosition + (continentalwarp * 100)) / 7, 3, 1.4, .9, 0.5, 1.2)/ 1.5,0, NoiseMathUtils::SmoothStepUE(1.6 + weight1, 1.72 + weight1, ContentialNoise + orogeny));
	const double MountainsPlatue = NoiseMathUtils::Lerp(NoiseMathUtils::Lerp(0,Flatten, NoiseMathUtils::SmoothStepUE(1.6 + weight1, 1.64 + weight1, ContentialNoise + orogeny)), 0, NoiseMathUtils::SmoothStepUE(1.63 + weight1, 1.72  + weight1, ContentialNoise + orogeny));
	const double ContentialUplift = (NoiseClass.Fractal(continentalWarpedPosition * 0.05, 3, 2, 0.5) / 2) + 0.1;
	const double Sub = NoiseMathUtils::Lerp((hills * -1 + 1.5) / 4, 0, NoiseMathUtils::SmoothStepUE(1.6 + weight1, 1.66 + weight1, ContentialNoise + orogeny));
	const double MountainRanges = NoiseMathUtils::Lerp(orogeny / 170, MountainsPlatue + ContentialUplift, NoiseMathUtils::SmoothStepUE(0, 1, (NoiseMathUtils::SmoothStepUE(NoiseParameters->OrogenyWeightStart, OreganysSum, ContentialNoise + orogeny) - (BigMount / 4) - Sub - (veins))));

	// generates the presence of hotspots, A phenomena where magma rises from deep within the mantle, creating localized volcanic activity and potential mountain formation.
	const double Hotspots = NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->HotSpotNoiseScale, 4, 2, 0.5);
	const double HotSpotMountains = NoiseMathUtils::Lerp(0, NoiseMathUtils::SmoothStepUE(0,2,Flatten2 * 2 + (NoiseMathUtils::SmoothStepUE(NoiseParameters->HotSpotWeightStart, Hotspotsum, Hotspots) - 0.2)) * 1.5, NoiseMathUtils::SmoothStepUE(NoiseParameters->HotSpotWeightStart, Hotspotsum,Hotspots));

	double crater1 = 0;
	if (NoiseParameters->GenCraters) {

		crater1 = NoiseClass.GetCraterFractal(NoiseClass, NoisePosition / NoiseParameters->CraterNoiseSize, NoiseParameters->CraterOctave, NoiseParameters->CraterLacunarity, NoiseParameters->CraterPersistence, NoiseParameters->CraterFrequency, NoiseParameters->CraterWarp, NoiseParameters->CraterSize, NoiseParameters->CraterRimHeight, NoiseParameters->CraterRimOctaveLimit, NoiseParameters->CraterRimPower, NoiseParameters->CraterHeightMultiplier);

	}
	Data.HeightNormalize += MountainRanges + (smoothHills / 12) + HotSpotMountains;

	// - Generation for micro hills for flatlands, alongside a mask to add variation for ultraflat lands
	const double MicroHillsLayer = NoiseMathUtils::SmoothStepUE(0.0032, 0.004, Data.HeightNormalize + 0.0032) + NoiseMathUtils::SmoothStepUE(1, 1.01, -Data.HeightNormalize + 1);
	const double MicroHillsMask = pow(NoiseClass.Fractal(continentalWarpedPosition * 0.09, 2, 2, 0.5), 2);
	Data.HeightNormalize += (Microhills * MicroHillsMask * MicroHillsLayer / 80);

	if (NoiseParameters->GenCraters) {

		Data.HeightNormalize += crater1 / 7 - 0.17;

	}
	// - Used to get the value of the land height to change the constant within the smoothstep funtion below this varible line.
	const double landValue = NoiseMathUtils::Lerp(.98, .5 + ((L1 * -1 + 1) / 9), NoiseMathUtils::SmoothStepUE(0, 0.5, pow(Data.HeightNormalize, 0.5)));
	
	// - applies the river onto the terrian along with making increased erosion or softening to terrian at higher elevation
	double river = NoiseMathUtils::Lerp(NoiseMathUtils::SmoothStepUE(landValue, 1, RiverNoise), 0 , NoiseMathUtils::SmoothStepUE(.6,.73,ContentialNoise));
	river = NoiseMathUtils::Lerp(0, river, NoiseMathUtils::SmoothStepUE(.525, .55, ContentialNoise + orogeny - 1));
	Data.HeightNormalize = NoiseMathUtils::Lerp(Data.HeightNormalize, -0.005, river);

	
	
	// - A dunes varible used for dry conditions
	const double Dunes = NoiseClass.Ridge(NoisePosition * 60) / 500;

	// - Calculate turbulence intensity to simulate atmospheric disturbances affecting weather patterns.
	const double Turbulance = NoiseClass.Fractal(NoisePosition * 0.0015 * NoiseParameters->HadleyCellsAmount,3,2,.7) * NoiseParameters->HadleyCellsTurbulance;

	// - Determine the influence of Hadley cells based on latitude and turbulence, regulating precipitation distribution.
	const double hadleycells = NoiseMathUtils::SmoothStepUE(-0.5, 1, cos(lattitude * 7 * NoiseParameters->HadleyCellsAmount + Turbulance) * 1);

	// - Generate precipitation cells using fractal noise, adjusted by Hadley cell activity and latitude.
	const double cell1 = pow(NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->TurbulantCellsScale + (N1 * 2), 2, 2, 0.5), 4);
	const double cell2 = pow(NoiseClass.Fractal(continentalWarpedPosition * NoiseParameters->TurbulantCellsScale + 20 + (N1 * 2), 2, 2, 0.5), 1.5) * 1.3;
	const double Randomcells = NoiseMathUtils::SmoothStepUE(.2,1, pow(NoiseClass.Fractal(continentalWarpedPosition * 0.006 + (N1 * 2), 3, 2, 0.5), 1.5));

	//  - Adjust the precipitation amount based on latitude and additional atmospheric conditions.
	const double parcipitationCells = NoiseMathUtils::Lerp(cell1, cell2, hadleycells) * (NoiseMathUtils::SmoothStepUE(-0.2, 1, cos(lattitude * 2) * 1) + 1) / 1.75;

	// - formation for icecaps
	const double IceCapDeformation = NoiseClass.Fractal(continentalWarpedPosition * 0.005, 3, 2, 0.5);
	Data.HeightNormalize = NoiseMathUtils::Lerp(-0.1 + (ContentialNoise / 3) + (Dunes * Flatten2 * 4) + HotSpotMountains, Data.HeightNormalize, NoiseMathUtils::SmoothStepUE(.35, 0.36, (LatitudeVar + 0.06 - (NoiseParameters->TemperatureOffset / 1.05)) - ((IceCapDeformation + (hills / 150)) / 10)));

	// -  decreases the temprature at an attuide alongside an additional masking layer for mountains to similate Cirque glacier. 
	const double temp1 = 0 - NoiseMathUtils::Lerp(0, ((hills + (BigMount / 3)) / 6), NoiseMathUtils::SmoothStepUE(0.06, 0.5, Data.HeightNormalize));
	Data.Temperature = NoiseMathUtils::SmoothStepUE(-.4, 1, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)) * (ContentialNoise * 1.2) - temp1 ;
	Data.Temperature -= NoiseParameters->TemperatureOffset;


	const double latvar = NoiseMathUtils::Lerp(.11, .52, NoiseMathUtils::SmoothStepUE(-.2 + NoiseParameters->TemperatureOffset, 0.6 + NoiseParameters->TemperatureOffset, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)));

	Data.Temperature = NoiseMathUtils::Lerp(Data.Temperature, 0, NoiseMathUtils::SmoothStepUE(0, latvar, Data.HeightNormalize));

	// - Adjust the climate type based on the planet's rotational spin; higher values imply more chaotic weather patterns, while lower value gives less distribution depending on the precipitation value.
	double climateType = NoiseMathUtils::Lerp(parcipitationCells, Randomcells, NoiseMathUtils::Clamp01((NoiseParameters->HadleyCellsAmount - 1.1) + (abs(NoiseParameters->HadleyCellsTurbulance) - 3.5)));
	climateType = NoiseMathUtils::Lerp(NoiseMathUtils::SmoothStepUE(0.25,0.6, -ContentialNoise + 1), climateType, NoiseMathUtils::Clamp01(NoiseParameters->HadleyCellsAmount * 2));

	// - applies the various cells and factors together
	const double ClimateElvationFactor = (NoiseParameters->ElevationClimateShadow - 1) / 5;

	const double ClimateShadowLatitude = NoiseMathUtils::Lerp(0, 0.25 + ClimateElvationFactor, NoiseMathUtils::SmoothStepUE(-0.4 + NoiseParameters->TemperatureOffset, 0.7 + NoiseParameters->TemperatureOffset, cos(lattitude * 2 + NoiseParameters->TemperatureAysemetry)));


	Data.Humidity = NoiseMathUtils::Lerp(climateType, 0 , NoiseMathUtils::SmoothStepUE(ClimateShadowLatitude, ClimateShadowLatitude + 0.1,Data.HeightNormalize));
	Data.Humidity += NoiseMathUtils::Lerp(orogeny /2, 0, NoiseMathUtils::SmoothStepUE(0.5,0.63, ContentialNoise));
	Data.Humidity = log((Data.Humidity * NoiseParameters->humidityMultiplier) + 1);
	
	
	// - adds dunes in desurts.
	const double dunesLayer = NoiseMathUtils::Lerp(Dunes, 0, NoiseMathUtils::SmoothStepUE(0, 0.15, Data.Humidity));
	Data.HeightNormalize += NoiseMathUtils::Lerp(dunesLayer, 0, NoiseMathUtils::SmoothStepUE(0.05,0.1,Data.HeightNormalize)) * MicroHillsLayer;

	// - adds icecaps.
	const double Ice1 = NoiseMathUtils::SmoothStepUE(.62, 1, ContentialNoise) * 3;
	Data.HeightNormalize += NoiseMathUtils::Lerp(Ice1 + (NoiseMathUtils::Clamp(MountainRanges - Ice1, 0, 5)), 0, NoiseMathUtils::SmoothStepUE(.3, .4, LatitudeVar));
	
	if (NoiseParameters->ShowOceanElvation) {
		Data.Humidity = NoiseMathUtils::Clamp01((Data.HeightNormalize - NoiseParameters->HeightTester));
	}
	//Data.Humidity = parcipitationCells;
	
	

	return Data;
}


FNoiseData URockyWorldsNoise::GetNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	double LandMassNoise;
	FNoiseData Data = FNoiseData();
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	Data = GetHeight(NoiseClass, position, PlanetPosition, NoiseScale, NoiseIntensity, PlanetScale, FlatWorld, lattitude, NoisePosition, ActualData, UsePlanetary, LandMassNoise);

	Data.Height = Data.HeightNormalize * NoiseIntensity;

	return Data;

}


FNoiseData URockyWorldsNoise::GetOceanNoise(CustomNoise NoiseClass, const DVector& position, const DVector& PlanetPosition, double NoiseScale, double NoiseIntensity, double PlanetScale, bool FlatWorld, double lattitude, DVector& NoisePosition, FNoiseData ActualData, bool UsePlanetary)
{
	//Noise scale with the planet, to avoid issue if the position value is too high
	NoisePosition = (position / PlanetScale) * NoiseScale;

	FNoiseData Data = FNoiseData();

	Data.Height = Data.HeightNormalize * NoiseIntensity;
	return Data;

}

URockyWorldsNoise::URockyWorldsNoise() {
	NoiseParameters = CreateDefaultSubobject<URockyWorlds>(TEXT("Noise Parameters"));
}