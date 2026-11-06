// Copyright 2021 IOLACORP STUDIO. All Rights Reserved



#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "NoiseMathUtils.h"
#include "WorldScapeCommon/Public/DoublePrecisionUtils.h"
#include <array>
#include <utility>

enum ENoiseType
{
    Simplex,
    Cellular,
};


enum ECellularDistanceType
{
    Euclidean,
    EuclideanSq,
    Manhattan,
    Hybrid
};

enum ECellularType
{
    CellValue,
    Distance,
    Distance2,
    Distance2Add,
    Distance2Sub,
    Distance2Mul,
    Distance2Div
};

 /**
  * @brief Multiple Noise Implementation in a single class.
  */
class WORLDSCAPENOISE_API CustomNoise {
public:
    /**
     * 3D Cellular noise
     *
     * @param[in] position FVector coordinate
     * @param[in] CellularDistanceType ECellularDistanceType Cellular Distance Type
     * @param[in] CellularType ECellularType Cellular Type
     * @param[in] cellularJitter float How much are the cellular jiterred
     *
     * @return Noise value in the range[-1; 1], value of 0 on all integer coordinates.
     */

    float CellularNoise(const FVector& position, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1.f)
    {
        return CellularNoise((float)position.X, (float)position.Y, (float)position.Z, CellularDistanceType, CellularType,(float)cellularJitter);
    }
    float CellularNoise(float x, float y, float z, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1);

    /**
     * 3D Cellular noise
     *
     * @param[in] position DVector coordinate
     * @param[in] CellularDistanceType ECellularDistanceType Cellular Distance Type
     * @param[in] CellularType ECellularType Cellular Type
     * @param[in] cellularJitter double How much are the cellular jiterred
     *
     * @return Noise value in the range[-1; 1], value of 0 on all integer coordinates.
     */
    double CellularNoise(const DVector& position, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1.)
    {
        return CellularNoise((double)position.X, (double)position.Y, (double)position.Z, CellularDistanceType, CellularType, (double)cellularJitter);
    }
    double CellularNoise(double x, double y, double z, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1);


    /**
     * 3D Open simplex noise
     *
     * @param[in] position FVector coordinate
     *
     * @return Noise value in the range[-1; 1], value of 0 on all integer coordinates.
     */
    double OpenSimplexNoise(const FVector& position) const;
    double OpenSimplexNoise(const DVector& position) const;
    
    double m_stretch3d = (-1.0 / 6);
    double m_squish3d = (1.0 / 3);
    double m_norm3d = 103;
    std::array<short, 256> m_perm;
    std::array<short, 256> m_permGradIndex3d;
    std::array<char, 72> m_gradients3d;
    double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const;

    double OpenSimplexNoise(double x, double y, double z) const;
    //double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const;
    
    float Noise(const FVector& position) {
        return Noise(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance,1);
    }
    float Noise(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1);
    
    float Noise_01(const FVector& position) {
        return Noise_01(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance,1);
    }
    float Noise_01(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1);

    // 3D Perlin simplex Ridged noise
    float Ridge(const FVector& position) {
        return Ridge(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance,1);
    }
    float Ridge(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1);

    float Ridge2(const FVector& position) {
        return Ridge2(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    float Ridge2(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1, float SmoothMin = 0.5, float SmoothMax = 1);

    float BillowNoise(const FVector& position) {
        return BillowNoise(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    float BillowNoise(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, float cellularJitter = 1);


    // Fractal/Fractional Brownian Motion (fBm) noise summation
    float Fractal(const FVector& position, int octave, float lacunarity = 2.f, float persistence = 0.5f) {
        return Fractal( position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance,1,octave, lacunarity, persistence);
    }
    float Fractal(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, float cellularJitter, int octave, float lacunarity = 2.f, float persistence = 0.5f);

    float BillowFractal(const FVector& position, int octave, float lacunarity = 2.f, float persistence = 0.5f) {
        return BillowFractal(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence);
    }
    float BillowFractal(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, float cellularJitter, int octave, float lacunarity = 2.f, float persistence = 0.5f);

    float SharpFractal(const FVector& position, int octave, float lacunarity = 2.f, float persistence = 0.5f, float SmoothMin = 0.5, float SmoothMax = 1) {
        return SharpFractal(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence, SmoothMin, SmoothMax);
    }
    float SharpFractal(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, float cellularJitter, int octave, float lacunarity = 2.f, float persistence = 0.5f, float SmoothMin = 0.5, float SmoothMax = 1);



    // Fractal/Fractional Brownian Motion (fBm) noise summation
    float JprdanTurbulance(const FVector& position, int octave, float lacunarity = 2.f, float persistence = 0.5f, float persistance1 = 0.8f, float warp = 0.35f, float warp0 = 0.4f, float damp = 0.8f, float damp0 = 1.0f, float dampScale = 1.0f ) {
        return JprdanTurbulance(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence, persistance1, warp, warp0, damp, damp0, dampScale);
    }
    float JprdanTurbulance(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, float cellularJitter, int octave, float lacunarity = 2.f, float persistence = 0.5f, float persistance1 = 0.8f, float warp = 0.35f, float warp0 = 0.4f, float damp = 0.8f, float damp0 = 1.0f, float dampScale = 1.0f);




    // Fractal/Fractional Brownian Motion (fBm) Ridged noise summation
    float FractalRidge(const FVector& position, int octave, float lacunarity = 2.f, float persistence = 0.5f) {
        return FractalRidge(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance,1, octave,  lacunarity, persistence);
    }
    float FractalRidge(const FVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, float cellularJitter, int octave, float lacunarity = 2.f, float persistence = 0.5f);



    double Noise(const DVector& position) {
        return Noise(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    double Noise(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1);

    double Noise_01(const DVector& position) {
        return Noise_01(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    double Noise_01(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1);

    // 3D Perlin simplex Ridged noise
    double Ridge(const DVector& position) {
        return Ridge(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    double Ridge(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1);

    double Ridge2(const DVector& position) {
        return Ridge2(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    double Ridge2(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1, double SmoothMin = 0.5, double SmoothMax = 1);

    double BillowNoise(const DVector& position) {
        return Ridge(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1);
    }
    double BillowNoise(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType = ECellularDistanceType::Euclidean, ECellularType CellularType = ECellularType::Distance, double cellularJitter = 1);


    // Fractal/Fractional Brownian Motion (fBm) noise summation
    double Fractal(const DVector& position, int octave, double lacunarity = 2.f, double persistence = 0.5f) {
        return Fractal(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence);
    }
    double Fractal(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, double cellularJitter, int octave, double lacunarity = 2.f, double persistence = 0.5f);
    
    double BillowFractal(const DVector& position, int octave, double lacunarity = 2.f, double persistence = 0.5f) {
        return BillowFractal(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence);
    }
    double BillowFractal(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, double cellularJitter, int octave, double lacunarity = 2.f, double persistence = 0.5f);


    double JprdanTurbulance(const DVector& position, int octave, double lacunarity = 2.f, double persistence = 0.5f, double persistence1 = 0.45f, double warp = 0.668f, double warp0 = 0.768f, double damp = 0.0f, double damp0 = 0.1f, double dampScale = 0.4f) {
        return JprdanTurbulance(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence, persistence1, warp, warp0, damp, damp0, dampScale);
    }
    double JprdanTurbulance(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, double cellularJitter, int octave, double lacunarity = 2.0f, double persistence = 0.5f, double persistence1 = 0.45f, double warp = 0.768f, double warp0 = 0.668f, double damp = 0.0f, double damp0 = 1.0f, double dampScale = 0.4f);

    double SharpFractal(const DVector& position, int octave, double lacunarity = 2.f, double persistence = 0.5f, double SmoothMin = 0.5, double SmoothMax = 1) {
        return SharpFractal(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence, SmoothMin, SmoothMax);
    }
    double SharpFractal(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, double cellularJitter, int octave, double lacunarity = 2.f, double persistence = 0.5f, double SmoothMin = 0.5, double SmoothMax = 1);


    // Fractal/Fractional Brownian Motion (fBm) Ridged noise summation
    double FractalRidge(const DVector& position, int octave, double lacunarity = 2.f, double persistence = 0.5f) {
        return FractalRidge(position, ENoiseType::Simplex, ECellularDistanceType::Euclidean, ECellularType::Distance, 1, octave, lacunarity, persistence);
    }
    double FractalRidge(const DVector& position, ENoiseType NoiseType, ECellularDistanceType CellularDistanceType, ECellularType CellularType, double cellularJitter, int octave, double lacunarity = 2.f, double persistence = 0.5f);


    double CavityShape(double distance);
    double RimShape(double distance, double rimSteepness, int power);
    double CraterShape(double distance, double rimSteepness, double floorHeight, double smoothness, int rimPower);
    double GetCrater(CustomNoise NoiseClass, const DVector& position, int octave, double frequncy, double warpIntensity, double CraterSize, double CraterRimHeight, double CraterRimOctaveLimit, int CraterRimPower, double CraterHeightMultiplier, int SizeGeneration);
    double GetCraterFractal(CustomNoise NoiseClass, const DVector& position, int octave, double lacunarity, double persistence, double frequncy, double warpIntensity, double CraterSize, double CraterRimHeight, double CraterRimOctaveLimit, int CraterRimPower, double CraterHeightMultiplier, bool Min = false);





    //SetSeed
    void SetSeed(int seed);

    static float grad(int hash, float x, float y, float z);
    static double grad(int hash, double x, double y, double z);
    FVector RandomFVector(int hash);
    DVector RandomDVector(int hash);
    CustomNoise(int seed = 0);

private:
    int Seed;
    uint8 perm[256];
    uint8 PermutationHash(int i);
};