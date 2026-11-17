#include "ReprocessingAI.h"
ReprocessingAI::Quote ReprocessingAI::Calculate(const FSSWProductionRecipe& Recipe, int32 Runs, double ME, double TE, double Recovery)
{
    Quote Result;
    if (Runs <= 0 || Runs > 1000000 || !FMath::IsFinite(ME) || ME < 0 || ME > 0.1 ||
        !FMath::IsFinite(TE) || TE < 0 || TE > 0.2 || !FMath::IsFinite(Recovery) || Recovery <= 0 || Recovery > 1 ||
        !Recipe.CanStart(Recipe.InputTonnes, Recipe.OutputTonnes) ||
        !FMath::IsFinite(Recipe.InputTonnes) || !FMath::IsFinite(Recipe.OutputTonnes) || !FMath::IsFinite(Recipe.SecondsPerBatch) ||
        !FMath::IsFinite(Recipe.OutputQuantity) || Recipe.OutputQuantity < 0) return Result;
    Result.InputTonnes = Recipe.InputTonnes * Runs * (1 - ME);
    Result.OutputTonnes = Recipe.OutputTonnes * Runs * Recovery;
    Result.OutputQuantity = Recipe.OutputQuantity * Runs * Recovery;
    Result.Seconds = Recipe.SecondsPerBatch * Runs * (1 - TE);
    Result.WasteTonnes = Result.InputTonnes - Result.OutputTonnes;
    Result.bValid = Result.WasteTonnes >= 0 && FMath::IsFinite(Result.Seconds) && Result.Seconds > 0;
    return Result;
}
