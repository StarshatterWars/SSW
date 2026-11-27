#include "GraphicsProfiles.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
namespace SSWGraphicsProfiles
{
const FProfile& Get(int32 Index)
{
    // Immutable numeric policy only; no command-line or UObject access at static init.
    static const FProfile Profiles[] = {
        {TEXT("INTEGRATED GRAPHICS"),{1,2,0,2,0,0,0,0,0},75,30,.45f},
        {TEXT("LOW"),{1,2,1,2,1,1,0,0,0},90,60,.65f},
        {TEXT("MEDIUM"),{2,2,2,2,2,2,1,1,1},100,60,.8f},
        {TEXT("HIGH"),{3,3,3,3,3,3,3,3,2},100,0,1},
        {TEXT("ULTRA"),{3,3,3,3,3,3,3,3,3},100,0,1}
    };
    return Profiles[FMath::Clamp(Index,0,Count-1)];
}
FIntPoint OutputSize(FIntPoint Requested)
{
    return FIntPoint(FMath::Max(1920,Requested.X),FMath::Max(1080,Requested.Y));
}
void ApplyToRuntime(int32 Index)
{
    const auto& Profile=Get(Index);
    if(auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr)
    {
        Settings->SetViewDistanceQuality(Profile.Quality[0]);Settings->SetAntiAliasingQuality(Profile.Quality[1]);
        Settings->SetShadowQuality(Profile.Quality[2]);Settings->SetTextureQuality(Profile.Quality[3]);
        Settings->SetVisualEffectQuality(Profile.Quality[4]);Settings->SetPostProcessingQuality(Profile.Quality[5]);
        Settings->SetGlobalIlluminationQuality(Profile.Quality[6]);Settings->SetReflectionQuality(Profile.Quality[7]);Settings->SetFoliageQuality(Profile.Quality[8]);
        Settings->SetScreenResolution(OutputSize(Settings->GetScreenResolution()));
        Settings->SetResolutionScaleValueEx(Profile.RenderPercent);Settings->SetFrameRateLimit(Profile.FrameLimit);
        Settings->ApplySettings(false);
    }
    if(auto* AA=IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))AA->Set(Index<2?2:4,ECVF_SetByGameSetting);
    ApplyVisualBudget(Profile);
}
void ApplyVisualBudget(int32 Index)
{
    ApplyVisualBudget(Get(Index));
}
void ApplyVisualBudget(const FProfile& Profile)
{
    const auto Set=[](const TCHAR* Name,float Value)
    { if(auto* Variable=IConsoleManager::Get().FindConsoleVariable(Name))Variable->Set(Value,ECVF_SetByGameSetting); };
    Set(TEXT("r.Lumen.DiffuseIndirect.Allow"),Profile.Quality[6]>=2?1.f:0.f);
    Set(TEXT("r.Lumen.Reflections.Allow"),Profile.Quality[7]>=2?1.f:0.f);
    Set(TEXT("fx.Niagara.QualityLevel"),float(Profile.Quality[4]));
    Set(TEXT("fx.NiagaraGlobalSpawnCountScale"),Profile.ParticleScale);
    Set(TEXT("r.ParticleLODBias"),Profile.Quality[4]==0?2.f:Profile.Quality[4]==1?1.f:0.f);
    Set(TEXT("r.MaterialQualityLevel"),Profile.Quality[4]<2?0.f:1.f);
    // These affect presentation only; no simulation, OOB, collision or sensor pruning.
}
}
