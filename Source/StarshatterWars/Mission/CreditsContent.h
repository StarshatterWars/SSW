#pragma once
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace CreditsContent
{
    inline FString Load()
    {
        FString Text;
        for(const FString& Path:{FPaths::ProjectContentDir()/TEXT("Data/credits.txt"),FPaths::ProjectDir()/TEXT("Data/credits.txt")})
            if(FFileHelper::LoadFileToString(Text,*Path) && !Text.TrimStartAndEnd().IsEmpty())return Text;
        return TEXT("STARSHATTER WARS\nA Fractal Dev Studios Production\n\nCredits are unavailable.");
    }
}
