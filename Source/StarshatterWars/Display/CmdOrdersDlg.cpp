#include "CmdOrdersDlg.h"

#include "CmdDlg.h"
#include "GameStructs.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    const FLinearColor PanelColor(0.075f, 0.101f, 0.145f, 1.0f);
    const FLinearColor AccentColor(0.29f, 0.65f, 0.86f, 1.0f);
    const FLinearColor TextColor(0.94f, 0.96f, 0.98f, 1.0f);
    const FLinearColor MutedColor(0.67f, 0.75f, 0.83f, 1.0f);

    UTextBlock* MakeText(UWidgetTree* Tree, const FName Name, const FString& InitialText,
                         const FLinearColor& Color)
    {
        UTextBlock* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Result->SetText(FText::FromString(InitialText));
        Result->SetColorAndOpacity(FSlateColor(Color));
        Result->SetAutoWrapText(true);
        return Result;
    }

    void AddWithPadding(UVerticalBox* Parent, UWidget* Child, float BottomPadding = 12.0f)
    {
        UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child);
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
    }

    UVerticalBox* AddSection(UWidgetTree* Tree, UVerticalBox* Parent,
                              const FName BorderName, const FString& Heading)
    {
        UBorder* Card = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
        Card->SetBrushColor(PanelColor);
        Card->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 14.0f));

        UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);
        AddWithPadding(Content, MakeText(Tree, NAME_None, Heading, AccentColor), 9.0f);
        AddWithPadding(Parent, Card, 12.0f);
        return Content;
    }
}

UCmdOrdersDlg::UCmdOrdersDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdOrdersDlg::NativeConstruct()
{
    Super::NativeConstruct();
    BuildModernOrdersLayout();
    SetCampaignOrders();
}

void UCmdOrdersDlg::BuildModernOrdersLayout()
{
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdOrdersDlg] Cannot build layout: missing WidgetTree"));
        return;
    }

    // The Blueprint's CanvasRoot is the live root. Do NOT replace RootWidget
    // during NativeConstruct: Slate can retain the original empty root.
    UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!CanvasRoot)
    {
        CanvasRoot = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasRoot")));
    }

    if (!CanvasRoot)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdOrdersDlg] Expected WBP_CmdOrdersDlg root CanvasPanel named CanvasRoot; actual=%s"),
            *GetNameSafe(WidgetTree->RootWidget));
        return;
    }

    // NativeConstruct can run again if the widget is removed/readded.
    // Rebuild once per widget tree and avoid duplicated visual elements.
    if (WidgetTree->FindWidget(TEXT("Orders_ModernRoot")))
    {
        return;
    }

    // Full-panel background, sourced from the project UI texture rather than a
    // solid color. Keep it behind the scrollable content in the Blueprint canvas.
    UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Orders_PanelBackground"));
    if (UTexture2D* PanelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
    {
        BackgroundImage->SetBrushFromTexture(PanelTexture, true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdOrdersDlg] Background texture missing: /Game/UI/Panel.Panel"));
    }
    BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* BackgroundSlot = CanvasRoot->AddChildToCanvas(BackgroundImage);
    BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    BackgroundSlot->SetOffsets(FMargin(0.f));
    BackgroundSlot->SetAlignment(FVector2D::ZeroVector);
    BackgroundSlot->SetZOrder(-1);

    UScrollBox* Root = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Orders_ModernRoot"));
    UCanvasPanelSlot* RootSlot = CanvasRoot->AddChildToCanvas(Root);
    if (!RootSlot)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdOrdersDlg] Failed to attach modern Orders to CanvasRoot"));
        return;
    }
    RootSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    RootSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
    RootSlot->SetAlignment(FVector2D(0.f, 0.f));
    RootSlot->SetZOrder(0);
    Root->SetScrollBarVisibility(ESlateVisibility::Visible);

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    // Keep the background texture visible behind the Orders content.
    Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
    Backdrop->SetPadding(FMargin(24.0f, 20.0f, 24.0f, 28.0f));
    Root->AddChild(Backdrop);

    UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>();
    Backdrop->SetContent(Page);

    AddWithPadding(Page, MakeText(WidgetTree, NAME_None, TEXT("OPERATIONS  /  COMMAND ORDERS"), AccentColor), 8.0f);
    CampaignNameText = MakeText(WidgetTree, TEXT("Orders_CampaignName"), TEXT("NO CAMPAIGN SELECTED"), TextColor);
    AddWithPadding(Page, CampaignNameText, 12.0f);

    UVerticalBox* Location = AddSection(WidgetTree, Page, TEXT("Orders_LocationCard"), TEXT("THEATER OF OPERATIONS"));
    LocationSystemText = MakeText(WidgetTree, TEXT("Orders_LocationSystem"), FString(), TextColor);
    AddWithPadding(Location, LocationSystemText, 0.0f);

    UVerticalBox* Summary = AddSection(WidgetTree, Page, TEXT("Orders_DescriptionCard"), TEXT("CAMPAIGN BRIEFING"));
    DescriptionText = MakeText(WidgetTree, TEXT("Orders_Description"), FString(), TextColor);
    AddWithPadding(Summary, DescriptionText, 0.0f);

    UVerticalBox* Situation = AddSection(WidgetTree, Page, TEXT("Orders_SituationCard"), TEXT("CURRENT SITUATION"));
    SituationText = MakeText(WidgetTree, TEXT("Orders_Situation"), FString(), TextColor);
    AddWithPadding(Situation, SituationText, 0.0f);

    UVerticalBox* Orders = AddSection(WidgetTree, Page, TEXT("Orders_DirectivesCard"), TEXT("COMMAND DIRECTIVES"));
    UTextBlock** OrderFields[] = { &Orders1Text, &Orders2Text, &Orders3Text, &Orders4Text };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        AddWithPadding(Orders, MakeText(WidgetTree, NAME_None,
            FString::Printf(TEXT("DIRECTIVE %02d"), Index + 1), MutedColor), 4.0f);
        *OrderFields[Index] = MakeText(WidgetTree,
            FName(*FString::Printf(TEXT("Orders_Directive_%d"), Index + 1)), FString(), TextColor);
        AddWithPadding(Orders, *OrderFields[Index], Index == 3 ? 0.0f : 12.0f);
    }

    UE_LOG(LogTemp, Log, TEXT("[CmdOrdersDlg] Modern native UMG orders layout attached to CanvasRoot"));
}

void UCmdOrdersDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
    SetCampaignOrders();
}

void UCmdOrdersDlg::ShowOrdersDlg()
{
    SetCampaignOrders();
    SetVisibility(ESlateVisibility::Visible);
}

void UCmdOrdersDlg::SetCampaignOrders()
{
    // Avoid stale text when parent campaign changes or is not yet available.
    if (!ParentCmdDlg || !ParentCmdDlg->HasCurrentCampaign())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdOrdersDlg] No parent campaign data"));
        if (CampaignNameText) CampaignNameText->SetText(FText::FromString(TEXT("NO CAMPAIGN SELECTED")));
        if (DescriptionText) DescriptionText->SetText(FText::GetEmpty());
        if (SituationText) SituationText->SetText(FText::GetEmpty());
        if (Orders1Text) Orders1Text->SetText(FText::GetEmpty());
        if (Orders2Text) Orders2Text->SetText(FText::GetEmpty());
        if (Orders3Text) Orders3Text->SetText(FText::GetEmpty());
        if (Orders4Text) Orders4Text->SetText(FText::GetEmpty());
        if (LocationSystemText) LocationSystemText->SetText(FText::GetEmpty());
        return;
    }

    const FS_Campaign& CampaignData = ParentCmdDlg->GetCurrentCampaignData();
    TArray<FString> Directives = CampaignData.Orders;
    Directives.SetNum(4);

    if (CampaignNameText) CampaignNameText->SetText(FText::FromString(CampaignData.Name));
    if (DescriptionText) DescriptionText->SetText(FText::FromString(CampaignData.Description));
    if (SituationText) SituationText->SetText(FText::FromString(CampaignData.Situation));
    if (Orders1Text) Orders1Text->SetText(FText::FromString(Directives[0]));
    if (Orders2Text) Orders2Text->SetText(FText::FromString(Directives[1]));
    if (Orders3Text) Orders3Text->SetText(FText::FromString(Directives[2]));
    if (Orders4Text) Orders4Text->SetText(FText::FromString(Directives[3]));
    if (LocationSystemText)
    {
        LocationSystemText->SetText(FText::FromString(CampaignData.System + TEXT(" / ") + CampaignData.Region));
    }
}
