/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdDlg implementation (Unreal port)
*/

#include "CmdDlg.h"
#include "Misc/DateTime.h"

// UMG:
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Texture2D.h"
#include "MissionUIStyle.h"
#include "Blueprint/WidgetTree.h"

// Starshatter core:
#include "Campaign.h"
#include "CombatGroup.h"
#include "Starshatter.h"
#include "FormatUtil.h"
#include "Mouse.h"

// Your campaign screen widget (port of CmpnScreen):
#include "CmpnScreen.h"
#include "CmpFileDlg.h"
#include "MenuButton.h"
#include "GameStructs.h"

#include "SelectableButtonGroup.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"

#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarshatterAudioSubsystem.h"

#include "CmdOrdersDlg.h"
#include "CmdMissionsDlg.h"
#include "CmdForceDlg.h"
#include "CmdIntelDlg.h"
#include "CmdTheaterDlg.h"
#include "CampaignMissionDlg.h"
#include "Mission.h"
#include "GameStructs_UI.h"

#include "MissionListObject.h"

#include "TimerSubsystem.h"
#include "FormattingUtils.h"
#include "SSWGameInstance.h"
#include "SSWRuntimeSubsystem.h"

#include "Mouse.h"


void UCmdDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: GameInstance is NULL"));
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: PlayerSubsystem is NULL"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] NativeConstruct: GameDataSubsystem is NULL"));
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    HandleGameTimers();

    const FS_Campaign* FoundCampaign =
        DataSubsystem->GetCampaignByIndex1Based(PlayerInfo.Campaign);

    if (FoundCampaign)
    {
        CurrentCampaignData = *FoundCampaign;
        bHasCurrentCampaign = true;

        UE_LOG(LogTemp, Log,
            TEXT("[CmdDlg] Loaded Campaign: %s (Index=%d)"),
            *CurrentCampaignData.Name,
            PlayerInfo.Campaign);
    }
    else
    {
        bHasCurrentCampaign = false;

        UE_LOG(LogTemp, Warning,
            TEXT("[CmdDlg] NativeConstruct: No campaign found for index %d"),
            PlayerInfo.Campaign);
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("CAMPAIGN OPERATIONS")));
        TitleText->SetVisibility(ESlateVisibility::Collapsed);
    }

    BuildOperationsChrome();
    RefreshOperationsHeader();

    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
    {
        UE_LOG(LogTemp, Log, TEXT("[CmdDlg] Old menu bindings absent; using code header."));
    }
    else
    {

    MenuButtonContainer->ClearChildren();
    AllMenuButtons.Empty();

    for (int32 i = 0; i < 5; ++i)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        NewButton->SetLayoutMode(ELayoutMode::FillWidth);
        NewButton->SetButtonSize(256.0f, 32.0f);
        NewButton->SetLabelFontSizeValue(16);
        NewButton->SetMenuOption(MenuItems[i]);
        NewButton->SetButtonText(FText::FromString(MenuItems[i]).ToUpper());

        if (UVerticalBoxSlot* VBoxSlot = MenuButtonContainer->AddChildToVerticalBox(NewButton))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Center);
        }

        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UCmdDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UCmdDlg::OnMenuToggleSelected);

        NewButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnMenuToggleHovered);
        NewButton->OnHovered.AddDynamic(this, &UCmdDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveDynamic(this, &UCmdDlg::OnCancelButtonClicked);
        ReturnButton->OnClicked.AddDynamic(this, &UCmdDlg::OnCancelButtonClicked);

        ReturnButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnCancelButtonHovered);
        ReturnButton->OnHovered.AddDynamic(this, &UCmdDlg::OnCancelButtonHovered);

        ReturnButton->OnUnhovered.RemoveDynamic(this, &UCmdDlg::OnCancelButtonUnHovered);
        ReturnButton->OnUnhovered.AddDynamic(this, &UCmdDlg::OnCancelButtonUnHovered);

        if (ReturnButtonText)
        {
            ReturnButtonText->SetText(FText::FromString(TEXT("CANCEL")));
        }
    }

    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveDynamic(this, &UCmdDlg::OnMissionButtonClicked);
        MissionButton->OnClicked.AddDynamic(this, &UCmdDlg::OnMissionButtonClicked);

        MissionButton->OnHovered.RemoveDynamic(this, &UCmdDlg::OnMissionButtonHovered);
        MissionButton->OnHovered.AddDynamic(this, &UCmdDlg::OnMissionButtonHovered);

        MissionButton->OnUnhovered.RemoveDynamic(this, &UCmdDlg::OnMissionButtonUnHovered);
        MissionButton->OnUnhovered.AddDynamic(this, &UCmdDlg::OnMissionButtonUnHovered);

        if (MissionButtonText)
        {
            MissionButtonText->SetText(FText::FromString(TEXT("ACCEPT")));
        }
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));
        UE_LOG(LogTemp, Log, TEXT("[CmdDlg] Player Name: %s"), *PlayerInfo.Name);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(0);
    }

    if (bHasCurrentCampaign)
    {
        if (CurrentLocationText)
        {
            CurrentLocationText->SetText(
                FText::FromString(CurrentCampaignData.System + TEXT(" System")).ToUpper());
        }

        if (CampaignNameText)
        {
            CampaignNameText->SetText(
                FText::FromString(CurrentCampaignData.Name));
        }
    }

    if (btn_save) btn_save->OnClicked.AddDynamic(this, &UCmdDlg::OnSaveClicked);
    if (btn_exit) btn_exit->OnClicked.AddDynamic(this, &UCmdDlg::OnExitClicked);

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    const FString Frm = GetLegacyFormText();
    if (!Frm.IsEmpty())
    {
        FString Err;
        FParsedForm Parsed;

        if (ParseLegacyForm(Frm, Parsed, Err))
        {
            ParsedForm = Parsed;
            ApplyLegacyFormDefaults(ParsedForm);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] ParseLegacyForm failed: %s"), *Err);
        }
    }
}

void UCmdDlg::BuildOperationsChrome()
{
    if (!WidgetTree || WidgetTree->FindWidget(TEXT("OperationsChrome"))) return;

    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Root) Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasRoot")));
    if (!Root) Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasPanel_27")));
    if (!Root)
    {
        Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasRoot"));
        WidgetTree->RootWidget = Root;
    }

    for (UWidget* Child : Root->GetAllChildren())
    {
        if (!Child || Child == OperationalSwitcher) continue;
        if (UImage* Image = Cast<UImage>(Child))
            Image->SetVisibility(ESlateVisibility::Collapsed);
        else if (Child->GetName().Contains(TEXT("Title")))
            Child->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (MenuButtonContainer) MenuButtonContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (ReturnButton) ReturnButton->SetVisibility(ESlateVisibility::Collapsed);
    if (MissionButton) MissionButton->SetVisibility(ESlateVisibility::Collapsed);
    if (UWidget* LeftPanel = WidgetTree->FindWidget(TEXT("LeftDialogPanel")))
        LeftPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (UWidget* OldFrame = WidgetTree->FindWidget(TEXT("CampaignSelectBackground")))
        OldFrame->SetVisibility(ESlateVisibility::Collapsed);
    if (UWidget* OldPanel = WidgetTree->FindWidget(TEXT("Border_5")))
        OldPanel->SetVisibility(ESlateVisibility::Collapsed);

    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OperationsFrame"));
    Frame->SetWidthOverride(1920.f);
    Frame->SetHeightOverride(1080.f);
    Frame->SetMinDesiredWidth(1920.f);
    Frame->SetMinDesiredHeight(1080.f);
    Frame->SetMaxDesiredWidth(1920.f);
    Frame->SetMaxDesiredHeight(1080.f);
    Frame->SetClipping(EWidgetClipping::ClipToBounds);
    UBorder* FrameBorder = WidgetTree->ConstructWidget<UBorder>();
    FrameBorder->SetPadding(FMargin(0.f));
    FrameBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
    FrameBorder->SetClipping(EWidgetClipping::ClipToBounds);
    if (UTexture2D* PanelTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
    {
        FSlateBrush PanelBrush;
        PanelBrush.SetResourceObject(PanelTex);
        PanelBrush.DrawAs = ESlateBrushDrawType::Image;
        FrameBorder->SetBrush(PanelBrush);
    }
    Frame->SetContent(FrameBorder);
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("OperationsFit"));
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->SetStretchDirection(EStretchDirection::Both);
    Fit->SetContent(Frame);
    if (UCanvasPanelSlot* Main = Root->AddChildToCanvas(Fit))
    {
        Main->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Main->SetOffsets(FMargin(0.f));
        Main->SetZOrder(20);
    }

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OperationsChrome"));
    FrameBorder->SetContent(Column);

    UBorder* Header = WidgetTree->ConstructWidget<UBorder>();
    Header->SetBrushColor(FLinearColor(0.105f, 0.113f, 0.125f, 1.f));
    Header->SetPadding(FMargin(18.f, 9.f));
    UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    UTextBlock* OperationsTitle = WidgetTree->ConstructWidget<UTextBlock>();
    OperationsTitle->SetText(FText::FromString(TEXT("OPERATIONS")));
    OperationsTitle->SetFont(MissionUIStyle::GetHeaderFont(38));
    OperationsTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
    if (UHorizontalBoxSlot* LeftSlot = TitleRow->AddChildToHorizontalBox(OperationsTitle))
    {
        LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LeftSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (!CampaignNameText) CampaignNameText = WidgetTree->ConstructWidget<UTextBlock>();
    CampaignNameText->SetText(FText::FromString(bHasCurrentCampaign ? CurrentCampaignData.Name.ToUpper() : TEXT("")));
    CampaignNameText->SetFont(MissionUIStyle::GetHeaderFont(38));
    CampaignNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
    CampaignNameText->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot* RightSlot = TitleRow->AddChildToHorizontalBox(CampaignNameText))
    {
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        RightSlot->SetHorizontalAlignment(HAlign_Fill);
        RightSlot->SetVerticalAlignment(VAlign_Center);
    }
    Header->SetContent(TitleRow);
    Column->AddChildToVerticalBox(Header);

    UBorder* StatusBar = WidgetTree->ConstructWidget<UBorder>();
    StatusBar->SetBrushColor(FLinearColor(0.07f, 0.08f, 0.09f, 1.f));
    StatusBar->SetPadding(FMargin(12.f, 8.f));
    UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto AddStatus = [this, StatusRow](const TCHAR* Caption, UTextBlock*& Value)
    {
        UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>();
        UTextBlock* CaptionText = WidgetTree->ConstructWidget<UTextBlock>();
        CaptionText->SetText(FText::FromString(Caption));
        CaptionText->SetFont(MissionUIStyle::GetInfoLabelFont(12));
        CaptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f, 1.f)));
        CaptionText->SetJustification(ETextJustify::Left);
        CaptionText->SetAutoWrapText(false);
        if (UVerticalBoxSlot* CaptionSlot = Cell->AddChildToVerticalBox(CaptionText))
            CaptionSlot->SetHorizontalAlignment(HAlign_Left);
        if (!Value) Value = WidgetTree->ConstructWidget<UTextBlock>();
        Value->SetText(FText::FromString(TEXT("--")));
        Value->SetFont(MissionUIStyle::GetInfoValueFont(16));
        Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f, 1.f)));
        Value->SetJustification(ETextJustify::Left);
        Value->SetAutoWrapText(false);
        if (UVerticalBoxSlot* ValueSlot = Cell->AddChildToVerticalBox(Value))
        {
            ValueSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
            ValueSlot->SetHorizontalAlignment(HAlign_Left);
        }
        if (UHorizontalBoxSlot* CellSlot = StatusRow->AddChildToHorizontalBox(Cell))
        {
            CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CellSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
            CellSlot->SetHorizontalAlignment(HAlign_Left);
            CellSlot->SetVerticalAlignment(VAlign_Top);
        }
    };
    AddStatus(TEXT("TIME"), GameTimeText);
    AddStatus(TEXT("CAMPAIGN"), CampaignTPlusText);
    AddStatus(TEXT("PILOT"), PlayerNameText);
    AddStatus(TEXT("UNIT"), CurrentUnitText);
    AddStatus(TEXT("SCORE"), PlayerScoreText);
    StatusBar->SetContent(StatusRow);
    Column->AddChildToVerticalBox(StatusBar)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

    UBorder* Rule = WidgetTree->ConstructWidget<UBorder>();
    Rule->SetBrushColor(FLinearColor(0.20f, 0.21f, 0.23f, 1.f));
    USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>();
    RuleSize->SetHeightOverride(5.f);
    Rule->SetContent(RuleSize);
    Column->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
    auto MakeIconTab = [this](const TCHAR* IconPath)->UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(40.f);
        IconBox->SetHeightOverride(40.f);
        UImage* Icon = WidgetTree->ConstructWidget<UImage>();
        if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, IconPath))
            Icon->SetBrushFromTexture(Tex, true);
        IconBox->SetContent(Icon);
        Button->SetContent(IconBox);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(IconBox->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(4.f));
        }
        return Button;
    };
    auto MakeTab = [this, Tabs](const TCHAR* Name)->UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
        Caption->SetText(FText::FromString(Name));
        Caption->SetFont(MissionUIStyle::GetHeaderFont(22));
        Caption->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Caption->SetJustification(ETextJustify::Center);
        Button->SetContent(Caption);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Caption->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(10.f, 4.f));
        }
        if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Button))
        {
            TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TabSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
        }
        return Button;
    };
    OperationsMenuTab = MakeIconTab(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Menu_256x256.T_Menu_256x256"));
    if (UHorizontalBoxSlot* MenuSlot = Tabs->AddChildToHorizontalBox(OperationsMenuTab))
    {
        MenuSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        MenuSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
        MenuSlot->SetVerticalAlignment(VAlign_Center);
    }
    OperationsOrdersTab = MakeTab(TEXT("ORDERS"));
    OperationsTheaterTab = MakeTab(TEXT("THEATER"));
    OperationsForcesTab = MakeTab(TEXT("FORCES"));
    OperationsIntelTab = MakeTab(TEXT("INTEL"));
    OperationsMissionTab = MakeTab(TEXT("MISSION"));
    OperationsPlayTab = MakeIconTab(TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Briefing_256x256.T_Briefing_256x256"));
    if (UHorizontalBoxSlot* PlaySlot = Tabs->AddChildToHorizontalBox(OperationsPlayTab))
    {
        PlaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        PlaySlot->SetPadding(FMargin(0.f));
        PlaySlot->SetVerticalAlignment(VAlign_Center);
    }
    OperationsMenuTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OnCancelButtonClicked);
    OperationsOrdersTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OpenOperationsOrders);
    OperationsTheaterTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OpenOperationsTheater);
    OperationsForcesTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OpenOperationsForces);
    OperationsIntelTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OpenOperationsIntel);
    OperationsMissionTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OpenOperationsMissions);
    OperationsPlayTab->OnClicked.AddUniqueDynamic(this, &UCmdDlg::OnMissionButtonClicked);
    RefreshOperationsTabStyles();

    UCanvasPanel* Host = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OperationsContentHost"));
    OperationsContentHost = Host;
    if (UVerticalBoxSlot* HostSlot = Column->AddChildToVerticalBox(Host))
    {
        FSlateChildSize Fill;
        Fill.SizeRule = ESlateSizeRule::Fill;
        Fill.Value = 1.f;
        HostSlot->SetSize(Fill);
        HostSlot->SetHorizontalAlignment(HAlign_Fill);
        HostSlot->SetVerticalAlignment(VAlign_Fill);
    }
    if (!OperationalSwitcher)
        OperationalSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("OperationalSwitcher"));
    if (OperationalSwitcher)
    {
        OperationalSwitcher->RemoveFromParent();
        if (UCanvasPanelSlot* ContentSlot = Host->AddChildToCanvas(OperationalSwitcher))
        {
            ContentSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            ContentSlot->SetOffsets(FMargin(0.f));
        }
        OperationalSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void UCmdDlg::RefreshOperationsHeader()
{
    if (!CampaignNameText) return;
    if (bHasCurrentCampaign && !CurrentCampaignData.Name.IsEmpty())
        CampaignNameText->SetText(FText::FromString(CurrentCampaignData.Name.ToUpper()));
    else if (Campaign* Live = Campaign::GetCampaign())
        CampaignNameText->SetText(FText::FromString(FString(UTF8_TO_TCHAR(Live->GetName())).ToUpper()));
}

void UCmdDlg::RefreshOperationsTabStyles()
{
    auto Style = [](UButton* Button, const TCHAR* NormalPath, const TCHAR* PressedPath)
    {
        if (!Button) return;
        UTexture2D* Normal = LoadObject<UTexture2D>(nullptr, NormalPath);
        UTexture2D* Pressed = LoadObject<UTexture2D>(nullptr, PressedPath);
        FButtonStyle ButtonStyle;
        auto SetBrush = [](FSlateBrush& Brush, UTexture2D* Texture)
        {
            Brush = FSlateBrush();
            if (!Texture) return;
            Brush.SetResourceObject(Texture);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(64.f, 64.f);
            Brush.Margin = FMargin(0.f);
            Brush.TintColor = FSlateColor(FLinearColor::White);
        };
        SetBrush(ButtonStyle.Normal, Normal);
        SetBrush(ButtonStyle.Hovered, Pressed ? Pressed : Normal);
        SetBrush(ButtonStyle.Pressed, Pressed ? Pressed : Normal);
        SetBrush(ButtonStyle.Disabled, Normal);
        Button->SetStyle(ButtonStyle);
    };
    const TCHAR* BlueN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Blue_n.T_Btn01Blue_n");
    const TCHAR* BlueP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Blue_p.T_Btn01Blue_p");
    const TCHAR* CyanN = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Cyan_n.T_Btn01Cyan_n");
    const TCHAR* CyanP = TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Cyan_p.T_Btn01Cyan_p");
    Style(OperationsMenuTab, TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Red_n.T_Btn01Red_n"), TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Red_p.T_Btn01Red_p"));
    Style(OperationsPlayTab, TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Green_n.T_Btn01Green_n"), TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01Green_p.T_Btn01Green_p"));
    Style(OperationsOrdersTab, CurrentScreen == ECmdScreen::Orders ? BlueN : CyanN, CurrentScreen == ECmdScreen::Orders ? BlueP : CyanP);
    Style(OperationsTheaterTab, CurrentScreen == ECmdScreen::Theater ? BlueN : CyanN, CurrentScreen == ECmdScreen::Theater ? BlueP : CyanP);
    Style(OperationsForcesTab, CurrentScreen == ECmdScreen::Forces ? BlueN : CyanN, CurrentScreen == ECmdScreen::Forces ? BlueP : CyanP);
    Style(OperationsIntelTab, CurrentScreen == ECmdScreen::Intel ? BlueN : CyanN, CurrentScreen == ECmdScreen::Intel ? BlueP : CyanP);
    Style(OperationsMissionTab, CurrentScreen == ECmdScreen::Missions ? BlueN : CyanN, CurrentScreen == ECmdScreen::Missions ? BlueP : CyanP);
}

void UCmdDlg::OpenOperationsOrders() { CurrentScreen = ECmdScreen::Orders; LoadOrdersInfo(); RefreshOperationsTabStyles(); }
void UCmdDlg::OpenOperationsTheater() { CurrentScreen = ECmdScreen::Theater; LoadTheaterInfo(); RefreshOperationsTabStyles(); }
void UCmdDlg::OpenOperationsForces() { CurrentScreen = ECmdScreen::Forces; LoadForcesInfo(); RefreshOperationsTabStyles(); }
void UCmdDlg::OpenOperationsIntel() { CurrentScreen = ECmdScreen::Intel; LoadIntelInfo(); RefreshOperationsTabStyles(); }
void UCmdDlg::OpenOperationsMissions() { CurrentScreen = ECmdScreen::Missions; LoadMissionsInfo(); RefreshOperationsTabStyles(); }

void UCmdDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->SetParentCmdDlg(this);
        CmdOrdersPanel->ShowOrdersDlg();
    }

    if (CmdMissionsPanel)
    {
        CmdMissionsPanel->SetParentCmdDlg(this);
    }

    if (CmdForcesPanel)
    {
        CmdForcesPanel->SetParentCmdDlg(this);
    }

    if (CmdIntelPanel)
    {
        CmdIntelPanel->SetParentCmdDlg(this);
    }

    if (CmdTheaterPanel)
    {
        CmdTheaterPanel->SetParentCmdDlg(this);
    }
}

void UCmdDlg::NativeOnInitialized()
{
}

void UCmdDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame(InDeltaTime);
}

UCmdDlg::UCmdDlg(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UCmdDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCmdDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCmdDlg::OnMenuButtonSelected(UMenuButton* SelectedButton)
{
    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button == SelectedButton);
        }
    }
}

void UCmdDlg::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton) return;

    UE_LOG(LogTemp, Log, TEXT("Selected button: %s"), *GetNameSafe(SelectedButton));

    const FString& MenuOption = SelectedButton->MenuOption;

    if (MenuOption == MenuItems[0])
    {
        CurrentScreen = ECmdScreen::Orders;
        LoadOrdersInfo();
    }
    else if (MenuOption == MenuItems[1])
    {
        CurrentScreen = ECmdScreen::Theater;
        LoadTheaterInfo();
    }
    else if (MenuOption == MenuItems[2])
    {
        CurrentScreen = ECmdScreen::Forces;
        LoadForcesInfo();
    }
    else if (MenuOption == MenuItems[3])
    {
        CurrentScreen = ECmdScreen::Intel;
        LoadIntelInfo();
    }
    else if (MenuOption == MenuItems[4])
    {
        CurrentScreen = ECmdScreen::Missions;
        LoadMissionsInfo();
    }

    UpdateMissionButton();
}

void UCmdDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton)
        return;

    UE_LOG(LogTemp, Log, TEXT("Hovered over: %s"), *HoveredButton->MenuOption);

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayHoverSound(this);
    }
}

void UCmdDlg::BindFormWidgets()
{
    BindButton(1, btn_save);
    BindButton(2, btn_exit);
}

void UCmdDlg::SetManager(UCmpnScreen* InManager)
{
    CmpnScreen = InManager;
}

void UCmdDlg::ShowCmdDlg()
{
    CampaignPtr = Campaign::GetCampaign();
    RefreshOperationsHeader();

    if (txt_name)
    {
        if (CampaignPtr)
            txt_name->SetText(FText::FromString(CampaignPtr->GetName()));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    if (CampaignPtr)
    {
        const bool bTraining = CampaignPtr->IsTraining();

        if (btn_save)
            btn_save->SetIsEnabled(!bTraining);
    }

    SetVisibility(ESlateVisibility::Visible);

    CurrentScreen = ECmdScreen::Missions;
    LoadMissionsInfo();
    RefreshOperationsTabStyles();

    UpdateMissionButton();

    // Prime intel data immediately on command dialog open
    IntelRefreshCounterSeconds = 0;
    RefreshIntelDataBackground();
}

void UCmdDlg::ExecFrame(double DeltaTime)
{
    UTimerSubsystem* Timer = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTimerSubsystem>() : nullptr;
    USSWGameInstance* SSW = Cast<USSWGameInstance>(GetGameInstance());
    if (SSW && Timer)
    {
        if (!SSW->CampaignSave)
        {
            UStarshatterPlayerSubsystem* PlayerSS = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
            if (PlayerSS && !PlayerSS->HasLoaded())
                PlayerSS->LoadPlayer();
            const FS_PlayerGameInfo Info = PlayerSS ? PlayerSS->GetPlayerInfo() : FS_PlayerGameInfo();
            const int32 CampaignIndex = Info.Campaign > 0 ? Info.Campaign : 1;
            SSW->LoadOrCreateCampaignSave(CampaignIndex, Info.CampaignRowName, FString());
        }
        if (SSW->CampaignSave && !Timer->HasCampaignSave())
            Timer->SetCampaignSave(SSW->CampaignSave);
        if (!Timer->IsClockRunning())
            Timer->StartClock();
    }

    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (CurrentUnitText && CampaignPtr)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            CurrentUnitText->SetText(FText::FromString(G->GetDescription()));
    }

    if (PlayerScoreText && CampaignPtr)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        PlayerScoreText->SetText(FText::FromString(ScoreStr));
        PlayerScoreText->SetJustification(ETextJustify::Left);
    }

    if (CampaignTPlusText)
    {
        // Campaign T+ must come from the persistent campaign/universe clock,
        // not CampaignPtr->GetTime().  The legacy value can be negative or
        // overflow during startup, which produced values such as
        // -2147483648 in the UI.
        USSWGameInstance* SSWInstance =
            Cast<USSWGameInstance>(GetGameInstance());

        if (SSWInstance &&
            Timer &&
            SSWInstance->CampaignSave)
        {
            const uint64 Now =
                Timer->GetUniverseTimeSeconds();

            const uint64 Start =
                SSWInstance->CampaignSave->CampaignStartUniverseSeconds;

            // Prevent unsigned underflow if an older save contains
            // an anchor later than the current universe clock.
            const uint64 TPlusSeconds =
                (Now >= Start)
                    ? (Now - Start)
                    : 0ULL;

            const uint64 Days =
                TPlusSeconds / 86400ULL;

            const uint64 RemainderAfterDays =
                TPlusSeconds % 86400ULL;

            const uint64 Hours =
                RemainderAfterDays / 3600ULL;

            const uint64 Minutes =
                (RemainderAfterDays % 3600ULL) / 60ULL;

            const uint64 Seconds =
                RemainderAfterDays % 60ULL;

            CampaignTPlusText->SetText(
                FText::FromString(
                    FString::Printf(
                        TEXT("Day %02llu %02llu:%02llu:%02llu"),
                        (unsigned long long)(Days + 1ULL),
                        (unsigned long long)Hours,
                        (unsigned long long)Minutes,
                        (unsigned long long)Seconds)));
        }
        else
        {
            CampaignTPlusText->SetText(
                FText::FromString(
                    TEXT("Day 01 00:00:00")));
        }
    }

    const int32 Unread = CampaignPtr->CountNewEvents();

    RefreshCommandButtons();

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg();
    }

    if (AllMenuButtons.IsValidIndex(3) && AllMenuButtons[3])
    {
        if (UTextBlock* Label = Cast<UTextBlock>(AllMenuButtons[3]->GetWidgetFromName(TEXT("Label"))))
        {
            if (Unread > 0)
            {
                Label->SetText(FText::FromString(FString::Printf(TEXT("INTEL (%d)"), Unread)));
            }
            else
            {
                Label->SetText(FText::FromString(TEXT("INTEL")));
            }
        }
    }

    UpdateMissionButton();
}

void UCmdDlg::OnSaveClicked()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (CampaignPtr && CmpnScreen)
    {
        CmpnScreen->ShowCmpFileDlg();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdDlg: CampaignPtr or CmpnScreen is null (OnSaveClicked)."));
    }
}

#include "SSWRuntimeSubsystem.h"

void UCmdDlg::OnCancelButtonClicked()
{
    SetDialogInputEnabled(false);
    SetVisibility(ESlateVisibility::Hidden);

    Mouse::Show(true);

    if (manager)
    {
        manager->ShowCampaignSelectDlg();
    }
    else
    {
        HideDlg();
    }
}

void UCmdDlg::OnCancelButtonHovered()
{
    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayHoverSound(this);
    }
}

void UCmdDlg::OnCancelButtonUnHovered()
{
}

void UCmdDlg::OnMissionButtonClicked()
{
    if (!manager)
        if (USSWGameInstance* SSW = Cast<USSWGameInstance>(GetGameInstance()))
            manager = SSW->MainMenuScreen.Get();

    if (!CmdMissionsPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: no missions panel"));
        return;
    }

    if (!CmdMissionsPanel->CanAcceptSelectedMission())
        return;

    const int32 SelectedMissionId = CmdMissionsPanel->GetSelectedMissionId();
    Campaign* ActiveCampaign = CampaignPtr ? CampaignPtr : Campaign::GetCampaign();
    if (!ActiveCampaign || SelectedMissionId <= 0 || !ActiveCampaign->GetMissionInfo(SelectedMissionId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: no accepted mission"));
        return;
    }

    ActiveCampaign->SetMissionId(SelectedMissionId);
    if (!ActiveCampaign->GetMission(SelectedMissionId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] OnMissionButtonClicked: failed to load mission %d"), SelectedMissionId);
        return;
    }

    if (UGameInstance* GI = GetGameInstance())
        if (USSWRuntimeSubsystem* Runtime = GI->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->SetGameMode(EGameMode::PREP);

    if (manager)
    {
        SetVisibility(ESlateVisibility::Collapsed);
        manager->ShowMissionDlg();
    }
}

void UCmdDlg::OnMissionButtonHovered()
{
    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayHoverSound(this);
    }
}

void UCmdDlg::OnMissionButtonUnHovered()
{
}

void UCmdDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UCmdDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCmdDlg::RefreshUIFromSubsystem()
{
}

int32 UCmdDlg::GetIntelEntryCount() const
{
    Campaign* CurrentCampaign = Campaign::GetCampaign();
    if (!CurrentCampaign)
    {
        return 0;
    }

    return CurrentCampaign->GetEvents().size();
}

void UCmdDlg::LoadForcesInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(2);
        OperationalSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (CmdMissionsPanel)
        CmdMissionsPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsCampaignMissions)
        OperationsCampaignMissions->SetVisibility(ESlateVisibility::Collapsed);

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("FORCES"));
    }

    if (CmdForcesPanel)
    {
        CmdForcesPanel->ShowForceDlg();
    }
}

void UCmdDlg::LoadOrdersInfo()
{
    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(0);
        OperationalSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (CmdMissionsPanel)
        CmdMissionsPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsCampaignMissions)
        OperationsCampaignMissions->SetVisibility(ESlateVisibility::Collapsed);

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("ORDERS"));
    }

    if (CmdOrdersPanel)
    {
        CmdOrdersPanel->ShowOrdersDlg();
    }
}

void UCmdDlg::LoadMissionsInfo()
{
    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    BuildOperationsChrome();
    if (OperationalSwitcher)
        OperationalSwitcher->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsCampaignMissions)
        OperationsCampaignMissions->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsModeText)
        OperationsModeText->SetText(FText::FromString("MISSIONS"));

    if (!CmdMissionsPanel)
        CmdMissionsPanel = CreateWidget<UCmdMissionsDlg>(this, UCmdMissionsDlg::StaticClass());
    if (CmdMissionsPanel)
        CmdMissionsPanel->SetParentCmdDlg(this);

    if (CmdMissionsPanel && OperationsContentHost && CmdMissionsPanel->GetParent() != OperationsContentHost)
    {
        CmdMissionsPanel->RemoveFromParent();
        if (UCanvasPanelSlot* ContentSlot = OperationsContentHost->AddChildToCanvas(CmdMissionsPanel))
        {
            ContentSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            ContentSlot->SetOffsets(FMargin(0.f));
        }
    }
    if (CmdMissionsPanel)
    {
        CmdMissionsPanel->ShowMissionsDlg();
        CmdMissionsPanel->SetVisibility(ESlateVisibility::Visible);
    }
    RefreshOperationsHeader();
    UpdateMissionButton();
}

void UCmdDlg::LoadIntelInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (GetIntelEntryCount() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] LoadIntelInfo: no intel entries"));
        return;
    }

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(3);
        OperationalSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (CmdIntelPanel && CmdOrdersPanel)
    {
        if (UWidgetSwitcherSlot* IntelSlot = Cast<UWidgetSwitcherSlot>(CmdIntelPanel->Slot))
        {
            IntelSlot->SetHorizontalAlignment(HAlign_Fill);
            IntelSlot->SetVerticalAlignment(VAlign_Fill);
            IntelSlot->SetPadding(FMargin(0.f));
        }
        if (UWidgetSwitcherSlot* OrdersSlot = Cast<UWidgetSwitcherSlot>(CmdOrdersPanel->Slot))
        {
            OrdersSlot->SetHorizontalAlignment(HAlign_Fill);
            OrdersSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }
    if (CmdMissionsPanel)
        CmdMissionsPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsCampaignMissions)
        OperationsCampaignMissions->SetVisibility(ESlateVisibility::Collapsed);

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("INTEL"));
    }

    IntelRefreshCounterSeconds = 0;
    RefreshIntelPanel();
}

void UCmdDlg::LoadTheaterInfo()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    if (OperationalSwitcher)
    {
        OperationalSwitcher->SetActiveWidgetIndex(1);
        OperationalSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (CmdMissionsPanel)
        CmdMissionsPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (OperationsCampaignMissions)
        OperationsCampaignMissions->SetVisibility(ESlateVisibility::Collapsed);

    if (OperationsModeText)
    {
        OperationsModeText->SetText(FText::FromString("THEATER"));
    }

    if (CmdTheaterPanel)
    {
        CmdTheaterPanel->ShowTheaterDlg();
    }
}

void UCmdDlg::RefreshIntelPanel()
{
    if (ShouldDisableCommandPanels())
    {
        return;
    }

    if (!CmdIntelPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdDlg] RefreshIntelPanel: CmdIntelPanel is NULL"));
        return;
    }

    CmdIntelPanel->ShowIntelDlg();
}
void UCmdDlg::HandleGameTimers()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdDlg] HandleGameTimers: GameInstance is NULL"));
        return;
    }

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GI);

    if (!SSWInstance) return;

    // CmdDlg can be constructed before the universe save has been injected
    // into TimerSubsystem.  Guarantee the canonical Babylon 5 clock exists
    // before GameTimeText reads it.
    if (Timer && Timer->UniverseBaseUnixSeconds <= 0)
    {
        SSWInstance->LoadOrCreateUniverse();
        Timer = GI->GetSubsystem<UTimerSubsystem>();
    }

    if (Timer)
    {
        Timer->OnUniverseSecond.AddUObject(this, &UCmdDlg::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.AddUObject(this, &UCmdDlg::HandleUniverseMinuteTick);
        Timer->OnCampaignTPlusChanged.AddUObject(this, &UCmdDlg::HandleCampaignTPlusChanged);

        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        if (SSWInstance->CampaignSave)
        {
            HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
        }
    }
}

void UCmdDlg::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;

    if (GameTimeText)
    {
        // The game universe begins on 2228-01-01.  Normally this base is
        // supplied by UniverseSave/SSWGameInstance.  Keep a defensive
        // fallback here so the UI can never regress to Unix epoch 1970.
        int64 BaseUnixSeconds =
            Timer->UniverseBaseUnixSeconds;

        if (BaseUnixSeconds <= 0)
        {
            BaseUnixSeconds =
                FDateTime(
                    2228,
                    1,
                    1,
                    0,
                    0,
                    0)
                .ToUnixTimestamp();

            Timer->UniverseBaseUnixSeconds =
                BaseUnixSeconds;
        }

        const int64 CurrentUnixSeconds =
            BaseUnixSeconds +
            static_cast<int64>(UniverseSecondsNow);

        const FDateTime UniverseDateTime =
            FDateTime::FromUnixTimestamp(
                CurrentUnixSeconds);

        GameTimeText->SetText(
            FText::FromString(
                UniverseDateTime.ToString(
                    TEXT("%Y-%m-%d %H:%M:%S"))));
    }

    // Keep intel data fresh every 10 seconds even when hidden.
    ++IntelRefreshCounterSeconds;

    if (IntelRefreshCounterSeconds >= 10)
    {
        IntelRefreshCounterSeconds = 0;
        RefreshIntelDataBackground();

        UE_LOG(LogTemp, Verbose,
            TEXT("[CmdDlg] Intel data refreshed in background"));
    }
}

void UCmdDlg::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;
}

void UCmdDlg::HandleCampaignTPlusChanged(
    uint64 UniverseSecondsNow,
    uint64 TPlusSeconds)
{
    if (!CampaignTPlusText)
    {
        return;
    }

    // TPlusSeconds is expected to be elapsed campaign time.  Clamp/format
    // entirely in uint64 so no 32-bit signed overflow can reach the UI.
    const uint64 Days =
        TPlusSeconds / 86400ULL;

    const uint64 RemainderAfterDays =
        TPlusSeconds % 86400ULL;

    const uint64 Hours =
        RemainderAfterDays / 3600ULL;

    const uint64 Minutes =
        (RemainderAfterDays % 3600ULL) / 60ULL;

    const uint64 Seconds =
        RemainderAfterDays % 60ULL;

    CampaignTPlusText->SetText(
        FText::FromString(
            FString::Printf(
                TEXT("Day %02llu %02llu:%02llu:%02llu"),
                (unsigned long long)(Days + 1ULL),
                (unsigned long long)Hours,
                (unsigned long long)Minutes,
                (unsigned long long)Seconds)));
}

bool UCmdDlg::ShouldDisableCommandPanels() const
{
    if (!CampaignPtr)
    {
        return false;
    }

    return CampaignPtr->IsScripted();
}

void UCmdDlg::RefreshCommandButtons()
{
    const bool bDisablePanels = ShouldDisableCommandPanels();
    const bool bHasIntelEntries = GetIntelEntryCount() > 0;

    if (AllMenuButtons.IsValidIndex(1) && AllMenuButtons[1])
    {
        AllMenuButtons[1]->SetIsEnabled(!bDisablePanels);
    }

    if (AllMenuButtons.IsValidIndex(2) && AllMenuButtons[2])
    {
        AllMenuButtons[2]->SetIsEnabled(!bDisablePanels);
    }

    if (AllMenuButtons.IsValidIndex(3) && AllMenuButtons[3])
    {
        AllMenuButtons[3]->SetIsEnabled(!bDisablePanels && bHasIntelEntries);
    }
    if (OperationsTheaterTab) OperationsTheaterTab->SetIsEnabled(!bDisablePanels);
    if (OperationsForcesTab) OperationsForcesTab->SetIsEnabled(!bDisablePanels);
    if (OperationsIntelTab) OperationsIntelTab->SetIsEnabled(!bDisablePanels && bHasIntelEntries);

    bLastDisableState = bDisablePanels;
}

void UCmdDlg::UpdateMissionButton()
{
    bool bEnable = CurrentScreen == ECmdScreen::Missions && CmdMissionsPanel && CmdMissionsPanel->CanAcceptSelectedMission();
    if (MissionButton) MissionButton->SetIsEnabled(bEnable);
    if (OperationsPlayTab) OperationsPlayTab->SetIsEnabled(bEnable);
}

void UCmdDlg::ShowMissionsPanel()
{
    CurrentScreen = ECmdScreen::Missions;
    LoadMissionsInfo();
    UpdateMissionButton();
}

void UCmdDlg::RefreshIntelDataBackground()
{
    if (!CmdIntelPanel)
    {
        return;
    }

    CmdIntelPanel->RefreshIntelData();
}

void UCmdDlg::OnExitClicked()
{
    Mouse::Show(false);

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdDlg: GameInstance is null (OnExitClicked)."));
        return;
    }

    USSWRuntimeSubsystem* RuntimeSS = GI->GetSubsystem<USSWRuntimeSubsystem>();
    if (!RuntimeSS)
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdDlg: RuntimeSubsystem is null (OnExitClicked)."));
        return;
    }

    RuntimeSS->SetGameMode(EGameMode::MENU);
}