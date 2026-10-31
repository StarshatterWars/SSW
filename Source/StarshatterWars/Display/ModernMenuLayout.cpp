#include "ModernMenuLayout.h"
#include "MissionUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

ModernMenuLayout::FShell ModernMenuLayout::Build(UWidgetTree* Tree,const FString& Title,bool bTransparentBody)
{
    FShell Result;
    auto* Scale=Tree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);
    auto* Frame=Tree->ConstructWidget<USizeBox>();Frame->SetWidthOverride(1920);Frame->SetHeightOverride(1080);Scale->SetContent(Frame);
    auto* Background=Tree->ConstructWidget<UBorder>();Background->SetPadding(FMargin(18));
    Background->SetBrushColor(FLinearColor(0.025f,0.055f,0.083f,bTransparentBody?0.f:1.f));Frame->SetContent(Background);
    auto* Column=Tree->ConstructWidget<UVerticalBox>();Background->SetContent(Column);
    auto* Band=Tree->ConstructWidget<UBorder>();Band->SetBrushColor(FLinearColor(0.105f,0.113f,0.125f,1));Band->SetPadding(FMargin(18,9));
    auto* Heading=Tree->ConstructWidget<UTextBlock>();Heading->SetText(FText::FromString(Title));Heading->SetFont(MissionUIStyle::GetHeaderFont(38));Heading->SetColorAndOpacity(FLinearColor::White);Band->SetContent(Heading);Column->AddChildToVerticalBox(Band);
    auto* Rule=Tree->ConstructWidget<UBorder>();Rule->SetBrushColor(FLinearColor(0.16f,0.77f,0.91f,1));
    auto* RuleHeight=Tree->ConstructWidget<USizeBox>();RuleHeight->SetHeightOverride(5);RuleHeight->SetContent(Rule);Column->AddChildToVerticalBox(RuleHeight);
    Result.Actions=Tree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Result.Actions)->SetPadding(FMargin(0,12));
    Result.Body=Tree->ConstructWidget<UVerticalBox>();Column->AddChildToVerticalBox(Result.Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Result.Root=Scale;return Result;
}

UButton* ModernMenuLayout::AddButton(UWidgetTree* Tree,UHorizontalBox* Strip,UButton* Existing,const FString& Caption)
{
    auto* Button=Existing?Existing:Tree->ConstructWidget<UButton>();Button->RemoveFromParent();
    Button->SetVisibility(ESlateVisibility::Visible);Button->SetRenderOpacity(1.f);
    auto* Text=Tree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Caption));Text->SetJustification(ETextJustify::Center);Button->SetContent(Text);
    MissionUIStyle::ApplyActionButtonStyle(Button,TEXT("Blue"));Text->SetFont(MissionUIStyle::GetHeaderFont(22));
    auto* Cell=Strip->AddChildToHorizontalBox(Button);Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Cell->SetPadding(FMargin(5,0));Cell->SetVerticalAlignment(VAlign_Fill);
    return Button;
}

UButton* ModernMenuLayout::AddArrow(UWidgetTree* Tree,UHorizontalBox* Strip,UButton* Existing,bool bAccept)
{
    auto* Button=Existing?Existing:Tree->ConstructWidget<UButton>();Button->RemoveFromParent();Button->SetVisibility(ESlateVisibility::Visible);
    MissionUIStyle::ApplyActionButtonStyle(Button,bAccept?TEXT("Green"):TEXT("Red"));
    auto* Icon=Tree->ConstructWidget<UImage>();
    MissionUIStyle::SetActionIcon(Icon,bAccept?TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"):TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_L_256x256.T_Arrow_L_256x256"),bAccept?FLinearColor(0.1f,1,0.2f,1):FLinearColor(1,0.12f,0.08f,1));
    auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(40);Size->SetHeightOverride(40);Size->SetContent(Icon);Button->SetContent(Size);
    Button->SetToolTipText(FText::FromString(bAccept?TEXT("Save profile and return"):TEXT("Back")));
    Strip->AddChildToHorizontalBox(Button)->SetPadding(FMargin(6,0));return Button;
}

void ModernMenuLayout::SetSelected(UButton* Button,bool bSelected)
{
    if(!Button) return;
    MissionUIStyle::ApplyActionButtonStyle(Button,TEXT("Blue"));
    if(bSelected){auto Style=Button->GetStyle();Style.Normal=Style.Hovered;Button->SetStyle(Style);}
    if(auto* Text=Cast<UTextBlock>(Button->GetContent())) Text->SetFont(MissionUIStyle::GetHeaderFont(22));
}
