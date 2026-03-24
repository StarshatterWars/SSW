// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "SystemMarker.h"
#include "Components/Border.h"

void USystemMarker::Init(const FS_Galaxy& System)
{    
    UE_LOG(LogTemp, Log, TEXT("USystemMarker::Init() Creating Widget: %s"), *System.Name);

    SystemData = System;

    SetToolTipText(FText::FromString(System.Name));

    // Optional: Add border color by faction
    FLinearColor EmpireColor;
    SystemName = System.Name;

    switch (System.Empire) {
    case EEMPIRE_NAME::Terellian: EmpireColor = FLinearColor::Green; break;
    case EEMPIRE_NAME::Marakan: EmpireColor = FLinearColor::Red; break;
    default: EmpireColor = FLinearColor::Gray; break;
    }

    if (SystemNameText) {
        SystemNameText->SetText(FText::FromString(System.Name));
        SystemNameText->SetColorAndOpacity(EmpireColor);
    }

    FString ProjectPath = FPaths::ProjectContentDir();
    ProjectPath.Append(TEXT("GameData/Galaxy/StarIcons/"));
    
    switch (System.Stellar[0].Class) {
        case ESPECTRAL_CLASS::A:
            ProjectPath.Append(TEXT("StarA_map.png"));
            break;
        case ESPECTRAL_CLASS::B:
            ProjectPath.Append(TEXT("StarB_map.png"));
            break;
        case ESPECTRAL_CLASS::F:
            ProjectPath.Append(TEXT("StarF_map.png"));
            break;
        case ESPECTRAL_CLASS::G:
            ProjectPath.Append(TEXT("StarG_map.png"));
            break;
        case ESPECTRAL_CLASS::K:
            ProjectPath.Append(TEXT("StarK_map.png"));
            break;
        case ESPECTRAL_CLASS::M:
            ProjectPath.Append(TEXT("StarM_map.png"));
            break;
        case ESPECTRAL_CLASS::O:
            ProjectPath.Append(TEXT("StarO_map.png"));
            break;
        default:
            ProjectPath.Append(TEXT("StarG_map.png"));
            break;
    }

    UTexture2D* LoadedTexture = LoadTextureFromFile(ProjectPath);
    if (LoadedTexture)
    {
        UE_LOG(LogTemp, Log, TEXT("USystemMarker::Init() Creating Image: %s"), *ProjectPath);
        FSlateBrush Brush = CreateBrushFromTexture(LoadedTexture, FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
        
        if(!StarImage) {
            UE_LOG(LogTemp, Log, TEXT("USystemMarker::Init() StarImage notxFound")); 
        } else {
            StarImage->SetBrush(Brush);
        }

        SetSelected(false);
    }

    // Optional: Add border color by faction

    switch (System.Iff) {
    case 0: Tint = FLinearColor::Gray; break;
    case 1: Tint = FLinearColor::Green; break;
    case 2: Tint = FLinearColor::Red; break;
    default: Tint = FLinearColor::Gray; break;
    }

    if (IffImage) {
        IffImage->SetColorAndOpacity(Tint);
    }
}

UTexture2D* USystemMarker::LoadTextureFromFile(FString Path)
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    UTexture2D* LoadedTexture = SSWInstance->LoadPNGTextureFromFile(Path);
    return LoadedTexture;
}

FSlateBrush USystemMarker::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize)
{
    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = ImageSize;
    Brush.DrawAs = ESlateBrushDrawType::Image;
    return Brush;
}

void USystemMarker::NativeConstruct()
{
    Super::NativeConstruct();
    //UE_LOG(LogTemp, Log, TEXT("StarImage is %s"), StarImage ? TEXT("Valid") : TEXT("NULL"));
}

void USystemMarker::SetSelected(bool bIsSelected)
{
    USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
    
    SSWInstance->SelectedSystem = SystemName;

    if (HighlightBorder)
    {
        HighlightBorder->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    if (IffImage)
    {
        if(bIsSelected) {     
            IffImage->SetColorAndOpacity(FLinearColor::Yellow);
        }
        else {
            IffImage->SetColorAndOpacity(Tint);
        }
     }

    if (bIsSelected)
    {
        PlayGlow();   // Blueprint event
    }
    else
    {
        StopGlow();   // Blueprint event
    }
}

FReply USystemMarker::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OnClicked.ExecuteIfBound(SystemData.Name);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

