/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSelectDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Code-built campaign selection screen with a custom styled
    campaign dropdown list.
*/

#include "CampaignSelectDlg.h"

#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Campaign.h"
#include "CampaignSave.h"
#include "CmpLoadDlg.h"
#include "Game.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Mouse.h"
#include "SSWGameInstance.h"
#include "Starshatter.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "TimerSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
    static int32 GCampaignSelectUniqueNameCounter = 0;

    static FName MakeUniqueWidgetName(const TCHAR* BaseName)
    {
        return FName(*FString::Printf(TEXT("%s_%d"), BaseName, ++GCampaignSelectUniqueNameCounter));
    }
}

UCampaignSelectDlg::UCampaignSelectDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCampaignSelectDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UCampaignSelectDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    stars = Starshatter::GetInstance();
    select_msg = Game::GetText("CmpSelectDlg.select_msg");
}

void UCampaignSelectDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildWidgetTreeIfNeeded();
    HookupEvents();
    RegisterControls();
    RefreshUIFromSubsystem();
}

void UCampaignSelectDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UCampaignSelectDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCampaignSelectDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCampaignSelectDlg::BuildWidgetTreeIfNeeded()
{
    if (!WidgetTree)
    {
        return;
    }

    RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);

    if (!RootCanvas)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(),
            TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    if (WidgetTree->FindWidget(TEXT("CampaignSelect_MainBorder")) != nullptr)
    {
        return;
    }

    BackgroundImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(),
        TEXT("CampaignSelect_Background"));

    if (UCanvasPanelSlot* BgCanvasSlot = RootCanvas->AddChildToCanvas(BackgroundImage))
    {
        BgCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        BgCanvasSlot->SetOffsets(FMargin(0.f));
        BgCanvasSlot->SetZOrder(0);
    }

    if (UTexture2D* FrameTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Frame1.Frame1")))
    {
        BackgroundImage->SetBrushFromTexture(FrameTex, true);
    }

    TitleText = CreateText(
        TEXT("CampaignSelect_TitleText"),
        TEXT("DYNAMIC CAMPAIGNS"),
        18,
        MissionUIStyle::HeaderText,
        ETextJustify::Left,
        false);

    if (UCanvasPanelSlot* TitleCanvasSlot = RootCanvas->AddChildToCanvas(TitleText))
    {
        TitleCanvasSlot->SetPosition(FVector2D(12.f, 32.f));
        TitleCanvasSlot->SetAutoSize(true);
        TitleCanvasSlot->SetZOrder(1);
    }

    MainBorder = CreatePanelBorder(
        TEXT("CampaignSelect_MainBorder"),
        MissionUIStyle::PanelBG);

    if (UCanvasPanelSlot* MainCanvasSlot = RootCanvas->AddChildToCanvas(MainBorder))
    {
        MainCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        MainCanvasSlot->SetOffsets(FMargin(36.f, 120.f, 36.f, 32.f));
        MainCanvasSlot->SetZOrder(1);
    }

    UCanvasPanel* MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(),
        TEXT("CampaignSelect_MainCanvas"));
    MainBorder->SetContent(MainCanvas);

    UBorder* NameBorder = CreatePanelBorder(
        TEXT("CampaignSelect_NameBorder"),
        MissionUIStyle::HeaderBG);

    if (UCanvasPanelSlot* NameCanvasSlot = MainCanvas->AddChildToCanvas(NameBorder))
    {
        NameCanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
        NameCanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
        NameCanvasSlot->SetPosition(FVector2D(-14.f, 18.f));
        NameCanvasSlot->SetSize(FVector2D(256.f, 32.f));
    }

    PlayerNameText = CreateText(
        TEXT("CampaignSelect_PlayerNameText"),
        TEXT("PLAYER"),
        16,
        MissionUIStyle::InfoValueText,
        ETextJustify::Right,
        false);

    NameBorder->SetContent(PlayerNameText);

    CampaignNameText = CreateText(
        TEXT("CampaignSelect_CampaignNameText"),
        TEXT("CAMPAIGN"),
        20,
        MissionUIStyle::HeaderText,
        ETextJustify::Center,
        false);

    if (UCanvasPanelSlot* CampaignNameCanvasSlot = MainCanvas->AddChildToCanvas(CampaignNameText))
    {
        CampaignNameCanvasSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
        CampaignNameCanvasSlot->SetAlignment(FVector2D(0.5f, 0.f));
        CampaignNameCanvasSlot->SetPosition(FVector2D(0.f, 52.f));
        CampaignNameCanvasSlot->SetSize(FVector2D(512.f, 32.f));
    }

    BuildCampaignDropdown(MainCanvas);

    UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),
        TEXT("CampaignSelect_MainRow"));

    if (UCanvasPanelSlot* MainRowCanvasSlot = MainCanvas->AddChildToCanvas(MainRow))
    {
        MainRowCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        MainRowCanvasSlot->SetOffsets(FMargin(32.f, 160.f, 32.f, 100.f));
    }

    UBorder* LeftBorder = CreatePanelBorder(
        TEXT("CampaignSelect_LeftBorder"),
        MissionUIStyle::PanelBG);

    UBorder* RightBorder = CreatePanelBorder(
        TEXT("CampaignSelect_RightBorder"),
        MissionUIStyle::PanelBG);

    {
        UHorizontalBoxSlot* LeftHBoxSlot = MainRow->AddChildToHorizontalBox(LeftBorder);
        LeftHBoxSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
        LeftHBoxSlot->SetHorizontalAlignment(HAlign_Left);
        LeftHBoxSlot->SetVerticalAlignment(VAlign_Fill);
        LeftHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    {
        UHorizontalBoxSlot* RightHBoxSlot = MainRow->AddChildToHorizontalBox(RightBorder);
        RightHBoxSlot->SetPadding(FMargin(0.f));
        RightHBoxSlot->SetHorizontalAlignment(HAlign_Fill);
        RightHBoxSlot->SetVerticalAlignment(VAlign_Fill);
        RightHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    USizeBox* LeftSize = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),
        TEXT("CampaignSelect_LeftSize"));
    LeftSize->SetWidthOverride(520.f);
    LeftSize->SetHeightOverride(384.f);
    LeftBorder->SetContent(LeftSize);

    UBorder* PictureInnerBorder = CreatePanelBorder(
        TEXT("CampaignSelect_PictureInnerBorder"),
        MissionUIStyle::HeaderBG);
    LeftSize->SetContent(PictureInnerBorder);

    CampaignImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(),
        TEXT("CampaignSelect_CampaignImage"));

    CampaignImage->SetBrushSize(FVector2D(520.f, 520.f));

    PictureInnerBorder->SetContent(CampaignImage);

    UVerticalBox* RightVBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),
        TEXT("CampaignSelect_RightVBox"));
    RightBorder->SetContent(RightVBox);

    auto AddHeader = [&](const FString& HeaderText, const TCHAR* BaseName) -> void
        {
            UBorder* HeaderBorder = CreatePanelBorder(
                MakeUniqueWidgetName(BaseName),
                MissionUIStyle::HeaderBG);

            UTextBlock* HeaderLabel = CreateText(
                MakeUniqueWidgetName(TEXT("CampaignSelect_HeaderLabel")),
                HeaderText,
                20,
                MissionUIStyle::HeaderText,
                ETextJustify::Left,
                false);

            HeaderBorder->SetContent(HeaderLabel);

            UVerticalBoxSlot* HeaderVBoxSlot = RightVBox->AddChildToVerticalBox(HeaderBorder);
            HeaderVBoxSlot->SetPadding(FMargin(4.f, 2.f, 4.f, 2.f));
            HeaderVBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        };

    auto AddBodyText = [&](TObjectPtr<UTextBlock>& OutText, float Height, const TCHAR* SizeBoxBase, const TCHAR* TextBase, bool bWrap = true) -> void
        {
            USizeBox* BodySizeBox = WidgetTree->ConstructWidget<USizeBox>(
                USizeBox::StaticClass(),
                MakeUniqueWidgetName(SizeBoxBase));
            BodySizeBox->SetHeightOverride(Height);

            OutText = CreateText(
                MakeUniqueWidgetName(TextBase),
                TEXT(""),
                16,
                MissionUIStyle::InfoValueText,
                ETextJustify::Left,
                bWrap);

            BodySizeBox->SetContent(OutText);

            UVerticalBoxSlot* BodyVBoxSlot = RightVBox->AddChildToVerticalBox(BodySizeBox);
            BodyVBoxSlot->SetPadding(FMargin(6.f, 0.f, 6.f, 0.f));
            BodyVBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        };

    AddHeader(TEXT("Description"), TEXT("CampaignSelect_DescriptionHeaderBorder"));
    AddBodyText(DescriptionText, 128.f, TEXT("CampaignSelect_DescriptionSizeBox"), TEXT("CampaignSelect_DescriptionText"), true);

    AddHeader(TEXT("Situation"), TEXT("CampaignSelect_SituationHeaderBorder"));
    AddBodyText(SituationText, 192.f, TEXT("CampaignSelect_SituationSizeBox"), TEXT("CampaignSelect_SituationText"), true);

    AddHeader(TEXT("Orders"), TEXT("CampaignSelect_OrdersHeaderBorder"));
    AddBodyText(Orders1Text, 28.f, TEXT("CampaignSelect_Orders1SizeBox"), TEXT("CampaignSelect_Orders1Text"), true);
    AddBodyText(Orders2Text, 28.f, TEXT("CampaignSelect_Orders2SizeBox"), TEXT("CampaignSelect_Orders2Text"), true);
    AddBodyText(Orders3Text, 28.f, TEXT("CampaignSelect_Orders3SizeBox"), TEXT("CampaignSelect_Orders3Text"), true);
    AddBodyText(Orders4Text, 28.f, TEXT("CampaignSelect_Orders4SizeBox"), TEXT("CampaignSelect_Orders4Text"), true);

    UHorizontalBox* MetaHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),
        TEXT("CampaignSelect_MetaHeaderRow"));
    {
        UVerticalBoxSlot* MetaHeaderVBoxSlot = RightVBox->AddChildToVerticalBox(MetaHeaderRow);
        MetaHeaderVBoxSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    }

    UBorder* LocationHeader = CreatePanelBorder(
        TEXT("CampaignSelect_LocationHeader"),
        MissionUIStyle::HeaderBG);
    UTextBlock* LocationLabel = CreateText(
        TEXT("CampaignSelect_LocationLabel"),
        TEXT("Location"),
        20,
        MissionUIStyle::HeaderText,
        ETextJustify::Left,
        false);
    LocationHeader->SetContent(LocationLabel);

    UBorder* StartHeader = CreatePanelBorder(
        TEXT("CampaignSelect_StartHeader"),
        MissionUIStyle::HeaderBG);
    UTextBlock* StartLabel = CreateText(
        TEXT("CampaignSelect_StartLabel"),
        TEXT("Start"),
        20,
        MissionUIStyle::HeaderText,
        ETextJustify::Left,
        false);
    StartHeader->SetContent(StartLabel);

    {
        UHorizontalBoxSlot* LocationHeaderHBoxSlot = MetaHeaderRow->AddChildToHorizontalBox(LocationHeader);
        LocationHeaderHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LocationHeaderHBoxSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
    }

    {
        UHorizontalBoxSlot* StartHeaderHBoxSlot = MetaHeaderRow->AddChildToHorizontalBox(StartHeader);
        StartHeaderHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    UHorizontalBox* MetaValueRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),
        TEXT("CampaignSelect_MetaValueRow"));
    {
        UVerticalBoxSlot* MetaValueVBoxSlot = RightVBox->AddChildToVerticalBox(MetaValueRow);
        MetaValueVBoxSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
    }

    LocationSystemText = CreateText(
        TEXT("CampaignSelect_LocationSystemText"),
        TEXT(""),
        12,
        MissionUIStyle::InfoValueText,
        ETextJustify::Left,
        true);

    CampaignStartTimeText = CreateText(
        TEXT("CampaignSelect_CampaignStartTimeText"),
        TEXT(""),
        12,
        MissionUIStyle::InfoValueText,
        ETextJustify::Left,
        true);

    {
        UHorizontalBoxSlot* LocationValueHBoxSlot = MetaValueRow->AddChildToHorizontalBox(LocationSystemText);
        LocationValueHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LocationValueHBoxSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
    }

    {
        UHorizontalBoxSlot* StartValueHBoxSlot = MetaValueRow->AddChildToHorizontalBox(CampaignStartTimeText);
        StartValueHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),
        TEXT("CampaignSelect_ButtonRow"));

    if (UCanvasPanelSlot* ButtonRowCanvasSlot = MainCanvas->AddChildToCanvas(ButtonRow))
    {
        ButtonRowCanvasSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
        ButtonRowCanvasSlot->SetAlignment(FVector2D(1.f, 1.f));
        ButtonRowCanvasSlot->SetPosition(FVector2D(-100.f, -53.f));
        ButtonRowCanvasSlot->SetAutoSize(true);
    }

    UWidget* PlayButtonWidget = CreateMenuButton(
        TEXT("CampaignSelect_PlayButton"),
        PlayButton,
        PlayButtonText,
        TEXT("START"));

    UWidget* RestartButtonWidget = CreateMenuButton(
        TEXT("CampaignSelect_RestartButton"),
        RestartButton,
        RestartButtonText,
        TEXT("RESTART"));

    UWidget* CancelButtonWidget = CreateMenuButton(
        TEXT("CampaignSelect_CancelButton"),
        CancelButton,
        CancelButtonText,
        TEXT("CANCEL"));

    {
        UHorizontalBoxSlot* PlayButtonHBoxSlot = ButtonRow->AddChildToHorizontalBox(PlayButtonWidget);
        PlayButtonHBoxSlot->SetPadding(FMargin(0.f, 0.f, 36.f, 0.f));
        PlayButtonHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    {
        UHorizontalBoxSlot* RestartButtonHBoxSlot = ButtonRow->AddChildToHorizontalBox(RestartButtonWidget);
        RestartButtonHBoxSlot->SetPadding(FMargin(0.f, 0.f, 36.f, 0.f));
        RestartButtonHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    {
        UHorizontalBoxSlot* CancelButtonHBoxSlot = ButtonRow->AddChildToHorizontalBox(CancelButtonWidget);
        CancelButtonHBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ApplyButtonStyle(PlayButton);
    ApplyButtonStyle(RestartButton);
    ApplyButtonStyle(CancelButton);
}

void UCampaignSelectDlg::BuildCampaignDropdown(UCanvasPanel* MainCanvas)
{
    UBorder* CampaignSelectHeader = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownHeader"));
    CampaignSelectHeader->SetBrushColor(MissionUIStyle::HeaderBG);

    UTextBlock* CampaignSelectHeaderText = CreateText(
        TEXT("CampaignSelect_DropdownHeaderText"),
        TEXT("CAMPAIGN"),
        18,
        MissionUIStyle::HeaderText,
        ETextJustify::Left,
        false);

    CampaignSelectHeader->SetContent(CampaignSelectHeaderText);

    if (UCanvasPanelSlot* DropdownHeaderCanvasSlot = MainCanvas->AddChildToCanvas(CampaignSelectHeader))
    {
        DropdownHeaderCanvasSlot->SetPosition(FVector2D(36.f, 8.f));
        DropdownHeaderCanvasSlot->SetSize(FVector2D(300.f, 28.f));
        DropdownHeaderCanvasSlot->SetZOrder(3);
    }

    UBorder* CampaignDropdownFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownFrame"));
    CampaignDropdownFrame->SetBrushColor(MissionUIStyle::HeaderBG);

    if (UCanvasPanelSlot* FrameCanvasSlot = MainCanvas->AddChildToCanvas(CampaignDropdownFrame))
    {
        FrameCanvasSlot->SetPosition(FVector2D(36.f, 44.f));
        FrameCanvasSlot->SetSize(FVector2D(300.f, 40.f));
        FrameCanvasSlot->SetZOrder(3);
    }

    UBorder* CampaignDropdownInner = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownInner"));
    CampaignDropdownInner->SetBrushColor(MissionUIStyle::PanelBG);
    CampaignDropdownFrame->SetContent(CampaignDropdownInner);

    CampaignDropdownButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(),
        TEXT("CampaignSelect_DropdownButton"));
    CampaignDropdownInner->SetContent(CampaignDropdownButton);

    CampaignDropdownButtonText = CreateText(
        TEXT("CampaignSelect_DropdownButtonText"),
        TEXT("SELECT CAMPAIGN"),
        16,
        MissionUIStyle::ComboMenuBG,   
        ETextJustify::Left,
        false);

    if (UButtonSlot* DropdownButtonSlot = Cast<UButtonSlot>(CampaignDropdownButton->AddChild(CampaignDropdownButtonText)))
    {
        DropdownButtonSlot->SetPadding(FMargin(8.f, 4.f, 8.f, 4.f));
        DropdownButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        DropdownButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    CampaignDropdownPopupBorder = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownPopupBorder"));
    CampaignDropdownPopupBorder->SetBrushColor(MissionUIStyle::PanelBG);
    CampaignDropdownPopupBorder->SetVisibility(ESlateVisibility::Collapsed);

    if (UCanvasPanelSlot* PopupCanvasSlot = MainCanvas->AddChildToCanvas(CampaignDropdownPopupBorder))
    {
        PopupCanvasSlot->SetPosition(FVector2D(36.f, 80.f));
        PopupCanvasSlot->SetSize(FVector2D(300.f, 220.f));
        PopupCanvasSlot->SetZOrder(20);
    }

    CampaignDropdownScrollBox = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(),
        TEXT("CampaignSelect_DropdownScrollBox"));
    CampaignDropdownPopupBorder->SetContent(CampaignDropdownScrollBox);

    CampaignDropdownListBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),
        TEXT("CampaignSelect_DropdownListBox"));
    CampaignDropdownScrollBox->AddChild(CampaignDropdownListBox);
}

void UCampaignSelectDlg::RebuildCampaignDropdownOptions()
{
    if (!CampaignDropdownListBox || !WidgetTree)
    {
        return;
    }

    CampaignDropdownListBox->ClearChildren();
    CampaignOptionButtons.Reset();
    CampaignOptionButtonTexts.Reset();

    const FLinearColor PopupBG = FLinearColor(0.10f, 0.11f, 0.13f, 1.0f);
    const FLinearColor RowBG = FLinearColor(0.18f, 0.18f, 0.20f, 1.0f);
    const FLinearColor RowSelectedBG = FLinearColor(0.32f, 0.42f, 0.58f, 1.0f);
    const FLinearColor RowTextColor = FLinearColor(0.92f, 0.93f, 0.95f, 1.0f);

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetBrushColor(PopupBG);
    }

    for (int32 i = 0; i < CampaignDisplayNamesByOptionIndex.Num(); ++i)
    {
        const bool bSelected = (i == Selected);

        UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionRowBorder")));
        RowBorder->SetBrushColor(bSelected ? RowSelectedBG : RowBG);

        UButton* RowButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionButton")));

        // Keep button visually transparent so the border drives the row color.
        FButtonStyle TransparentButtonStyle = RowButton->GetStyle();
        TransparentButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor::Transparent);
        RowButton->SetStyle(TransparentButtonStyle);

        UTextBlock* RowText = CreateText(
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionText")),
            CampaignDisplayNamesByOptionIndex[i],
            14,
            RowTextColor,
            ETextJustify::Left,
            false);

        if (UButtonSlot* RowButtonSlot = Cast<UButtonSlot>(RowButton->AddChild(RowText)))
        {
            RowButtonSlot->SetPadding(FMargin(12.f, 3.f, 8.f, 3.f));
            RowButtonSlot->SetHorizontalAlignment(HAlign_Fill);
            RowButtonSlot->SetVerticalAlignment(VAlign_Center);
        }

        RowButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCampaignOptionClicked);
        RowButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCampaignOptionClicked);

        RowBorder->SetContent(RowButton);

        if (UVerticalBoxSlot* RowVBoxSlot = CampaignDropdownListBox->AddChildToVerticalBox(RowBorder))
        {
            RowVBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            RowVBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            RowVBoxSlot->SetVerticalAlignment(VAlign_Top);
        }

        CampaignOptionButtons.Add(RowButton);
        CampaignOptionButtonTexts.Add(RowText);
    }
}

void UCampaignSelectDlg::UpdateCampaignDropdownLabel()
{
    if (!CampaignDropdownButtonText)
    {
        return;
    }

    const FString Label =
        CampaignDisplayNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignDisplayNamesByOptionIndex[Selected]
        : TEXT("SELECT CAMPAIGN");

    CampaignDropdownButtonText->SetText(FText::FromString(Label));
}

void UCampaignSelectDlg::HideCampaignDropdown()
{
    bCampaignDropdownOpen = false;

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UCampaignSelectDlg::SelectCampaignOption(int32 NewIndex)
{
    if (!CampaignIndexByOptionIndex.IsValidIndex(NewIndex))
    {
        return;
    }

    Selected = NewIndex;
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(NewIndex)
        ? CampaignRowNamesByOptionIndex[NewIndex]
        : NAME_None;

    UpdateCampaignDropdownLabel();
    HideCampaignDropdown();
    RefreshFromSelection();
    RebuildCampaignDropdownOptions();
}

void UCampaignSelectDlg::HookupEvents()
{
    if (CampaignDropdownButton)
    {
        CampaignDropdownButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCampaignDropdownClicked);
        CampaignDropdownButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCampaignDropdownClicked);
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);
        PlayButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);

        PlayButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);
        PlayButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);

        PlayButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);
        PlayButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);
    }

    if (RestartButton)
    {
        RestartButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);
        RestartButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);

        RestartButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);
        RestartButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);

        RestartButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);
        RestartButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);
        CancelButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);

        CancelButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);
        CancelButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);

        CancelButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);
        CancelButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);
    }
}

void UCampaignSelectDlg::RegisterControls()
{
    PopulateCampaignDropdown();
}

void UCampaignSelectDlg::PopulateCampaignDropdown()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();

    if (!DataSubsystem || !PlayerSS)
    {
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    CampaignRowNamesByOptionIndex.Reset();
    CampaignIndexByOptionIndex.Reset();
    CampaignDisplayNamesByOptionIndex.Reset();

    const TArray<FS_Campaign>& Campaigns = DataSubsystem->GetAllCampaigns();

    for (const FS_Campaign& Row : Campaigns)
    {
        if (!Row.bAvailable)
        {
            continue;
        }

        CampaignDisplayNamesByOptionIndex.Add(Row.Name);
        CampaignRowNamesByOptionIndex.Add(Row.RowName);
        CampaignIndexByOptionIndex.Add(Row.Index + 1);
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerSS->GetPlayerInfo().Name));
    }

    int32 SelectedOptionIndex = 0;
    const int32 SavedCampaignIndex1Based = PlayerSS->GetPlayerInfo().Campaign;

    if (SavedCampaignIndex1Based > 0)
    {
        const int32 Found = CampaignIndexByOptionIndex.IndexOfByKey(SavedCampaignIndex1Based);
        if (Found != INDEX_NONE)
        {
            SelectedOptionIndex = Found;
        }
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(SelectedOptionIndex))
    {
        SelectedOptionIndex = CampaignIndexByOptionIndex.Num() > 0 ? 0 : INDEX_NONE;
    }

    Selected = SelectedOptionIndex;
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    UpdateCampaignDropdownLabel();
    RebuildCampaignDropdownOptions();

    if (Selected != INDEX_NONE)
    {
        RefreshFromSelection();
    }
    else
    {
        UpdateCampaignButtons();
    }
}

void UCampaignSelectDlg::RefreshFromSelection()
{
    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
    {
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem =
        GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        return;
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(Selected))
    {
        return;
    }

    const int32 CampaignIndex1Based = CampaignIndexByOptionIndex[Selected];
    const FS_Campaign* CampaignData = DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);
    if (!CampaignData)
    {
        return;
    }

    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignScreen] RefreshFromSelection: Failed to build runtime campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    TArray<FString> Orders = CampaignData->Orders;
    Orders.SetNum(4);

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::FromString(CampaignData->Name));
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(CampaignData->Description));
    }

    if (SituationText)
    {
        SituationText->SetText(FText::FromString(CampaignData->Situation));
    }

    if (Orders1Text)
    {
        Orders1Text->SetText(FText::FromString(Orders[0]));
    }

    if (Orders2Text)
    {
        Orders2Text->SetText(FText::FromString(Orders[1]));
    }

    if (Orders3Text)
    {
        Orders3Text->SetText(FText::FromString(Orders[2]));
    }

    if (Orders4Text)
    {
        Orders4Text->SetText(FText::FromString(Orders[3]));
    }

    if (LocationSystemText)
    {
        LocationSystemText->SetText(
            FText::FromString(CampaignData->System + TEXT("/") + CampaignData->Region));
    }

    if (CampaignStartTimeText)
    {
        CampaignStartTimeText->SetText(FText::FromString(CampaignData->Start));
    }

    if (CampaignImage)
    {
        if (UTexture2D* Texture = LoadCampaignTexture(CampaignIndex1Based))
        {
            CampaignImage->SetBrushFromTexture(Texture, true);
        }
        else
        {
            CampaignImage->SetBrush(FSlateBrush());
        }
    }

    UpdateCampaignButtons();
}

void UCampaignSelectDlg::RefreshUIFromSubsystem()
{
    PopulateCampaignDropdown();
}

void UCampaignSelectDlg::UpdateCampaignButtons()
{
    const bool bHasSave = DoesSelectedCampaignSaveExist();

    if (PlayButton)
    {
        PlayButton->SetIsEnabled(Selected != INDEX_NONE);
    }

    if (PlayButtonText)
    {
        PlayButtonText->SetText(FText::FromString(
            bHasSave ? TEXT("CONTINUE") : TEXT("START")));
    }

    if (RestartButton)
    {
        RestartButton->SetIsEnabled(bHasSave);
    }

    if (CancelButton)
    {
        CancelButton->SetIsEnabled(true);
    }
}

bool UCampaignSelectDlg::DoesSelectedCampaignSaveExist() const
{
    const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI || PickedRowName.IsNone())
    {
        return false;
    }

    const FString SlotName = UCampaignSave::MakeSlotNameFromRowName(PickedRowName);
    return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UCampaignSelectDlg::OnCampaignDropdownClicked()
{
    bCampaignDropdownOpen = !bCampaignDropdownOpen;

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetVisibility(
            bCampaignDropdownOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UCampaignSelectDlg::OnCampaignOptionClicked()
{
    for (int32 i = 0; i < CampaignOptionButtons.Num(); ++i)
    {
        UButton* Button = CampaignOptionButtons[i];
        if (Button && (Button->IsHovered() || Button->HasKeyboardFocus()))
        {
            SelectCampaignOption(i);
            return;
        }
    }
}

void UCampaignSelectDlg::OnPlayButtonClicked()
{
    PlayUISound(this, AcceptSound);
    StartSelectedCampaignFlow(false);
}

void UCampaignSelectDlg::OnRestartButtonClicked()
{
    PlayUISound(this, AcceptSound);
    StartSelectedCampaignFlow(true);
}

void UCampaignSelectDlg::OnPlayButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnPlayButtonUnHovered()
{
}

void UCampaignSelectDlg::OnRestartButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnRestartButtonUnHovered()
{
}

void UCampaignSelectDlg::OnCancelButtonClicked()
{
    HideCampaignDropdown();

    if (manager)
    {
        manager->ShowMenuDlg();
    }
    else
    {
        HideDlg();
    }
}

void UCampaignSelectDlg::OnCancelButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnCancelButtonUnHovered()
{
}

void UCampaignSelectDlg::PlayUISound(UObject* WorldContext, USoundBase* UISound)
{
    if (UISound)
    {
        UGameplayStatics::PlaySound2D(WorldContext, UISound);
    }
}

void UCampaignSelectDlg::StartSelectedCampaignFlow(bool bRestart)
{
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    if (PickedRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Campaign] StartSelectedCampaignFlow: no campaign selected"));
        return;
    }

    if (!manager)
    {
        UE_LOG(LogTemp, Error, TEXT("[Campaign] StartSelectedCampaignFlow: manager is null"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[Campaign] StartSelectedCampaignFlow: world is null"));
        return;
    }

    manager->ShowCmpLoadDlg();

    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UCampaignSelectDlg::FinishSelectedCampaignFlow, bRestart));
}

void UCampaignSelectDlg::FinishSelectedCampaignFlow(bool bRestart)
{
    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
    {
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
    UStarshatterGameDataSubsystem* DataSubsystem = GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);

    if (!PlayerSS || !DataSubsystem || !GI)
    {
        return;
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(Selected))
    {
        return;
    }

    const int32 CampaignIndex1Based = CampaignIndexByOptionIndex[Selected];
    const FS_Campaign* CampaignData = DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);
    if (!CampaignData)
    {
        return;
    }

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Campaign] Failed to activate campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    Campaign* CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        CampaignPtr->Start();
    }

    DataSubsystem->CampaignIndex = CampaignIndex1Based - 1;
    DataSubsystem->SelectedCampaignRowName = PickedRowName;
    DataSubsystem->BuildCombatRosterFromDataTables();

    GI->SelectedCampaignDisplayName = CampaignData->Name;
    GI->SelectedCampaignIndex = CampaignIndex1Based;
    GI->SelectedCampaignRowName = PickedRowName;

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    {
        FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
        PlayerInfo.Campaign = CampaignIndex1Based;
        PlayerInfo.CampaignRowName = PickedRowName;
        PlayerSS->SavePlayer(true);
    }

    if (bRestart)
    {
        GI->CreateNewCampaignSave(
            GI->SelectedCampaignIndex,
            GI->SelectedCampaignRowName,
            GI->SelectedCampaignDisplayName);
    }
    else
    {
        const bool bHasSave = DoesSelectedCampaignSaveExist();

        if (bHasSave)
        {
            GI->LoadOrCreateSelectedCampaignSave();
        }
        else
        {
            GI->CreateNewCampaignSave(
                GI->SelectedCampaignIndex,
                GI->SelectedCampaignRowName,
                GI->SelectedCampaignDisplayName);
        }

        if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
        {
            Timer->SetCampaignSave(GI->CampaignSave);

            if (!bHasSave)
            {
                Timer->RestartCampaignClock(true);
            }
        }
    }

    if (bRestart)
    {
        if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
        {
            Timer->SetCampaignSave(GI->CampaignSave);
            Timer->RestartCampaignClock(true);
        }
    }

    Mouse::Show(false);

    if (USSWRuntimeSubsystem* RuntimeSS = GIBase->GetSubsystem<USSWRuntimeSubsystem>())
    {
        RuntimeSS->SetGameMode(EGameMode::CLOD);
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        if (manager)
        {
            manager->ShowOperationsDlg();
        }
        return;
    }

    World->GetTimerManager().ClearTimer(CampaignLoadFinishTimer);
    World->GetTimerManager().SetTimer(
        CampaignLoadFinishTimer,
        this,
        &UCampaignSelectDlg::TryFinishCampaignLoadTransition,
        0.05f,
        true);
}

void UCampaignSelectDlg::TryFinishCampaignLoadTransition()
{
    if (!manager)
    {
        return;
    }

    UCmpLoadDlg* LoadDlg = manager->GetCmpLoadDlg();
    if (!LoadDlg)
    {
        manager->ShowOperationsDlg();
        return;
    }

    if (!LoadDlg->IsDone())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CampaignLoadFinishTimer);
    }

    manager->ShowOperationsDlg();
}

UTexture2D* UCampaignSelectDlg::LoadCampaignTexture(int32 CampaignIndex1Based) const
{
    if (CampaignIndex1Based <= 0)
    {
        return nullptr;
    }

    const FString IndexStr = FString::Printf(TEXT("%02d"), CampaignIndex1Based);
    const FString AssetPath = FString::Printf(
        TEXT("/Game/UI/Campaigns/%s/main.main"),
        *IndexStr);

    return LoadObject<UTexture2D>(nullptr, *AssetPath);
}

UTextBlock* UCampaignSelectDlg::CreateText(
    const FName Name,
    const FString& InText,
    int32 FontSize,
    const FLinearColor& Color,
    ETextJustify::Type Justification,
    bool bWrap)
{
    UTextBlock* TextWidget = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        Name);

    if (!TextWidget)
    {
        return nullptr;
    }

    TextWidget->SetText(FText::FromString(InText));
    TextWidget->SetColorAndOpacity(FSlateColor(Color));
    TextWidget->SetJustification(Justification);
    TextWidget->SetAutoWrapText(bWrap);

    if (FontSize >= 18)
    {
        TextWidget->SetFont(MissionUIStyle::GetHeaderFont(FontSize));
    }
    else if (FontSize <= 13)
    {
        TextWidget->SetFont(MissionUIStyle::GetInfoLabelFont(FontSize));
    }
    else if (FontSize == 14)
    {
        TextWidget->SetFont(MissionUIStyle::GetInfoValueFont(FontSize));
    }
    else
    {
        TextWidget->SetFont(MissionUIStyle::GetRowFont(FontSize));
    }

    return TextWidget;
}

UBorder* UCampaignSelectDlg::CreatePanelBorder(const FName Name, const FLinearColor& Color)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        Name);

    if (!Border)
    {
        return nullptr;
    }

    Border->SetBrushColor(Color);
    return Border;
}

UWidget* UCampaignSelectDlg::CreateMenuButton(
    const FName Name,
    TObjectPtr<UButton>& OutButton,
    TObjectPtr<UTextBlock>& OutText,
    const FString& Label)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    USizeBox* SizeWrapper = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),
        MakeUniqueWidgetName(TEXT("CampaignSelect_ButtonSizeWrapper")));

    SizeWrapper->SetWidthOverride(256.f);
    SizeWrapper->SetHeightOverride(40.f);

    OutButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(),
        Name);

    OutText = CreateText(
        MakeUniqueWidgetName(TEXT("CampaignSelect_ButtonText")),
        Label,
        16,
        FLinearColor::Black,
        ETextJustify::Center,
        false);

    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(OutButton->AddChild(OutText)))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Center);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(4.f, 2.f));
    }

    SizeWrapper->SetContent(OutButton);

    return SizeWrapper;
}

void UCampaignSelectDlg::ApplyButtonStyle(UButton* Button) const
{
    if (!Button)
    {
        return;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterUIStyleSubsystem* StyleSS = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
        {
            StyleSS->ApplyMenuButtonStyle(Button);
        }
    }
}

void UCampaignSelectDlg::ExecFrame(double DeltaTime)
{
    if (Keyboard::KeyDown(VK_RETURN))
    {
        if (PlayButton && PlayButton->GetIsEnabled())
        {
            OnPlayButtonClicked();
        }
    }
}

bool UCampaignSelectDlg::CanClose()
{
    AutoThreadSync a(sync);
    return !loading;
}

void UCampaignSelectDlg::ShowNewCampaigns()
{
    show_saved = false;
}

void UCampaignSelectDlg::ShowSavedCampaigns()
{
    show_saved = true;
}

void UCampaignSelectDlg::OnCampaignSelect()
{
}

void UCampaignSelectDlg::OnNew()
{
    ShowNewCampaigns();
}

void UCampaignSelectDlg::OnSaved()
{
    ShowSavedCampaigns();
}

void UCampaignSelectDlg::OnDelete()
{
}

void UCampaignSelectDlg::OnConfirmDelete()
{
}

void UCampaignSelectDlg::OnAccept()
{
    OnPlayButtonClicked();
}

void UCampaignSelectDlg::StartLoadProc()
{
}

void UCampaignSelectDlg::StopLoadProc()
{
}

uint32 UCampaignSelectDlg::LoadProc()
{
    return 0;
}

void UCampaignSelectDlg::BindFormWidgets()
{
}

FString UCampaignSelectDlg::GetLegacyFormText() const
{
    return FString();
}

void UCampaignSelectDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UCampaignSelectDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}