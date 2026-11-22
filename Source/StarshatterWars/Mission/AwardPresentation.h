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
#include "Components/ScaleBox.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "MissionUIStyle.h"
#include "Engine/Texture2D.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/NativeWidgetHost.h"
#include "FlightPanelStyle.h"
#include "PanelCloseIcon.h"
#include "PanelCloseGlyph.h"

namespace AwardPresentation
{
    inline void Build(UWidgetTree* Tree,UVerticalBox* Box,UTextBlock*& Name,UTextBlock*& Info,UImage*& Insignia,UButton*& Close)
    {
        Box->ClearChildren();
        auto Label=[&](const TCHAR* Text,int32 Size,bool Header){auto* W=Tree->ConstructWidget<UTextBlock>();W->SetText(FText::FromString(Text));W->SetFont(Header?MissionUIStyle::GetHeaderFont(Size):MissionUIStyle::GetLimerickFont(Size));W->SetColorAndOpacity(FSlateColor(MissionUIStyle::InfoValueText));W->SetAutoWrapText(true);return W;};
        auto Panel=[&](UWidget* Content){auto* W=Tree->ConstructWidget<UBorder>();W->SetBrush(FlightPanelStyle::Window());W->SetBrushColor(FLinearColor::White);W->SetPadding(FMargin(24,20));W->SetContent(Content);return W;};
        auto* Header=Tree->ConstructWidget<UHorizontalBox>();
        Header->AddChildToHorizontalBox(Label(TEXT("AWARDS AND PROMOTION"),16,true))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Close=Tree->ConstructWidget<UButton>(); MissionUIStyle::ApplyActionButtonStyle(Close,TEXT("Red"));
        Close->SetToolTipText(INVTEXT("Close and return to the live session"));
        auto* Icon=Tree->ConstructWidget<UPanelCloseGlyph>();
        Icon->SetContent(SNew(SPanelCloseIcon));
        auto* IconBox=Tree->ConstructWidget<USizeBox>(); IconBox->SetWidthOverride(28); IconBox->SetHeightOverride(28);
        IconBox->SetContent(Icon); Close->SetContent(IconBox);
        if(auto* Slot=Cast<UButtonSlot>(IconBox->Slot)) Slot->SetPadding(FMargin(2));
        Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);
        auto* Title=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("AwardTitleBar"));
        Title->SetBrush(FlightPanelStyle::TitleBar()); Title->SetBrushColor(FLinearColor::White);
        Title->SetPadding(FMargin(10,6)); Title->SetContent(Header);
        Box->AddChildToVerticalBox(Title)->SetPadding(FMargin(0,0,0,10));
        auto* Content=Tree->ConstructWidget<UVerticalBox>();
        Name=Label(TEXT(""),20,true);Name->SetJustification(ETextJustify::Center);Content->AddChildToVerticalBox(Name)->SetPadding(FMargin(0,16));
        Insignia=Tree->ConstructWidget<UImage>();auto* ImageBox=Tree->ConstructWidget<USizeBox>();ImageBox->SetWidthOverride(256);ImageBox->SetHeightOverride(256);auto* Fit=Tree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetContent(Insignia);ImageBox->SetContent(Fit);
        Content->AddChildToVerticalBox(ImageBox)->SetHorizontalAlignment(HAlign_Center);
        Info=Label(TEXT(""),13,false);Info->SetJustification(ETextJustify::Center);Content->AddChildToVerticalBox(Info)->SetPadding(FMargin(32,24));
        Box->AddChildToVerticalBox(Panel(Content))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    inline UWidget* Mount(UWidgetTree* Tree, UCanvasPanel* Canvas, UVerticalBox* Content)
    {
        auto* Frame=Tree->ConstructWidget<UBorder>(); Frame->SetBrush(FlightPanelStyle::Window());
        Frame->SetBrushColor(FLinearColor::White); Frame->SetPadding(FMargin(12)); Frame->SetContent(Content);
        auto* Size=Tree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(680); Size->SetHeightOverride(560); Size->SetContent(Frame);
        auto* Fit=Tree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::DownOnly); Fit->SetContent(Size);
        auto* Slot=Canvas->AddChildToCanvas(Fit); Slot->SetAnchors(FAnchors(.5f)); Slot->SetAlignment(FVector2D(.5));
        Slot->SetPosition(FVector2D::ZeroVector); Slot->SetSize(FVector2D(680,560));
        return Fit;
    }
    inline void Resize(UCanvasPanel* Canvas,UWidget* Frame)
    {
        if(!Canvas || !Frame)return;
        const FVector2D View=Canvas->GetCachedGeometry().GetLocalSize();
        if(View.X>48 && View.Y>48)if(auto* Slot=Cast<UCanvasPanelSlot>(Frame->Slot))
            Slot->SetSize(FVector2D(FMath::Min(680.0,View.X-48),FMath::Min(560.0,View.Y-48)));
    }
    inline void Drag(UCanvasPanel* Canvas,UWidget* Frame,const FPointerEvent& Event)
    {
        if(!Canvas || !Frame)return;
        auto* Slot=Cast<UCanvasPanelSlot>(Frame->Slot);if(!Slot)return;
        const auto& Geometry=Canvas->GetCachedGeometry();
        const FVector2D Available=FVector2D(Geometry.GetLocalSize())-Slot->GetSize();
        const FVector2D Limit(FMath::Max(0.0,Available.X)*.5,FMath::Max(0.0,Available.Y)*.5);
        FVector2D Position=Slot->GetPosition()+Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition())-Geometry.AbsoluteToLocal(Event.GetLastScreenSpacePosition());
        Position.X=FMath::Clamp(Position.X,-Limit.X,Limit.X);Position.Y=FMath::Clamp(Position.Y,-Limit.Y,Limit.Y);
        Slot->SetPosition(Position);
    }

}
