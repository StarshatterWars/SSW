#pragma once
#include "GameStructs_products.h"

// A deterministic quote shared by automated industry and production UI.
class ReprocessingAI
{
public:
    struct Quote
    {
        bool bValid = false;
        double InputTonnes = 0;
        double OutputTonnes = 0;
        double OutputQuantity = 0;
        double WasteTonnes = 0;
        double Seconds = 0;
    };
    static Quote Calculate(const FSSWProductionRecipe& Recipe, int32 Runs = 1,
        double MaterialEfficiency = 0, double TimeEfficiency = 0, double Recovery = 1);
};
