#pragma once
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "MissionUIStyle.h"
#include "Engine/Texture2D.h"

namespace AwardPresentation
{
    inline void Build(UWidgetTree* Tree,UVerticalBox* Box,UTextBlock*& Name,UTextBlock*& Info,UImage*& Insignia,UButton*& Close)
    {
        Box->ClearChildren();
        auto Label=[&](const TCHAR* Text,int32 Size,bool Header){auto* W=Tree->ConstructWidget<UTextBlock>();W->SetText(FText::FromString(Text));W->SetFont(Header?MissionUIStyle::GetHeaderFont(Size):MissionUIStyle::GetLimerickFont(Size));W->SetColorAndOpacity(FSlateColor(MissionUIStyle::InfoValueText));W->SetAutoWrapText(true);return W;};
        auto Panel=[&](UWidget* Content){auto* W=Tree->ConstructWidget<UBorder>();W->SetBrushColor(MissionUIStyle::PanelBG);W->SetPadding(FMargin(24,20));W->SetContent(Content);return W;};
        auto* Header=Tree->ConstructWidget<UHorizontalBox>();
        Header->AddChildToHorizontalBox(Label(TEXT("CONGRATULATIONS"),28,true))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Close=Tree->ConstructWidget<UButton>();MissionUIStyle::ApplyActionButtonStyle(Close,TEXT("Green"));Close->SetToolTipText(FText::FromString(TEXT("Accept award and return")));
        auto* Icon=Tree->ConstructWidget<UImage>();MissionUIStyle::SetActionIcon(Icon,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Check_256x256.T_Check_256x256"),FLinearColor::White);
        if(auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Check_256x256.T_Check_256x256")))Icon->SetBrushFromTexture(Texture);
        FSlateBrush IconBrush=Icon->GetBrush();IconBrush.DrawAs=ESlateBrushDrawType::Image;IconBrush.ImageSize=FVector2D(40,40);Icon->SetBrush(IconBrush);
        auto* IconBox=Tree->ConstructWidget<USizeBox>();IconBox->SetWidthOverride(48);IconBox->SetHeightOverride(48);IconBox->SetContent(Icon);Close->SetContent(IconBox);
        if(auto* Slot=Cast<UButtonSlot>(IconBox->Slot)){Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(8));}
        Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);
        Box->AddChildToVerticalBox(Panel(Header))->SetPadding(FMargin(0,0,0,16));
        auto* Content=Tree->ConstructWidget<UVerticalBox>();
        Name=Label(TEXT(""),30,true);Name->SetJustification(ETextJustify::Center);Content->AddChildToVerticalBox(Name)->SetPadding(FMargin(0,16));
        Insignia=Tree->ConstructWidget<UImage>();auto* ImageBox=Tree->ConstructWidget<USizeBox>();ImageBox->SetWidthOverride(256);ImageBox->SetHeightOverride(256);ImageBox->SetContent(Insignia);
        Content->AddChildToVerticalBox(ImageBox)->SetHorizontalAlignment(HAlign_Center);
        Info=Label(TEXT(""),18,false);Info->SetJustification(ETextJustify::Center);Content->AddChildToVerticalBox(Info)->SetPadding(FMargin(32,24));
        Box->AddChildToVerticalBox(Panel(Content))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
}
