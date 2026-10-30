#pragma once
#include "CoreMinimal.h"
#include "Campaign.h"
#include "StarSystem.h"

namespace CampaignSystemSelectionRules
{
    inline TArray<FString> GetSystemNames(const Campaign* ActiveCampaign)
    {
        TArray<FString> Names;
        if (!ActiveCampaign) return Names;
        const List<StarSystem>& Systems = ActiveCampaign->GetSystemList();
        for (int Index = 0; Index < Systems.size(); ++Index)
            if (const StarSystem* System = Systems[Index])
            {
                const FString Name = FString(UTF8_TO_TCHAR(System->GetName())).TrimStartAndEnd();
                if (!Name.IsEmpty()) Names.AddUnique(Name);
            }
        return Names;
    }

    inline bool CanSelect(const Campaign* ActiveCampaign, const FString& Name)
    {
        const FString CleanName = Name.TrimStartAndEnd();
        if (CleanName.IsEmpty()) return false;
        for (const FString& Candidate : GetSystemNames(ActiveCampaign))
            if (Candidate.Equals(CleanName, ESearchCase::IgnoreCase)) return true;
        return false;
    }
}
