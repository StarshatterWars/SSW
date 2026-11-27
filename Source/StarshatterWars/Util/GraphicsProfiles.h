#pragma once
#include "CoreMinimal.h"
#include <array>
namespace SSWGraphicsProfiles
{
    inline constexpr int32 Count=5;
    struct FProfile
    {
        const TCHAR* Label;
        std::array<int32,9> Quality;
        float RenderPercent, FrameLimit, ParticleScale;
    };
    const FProfile& Get(int32 Index);
    FIntPoint OutputSize(FIntPoint Requested);
    void ApplyVisualBudget(int32 Index);
    void ApplyToRuntime(int32 Index);
    void ApplyVisualBudget(const FProfile& Profile);
}
