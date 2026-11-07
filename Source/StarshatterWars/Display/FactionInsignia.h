#pragma once
#include "Components/Image.h"
#include "Engine/Texture2D.h"
namespace FactionInsignia
{
inline FString Path(const FString& Empire)
{
    if(Empire.Contains(TEXT("Marak"),ESearchCase::IgnoreCase)) return TEXT("/Game/UI/Factions/T_MarakanLogo.T_MarakanLogo");
    if(Empire.Contains(TEXT("Dant"),ESearchCase::IgnoreCase)) return TEXT("/Game/UI/Factions/T_DantariLogo.T_DantariLogo");
    if(Empire.Contains(TEXT("Terell"),ESearchCase::IgnoreCase) || Empire.Contains(TEXT("Alliance"),ESearchCase::IgnoreCase)) return TEXT("/Game/UI/Campaigns/03/Alliance.Alliance");
    return FString();
}
inline void Set(UImage* Image,const FString& Empire,float Size=48.f)
{
    if(!Image)return;
    const FString Asset=Path(Empire);
    if(Asset.IsEmpty()){Image->SetVisibility(ESlateVisibility::Hidden);return;}
    auto* Current=Image->GetBrush().GetResourceObject();
    if(!Current || Current->GetPathName()!=Asset)
        if(auto* Texture=LoadObject<UTexture2D>(nullptr,*Asset))Image->SetBrushFromTexture(Texture);
    Image->SetDesiredSizeOverride(FVector2D(Size));
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
}
}
