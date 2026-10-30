/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe
    FILE:         MissionBriefingDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Briefing Screen (Unreal Port)

    - Uses CmdDlg-style MenuButton system (SIT / PKG / NAV / WEP)
    - Drives UI state via WidgetSwitcher (MissionSwitcher)
    - Hosts child mission subpanels
    - Displays mission header (name, system, sector, time)
    - Handles Accept / Cancel flow
    - Calculates Time-On-Target using navigation instructions
*/

#include "MissionBriefingDlg.h"
#include "Blueprint/WidgetTree.h"
#include "MissionUIStyle.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/ScaleBox.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FlightDeck.h"
#include "MissionObjectTarget.h"

#include "Sensor.h"
#include "WeaponGroup.h"

#include "EngineeringPopup.h"
#include "EngineeringDlg.h"
#include "WeaponsDlg.h"
#include "ContactsDlg.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"
#include "WeaponsPopup.h"
#include "ObjectivesDlg.h"
#include "FlightOpsDlg.h"
#include "NavigationPopup.h"
#include "NavigationDlg.h"
#include "MissionCameraRig.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "SimContact.h"
#include "MissionPlanner.h"
#include "MissionDebriefDlg.h"
#include "GameScreen.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "QuitView.h"
#include "QuitMissionMenu.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "SSWRuntimeSubsystem.h"
#include "Sim.h"
#include "TimerSubsystem.h"
#include "SimEvent.h"
#include "OptionsScreen.h"
#include "Kismet/GameplayStatics.h"

#include "MenuScreen.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Camera/CameraActor.h"
#include "TimerManager.h"
#include "Templates/UnrealTemplate.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/PackageName.h"
#include "EngineUtils.h"
#include "PlanetActor.h"
#include "SystemSceneBuilder.h"
#include "GasGiantActor.h"
#include "CentralSun.h"
#include "ShipActor.h"
#include "Ship.h"
#include "LandingGear.h"
#include "Shield.h"
#include "HUDSounds.h"
#include "SimRegion.h"
#include "MissionTargetOverlay.h"
#include "FighterHUDPanels.h"
#include "FighterHUDDetails.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#include "MissionObjectiveDlg.h"
#include "MissionPackageDlg.h"
#include "MissionNavDlg.h"
#include "MissionWeaponDlg.h"

// Legacy sim/campaign:
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "StarSystem.h"
#include "FormatUtil.h"
#include "Instruction.h"
#include "WaypointActor.h"
#include "SimElement.h"
#include "MissionElement.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "FormattingUtils.h"
#include "CampaignSituationReport.h"

// UI:
#include "MenuButton.h"
#include "SelectableButtonGroup.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"

// Input:
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"

// Optional sound:
#include "SSWGameInstance.h"

#include "StarshatterPlayerSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarshatterAudioSubsystem.h"

UMissionBriefingDlg::UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UQuitMissionMenu> MenuBlueprint(
        TEXT("/Game/Screens/Mission/WBP_QuitMissionMenu"));
    if (MenuBlueprint.Succeeded()) QuitMissionMenuClass = MenuBlueprint.Class;
}

void UMissionBriefingDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionBriefingDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionBriefingDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyBriefingPresentation();

    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
    }

    if (MissionPackagePanel)
    {
        MissionPackagePanel->SetParentDlg(this);
    }

    if (MissionNavPanel)
    {
        MissionNavPanel->SetParentDlg(this);
    }

    if (MissionWepPanel)
    {
        MissionWepPanel->SetParentDlg(this);
    }
}

void UMissionBriefingDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (RootSizeBox)
    {
        RootSizeBox->SetWidthOverride(1920.f);
        RootSizeBox->SetHeightOverride(1080.f);
    }

    UCanvasPanelSlot* CanvasSlot = RootSizeBox ? Cast<UCanvasPanelSlot>(RootSizeBox->Slot) : nullptr;
    if (CanvasSlot)
    {
        CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: GameInstance is NULL"));
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: PlayerSubsystem is NULL"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefingDlg] NativeConstruct: GameDataSubsystem is NULL"));
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    const FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetPlayerInfo();

    HandleGameTimers();

    const FS_Campaign* FoundCampaign = DataSubsystem->GetCampaignByIndex1Based(PlayerInfo.Campaign);
    if (FoundCampaign)
    {
        CurrentCampaignData = *FoundCampaign;
        bHasCurrentCampaign = true;

        UE_LOG(LogTemp, Log,
            TEXT("[MissionBriefingDlg] Loaded Campaign: %s (Index=%d)"),
            *CurrentCampaignData.Name,
            PlayerInfo.Campaign);
    }
    else
    {
        bHasCurrentCampaign = false;

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] NativeConstruct: No campaign found for index %d"),
            PlayerInfo.Campaign);
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("MISSION OPERATIONS")));
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerInfo.Name));

        UE_LOG(LogTemp, Log,
            TEXT("[MissionBriefingDlg] Player Name: %s"),
            *PlayerInfo.Name);
    }

    if (MissionSwitcher)
    {
        MissionSwitcher->SetActiveWidgetIndex(0);
    }

    if (bHasCurrentCampaign && CurrentLocationText)
    {
        CurrentLocationText->SetText(
            FText::FromString(CurrentCampaignData.System + TEXT(" System")).ToUpper());
    }

    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
        MissionButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
        ReturnButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);
    }

    ApplyBriefingPresentation();
    BuildMenuButtons();
    InitializeSubPanels();
    BuildBriefingChrome();
    RefreshHeader();

    UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] NativeConstruct: UI initialized"));
}

void UMissionBriefingDlg::NativeDestruct()
{
    if (Sim* Simulation = Sim::GetSim()) Simulation->OnMissionUnloading.RemoveAll(this);
    if (UGameInstance* GI = GetGameInstance()) {
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>()) {
            Timer->OnUniverseSecond.RemoveAll(this);
            Timer->OnUniverseMinute.RemoveAll(this);
            Timer->OnCampaignTPlusChanged.RemoveAll(this);
        }
    }
    CloseEmptySectorPreview();
    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
    }

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->OnSelected.RemoveAll(this);
            Button->OnHovered.RemoveAll(this);
        }
    }

    Super::NativeDestruct();
}

void UMissionBriefingDlg::ExecFrame()
{
    if (bStandaloneMission)
        return; // No story campaign score, unit or T+ for standalone missions.

    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    if (CurrentUnitText)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            CurrentUnitText->SetText(FText::FromString(G->GetDescription()));
    }

    if (PlayerScoreText)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        PlayerScoreText->SetText(FText::FromString(ScoreStr));
        PlayerScoreText->SetJustification(ETextJustify::Right);
    }

    if (MissionTPlusText)
    {
        const double T = CampaignPtr->GetTime();

        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, T);

        MissionTPlusText->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }
}
void UMissionBriefingDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    SyncMissionNavigationTarget();
    // Fallback for a plain mapped Boolean key when another action consumes
    // IA_AutoNav before its callback. The callback and this path are deduplicated.
    // Complex/chorded mappings remain owned by Enhanced Input.
    if (CanUseFighterHUD() && AutoNavInputContext && GetWorld() && !GetWorld()->IsPaused()) {
        if (APlayerController* AutoPC = GetOwningPlayer()) {
            for (const FEnhancedActionKeyMapping& AutoMapping : AutoNavInputContext->GetMappings()) {
                if (AutoMapping.Action && AutoMapping.Action->GetName().Equals(TEXT("IA_AutoNav"), ESearchCase::IgnoreCase) &&
                    AutoMapping.Triggers.Num() == 0 && AutoMapping.Modifiers.Num() == 0 &&
                    AutoMapping.Key.IsValid() && AutoPC->WasInputKeyJustPressed(AutoMapping.Key)) {
                    ToggleMissionAutoNav();
                    break;
                }
            }
        }
    }

    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) ClearMissionRotationInput();

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (PC->WasInputKeyJustPressed(EKeys::Enter))
        {
            OnCommit();
        }
    }
    ExecFrame();

    if (bLiveMissionStarted && !bMissionCompleteHandedOff)
    {
        Sim* Simulation = Sim::GetSim();
        if (Simulation && Simulation->IsComplete() && Manager && Manager->GetDebriefDlg())
        {
            bMissionCompleteHandedOff = true;
            bLiveMissionStarted = false;
            CloseEmptySectorPreview(true);
            if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            {
                Runtime->SetPaused(true);
                Runtime->SetGameMode(EGameMode::PLAN);
                Runtime->EndMissionPresentation();
            }
            Simulation->CommitMission();
            Manager->ShowDebriefDlg();
            auto* Debrief = Manager->GetDebriefDlg();
            Debrief->Show();
            if (APlayerController* PC = GetOwningPlayer())
            {
                PC->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetWidgetToFocus(Debrief->TakeWidget());
                Mode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(Mode);
            }
        }
    }
}

FText UMissionBriefingDlg::ToTextFromUtf8(const char* Utf8)
{
    if (!Utf8 || !Utf8[0])
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString(UTF8_TO_TCHAR(Utf8)));
}

void UMissionBriefingDlg::ApplyBriefingPresentation()
{
    if (BriefingFrame || !bModernBriefingPresentation) return;

    // Keep Blueprint fonts and geometry; adjust only the briefing's visual hierarchy.
    auto StyleText = [](UTextBlock* Text, int32 Size, const FLinearColor& Color)
    {
        if (!Text) return;
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Color));
    };
    StyleText(TitleText, 24, BriefingAccentColor);
    StyleText(MissionNameText, 28, BriefingTextColor);
    for (UTextBlock* Text : { MissionSystemText, MissionSectorText,
        MissionTimeStartText, MissionTimeTargetText })
    {
        StyleText(Text, 18, BriefingTextColor);
    }
    StyleText(MissionTimeTargetLabelText, 16, BriefingAccentColor);

    if (MissionButton)
        MissionButton->SetToolTipText(NSLOCTEXT("MissionBriefing", "AcceptTip", "Accept the mission using the current briefing and loadout."));
    if (ReturnButton)
        ReturnButton->SetToolTipText(NSLOCTEXT("MissionBriefing", "CancelTip", "Return without launching the mission."));
}


void UMissionBriefingDlg::BuildBriefingChrome()
{
    if (!WidgetTree || !MissionSwitcher) return;
    if (BriefingFrame)
    {
        if (TitleText) TitleText->SetText(NSLOCTEXT("MissionBriefing", "HeaderTitle", "MISSION BRIEFING"));
        RefreshBriefingTabStyles();
        return;
    }

    UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Root && RootSizeBox)
    {
        // Preserve the existing outer sizing when the Blueprint root is not a canvas.
        Root = WidgetTree->ConstructWidget<UCanvasPanel>();
        RootSizeBox->SetContent(Root);
    }
    if (!Root)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] No canvas/size-box host for briefing header"));
        return;
    }

    BriefingFrame = WidgetTree->ConstructWidget<USizeBox>();
    BriefingFrame->SetWidthOverride(1920.f);
    BriefingFrame->SetHeightOverride(1080.f);
    BriefingFrame->SetClipping(EWidgetClipping::ClipToBounds);
    UBorder* FrameBorder = WidgetTree->ConstructWidget<UBorder>();
    FrameBorder->SetPadding(FMargin(0.f));
    FrameBorder->SetBrushColor(FLinearColor::Transparent);
    if (UTexture2D* Panel = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Panel);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        FrameBorder->SetBrush(Brush);
    }
    BriefingFrame->SetContent(FrameBorder);
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->SetStretchDirection(EStretchDirection::Both);
    Fit->SetContent(BriefingFrame);

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    FrameBorder->SetContent(Column);
    auto AddBar = [this, Column](const FLinearColor& Color, const FMargin& BarPadding)->UBorder*
    {
        UBorder* Bar = WidgetTree->ConstructWidget<UBorder>();
        Bar->SetBrushColor(Color);
        Bar->SetPadding(BarPadding);
        Column->AddChildToVerticalBox(Bar);
        return Bar;
    };
    UBorder* Header = AddBar(FLinearColor(0.105f, 0.113f, 0.125f, 1.f), FMargin(18.f, 9.f));
    UHorizontalBox* Titles = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto AddTitle = [this, Titles](UTextBlock*& Text, bool Right)
    {
        if (!Text) Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->RemoveFromParent();
        Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Text->SetFont(MissionUIStyle::GetHeaderFont(38));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
        Text->SetJustification(Right ? ETextJustify::Right : ETextJustify::Left);
        if (UHorizontalBoxSlot* Cell = Titles->AddChildToHorizontalBox(Text))
        {
            Cell->SetSize(FSlateChildSize(Right ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
            Cell->SetVerticalAlignment(VAlign_Center);
            if (Right) Cell->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
        }
    };
    AddTitle(TitleText, false);
    TitleText->SetText(NSLOCTEXT("MissionBriefing", "HeaderTitle", "MISSION BRIEFING"));
    AddTitle(MissionNameText, true);
    Header->SetContent(Titles);

    auto AddStatusRow = [this, AddBar](const TArray<TPair<FString, UTextBlock**>>& Entries)
    {
        UBorder* Bar = AddBar(FLinearColor(0.07f, 0.08f, 0.09f, 1.f), FMargin(12.f, 8.f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        for (const auto& Entry : Entries)
        {
            UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>();
            UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
            Caption->SetText(FText::FromString(Entry.Key));
            Caption->SetFont(MissionUIStyle::GetInfoLabelFont(12));
            Caption->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
            Cell->AddChildToVerticalBox(Caption);
            UTextBlock*& Value = *Entry.Value;
            if (!Value) Value = WidgetTree->ConstructWidget<UTextBlock>();
            Value->RemoveFromParent();
            Value->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            Value->SetFont(MissionUIStyle::GetInfoValueFont(16));
            Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
            Value->SetJustification(ETextJustify::Left);
            Value->SetAutoWrapText(false);
            Cell->AddChildToVerticalBox(Value)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
            UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell);
            CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CellSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
            CellSlot->SetHorizontalAlignment(HAlign_Left);
        }
        Bar->SetContent(Row);
    };
    AddStatusRow({{TEXT("TIME"), &GameTimeText}, {TEXT("CAMPAIGN"), &MissionTPlusText},
        {TEXT("PILOT"), &PlayerNameText}, {TEXT("UNIT"), &CurrentUnitText}, {TEXT("SCORE"), &PlayerScoreText}});
    AddStatusRow({{TEXT("SYSTEM"), &MissionSystemText}, {TEXT("SECTOR"), &MissionSectorText},
        {TEXT("START"), &MissionTimeStartText}, {TEXT("TARGET"), &MissionTimeTargetText}});
    UBorder* Rule = AddBar(FLinearColor(0.20f, 0.21f, 0.23f, 1.f), FMargin(0.f));
    USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>();
    RuleSize->SetHeightOverride(5.f);
    Rule->SetContent(RuleSize);

    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 12.f));
    auto AddTab = [this, Tabs](UButton*& Button, UTextBlock*& Caption, const TCHAR* Label)
    {
        if (!Button) Button = WidgetTree->ConstructWidget<UButton>();
        Button->RemoveFromParent();
        Button->SetVisibility(ESlateVisibility::Visible);
        if (!Caption) Caption = WidgetTree->ConstructWidget<UTextBlock>();
        Caption->RemoveFromParent();
        Caption->SetText(FText::FromString(Label));
        Caption->SetFont(MissionUIStyle::GetHeaderFont(22));
        Caption->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Caption->SetJustification(ETextJustify::Center);
        Button->SetContent(Caption);
        if (UButtonSlot* Cell = Cast<UButtonSlot>(Caption->Slot))
        {
            Cell->SetPadding(FMargin(10.f, 4.f));
            Cell->SetHorizontalAlignment(HAlign_Center);
            Cell->SetVerticalAlignment(VAlign_Center);
        }
        UHorizontalBoxSlot* Cell = Tabs->AddChildToHorizontalBox(Button);
        Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Cell->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    };
    BriefingTabs.Empty();
    for (const TCHAR* Label : { TEXT("SITUATION"), TEXT("PACKAGE"), TEXT("NAVIGATION"), TEXT("WEAPONS") })
    {
        UButton* Button = nullptr;
        UTextBlock* Caption = nullptr;
        AddTab(Button, Caption, Label);
        Button->OnHovered.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleBriefingTabHovered);
        BriefingTabs.Add(Button);
    }
    BriefingTabs[0]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingSituation);
    BriefingTabs[1]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingPackage);
    BriefingTabs[2]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingNavigation);
    BriefingTabs[3]->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::OpenBriefingWeapons);
    auto AddArrow = [this, Tabs](UButton*& Button, const TCHAR* IconPath, const FText& Tip, bool AtStart)
    {
        if (!Button) Button = WidgetTree->ConstructWidget<UButton>();
        Button->RemoveFromParent();
        Button->SetVisibility(ESlateVisibility::Visible);
        Button->SetToolTipText(Tip);
        USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(40.f);
        IconBox->SetHeightOverride(40.f);
        UImage* Icon = WidgetTree->ConstructWidget<UImage>();
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, IconPath))
            Icon->SetBrushFromTexture(Texture, true);
        IconBox->SetContent(Icon);
        Button->SetContent(IconBox);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(IconBox->Slot))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Center);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
            ContentSlot->SetPadding(FMargin(4.f));
        }
        UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Button);
        TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        TabSlot->SetPadding(AtStart ? FMargin(0.f, 0.f, 10.f, 0.f) : FMargin(0.f));
        TabSlot->SetVerticalAlignment(VAlign_Center);
        // Keep the navigation order identical to CmdDlg: back, tabs, forward.
        if (AtStart)
        {
            TArray<UWidget*> Children = Tabs->GetAllChildren();
            Tabs->ClearChildren();
            UHorizontalBoxSlot* BackSlot = Tabs->AddChildToHorizontalBox(Button);
            BackSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
            BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            BackSlot->SetVerticalAlignment(VAlign_Center);
            for (UWidget* Child : Children)
            {
                if (Child == Button) continue;
                UHorizontalBoxSlot* Cell = Tabs->AddChildToHorizontalBox(Child);
                Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Cell->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
            }
        }
    };
    AddArrow(ReturnButton,
        TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_L_256x256.T_Arrow_L_256x256"),
        NSLOCTEXT("MissionBriefing", "BackToCaller", "Cancel and return to the previous screen"), true);
    AddArrow(MissionButton,
        TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"),
        NSLOCTEXT("MissionBriefing", "AcceptArrow", "Accept mission"), false);
    MissionButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    ReturnButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);

    MissionSwitcher->RemoveFromParent();
    UVerticalBoxSlot* Content = Column->AddChildToVerticalBox(MissionSwitcher);
    Content->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->SetHorizontalAlignment(HAlign_Fill);
    Content->SetVerticalAlignment(VAlign_Fill);
    // All live bindings were moved first; hide only the now obsolete Blueprint frame.
    for (UWidget* Child : Root->GetAllChildren())
        if (Child) Child->SetVisibility(ESlateVisibility::Collapsed);
    if (MenuButtonContainer) MenuButtonContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (UCanvasPanelSlot* Main = Root->AddChildToCanvas(Fit))
    {
        Main->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Main->SetOffsets(FMargin(0.f));
        Main->SetZOrder(20);
    }
    RefreshBriefingTabStyles();
}

void UMissionBriefingDlg::RefreshBriefingTabStyles()
{
    // Same texture states as CmdDlg::RefreshOperationsTabStyles.
    auto Style = [](UButton* Button, const TCHAR* Color)
    {
        if (!Button) return;
        FButtonStyle Style = Button->GetStyle();
        auto Brush = [Color](const TCHAR* State)->FSlateBrush
        {
            const FString Path = FString::Printf(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01%s_%s.T_Btn01%s_%s"), Color, State, Color, State);
            FSlateBrush Result;
            if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path))
            {
                Result.SetResourceObject(Texture);
                Result.DrawAs = ESlateBrushDrawType::Image;
                Result.ImageSize = FVector2D(64.f, 64.f);
            }
            else
            {
                Result.DrawAs = ESlateBrushDrawType::Box;
                Result.TintColor = FSlateColor(FLinearColor(0.1f, 0.2f, 0.3f, 1.f));
            }
            return Result;
        };
        Style.Normal = Brush(TEXT("n"));
        Style.Hovered = Brush(TEXT("p"));
        Style.Pressed = Style.Hovered;
        Style.Disabled = Style.Normal;
        Button->SetStyle(Style);
    };
    for (int32 Index = 0; Index < BriefingTabs.Num(); ++Index)
        Style(BriefingTabs[Index], Index == static_cast<int32>(CurrentMode) ? TEXT("Blue") : TEXT("Cyan"));
    if (BriefingFrame)
    {
        Style(MissionButton, TEXT("Green"));
        Style(ReturnButton, TEXT("Red"));
    }
}

void UMissionBriefingDlg::HandleBriefingTabHovered()
{
    if (UGameInstance* GI = GetGameInstance())
        if (UStarshatterAudioSubsystem* Audio = GI->GetSubsystem<UStarshatterAudioSubsystem>())
            Audio->PlayHoverSound(this);
}

void UMissionBriefingDlg::ActivateBriefingTab(EMissionBriefingMode Mode)
{
    if (UGameInstance* GI = GetGameInstance())
        if (UStarshatterAudioSubsystem* Audio = GI->GetSubsystem<UStarshatterAudioSubsystem>())
            Audio->PlayAcceptSound(this);
    SetMode(Mode);
}

void UMissionBriefingDlg::OpenBriefingSituation() { ActivateBriefingTab(EMissionBriefingMode::SIT); }
void UMissionBriefingDlg::OpenBriefingPackage() { ActivateBriefingTab(EMissionBriefingMode::PKG); }
void UMissionBriefingDlg::OpenBriefingNavigation() { ActivateBriefingTab(EMissionBriefingMode::NAV); }
void UMissionBriefingDlg::OpenBriefingWeapons() { ActivateBriefingTab(EMissionBriefingMode::WEP); }

void UMissionBriefingDlg::BuildMenuButtons()
{
    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
    {
        return;
    }

    UVerticalBox* MenuVBox = Cast<UVerticalBox>(MenuButtonContainer);
    if (!MenuVBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefingDlg] MenuButtonContainer is not a VerticalBox"));
        return;
    }

    MenuVBox->ClearChildren();
    AllMenuButtons.Empty();

    for (const FString& Item : MenuItems)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        NewButton->SetLayoutMode(ELayoutMode::FillWidth);
        NewButton->SetButtonSize(0.f, bModernBriefingPresentation ? FMath::Clamp(BriefingTabHeight, 32.f, 80.f) : 42.f);
        NewButton->SetLabelFontSizeValue(bModernBriefingPresentation ? FMath::Clamp(BriefingTabFontSize, 12, 24) : 16);
        NewButton->SetMenuOption(Item);
        NewButton->SetButtonText(FText::FromString(Item).ToUpper());

        if (UVerticalBoxSlot* VBoxSlot = MenuVBox->AddChildToVerticalBox(NewButton))
        {
            VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, bModernBriefingPresentation ? 6.f : 0.f));
            VBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            VBoxSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (bModernBriefingPresentation)
        {
            FText Label = FText::FromString(Item);
            if (Item == TEXT("SIT")) Label = NSLOCTEXT("MissionBriefing", "SituationTab", "SITUATION");
            else if (Item == TEXT("PKG")) Label = NSLOCTEXT("MissionBriefing", "PackageTab", "PACKAGE");
            else if (Item == TEXT("NAV")) Label = NSLOCTEXT("MissionBriefing", "NavigationTab", "NAVIGATION");
            else if (Item == TEXT("WEP")) Label = NSLOCTEXT("MissionBriefing", "WeaponsTab", "WEAPONS");

            // SetButtonText also changes MenuOption. Update only the visual label
            // here so the legacy SIT/PKG/NAV/WEP routing and selection still work.
            if (NewButton->Label) NewButton->Label->SetText(Label);
            NewButton->SetToolTipText(Label);
            NewButton->SelectedColor = FLinearColor(BriefingAccentColor.R * 0.25f,
                BriefingAccentColor.G * 0.25f, BriefingAccentColor.B * 0.25f, 1.f);
            NewButton->HoveredColor = FLinearColor(0.12f, 0.19f, 0.26f, 1.f);
            NewButton->SetSelected(false);
        }

        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);

        NewButton->OnHovered.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);
        NewButton->OnHovered.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }
}

void UMissionBriefingDlg::InitializeSubPanels()
{
    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
		MissionSituationPanel->SetManager(MissionScreen);
    }

    if (MissionPackagePanel)
    {
        MissionPackagePanel->SetParentDlg(this);
        MissionPackagePanel->SetManager(MissionScreen);;
    }

    if (MissionNavPanel)
    {
        MissionNavPanel->SetParentDlg(this);
		MissionNavPanel->SetManager(MissionScreen);
    }

    if (MissionWepPanel)
    {
        MissionWepPanel->SetParentDlg(this);
		MissionWepPanel->SetManager(MissionScreen);
    }
}

void UMissionBriefingDlg::SetStandaloneMission(Mission* InMission, Campaign* InRepository)
{
    bStandaloneMission = (InMission != nullptr);
    StandaloneRepository = bStandaloneMission ? InRepository : nullptr;
    MissionPtr = InMission;
    CampaignPtr = StandaloneRepository;
    InfoPtr = nullptr;
}

void UMissionBriefingDlg::ClearStandaloneMission()
{
    bStandaloneMission = false;
    StandaloneRepository = nullptr;
    MissionPtr = nullptr;
    CampaignPtr = nullptr;
    InfoPtr = nullptr;
}

void UMissionBriefingDlg::ShowMsnDlg()
{
    CloseEmptySectorPreview();
    PackageIndex = -1;
    if (bStandaloneMission)
    {
        // The caller owns the legacy Mission; no story campaign selection or
        // story campaign mission state is changed here.
        CampaignPtr = StandaloneRepository;
        UE_LOG(LogTemp, Log, TEXT("[MissionBriefingDlg] Standalone Mission=%p Repository=%p"),
            MissionPtr, CampaignPtr);
    }
    else
    {
        // Preserve the original campaign-driven briefing behavior.
        CampaignPtr = Campaign::GetCampaign();
        MissionPtr = nullptr;
        const int32 MissionId = CampaignPtr ? CampaignPtr->GetMissionId() : -1;
        if (CampaignPtr && MissionId > 0)
            MissionPtr = CampaignPtr->GetMission(MissionId);
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Campaign=%p MissionId=%d"), CampaignPtr, MissionId);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionBriefingDlg] ShowMsnDlg: CampaignPtr=%s MissionPtr=%s"),
        CampaignPtr ? TEXT("VALID") : TEXT("NULL"),
        MissionPtr ? TEXT("VALID") : TEXT("NULL"));

    if (MissionPtr)
    {
        const FString NameStr = ANSI_TO_TCHAR(MissionPtr->GetName());
        const FString DescStr = ANSI_TO_TCHAR(MissionPtr->GetDescription());
        const FString ObjStr = ANSI_TO_TCHAR(MissionPtr->GetObjective());
        const FString SitStr = ANSI_TO_TCHAR(MissionPtr->GetSituation());
        const FString SysStr = ANSI_TO_TCHAR(MissionPtr->GetSystem());
        const FString RegionStr = ANSI_TO_TCHAR(MissionPtr->GetRegion());
        const bool bIsScripted = MissionPtr->IsScripted();

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Mission Data: Name='%s' Desc='%s' Obj='%s' Sit='%s' Sys='%s' Region='%s' Scripted=%s Mission=%p Campaign=%p"),
            *NameStr,
            *DescStr,
            *ObjStr,
            *SitStr,
            *SysStr,
            *RegionStr,
            bIsScripted ? TEXT("true") : TEXT("false"),
            MissionPtr,
            CampaignPtr);

        if (!bIsScripted && !bStandaloneMission)
        {
            const bool bNeedsSitrep =
                SitStr.IsEmpty() ||
                SitStr.Equals(TEXT("Unknown"), ESearchCase::IgnoreCase) ||
                SitStr.Equals(TEXT("Mission Unknown"), ESearchCase::IgnoreCase) ||
                SitStr.Equals(TEXT("Unknown Mission"), ESearchCase::IgnoreCase) ||
                SitStr.Contains(TEXT("unknown"), ESearchCase::IgnoreCase);

            if (bNeedsSitrep)
            {
                if (CampaignPtr && MissionPtr->GetStarSystem())
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Generating situation report for '%s'"),
                        *NameStr);

                    CampaignSituationReport Sitrep(CampaignPtr, MissionPtr);
                    Sitrep.GenerateSituationReport();

                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Post-Sitrep: Situation='%s'"),
                        ANSI_TO_TCHAR(MissionPtr->GetSituation()));
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[MissionBriefingDlg] Skipping sitrep for '%s': CampaignPtr=%s StarSystem=%s"),
                        *NameStr,
                        CampaignPtr ? TEXT("VALID") : TEXT("NULL"),
                        MissionPtr->GetStarSystem() ? TEXT("VALID") : TEXT("NULL"));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionBriefingDlg] Scripted mission '%s': skipping CampaignSituationReport"),
                *NameStr);
        }
    }

    RefreshHeader();

    const bool bHasMission = (MissionPtr != nullptr);
    for (UButton* Tab : BriefingTabs)
        if (Tab) Tab->SetIsEnabled(bHasMission);

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetIsEnabled(bHasMission);
        }
    }

    if (MissionButton)
    {
        MissionButton->SetIsEnabled(bHasMission);
    }

    if (ReturnButton)
    {
        ReturnButton->SetIsEnabled(true);
    }

    if (MissionButtonText)
    {
        MissionButtonText->SetText(FText::FromString(TEXT("ACCEPT")));
    }

    if (ReturnButtonText)
    {
        ReturnButtonText->SetText(FText::FromString(TEXT("CANCEL")));
    }

    SetMode(EMissionBriefingMode::SIT);

    if (MissionSituationPanel)
    {
        MissionSituationPanel->SetParentDlg(this);
        MissionSituationPanel->RefreshFromMission();
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] MissionSituationPanel widget is null"));
    }
}

void UMissionBriefingDlg::RefreshHeader()
{
    if (MissionNameText)
    {
        MissionNameText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetName())
            : FText::FromString(TEXT("NO MISSION")));
    }

    if (MissionSystemText)
    {
        MissionSystemText->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            const char* SystemName = MissionPtr->GetSystem();

            if (SystemName && SystemName[0])
            {
                MissionSystemText->SetText(ToTextFromUtf8(SystemName));
            }
            else if (StarSystem* Sys = MissionPtr->GetStarSystem())
            {
                MissionSystemText->SetText(ToTextFromUtf8(Sys->GetName()));
            }
        }
    }

    if (MissionSectorText)
    {
        MissionSectorText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetRegion())
            : FText::GetEmpty());
    }

    if (MissionTimeStartText)
    {
        if (MissionPtr)
        {
            char Buf[32] = { 0 };
            FormatDayTime(Buf, MissionPtr->GetStart());
            MissionTimeStartText->SetText(ToTextFromUtf8(Buf));
        }
        else
        {
            MissionTimeStartText->SetText(FText::GetEmpty());
        }
    }

    if (MissionTimeTargetText && MissionTimeTargetLabelText)
    {
        if (bShowTimeOnTarget)
        {
            const int32 TimeOnTarget = CalcTimeOnTarget();
            if (TimeOnTarget > 0)
            {
                char Buf[32] = { 0 };
                FormatDayTime(Buf, TimeOnTarget);
                MissionTimeTargetText->SetText(ToTextFromUtf8(Buf));
                MissionTimeTargetLabelText->SetText(FText::FromString(TEXT("TARGET:")));
            }
            else
            {
                MissionTimeTargetText->SetText(FText::GetEmpty());
                MissionTimeTargetLabelText->SetText(FText::GetEmpty());
            }
        }
        else
        {
            MissionTimeTargetText->SetText(FText::GetEmpty());
            MissionTimeTargetLabelText->SetText(FText::GetEmpty());
        }
    }
}

void UMissionBriefingDlg::SetMode(EMissionBriefingMode NewMode)
{
    CurrentMode = NewMode;

    if (MissionSwitcher)
    {
        MissionSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentMode));
    }

    RefreshMenuSelection();
    RefreshBriefingTabStyles();

    switch (CurrentMode)
    {
    case EMissionBriefingMode::SIT:
        if (MissionSituationPanel)
        {
            MissionSituationPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::PKG:
        if (MissionPackagePanel)
        {
            MissionPackagePanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::NAV:
        if (MissionNavPanel)
        {
            MissionNavPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::WEP:
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionBriefingDlg] Enter WEP: this=%p MissionPtr=%p MissionWepPanel=%p"),
            this,
            GetMissionPtr(),
            MissionWepPanel);
        
        if (MissionWepPanel)
        {
            MissionWepPanel->RefreshFromMission();
        }
        break;

    default:
        break;
    }
}

void UMissionBriefingDlg::RefreshMenuSelection()
{
    const int32 ModeIndex = static_cast<int32>(CurrentMode);
    const FString ActiveLabel = MenuItems.IsValidIndex(ModeIndex)
        ? MenuItems[ModeIndex]
        : FString();

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }
}

void UMissionBriefingDlg::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayAcceptSound(this);
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("SIT"))
    {
        SetMode(EMissionBriefingMode::SIT);
    }
    else if (Option == TEXT("PKG"))
    {
        SetMode(EMissionBriefingMode::PKG);
    }
    else if (Option == TEXT("NAV"))
    {
        SetMode(EMissionBriefingMode::NAV);
    }
    else if (Option == TEXT("WEP"))
    {
        SetMode(EMissionBriefingMode::WEP);
    }
}

void UMissionBriefingDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton)
    {
        return;
    }

    if (UStarshatterAudioSubsystem* AudioSS =
        GetGameInstance()->GetSubsystem<UStarshatterAudioSubsystem>())
    {
        AudioSS->PlayHoverSound(this);
    }
}

int32 UMissionBriefingDlg::CalcTimeOnTarget() const
{
    if (!MissionPtr)
    {
        return 0;
    }

    const auto& Elements = MissionPtr->GetElements();
    if (Elements.size() == 0)
    {
        return 0;
    }

    MissionElement* Element = Elements[0];
    if (!Element)
    {
        return 0;
    }

    FVector Loc(
        Element->GetLocation().X,
        Element->GetLocation().Y,
        Element->GetLocation().Z
    );

    Swap(Loc.Y, Loc.Z);

    int32 MissionTime = MissionPtr->GetStart();

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        const int32 Action = static_cast<int32>(NavPt->GetAction());

        FVector NavLoc(
            NavPt->GetLocation().X,
            NavPt->GetLocation().Y,
            NavPt->GetLocation().Z
        );

        const double Dist = FVector::Dist(Loc, NavLoc);

        const double Speed = NavPt->GetSpeed();
        const int32 ETR = (Speed > 0.0)
            ? static_cast<int32>(Dist / Speed)
            : static_cast<int32>(Dist / 500.0);

        MissionTime += ETR;
        Loc = NavLoc;

        if (Action >= static_cast<int32>(EInstruction::Escort))
        {
            return MissionTime;
        }
    }

    return 0;
}

void UMissionBriefingDlg::HandleSimulationUnloading()
{
    // This path is also used by debrief, replacement missions and shutdown.
    CloseEmptySectorPreview(true);
}

void UMissionBriefingDlg::CloseEmptySectorPreview(bool bPreserveSimulation)
{
    if (bClosingMissionPreview) return;
    TGuardValue<bool> ClosingGuard(bClosingMissionPreview, true);
    // Close callbacks/input before their Ship* and MissionEvent* sources die.
    DisableMissionMenuInput();
    if (MissionCameraRig) MissionCameraRig->Reset(GetOwningPlayer());
    MissionPlayerCameraActor.Reset();
    MissionPlayerCameraDistance = 0;
    bMissionTargetInspection = false;
    if (bLiveMissionStarted && GetGameInstance())
    {
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
        {
            Runtime->SetPaused(true);
            Runtime->EndMissionPresentation();
        }
        if (!bPreserveSimulation)
            if (Sim* Simulation = Sim::GetSim())
                if (Simulation->GetMission() == MissionPtr) Simulation->UnloadMission();
        if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StopMissionRun();
        bLiveMissionStarted = false;
        if (!bPreserveSimulation)
            if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
                Runtime->SetGameMode(EGameMode::CMPN);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MissionTargetCameraTimer);
        if (MissionTargetOverlay.IsValid() && World->GetGameViewport())
            World->GetGameViewport()->RemoveViewportWidgetContent(MissionTargetOverlay.ToSharedRef());
    }
    bLiveMissionStarted = false;
    MissionTargetOverlay.Reset();
    SelectedMissionTarget.Reset();
    SelectedMissionTargetName = FText::GetEmpty();
    SelectedMissionTargetLocalBounds = FBox(ForceInit);
    MissionTitleRevealTime = -1.0;
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionCameraReadyChecks = 0;
    MissionCameraStableSince = -1.0;
    if (IsValid(MissionPreviewCamera))
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (PC->GetViewTarget() == MissionPreviewCamera.Get())
            {
                AActor* RestoreTarget = PreviousMissionViewTarget.Get();
                if (!RestoreTarget) RestoreTarget = PC;
                PC->SetViewTarget(RestoreTarget);
            }
        }
        MissionPreviewCamera->Destroy();
        MissionPreviewCamera = nullptr;
    }
    PreviousMissionViewTarget.Reset();
    if (MissionSystemLevel)
    {
        MissionSystemLevel->OnLevelShown.RemoveDynamic(
            this, &UMissionBriefingDlg::HandleMissionSystemLevelShown);
        MissionSystemLevel->SetShouldBeVisible(false);
        MissionSystemLevel->SetShouldBeLoaded(false);
        MissionSystemLevel->SetIsRequestingUnloadAndRemoval(true);
        MissionSystemLevel = nullptr;
    }
    MissionSystemPackage.Empty();
    if (MissionSceneCover.IsValid())
    {
        if (UWorld* World = GetWorld())
            if (UGameViewportClient* Viewport = World->GetGameViewport())
                Viewport->RemoveViewportWidgetContent(MissionSceneCover.ToSharedRef());
        MissionSceneCover.Reset();
    }
    if (EmptySectorOverlay.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (UGameViewportClient* Viewport = World->GetGameViewport())
                Viewport->RemoveViewportWidgetContent(EmptySectorOverlay.ToSharedRef());
        }
        EmptySectorOverlay.Reset();
    }
    for (const TWeakObjectPtr<AActor>& Actor : PreviewHiddenActors)
    {
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    }
    PreviewHiddenActors.Empty();
}

void UMissionBriefingDlg::HandleMissionSystemLevelShown()
{
    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World || !PC || !MissionSystemLevel) return;

    // Builders can spawn into the persistent level: also check the owner chain.
    UStaticMeshComponent* StarMesh = nullptr;
    for (TActorIterator<AActor> It(World); It && !StarMesh; ++It)
    {
        bool bBelongsToMission = false;
        for (AActor* Owner = *It; Owner; Owner = Owner->GetOwner())
        {
            if (Owner->GetLevel() == MissionSystemLevel->GetLoadedLevel())
            {
                bBelongsToMission = true;
                break;
            }
        }
        if (!bBelongsToMission) continue;
        TArray<UStaticMeshComponent*> Meshes;
        It->GetComponents<UStaticMeshComponent>(Meshes, true);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            if (IsValid(Mesh) && Mesh->GetFName() == FName(TEXT("SM_Star")) &&
                Mesh->GetStaticMesh())
            {
                StarMesh = Mesh;
                break;
            }
        }
    }
    if (!StarMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionBriefing] No mission SM_Star mesh to frame"));
        return;
    }

    // OnLevelShown runs before this level's first rendered frame. Hide the
    // complete star actor tree, including its corona and particle effects.
    AActor* SunRoot = StarMesh->GetOwner();
    while (SunRoot && SunRoot->GetParentActor()) SunRoot = SunRoot->GetParentActor();
    TArray<AActor*> SunActors;
    if (SunRoot)
    {
        SunActors.Add(SunRoot);
        SunRoot->GetAllChildActors(SunActors, true);
        SunActors.AddUnique(SunRoot);
    }
    for (AActor* Actor : SunActors)
    {
        if (IsValid(Actor) && !Actor->IsHidden())
        {
            MissionSunHiddenActors.AddUnique(Actor);
            Actor->SetActorHiddenInGame(true);
        }
    }

    FBoxSphereBounds Bounds = StarMesh->CalcBounds(StarMesh->GetComponentTransform());
    FString FocusName = TEXT("primary star");
    bool bFocusPlanet = false;
    const FString RegionName = MissionPtr
        ? ToTextFromUtf8(MissionPtr->GetRegion()).ToString().TrimStartAndEnd() : FString();
    for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
    {
        if (It->GetLevel() != MissionSystemLevel->GetLoadedLevel()) continue;
        FSpawnedSystemRegion Region;
        if (!It->GetRegionByName(RegionName, Region) &&
            !It->GetRegionByName(RegionName + TEXT("_REGION"), Region)) continue;
        for (const FSpawnedSystemBody& Body : It->GetSpawnedBodies())
        {
            if (Body.bIsOrbit || !IsValid(Body.Actor)) continue;
            if (!Body.BodyName.Equals(Region.AnchorBodyName, ESearchCase::IgnoreCase) &&
                Body.Actor != Region.ParentActor) continue;
            FocusName = Body.BodyName;
            bFocusPlanet = Body.ParentActor != nullptr || Body.bIsMoon;
            if (bFocusPlanet)
            {
                // Use the physical surface, excluding orbit rings and effects.
                UStaticMeshComponent* Surface = nullptr;
                if (APlanetActor* Planet = Cast<APlanetActor>(Body.Actor.Get()))
                    Surface = Planet->GetPlanetMeshComponent();
                if (Surface && Surface->GetStaticMesh())
                    Bounds = Surface->CalcBounds(Surface->GetComponentTransform());
                else
                {
                    const double R = FMath::Max(1.0f, Body.VisualRadiusUnits);
                    Bounds = FBoxSphereBounds(Body.Actor->GetActorLocation(), FVector(R), R);
                }
                TArray<AActor*> FocusActors;
                Body.Actor->GetAllChildActors(FocusActors, true);
                FocusActors.AddUnique(Body.Actor.Get());
                for (AActor* Actor : FocusActors)
                {
                    if (IsValid(Actor) && !Actor->IsHidden())
                    {
                        MissionSunHiddenActors.AddUnique(Actor);
                        Actor->SetActorHiddenInGame(true);
                    }
                }
            }
            break;
        }
        break;
    }
    if (!StartLiveMission())
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Live mission startup failed; returning to briefing"));
        CloseEmptySectorPreview();
        if (manager) manager->ShowMissionDlg();
        return;
    }
    const double Radius = Bounds.BoxExtent.GetMax();
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Radius <= KINDA_SMALL_NUMBER || Width <= 0 || Height <= 0) return;

    if (!IsValid(MissionPreviewCamera))
    {
        MissionPreviewCamera = World->SpawnActor<ACameraActor>();
        if (!MissionPreviewCamera) return;
        PreviousMissionViewTarget = PC->GetViewTarget();
    }
    UCameraComponent* Camera = MissionPreviewCamera->GetCameraComponent();
    Camera->SetFieldOfView(60.0f);
    Camera->bConstrainAspectRatio = false;
    Camera->bOverrideAspectRatioAxisConstraint = true;
    Camera->AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;
    const double FocalPixels = Width / (2.0 * FMath::Tan(FMath::DegreesToRadians(30.0)));
    const double DiameterPixels = bFocusPlanet ? 128.0 : 64.0;
    const double Ratio = FocalPixels / (DiameterPixels * 0.5);
    const double Distance = Radius * FMath::Sqrt(1.0 + Ratio * Ratio);
    const FRotator Rotation = PC->PlayerCameraManager
        ? PC->PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator;
    MissionPreviewCamera->SetActorLocationAndRotation(
        Bounds.Origin - Rotation.Vector() * Distance, Rotation);
    PC->SetViewTarget(MissionPreviewCamera.Get());
    // Position the flight camera before the cover's existing warmup/reveal checks.
    bMissionTargetInspection = false;
    MissionPlayerCameraActor.Reset();
    MissionPlayerCameraSearchTime = 0;
    MissionPlayerCameraDistance = 0;
    MissionTargetZoomDistance = 1.0;
    MissionTargetLastUpdate = World->GetTimeSeconds();
    UpdateMissionTargetCamera();
    FTimerManagerTimerParameters FlightCameraTimerParameters;
    FlightCameraTimerParameters.bLoop = true;
    FlightCameraTimerParameters.bMaxOncePerFrame = true;
    World->GetTimerManager().SetTimer(MissionTargetCameraTimer, this,
        &UMissionBriefingDlg::UpdateMissionTargetCamera, 0.000001f, FlightCameraTimerParameters);
    // Warm up all visuals behind the cover, including the star's effects.
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionCameraReadyChecks = 0;
    MissionCameraStableSince = -1.0;
    World->GetTimerManager().SetTimer(MissionSunRevealTimer, this,
        &UMissionBriefingDlg::RevealMissionSunWhenCameraReady, 0.05f, true);
    if (PC->PlayerCameraManager) PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Framed region object %s at distance %.2f"),
        *FocusName, Distance);
}

void UMissionBriefingDlg::RevealMissionSunWhenCameraReady()
{
    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World) return;
    if (!MissionSystemLevel || !IsValid(MissionPreviewCamera))
    {
        World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
        return;
    }
    const bool bReady = PC && PC->PlayerCameraManager &&
        PC->GetViewTarget() == MissionPreviewCamera.Get() &&
        PC->PlayerCameraManager->GetCameraLocation().Equals(
            MissionPreviewCamera->GetActorLocation(), 1.0f) &&
        PC->PlayerCameraManager->GetCameraRotation().Equals(
            MissionPreviewCamera->GetActorRotation(), 0.1f) &&
        FMath::IsNearlyEqual(PC->PlayerCameraManager->GetFOVAngle(),
            MissionPreviewCamera->GetCameraComponent()->FieldOfView, 0.1f);
    MissionCameraReadyChecks = bReady ? MissionCameraReadyChecks + 1 : 0;
    // Allow 1.5 seconds of stable camera time for the fully rendered scene to settle.
    // Measure elapsed time, since timer callbacks can catch up within one frame.
    if (!bReady)
    {
        MissionCameraStableSince = -1.0;
        return;
    }
    const double Now = World->GetRealTimeSeconds();
    if (MissionCameraStableSince < 0.0) MissionCameraStableSince = Now;
    if (MissionCameraReadyChecks < 2 || Now - MissionCameraStableSince < 1.5) return;
    if (bLiveMissionStarted)
    {
        if (auto* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>()) Timer->StartMissionRun(true);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(false);
    }
    for (const TWeakObjectPtr<AActor>& Actor : MissionSunHiddenActors)
        if (Actor.IsValid()) Actor->SetActorHiddenInGame(false);
    MissionSunHiddenActors.Empty();
    MissionTitleRevealTime = World->GetTimeSeconds();
    if (MissionSceneCover.IsValid())
    {
        if (UGameViewportClient* Viewport = World->GetGameViewport())
            Viewport->RemoveViewportWidgetContent(MissionSceneCover.ToSharedRef());
        MissionSceneCover.Reset();
    }
    World->GetTimerManager().ClearTimer(MissionSunRevealTimer);
}

bool UMissionBriefingDlg::EnableMissionMenuInput()
{
    if (Sim* Simulation = Sim::GetSim()) {
        Simulation->OnMissionUnloading.RemoveAll(this);
        Simulation->OnMissionUnloading.AddUObject(this, &UMissionBriefingDlg::HandleSimulationUnloading);
    }
    APlayerController* PC = GetOwningPlayer();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || !manager) return false;
    MissionMenuContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Game.IMC_Game"));
    UInputAction* Action = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Quit.IA_Quit"));
    if (!MissionMenuContext || !Action) return false;

    // BaseInput.ini binds F1-F5 to viewmode commands outside Enhanced Input.
    // Remove only those display-mode commands from this player's live bindings.
    // This is idempotent and also covers shifted variants of the same keys.
    if (UPlayerInput* PlayerInput = PC->PlayerInput)
    {
        const int32 Removed = PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Binding)
        {
            const bool bCameraKey = Binding.Key == EKeys::F1 || Binding.Key == EKeys::F2 ||
                Binding.Key == EKeys::F3 || Binding.Key == EKeys::F4 || Binding.Key == EKeys::F5;
            return bCameraKey && Binding.Command.TrimStart().StartsWith(TEXT("viewmode "), ESearchCase::IgnoreCase);
        });
        UE_LOG(LogTemp, Log, TEXT("[MissionCamera] Removed %d UE F1-F5 viewmode debug bindings."), Removed);
    }

    MissionMenuInput = NewObject<UEnhancedInputComponent>(PC);
    MissionMenuInput->RegisterComponent();
    MissionMenuInput->Priority = 1000;

    MissionStrafeAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Strafe.IA_Strafe"));
    if (MissionStrafeAction && MissionStrafeAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionStrafe);
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionStrafe);
        MissionMenuInput->BindAction(MissionStrafeAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionStrafe);
    }

    MissionForwardThrustAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_ForwardThrust.IA_ForwardThrust"));
    if (MissionForwardThrustAction && MissionForwardThrustAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionForwardThrust);
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionForwardThrust);
        MissionMenuInput->BindAction(MissionForwardThrustAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionForwardThrust);
    }

    MissionVerticalThrustAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_VerticalThrust.IA_VerticalThrust"));
    if (MissionVerticalThrustAction && MissionVerticalThrustAction->ValueType==EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionVerticalThrust);
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionVerticalThrust);
        MissionMenuInput->BindAction(MissionVerticalThrustAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionVerticalThrust);
    }

    MissionAugmenterAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Augmenter.IA_Augmenter"));
    if (MissionAugmenterAction && MissionAugmenterAction->ValueType==EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Triggered,this,&UMissionBriefingDlg::OnMissionAugmenter);
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Completed,this,&UMissionBriefingDlg::ReleaseMissionAugmenter);
        MissionMenuInput->BindAction(MissionAugmenterAction.Get(),ETriggerEvent::Canceled,this,&UMissionBriefingDlg::ReleaseMissionAugmenter);
    }

    MissionShieldsUpAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsUp.IA_ShieldsUp"));
    if (MissionShieldsUpAction && MissionShieldsUpAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsUpAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsUp);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsUp must be Boolean."));

    MissionShieldsDownAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsDown.IA_ShieldsDown"));
    if (MissionShieldsDownAction && MissionShieldsDownAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsDownAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsDown);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsDown must be Boolean."));

    MissionShieldsFullAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsFull.IA_ShieldsFull"));
    if (MissionShieldsFullAction && MissionShieldsFullAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsFullAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsFull);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsFull must be Boolean."));

    MissionShieldsZeroAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ShieldsZero.IA_ShieldsZero"));
    if (MissionShieldsZeroAction && MissionShieldsZeroAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionShieldsZeroAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionShieldsZero);
    else UE_LOG(LogTemp, Warning, TEXT("[ShieldInput] IA_ShieldsZero must be Boolean."));

    MissionPitchAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Pitch.IA_Pitch"));
    if (MissionPitchAction && MissionPitchAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionPitch);
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionPitch);
        MissionMenuInput->BindAction(MissionPitchAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionPitch);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Pitch must be Axis1D in /Game/Input."));

    MissionYawAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Yaw.IA_Yaw"));
    if (MissionYawAction && MissionYawAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionYaw);
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionYaw);
        MissionMenuInput->BindAction(MissionYawAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionYaw);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Yaw must be Axis1D in /Game/Input."));

    MissionRollAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Roll.IA_Roll"));
    if (MissionRollAction && MissionRollAction->ValueType == EInputActionValueType::Axis1D)
    {
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionRoll);
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionRoll);
        MissionMenuInput->BindAction(MissionRollAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionRoll);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[FlightInput] IA_Roll must be Axis1D in /Game/Input."));

    MissionThrottleAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Throttle.IA_Throttle"));
    if (MissionThrottleAction && MissionThrottleAction->ValueType == EInputActionValueType::Axis1D)
        MissionMenuInput->BindAction(MissionThrottleAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleStep);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_Throttle (Axis1D)."));

    MissionThrottleZeroAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ThrottleZero.IA_ThrottleZero"));
    if (MissionThrottleZeroAction && MissionThrottleZeroAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionThrottleZeroAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleZero);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_ThrottleZero (Boolean)."));

    MissionThrottleFullAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ThrottleFull.IA_ThrottleFull"));
    if (MissionThrottleFullAction && MissionThrottleFullAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(MissionThrottleFullAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::OnMissionThrottleFull);
    else UE_LOG(LogTemp, Warning, TEXT("[ThrottleInput] Missing or wrong value type: IA_ThrottleFull (Boolean)."));

    GearToggleAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_GearToggle.IA_GearToggle"));
    if (GearToggleAction && GearToggleAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(GearToggleAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleMissionGear);
    else UE_LOG(LogTemp,Warning,TEXT("[GearInput] Missing Boolean IA_GearToggle."));

    EngineeringAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_EngineeringPanel.IA_EngineeringPanel"));
    if (EngineeringAction && EngineeringAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(EngineeringAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleEngineering);
    else UE_LOG(LogTemp,Warning,TEXT("[Engineering] IA_EngineeringPanel must be Boolean and mapped in IMC_FighterHUD."));

    RadioMenuAction = LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_RadioMenu.IA_RadioMenu"));
    if (RadioMenuAction) MissionMenuInput->BindAction(RadioMenuAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleFighterRadio);
    RadioChoiceContext = NewObject<UInputMappingContext>(this,NAME_None,RF_Transient);
    RadioChoiceActions.Reset();
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Zero);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio0);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::One);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio1);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Two);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio2);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Three);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio3);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Four);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio4);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Five);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio5);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Six);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio6);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Seven);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio7);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Eight);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio8);
    }
    {
        UInputAction* Choice=NewObject<UInputAction>(this,NAME_None,RF_Transient);
        Choice->ValueType=EInputActionValueType::Boolean;
        Choice->bConsumeInput=true;
        RadioChoiceActions.Add(Choice);
        RadioChoiceContext->MapKey(Choice,EKeys::Nine);
        MissionMenuInput->BindAction(Choice,ETriggerEvent::Started,this,&UMissionBriefingDlg::SelectFighterRadio9);
    }


    // Transient context remains for the wheel and existing HUD shortcuts only.
    MissionTargetContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
    // Use the editor-authored actions and IMC mappings, including chord triggers.
    PreviousMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_PreviousTarget.IA_PreviousTarget"));
    NextMissionTargetAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NextTarget.IA_NextTarget"));
    bPreviousTargetHeld = bNextTargetHeld = false;
    if (PreviousMissionTargetAction && PreviousMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::PreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
        MissionMenuInput->BindAction(PreviousMissionTargetAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleasePreviousMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_PreviousTarget."));
    if (NextMissionTargetAction && NextMissionTargetAction->ValueType == EInputActionValueType::Boolean)
    {
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Triggered, this, &UMissionBriefingDlg::NextMissionTarget);
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseNextMissionTarget);
        MissionMenuInput->BindAction(NextMissionTargetAction.Get(), ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseNextMissionTarget);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[MissionInput] Missing Boolean IA_NextTarget."));
    MissionMenuInput->BindAction(Action, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionMenu);
    // Use the transient wheel context so asset paths, Hold triggers,
    // or boolean action settings cannot suppress the one-frame wheel pulse.
    MissionTargetZoomAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    MissionTargetZoomAction->ValueType = EInputActionValueType::Axis1D;
    MissionTargetZoomAction->bConsumeInput = true;
    MissionTargetContext->MapKey(MissionTargetZoomAction.Get(), EKeys::MouseWheelAxis);
    MissionMenuInput->BindAction(MissionTargetZoomAction.Get(), ETriggerEvent::Triggered,
        this, &UMissionBriefingDlg::OnTargetZoom);
    FighterHUDToggleAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    FighterHUDToggleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterHUDToggleAction.Get(), EKeys::H);
    MissionMenuInput->BindAction(FighterHUDToggleAction.Get(), ETriggerEvent::Started,
        this, &UMissionBriefingDlg::ToggleFighterHUD);
    bFighterHUDVisible = true;
    if (!FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> HUDOwner(this);
        FighterHUDPanels = SNew(SFighterHUDPanels).ShowPanels_Lambda([HUDOwner]()
        {
            return HUDOwner.IsValid() && HUDOwner->bLiveMissionStarted &&
                HUDOwner->bFighterHUDVisible && !HUDOwner->MissionSceneCover.IsValid() &&
                !HUDOwner->bMissionControlsOpen &&
                !(HUDOwner->MissionQuitMenu && HUDOwner->MissionQuitMenu->IsMenuShown());
        });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDPanels.ToSharedRef(), 800);
    }
    FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD"));
    // Compatibility with the existing asset's current name.
    if (!FighterInputContext)
        FighterInputContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Fighter.IMC_Fighter"));
    bAddedFighterInputContext = FighterInputContext && !Input->HasMappingContext(FighterInputContext.Get());
    if (bAddedFighterInputContext) Input->AddMappingContext(FighterInputContext.Get(), 1000);
    if (!FighterInputContext)
        UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing IMC_Fighter (or IMC_FighterHUD) in /Game/Input."));
    FlightOpsPanelAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_FlightOpsPanel.IA_FlightOpsPanel"));
    if(FlightOpsPanelAction && FlightOpsPanelAction->ValueType==EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FlightOpsPanelAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleFlightOpsPopup);
    ObjectivesPanelAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_ObjectivesPanel.IA_ObjectivesPanel"));
    if(ObjectivesPanelAction && ObjectivesPanelAction->ValueType==EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(ObjectivesPanelAction.Get(),ETriggerEvent::Started,this,&UMissionBriefingDlg::ToggleObjectivesPopup);
    else UE_LOG(LogTemp,Warning,TEXT("[MissionHUD] Create Boolean IA_ObjectivesPanel and map it in IMC_FighterHUD."));
    NavMapAction = nullptr;
    // Search both authored mission contexts before installing a fallback.
    const UInputMappingContext* NavigationContexts[] = {FighterInputContext.Get(), MissionMenuContext.Get()};
    for (const UInputMappingContext* NavContext : NavigationContexts)
    {
        if (!NavContext) continue;
        for (const auto& NavMapping : NavContext->GetMappings())
        {
            const UInputAction* MappedNavigationAction = NavMapping.Action.Get();
            if (!MappedNavigationAction) continue;
            const FString NavigationActionName = MappedNavigationAction->GetName();
            if (NavigationActionName == TEXT("IA_NavMap") || NavigationActionName == TEXT("IA_NavigationPanel"))
            {
                NavMapAction = const_cast<UInputAction*>(MappedNavigationAction);
                break;
            }
        }
        if (NavMapAction) break;
    }
    if (!NavMapAction)
    {
        NavMapAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NavMap.IA_NavMap"));
        if (!NavMapAction)
            NavMapAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_NavigationPanel.IA_NavigationPanel"));
        if (!NavMapAction)
        {
            NavMapAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
            NavMapAction->ValueType = EInputActionValueType::Boolean;
        }
        MissionTargetContext->MapKey(NavMapAction.Get(), EKeys::N);
        UE_LOG(LogTemp, Display, TEXT("[NavigationInput] No authored navigation mapping; using N for %s"), *GetNameSafe(NavMapAction.Get()));
    }
    MissionMenuInput->BindAction(NavMapAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleNavigationPopup);
    UE_LOG(LogTemp, Display, TEXT("[NavigationInput] Bound %s"), *GetNameSafe(NavMapAction.Get()));
    WeaponsPanelAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_WeaponsPanel.IA_WeaponsPanel"));
    if (WeaponsPanelAction && WeaponsPanelAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(WeaponsPanelAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterWeaponsPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_WeaponsPanel in /Game/Input."));
    HUDWarningsAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_HUDWarnings.IA_HUDWarnings"));
    if (HUDWarningsAction && HUDWarningsAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(HUDWarningsAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleFighterCautionPanel);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing Boolean IA_HUDWarnings in /Game/Input."));
    FighterMFDLeftAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDLeftCycle.IA_MFDLeftCycle"));
    FighterMFDRightAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_MFDRightCycle.IA_MFDRightCycle"));
    if (FighterMFDLeftAction && FighterMFDLeftAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDLeftAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDLeft);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDLeftCycle is missing or is not Boolean."));
    if (FighterMFDRightAction && FighterMFDRightAction->ValueType == EInputActionValueType::Boolean)
        MissionMenuInput->BindAction(FighterMFDRightAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterMFDRight);
    else UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] IA_MFDRightCycle is missing or is not Boolean."));
    MissionWeaponActions.Reset();
    auto BindWeaponAction = [this](const TCHAR* ActionName, ETriggerEvent Event, void (UMissionBriefingDlg::*Handler)()) {
        const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), ActionName, ActionName);
        UInputAction* Action = LoadObject<UInputAction>(nullptr, *Path);
        if (Action && Action->ValueType == EInputActionValueType::Boolean) {
            MissionWeaponActions.Add(Action);
            MissionMenuInput->BindAction(Action, Event, this, Handler);
        } else UE_LOG(LogTemp, Warning, TEXT("[WeaponsInput] Missing Boolean action %s"), ActionName);
    };
    // Resolve the actual mapped action, including assets moved to subfolders.
    // Give its existing mappings priority over overlapping flight/FLCS actions.
    // No physical key is hard-coded: changing M to B in the IMC is respected.
    if (AutoNavInputContext) Input->RemoveMappingContext(AutoNavInputContext.Get());
    AutoNavInputContext = NewObject<UInputMappingContext>(this);
    TSet<const UInputAction*> BoundAutoActions;
    const UInputMappingContext* AutoContexts[] = { FighterInputContext.Get(), MissionMenuContext.Get() };
    for (const UInputMappingContext* Context : AutoContexts) {
        if (!Context) continue;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
            if (!Mapping.Action || !Mapping.Action->GetName().Equals(TEXT("IA_AutoNav"), ESearchCase::IgnoreCase)) continue;
            UInputAction* AutoAction = const_cast<UInputAction*>(Mapping.Action.Get());
            if (AutoAction->ValueType != EInputActionValueType::Boolean) {
                UE_LOG(LogTemp, Error, TEXT("[AutoNavInput] %s must be Boolean"), *AutoAction->GetPathName());
                continue;
            }
            AutoNavInputContext->MapKey(AutoAction, Mapping.Key) = Mapping;
            if (!BoundAutoActions.Contains(AutoAction)) {
                BoundAutoActions.Add(AutoAction);
                MissionWeaponActions.Add(AutoAction);
                MissionMenuInput->BindAction(AutoAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleMissionAutoNav);
            }
            UE_LOG(LogTemp, Display, TEXT("[AutoNavInput] %s -> %s"), *Mapping.Key.ToString(), *AutoAction->GetPathName());
        }
    }
    // Raise the configured boost mapping above camera defaults.
    for (const UInputMappingContext* Context : AutoContexts) {
        if (!Context) continue;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
            if (!Mapping.Action || !Mapping.Action->GetName().Equals(TEXT("IA_Augmenter"), ESearchCase::IgnoreCase)) continue;
            UInputAction* BoostAction = const_cast<UInputAction*>(Mapping.Action.Get());
            if (BoostAction->ValueType != EInputActionValueType::Boolean) continue;
            AutoNavInputContext->MapKey(BoostAction, Mapping.Key) = Mapping;
            if (BoostAction != MissionAugmenterAction.Get() && !MissionWeaponActions.Contains(BoostAction)) {
                MissionWeaponActions.Add(BoostAction);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Triggered, this, &UMissionBriefingDlg::OnMissionAugmenter);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionAugmenter);
                MissionMenuInput->BindAction(BoostAction, ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionAugmenter);
            }
            UE_LOG(LogTemp, Display, TEXT("[AugmenterInput] %s -> %s"), *Mapping.Key.ToString(), *BoostAction->GetPathName());
        }
    }
    if (AutoNavInputContext->GetMappings().Num()) Input->AddMappingContext(AutoNavInputContext.Get(), 1105);
    else UE_LOG(LogTemp, Error, TEXT("[AutoNavInput] No IA_AutoNav mapping found in the active fighter/menu contexts."));
    BindWeaponAction(TEXT("IA_FirePrimary"), ETriggerEvent::Triggered, &UMissionBriefingDlg::FireMissionPrimary);
    BindWeaponAction(TEXT("IA_FireSecondary"), ETriggerEvent::Started, &UMissionBriefingDlg::FireMissionSecondary);
    BindWeaponAction(TEXT("IA_Decoy"), ETriggerEvent::Started, &UMissionBriefingDlg::FireMissionDecoy);
    BindWeaponAction(TEXT("IA_LaunchProbe"), ETriggerEvent::Started, &UMissionBriefingDlg::LaunchMissionProbe);
    BindWeaponAction(TEXT("IA_PrimaryWeaponCycle"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionPrimary);
    BindWeaponAction(TEXT("IA_SecondaryWeaponCycle"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSecondary);
    BindWeaponAction(TEXT("IA_WeaponPattern"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionPattern);
    BindWeaponAction(TEXT("IA_WeaponSweep"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSweep);
    BindWeaponAction(TEXT("IA_DropWeaponTarget"), ETriggerEvent::Started, &UMissionBriefingDlg::DropMissionWeaponTarget);
    // Bind the authored asset from the active HUD context, including subfolders.
    UInputAction* ContactsAction = nullptr;
    if (FighterInputContext) for (const auto& Mapping : FighterInputContext->GetMappings())
        if (Mapping.Action && Mapping.Action->GetName() == TEXT("IA_ContactsPanel"))
            { ContactsAction = const_cast<UInputAction*>(Mapping.Action.Get()); break; }
    if (!ContactsAction) ContactsAction = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/Input/Targeting/IA_ContactsPanel.IA_ContactsPanel"));
    if (!ContactsAction) ContactsAction = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/Input/IA_ContactsPanel.IA_ContactsPanel"));
    if (ContactsAction) {
        MissionWeaponActions.Add(ContactsAction);
        MissionMenuInput->BindAction(ContactsAction, ETriggerEvent::Started, this, &UMissionBriefingDlg::ToggleContactsPanel);
    } else UE_LOG(LogTemp, Error, TEXT("[Contacts] IA_ContactsPanel not found"));
    BindWeaponAction(TEXT("IA_SensorMode"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSensor);
    BindWeaponAction(TEXT("IA_SensorGroundMode"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionSensorGround);
    bSensorRangeUpHeld = bSensorRangeDownHeld = false;
    // Started can precede chord satisfaction. Act only once the full chord triggers.
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Triggered, &UMissionBriefingDlg::MissionSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Completed, &UMissionBriefingDlg::ReleaseSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeUp"), ETriggerEvent::Canceled, &UMissionBriefingDlg::ReleaseSensorRangeUp);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Triggered, &UMissionBriefingDlg::MissionSensorRangeDown);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Completed, &UMissionBriefingDlg::ReleaseSensorRangeDown);
    BindWeaponAction(TEXT("IA_SensorRangeDown"), ETriggerEvent::Canceled, &UMissionBriefingDlg::ReleaseSensorRangeDown);
    BindWeaponAction(TEXT("IA_EMCONUp"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionEMCONUp);
    BindWeaponAction(TEXT("IA_EMCONDown"), ETriggerEvent::Started, &UMissionBriefingDlg::MissionEMCONDown);
    BindWeaponAction(TEXT("IA_TargetNearest"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionClosest);
    BindWeaponAction(TEXT("IA_TargetHostile"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionHostile);
    BindWeaponAction(TEXT("IA_TargetNearestThreat"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionThreat);
    BindWeaponAction(TEXT("IA_TargetSubsystemNext"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSubsystem);
    BindWeaponAction(TEXT("IA_TargetSubsystemPrevious"), ETriggerEvent::Started, &UMissionBriefingDlg::CycleMissionSubsystemPrevious);
    BindWeaponAction(TEXT("IA_TargetHostileThreat"), ETriggerEvent::Started, &UMissionBriefingDlg::LockMissionHostileThreat);
    FighterWeaponCycleAction = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    FighterWeaponCycleAction->ValueType = EInputActionValueType::Boolean;
    MissionTargetContext->MapKey(FighterWeaponCycleAction.Get(), EKeys::BackSpace);
    MissionMenuInput->BindAction(FighterWeaponCycleAction.Get(), ETriggerEvent::Started, this, &UMissionBriefingDlg::CycleFighterWeapon);
    if (!FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
    {
        TWeakObjectPtr<UMissionBriefingDlg> Owner(this);
        FighterHUDDetails = SNew(SFighterHUDDetails)
            .ShowHUD_Lambda([Owner]() { return Owner.IsValid() && Owner->bFighterHUDVisible && Owner->CanShowFighterHUD(); })
            .SelectedTarget_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .SelectedName_Lambda([Owner]() { return Owner.IsValid() ? Owner->SelectedMissionTargetName : FText::GetEmpty(); });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(FighterHUDDetails.ToSharedRef(), 810);
    }
    Input->AddMappingContext(MissionTargetContext.Get(), 1001);
    UE_LOG(LogTemp, Log, TEXT("[MissionZoom] MouseWheelAxis bound in mission context (priority 1001)."));

    bAddedMissionMenuContext = !Input->HasMappingContext(MissionMenuContext.Get());
    if (bAddedMissionMenuContext) Input->AddMappingContext(MissionMenuContext.Get(), 1000);
    BindMissionCameraInput();
    PC->PushInputComponent(MissionMenuInput.Get());
    // Existing briefing Blueprint defaults may still have None or the native
    // class saved. Resolve the designed menu rather than displaying plain Slate.
    if (!QuitMissionMenuClass || QuitMissionMenuClass == UQuitMissionMenu::StaticClass())
    {
        QuitMissionMenuClass = LoadClass<UQuitMissionMenu>(nullptr,
            TEXT("/Game/Screens/Mission/WBP_QuitMissionMenu.WBP_QuitMissionMenu_C"));
        if (!QuitMissionMenuClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] WBP_QuitMissionMenu could not load. Compile/save it and verify its parent is QuitMissionMenu."));
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Quit menu widget class: %s"), *GetNameSafe(QuitMissionMenuClass.Get()));
    MissionQuitMenu = new QuitView(nullptr);
    TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
    MissionQuitMenu->Configure(PC, [WeakThis](uintptr_t Choice)
    {
        if (WeakThis.IsValid()) WeakThis->HandleMissionMenuAction(Choice);
    }, QuitMissionMenuClass);
    return true;
}
void UMissionBriefingDlg::DisableMissionMenuInput()
{
    CloseNavigationPopup();
    NavMapAction=nullptr;
    CloseFlightOpsPopup();
    CloseObjectivesPopup();
    ObjectivesPanelAction=nullptr;
    CloseWeaponsPopup();
    CloseEngineering();
    EngineeringAction = nullptr;
    if (MissionCameraRig) MissionCameraRig->Reset(GetOwningPlayer());
    ClearMissionRotationInput();
    if (FighterHUDDetails.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDDetails.ToSharedRef());
    FighterHUDDetails.Reset();
    WeaponsPanelAction = nullptr;
    HUDWarningsAction = nullptr;
    FighterMFDLeftAction = nullptr;
    FighterMFDRightAction = nullptr;
    FighterWeaponCycleAction = nullptr;
    if (FighterHUDPanels.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FighterHUDPanels.ToSharedRef());
    FighterHUDPanels.Reset();
    FighterHUDToggleAction = nullptr;
    bFighterHUDVisible = true;
    if (manager) manager->MissionOptionsReturn = nullptr;
    if (bMissionControlsOpen)
    {
        if (manager) manager->HideOptionsScreen();
        UGameplayStatics::SetGamePaused(GetWorld(), bControlsPreviousWorldPaused);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            Runtime->SetPaused(bControlsPreviousRuntimePaused);
        bMissionControlsOpen = false;
    }
    if (MissionQuitMenu) { delete MissionQuitMenu; MissionQuitMenu = nullptr; }
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (MissionMenuInput) PC->PopInputComponent(MissionMenuInput.Get());
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
            if (auto* Input = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (AutoNavInputContext) Input->RemoveMappingContext(AutoNavInputContext.Get());
                if (MissionCameraContext) Input->RemoveMappingContext(MissionCameraContext.Get());
                if (RadioChoiceContext) Input->RemoveMappingContext(RadioChoiceContext.Get());
                if (MissionTargetContext) Input->RemoveMappingContext(MissionTargetContext.Get());
                if (bAddedFighterInputContext && FighterInputContext)
                    Input->RemoveMappingContext(FighterInputContext.Get());
                if (bAddedMissionMenuContext && MissionMenuContext)
                    Input->RemoveMappingContext(MissionMenuContext.Get());
            }
    }
    if (MissionMenuInput) MissionMenuInput->DestroyComponent();
    MissionWeaponActions.Reset();
    AutoNavInputContext = nullptr;
    bPreviousTargetHeld = false;
    bNextTargetHeld = false;
    RadioChoiceActions.Reset(); RadioChoiceContext = nullptr; RadioMenuAction = nullptr;
    FlightOpsPanelAction = nullptr;
    GearToggleAction = nullptr;
    MissionThrottleAction = nullptr;
    MissionCameraViewAction = nullptr;
    MissionCameraContext = nullptr;
    MissionCameraActions.Reset();
    MissionCameraRig.Reset();
    MissionStrafeAction = nullptr;
    MissionForwardThrustAction = nullptr;
    MissionVerticalThrustAction = nullptr;
    MissionAugmenterAction = nullptr;
    MissionShieldsUpAction = nullptr;
    MissionShieldsDownAction = nullptr;
    MissionShieldsFullAction = nullptr;
    MissionShieldsZeroAction = nullptr;
    MissionPitchAction = nullptr;
    MissionYawAction = nullptr;
    MissionRollAction = nullptr;
    MissionThrottleZeroAction = nullptr;
    MissionThrottleFullAction = nullptr;
    MissionMenuInput = nullptr;
    MissionMenuContext = nullptr;
    MissionTargetContext = nullptr;
    PreviousMissionTargetAction = nullptr;
    NextMissionTargetAction = nullptr;
    MissionTargetZoomAction = nullptr;
    MissionTargetZoomDistance = 1.0;
    bAddedMissionMenuContext = false;
    FighterInputContext = nullptr;
    bAddedFighterInputContext = false;
}
void UMissionBriefingDlg::PreviousMissionTarget()
{
    if (bPreviousTargetHeld) return;
    bPreviousTargetHeld = true;
    CycleMissionTarget(-1);
}
void UMissionBriefingDlg::ReleasePreviousMissionTarget() { bPreviousTargetHeld = false; }
void UMissionBriefingDlg::NextMissionTarget()
{
    if (bNextTargetHeld) return;
    bNextTargetHeld = true;
    // Do not also advance when the user invokes Shift+T for reverse targeting.
    APlayerController* PC = GetOwningPlayer();
    if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))) return;
    CycleMissionTarget(1);
}
void UMissionBriefingDlg::ReleaseNextMissionTarget() { bNextTargetHeld = false; }
void UMissionBriefingDlg::CycleMissionTarget(int32 Direction)
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || bMissionControlsOpen || EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || !IsValid(MissionPreviewCamera)) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player) return;
    TArray<AActor*> Candidates = GetSelectableMissionContacts();
    if (Candidates.IsEmpty())
    {
        SelectedMissionTarget.Reset();
        Player->DropTarget();
        return;
    }
    const int32 Current = Candidates.IndexOfByPredicate([this](AActor* Actor) { return Actor == SelectedMissionTarget.Get(); });
    const int32 Index = Current == INDEX_NONE ? (Direction > 0 ? 0 : Candidates.Num()-1)
        : (Current + (Direction > 0 ? 1 : -1) + Candidates.Num()) % Candidates.Num();
    AActor* Selected = Candidates[Index];
    SelectMissionContact(Selected);
}

TArray<AActor*> UMissionBriefingDlg::GetSelectableMissionContacts() const
{
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Region) return {};
    Ship* Player = Simulation->GetPlayerShip();
    if (!Player || !Region->GetShips().contains(Player)) return {};
    TArray<AActor*> Candidates;
    for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
    {
        AShipActor* Actor = *It;
        if (!IsValid(Actor)) continue;
        Ship* ShipData = Actor->GetRuntimeShip();
        const bool bInRegion = ShipData && Region->GetShips().contains(ShipData);
        // Never dereference an unowned runtime pointer for diagnostics.
        const bool bDead = bInRegion && ShipData->IsDead();
        const bool bDestroying = Actor->IsActorBeingDestroyed();
        const bool bHidden = Actor->IsHidden();
        const bool bEligible = bInRegion && ShipData != Player && !bDead && !bDestroying && !bHidden;
        if (bEligible) Candidates.Add(Actor);
    }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Object = *It;
        if (!IsValid(Object) || !Object->ActorHasTag(TEXT("MissionNavigationTarget"))) continue;
        FMissionObjectData Data;
        if (ReadMissionObject(Object, Player, Region, Data)) Candidates.AddUnique(Object);
    }
    Candidates.Sort([](const AActor& A, const AActor& B) { return A.GetUniqueID() < B.GetUniqueID(); });
    return Candidates;
}

void UMissionBriefingDlg::SelectMissionContact(AActor* Selected)
{
    if (!IsValid(Selected) || !GetWorld() || !GetWorld()->GetGameViewport() ||
        !GetSelectableMissionContacts().Contains(Selected)) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region) return;
    SelectedMissionTarget = Selected;
    if (bMissionTargetInspection) MissionTargetZoomDistance = 1.0;
    SelectedMissionTargetLocalBounds = MissionTargetLocalBounds(Selected);
    AShipActor* ShipVisual = Cast<AShipActor>(Selected);
    Ship* SelectedShip = ShipVisual ? ShipVisual->GetRuntimeShip() : nullptr;
    // Use SSW sensor/EMCON gating and propagate the lock to weapons and mission events.
    if (SelectedShip) Player->LockTarget(SelectedShip);
    else Player->DropTarget(); // Navigation selection is not a weapons lock.
    FMissionObjectData ObjectData;
    ReadMissionObject(Selected, Player, Region, ObjectData);
    FString FullName = SelectedShip ? FString(ANSI_TO_TCHAR(SelectedShip->GetName())) : ObjectData.Name;
    if (CombatUnit* Unit = SelectedShip ? SelectedShip->GetCombatUnit() : nullptr)
    {
        const FString Abbreviation = UFormattingUtils::GetUnitDesignIndicator(Unit);
        FString Registry = FString(ANSI_TO_TCHAR(Unit->GetRegistryNumber().data())).TrimStartAndEnd();
        // Normalize CV6, CV-6, and a numeric registry to the same CV-6 prefix.
        if (Registry.StartsWith(Abbreviation, ESearchCase::IgnoreCase))
            Registry.RightChopInline(Abbreviation.Len());
        Registry.TrimStartAndEndInline();
        while (Registry.StartsWith(TEXT("-")))
        {
            Registry.RightChopInline(1);
            Registry.TrimStartInline();
        }
        const FString Prefix = Registry.IsEmpty() ? Abbreviation : Abbreviation + TEXT("-") + Registry;
        if (!FullName.StartsWith(Prefix + TEXT(" "), ESearchCase::IgnoreCase))
            FullName = Prefix + TEXT(" ") + FullName;
    }
    SelectedMissionTargetName = FText::FromString(FullName);
    if (!MissionTargetOverlay.IsValid())
    {
        TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
        MissionTargetOverlay = SNew(SMissionTargetOverlay).Controller(GetOwningPlayer())
            .RangeKm_Lambda([WeakThis]() {
                if (!WeakThis.IsValid()) return -1.0;
                Sim* S = Sim::GetSim();
                Ship* P = S ? S->GetPlayerShip() : nullptr;
                SimRegion* R = S ? S->GetActiveRegion() : nullptr;
                AActor* A = WeakThis->SelectedMissionTarget.Get();
                if (!P || !R || !R->GetShips().contains(P) || !IsValid(A)) return -1.0;
                if (auto* Visual = Cast<AShipActor>(A)) {
                    Ship* T = Visual->GetRuntimeShip();
                    if (T && R->GetShips().contains(T)) return FVector::Distance(P->GetLocation(),T->GetLocation())/1000.0;
                }
                FMissionObjectData Data;
                return ReadMissionObject(A,P,R,Data) ? Data.RangeKm : -1.0;
            })
            .Target_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTarget : TWeakObjectPtr<AActor>(); })
            .LocalBounds_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->SelectedMissionTargetLocalBounds : FBox(ForceInit); })
            .TargetColor_Lambda([WeakThis]()
            {
                const FLinearColor Neutral = FLinearColor::Yellow;
                if (!WeakThis.IsValid()) return Neutral;
                AShipActor* Actor = Cast<AShipActor>(WeakThis->SelectedMissionTarget.Get());
                Sim* CurrentSim = Sim::GetSim();
                SimRegion* Region = CurrentSim ? CurrentSim->GetActiveRegion() : nullptr;
                Ship* Data = IsValid(Actor) ? Actor->GetRuntimeShip() : nullptr;
                if (!Data || !Region || !Region->GetShips().contains(Data)) return Neutral;
                const int32 IFF = Data->GetIFF();
                if (IFF <= 0) return Neutral;
                Mission* CurrentMission = CurrentSim->GetMission();
                if (!CurrentMission) return Neutral;
                return IFF == CurrentMission->GetTeam()
                    ? FLinearColor(0.25f, 0.65f, 1.0f, 1.0f)
                    : FLinearColor::Red;
            })
            .TargetName_Lambda([WeakThis]()
            {
                if (!WeakThis.IsValid()) return FText::GetEmpty();
                Sim* Current=Sim::GetSim();
                SimRegion* Active=Current?Current->GetActiveRegion():nullptr;
                Ship* Pilot=Current?Current->GetPlayerShip():nullptr;
                AShipActor* SelectedActor=Cast<AShipActor>(WeakThis->SelectedMissionTarget.Get());
                if (!SelectedActor) return WeakThis->SelectedMissionTargetName;
                Ship* TargetData=IsValid(SelectedActor)?SelectedActor->GetRuntimeShip():nullptr;
                const bool Locked=Pilot && Active && Active->GetShips().contains(Pilot) && TargetData &&
                    Active->GetShips().contains(TargetData) && Pilot->GetTarget()==TargetData;
                return FText::FromString(WeakThis->SelectedMissionTargetName.ToString() + (Locked?TEXT(""):TEXT(" [NO LOCK]")));
            });
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(MissionTargetOverlay.ToSharedRef(), 900);
    }
    MissionTargetLastUpdate = GetWorld()->GetTimeSeconds();
    FTimerManagerTimerParameters TimerParameters;
    TimerParameters.bLoop = true;
    TimerParameters.bMaxOncePerFrame = true;
    GetWorld()->GetTimerManager().SetTimer(MissionTargetCameraTimer, this,
        &UMissionBriefingDlg::UpdateMissionTargetCamera, 0.000001f, TimerParameters);
}
bool UMissionBriefingDlg::CanUseFighterHUD() const
{
    return bLiveMissionStarted && !MissionSceneCover.IsValid() && !bMissionControlsOpen && !EngineeringPopup.IsValid() && !WeaponsPopup.IsValid() && !ObjectivesPopup.IsValid() && !FlightOpsPopup.IsValid() && !NavigationPopup.IsValid() &&
        !(MissionQuitMenu && MissionQuitMenu->IsMenuShown());
}

bool UMissionBriefingDlg::CanShowFighterHUD() const
{
    return bLiveMissionStarted && !MissionSceneCover.IsValid() && !bMissionControlsOpen && !EngineeringPopup.IsValid() && (!WeaponsPopup.IsValid() || bContactsOnlyPanel) && !ObjectivesPopup.IsValid() && !FlightOpsPopup.IsValid() && !NavigationPopup.IsValid() &&
        !(MissionQuitMenu && MissionQuitMenu->IsMenuShown());
}
void UMissionBriefingDlg::CycleFighterMFDLeft()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->CycleMFD(0);
}
void UMissionBriefingDlg::CycleFighterMFDRight()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->CycleMFD(1);
}
void UMissionBriefingDlg::CycleFighterWeapon()
{
    if (!CanUseFighterHUD() || !FighterHUDDetails.IsValid()) return;
    APlayerController* PC = GetOwningPlayer();
    const bool Primary = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
    FighterHUDDetails->CycleWeapon(Primary);
}

void UMissionBriefingDlg::ToggleFighterWeaponsPanel()
{
    CloseNavigationPopup();
    CloseFlightOpsPopup();
    CloseObjectivesPopup();
    CloseEngineering();
    if (WeaponsPopup.IsValid()) {
        const bool WasContacts = bContactsOnlyPanel;
        CloseWeaponsPopup();
        if (!WasContacts) return;
    }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    bWeaponsPreviousCursor=PC->bShowMouseCursor;
    UClass* PanelClass=LoadClass<UWeaponsDlg>(nullptr,TEXT("/Game/Screens/inGame/WBP_Weapons.WBP_Weapons_C"));
    WeaponsPanelWidget=CreateWidget<UWeaponsDlg>(PC,PanelClass?PanelClass:UWeaponsDlg::StaticClass());
    if(!WeaponsPanelWidget)return;
    WeaponsPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseWeaponsPopup();});
    WeaponsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    WeaponsPopup=WeaponsPanelWidget->TakeWidget();
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(WeaponsPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(WeaponsPopup);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor=true;
}

void UMissionBriefingDlg::CloseWeaponsPopup()
{
    if (!WeaponsPopup.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(WeaponsPopup.ToSharedRef());
    WeaponsPopup.Reset();
    bContactsOnlyPanel=false;
    if(WeaponsPanelWidget)WeaponsPanelWidget->RemoveFromParent();
    WeaponsPanelWidget=nullptr;
    if (APlayerController* PC=GetOwningPlayer()) {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=bWeaponsPreviousCursor;
    }
}


void UMissionBriefingDlg::ToggleFighterCautionPanel()
{
    if (CanUseFighterHUD() && FighterHUDDetails.IsValid()) FighterHUDDetails->ToggleCautionPanel();
}

void UMissionBriefingDlg::ToggleFighterHUD()
{
    if (!bLiveMissionStarted || MissionSceneCover.IsValid() || bMissionControlsOpen || EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    APlayerController* PC = GetOwningPlayer();
    if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))) return;
    bFighterHUDVisible = !bFighterHUDVisible;
    if (!bFighterHUDVisible && FighterHUDDetails.IsValid())
    {
        FighterHUDDetails->CloseRadio();
        UpdateFighterRadioInput();
    }
}

void UMissionBriefingDlg::OnTargetZoom(const FInputActionValue& Value)
{
    if (!bLiveMissionStarted ||
        !IsValid(MissionPreviewCamera) || MissionSceneCover.IsValid() || bMissionControlsOpen || EngineeringPopup.IsValid() || WeaponsPopup.IsValid() || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() ||
        (MissionQuitMenu && MissionQuitMenu->IsMenuShown())) return;
    const float Wheel = Value.Get<float>();
    if (!FMath::IsFinite(Wheel) || FMath::IsNearlyZero(Wheel)) return;
    // Positive wheel input moves closer; the existing camera timer smooths motion.
    MissionTargetZoomDistance = FMath::Clamp(
        MissionTargetZoomDistance * FMath::Pow(0.85, double(FMath::Clamp(Wheel, -10.0f, 10.0f))),
        0.5, 3.0);
}

void UMissionBriefingDlg::ToggleMissionMenu()
{
    if(NavigationPopup.IsValid()){CloseNavigationPopup();return;}
    if(FlightOpsPopup.IsValid()){CloseFlightOpsPopup();return;}
    if(ObjectivesPopup.IsValid()){CloseObjectivesPopup();return;}
    if (WeaponsPopup.IsValid()) { CloseWeaponsPopup(); return; }
    if (EngineeringPopup.IsValid()) { CloseEngineering(); return; }
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX = MissionCameraRig->LookY = MissionCameraRig->RangeInput = 0.0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    if (!MissionQuitMenu || !MissionSystemLevel || MissionSceneCover.IsValid()) return;
    if (MissionQuitMenu->IsMenuShown()) MissionQuitMenu->CloseMenu();
    else
    {
        TArray<UUserWidget*> Screens;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Screens, UGameScreen::StaticClass(), false);
        for (UUserWidget* Widget : Screens)
            if (auto* Screen = Cast<UGameScreen>(Widget))
                if (Screen->IsShown() && Screen->CloseTopmost()) return;
        MissionQuitMenu->ShowMenu();
    }
}
void UMissionBriefingDlg::HandleMissionMenuAction(uintptr_t Action)
{
    if (Action == QuitView::Resume) return;
    if (Action == QuitView::Controls)
    {
        if (!manager) return;
        bMissionControlsOpen = true;
        bControlsPreviousWorldPaused = UGameplayStatics::IsGamePaused(GetWorld());
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
            bControlsPreviousRuntimePaused = Runtime->IsPaused();
        // Keep gameplay paused while the existing options UI is active.
        UGameplayStatics::SetGamePaused(GetWorld(), true);
        if (auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(true);
        TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
        manager->MissionOptionsReturn = [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;
            auto* Self = WeakThis.Get();
            UGameplayStatics::SetGamePaused(Self->GetWorld(), Self->bControlsPreviousWorldPaused);
            if (auto* Runtime = Self->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>()) Runtime->SetPaused(Self->bControlsPreviousRuntimePaused);
            Self->bMissionControlsOpen = false;
            if (Self->MissionQuitMenu) Self->MissionQuitMenu->ShowMenu();
        };
        manager->ShowOptionsScreen();
        if (auto* Options = manager->GetOptionsScreen()) Options->ShowCtlDlg();
        else { auto Return = MoveTemp(manager->MissionOptionsReturn); if (Return) Return(); }
        return;
    }
    // Defer cleanup so we never delete QuitView during its button callback.
    TWeakObjectPtr<UMissionBriefingDlg> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis, Action]()
    {
        if (!WeakThis.IsValid()) return;
        auto* Self = WeakThis.Get();
        Sim* Simulation = Sim::GetSim();
        auto* Runtime = Self->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
        if (Runtime) Runtime->SetTimeCompression(1);
        if (Action == QuitView::Accept && Self->Manager && Self->Manager->GetDebriefDlg())
        {
            Self->CloseEmptySectorPreview(true);
            if (Runtime) Runtime->SetGameMode(EGameMode::PLAN);
            Self->Manager->ShowDebriefDlg();
            auto* Debrief = Self->Manager->GetDebriefDlg();
            Debrief->Show();
            if (APlayerController* PC = Self->GetOwningPlayer())
            {
                PC->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetWidgetToFocus(Debrief->TakeWidget());
                Mode.SetHideCursorDuringCapture(false);
                PC->SetInputMode(Mode);
            }
            return;
        }
        if (Action == QuitView::Accept)
        {
            if (Simulation) Simulation->CommitMission();
        }
        // Visuals must release Ship* bindings before Sim tears down elements/regions.
        if (Runtime)
        {
            Runtime->SetPaused(true);
            Runtime->EndMissionPresentation();
        }
        TArray<AShipActor*> OldVisuals;
        for (TActorIterator<AShipActor> It(Self->GetWorld()); It; ++It)
            if (It->HasRuntimeShip()) OldVisuals.Add(*It);
        for (AShipActor* Actor : OldVisuals)
        {
            Actor->SetActorTickEnabled(false);
            Actor->BindRuntimeShip(nullptr);
            Actor->Destroy();
        }
        if (Simulation && Simulation->GetMission()) Simulation->UnloadMission();
        else ShipStats::Initialize();
        Self->CloseEmptySectorPreview();
        Self->MissionPtr = nullptr;
        Self->InfoPtr = nullptr;
        if (Action == QuitView::Abort)
            if (Campaign* Camp = Campaign::GetCampaign())
                if (Camp->GetCampaignId() < Campaign::SINGLE_MISSIONS) Camp->RollbackMission();
        if (Runtime) Runtime->SetGameMode(EGameMode::CMPN);
        if (Self->manager) Self->manager->ShowOperationsMissionsDlg();
    });
}

bool UMissionBriefingDlg::StartLiveMission()
{
    if (bLiveMissionStarted) return true;
    if (!MissionPtr || !MissionSystemLevel || !GetGameInstance()) return false;
    auto* Runtime = GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
    auto* Environment = GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    if (!Runtime || !Environment) return false;
    ASystemSceneBuilder* Builder = nullptr;
    for (TActorIterator<ASystemSceneBuilder> It(GetWorld()); It; ++It)
        if (It->GetLevel() == MissionSystemLevel->GetLoadedLevel()) { Builder = *It; break; }
    if (!Builder || Builder->GetSpawnedRegions().IsEmpty()) return false;
    if (!Sim::GetSim()) Runtime->CreateWorld();
    Sim* Simulation = Sim::GetSim();
    if (!Simulation) return false;
    Runtime->SetPaused(true);
    // Campaign previews can leave a Sim mission loaded. An explicit launch
    // replaces that session; detach visual proxies before Sim deletes its ships.
    if (Simulation->GetMission())
    {
        UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Clearing previous simulation before selected mission launch"));
        Runtime->EndMissionPresentation();
        TArray<AShipActor*> PreviousVisuals;
        for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
            if (It->HasRuntimeShip()) PreviousVisuals.Add(*It);
        for (AShipActor* Actor : PreviousVisuals)
        {
            Actor->SetActorTickEnabled(false);
            Actor->BindRuntimeShip(nullptr);
            Actor->Destroy();
        }
        Simulation->OnMissionUnloading.RemoveAll(this);
        Simulation->UnloadMission();
    }
    Environment->BuildSimRegionsForSim(Simulation);
    Runtime->BeginMissionPresentation(Builder);
    // Use the selected/generated campaign Mission with its current loadout and orders.
    MissionPtr->SetComplete(false);
    Simulation->OnMissionUnloading.RemoveAll(this);
    Simulation->OnMissionUnloading.AddUObject(this, &UMissionBriefingDlg::HandleSimulationUnloading);
    Simulation->LoadMission(MissionPtr, false);
    bLiveMissionStarted = true;
    bMissionCompleteHandedOff = false;
    Simulation->ExecMission();
    if (!Simulation->GetActiveRegion()) return false;
    Runtime->SetGameMode(EGameMode::PLAY);
    Runtime->SetPaused(true); // Remain behind the scene cover until camera warmup completes.
    UE_LOG(LogTemp, Log, TEXT("[MissionBriefing] Live mission started: %hs"), MissionPtr->GetName());
    return true;
}

void UMissionBriefingDlg::OnCommit()
{
    // Ignore a repeated launch while this preview is loading or visible.
    if (MissionSystemLevel) return;
    UWorld* World = GetWorld();
    UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
    if (!MissionPtr || !Viewport)
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] PROCEED blocked: no mission or game viewport. Mission=%p Viewport=%p"), MissionPtr, Viewport);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red,
            TEXT("MISSION PROCEED: Mission data or viewport unavailable. See Output Log."));
        return;
    }

    FString SystemName = ToTextFromUtf8(MissionPtr->GetSystem()).ToString();
    if (SystemName.IsEmpty())
    {
        if (StarSystem* System = MissionPtr->GetStarSystem())
            SystemName = ToTextFromUtf8(System->GetName()).ToString();
    }
    SystemName.TrimStartAndEndInline();
    const FString PackagePath = FString::Printf(TEXT("/Game/Maps/%s"), *SystemName);
    FString PackageFilename;
    if (SystemName.IsEmpty() || !FPackageName::IsValidLongPackageName(PackagePath) ||
        !FPackageName::DoesPackageExist(PackagePath, &PackageFilename))
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] PROCEED blocked: system map not found: %s (mission=%hs)"), *PackagePath, MissionPtr->GetName());
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 9.f, FColor::Red,
            FString::Printf(TEXT("MISSION PROCEED: Missing map %s"), *PackagePath));
        return; // Do not enter an empty mission when the authored map is absent.
    }


    CloseEmptySectorPreview();
    // Keep the world rendering behind an opaque Slate cover so materials,
    // exposure and the nebula can settle without showing their startup frames.
    MissionSceneCover = SNew(SBorder)
        .Visibility(EVisibility::HitTestInvisible)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor::Black);
    Viewport->AddViewportWidgetContent(MissionSceneCover.ToSharedRef(), 999);
    bool bLoadStarted = false;
    MissionSystemLevel = ULevelStreamingDynamic::LoadLevelInstance(
        World, PackagePath, FVector::ZeroVector, FRotator::ZeroRotator, bLoadStarted);
    if (!bLoadStarted || !MissionSystemLevel)
    {
        CloseEmptySectorPreview();
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Could not start system level load: %s"),
            *PackagePath);
        return;
    }
    if (!EnableMissionMenuInput())
    {
        UE_LOG(LogTemp, Error, TEXT("[MissionBriefing] Missing IMC_Game or IA_Quit"));
        CloseEmptySectorPreview();
        return;
    }
    MissionSystemPackage = PackagePath;
    MissionSystemLevel->OnLevelShown.AddDynamic(
        this, &UMissionBriefingDlg::HandleMissionSystemLevelShown);
    MissionSystemLevel->SetShouldBeLoaded(true);
    MissionSystemLevel->SetShouldBeVisible(true);

    // Hide all menu-owned dialogs, including the briefing's outer host.
    if (manager) manager->Hide();
    if (Manager) Manager->Hide();
    Hide();

    // Stream the authored system map; do not explicitly start the mission simulation.
    // Retain starfield actors and restore visible scene objects on return.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (MissionSystemLevel && Actor->GetLevel() == MissionSystemLevel->GetLoadedLevel()) continue;
        if (Actor->IsA<APlanetActor>() || Actor->IsA<AGasGiantActor>() ||
            Actor->IsA<ACentralSun>() || Actor->IsA<AShipActor>())
        {
            TArray<AActor*> VisualActors;
            VisualActors.Add(Actor);
            TArray<AActor*> Children;
            Actor->GetAllChildActors(Children, true);
            VisualActors.Append(Children);
            for (AActor* Visual : VisualActors)
            {
                if (IsValid(Visual) && !Visual->IsHidden())
                {
                    PreviewHiddenActors.AddUnique(Visual);
                    Visual->SetActorHiddenInGame(true);
                }
            }
        }
    }

    const auto MissionTitleOpacity = [WeakThis = TWeakObjectPtr<UMissionBriefingDlg>(this)]()
    {
        if (!WeakThis.IsValid()) return 0.0f;
        UWorld* TitleWorld = WeakThis->GetWorld();
        const double Start = WeakThis->MissionTitleRevealTime;
        if (!TitleWorld || Start < 0.0) return 1.0f;
        const double Elapsed = TitleWorld->GetTimeSeconds() - Start;
        return static_cast<float>(1.0 - FMath::Clamp(Elapsed - 10.0, 0.0, 1.0));
    };

    EmptySectorOverlay = SNew(SOverlay)
        .Visibility(EVisibility::HitTestInvisible)
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Top)
        .Padding(FMargin(32.0f, 24.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(ToTextFromUtf8(MissionPtr->GetName()))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
                .ColorAndOpacity_Lambda([MissionTitleOpacity]()
                {
                    return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, MissionTitleOpacity()));
                })
                .ShadowOffset(FVector2D(1.0f, 1.0f))
                .ShadowColorAndOpacity_Lambda([MissionTitleOpacity]()
                {
                    return FLinearColor(0.0f, 0.0f, 0.0f, MissionTitleOpacity());
                })

            ]
        ];
    Viewport->AddViewportWidgetContent(EmptySectorOverlay.ToSharedRef(), 1000);

    if (APlayerController* PC = GetOwningPlayer())
    {
        FInputModeGameOnly Input;
        PC->SetInputMode(Input);
        PC->bShowMouseCursor = true;
    }
}
void UMissionBriefingDlg::OnCancel()
{
    if (bStandaloneMission) ClearStandaloneMission();
    if (manager) manager->ReturnFromMissionBriefing();
    else Hide();
}

void UMissionBriefingDlg::HandleAcceptClicked()
{
    OnCommit();
}

void UMissionBriefingDlg::HandleCancelClicked()
{
    OnCancel();
}

void UMissionBriefingDlg::HandleGameTimers()
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

    if (Timer)
    {
        Timer->OnUniverseSecond.RemoveAll(this);
        Timer->OnUniverseSecond.AddUObject(this, &UMissionBriefingDlg::HandleUniverseSecondTick);
        Timer->OnUniverseMinute.RemoveAll(this);
        Timer->OnUniverseMinute.AddUObject(this, &UMissionBriefingDlg::HandleUniverseMinuteTick);
        Timer->OnCampaignTPlusChanged.RemoveAll(this);
        Timer->OnCampaignTPlusChanged.AddUObject(this, &UMissionBriefingDlg::HandleCampaignTPlusChanged);

        const uint64 Now = Timer->GetUniverseTimeSeconds();
        HandleUniverseSecondTick(Now);

        if (SSWInstance->CampaignSave)
        {
            HandleCampaignTPlusChanged(Now, SSWInstance->CampaignSave->GetTPlusSeconds(Now));
        }
    }
}

void UMissionBriefingDlg::HandleUniverseSecondTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;

    if (GameTimeText)
    {
        GameTimeText->SetText(
            FText::FromString(Timer->GetUniverseDateTimeString())
        );
    }
}

void UMissionBriefingDlg::HandleUniverseMinuteTick(uint64 UniverseSecondsNow)
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI) return;
}

void UMissionBriefingDlg::HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds)
{
    if (!MissionTPlusText) return;
}
void UMissionBriefingDlg::UpdateFighterRadioInput()
{
    APlayerController* PC=GetOwningPlayer();
    ULocalPlayer* LP=PC?PC->GetLocalPlayer():nullptr;
    auto* Input=LP?LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>():nullptr;
    if (!Input || !RadioChoiceContext) return;
    if (FighterHUDDetails.IsValid() && FighterHUDDetails->IsRadioOpen())
    {
        if (!Input->HasMappingContext(RadioChoiceContext.Get())) Input->AddMappingContext(RadioChoiceContext.Get(),1100);
    }
    else Input->RemoveMappingContext(RadioChoiceContext.Get());
}
void UMissionBriefingDlg::ToggleFighterRadio()
{
    if (!CanUseFighterHUD() || !FighterHUDDetails.IsValid()) return;
    bFighterHUDVisible=true;
    FighterHUDDetails->ToggleRadio();
    UpdateFighterRadioInput();
}
void UMissionBriefingDlg::SelectFighterRadio(int32 Number)
{
    if (CanUseFighterHUD() && bFighterHUDVisible && FighterHUDDetails.IsValid()) FighterHUDDetails->RadioSelect(Number);
    UpdateFighterRadioInput();
}
void UMissionBriefingDlg::SelectFighterRadio0() { SelectFighterRadio(0); }
void UMissionBriefingDlg::SelectFighterRadio1() { SelectFighterRadio(1); }
void UMissionBriefingDlg::SelectFighterRadio2() { SelectFighterRadio(2); }
void UMissionBriefingDlg::SelectFighterRadio3() { SelectFighterRadio(3); }
void UMissionBriefingDlg::SelectFighterRadio4() { SelectFighterRadio(4); }
void UMissionBriefingDlg::SelectFighterRadio5() { SelectFighterRadio(5); }
void UMissionBriefingDlg::SelectFighterRadio6() { SelectFighterRadio(6); }
void UMissionBriefingDlg::SelectFighterRadio7() { SelectFighterRadio(7); }
void UMissionBriefingDlg::SelectFighterRadio8() { SelectFighterRadio(8); }
void UMissionBriefingDlg::SelectFighterRadio9() { SelectFighterRadio(9); }

void UMissionBriefingDlg::ToggleMissionGear()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();
    Ship* P=S?S->GetPlayerShip():nullptr;
    SimRegion* R=S?S->GetActiveRegion():nullptr;
    if (!P || !R || !R->GetShips().contains(P) || P->IsDead() || P->IsDying()) return;
    LandingGear* Gear=P->GetGear();
    if (!Gear) { UE_LOG(LogTemp,Warning,TEXT("[GearInput] Player has no authored landing gear.")); return; }
    const int32 Before=int32(Gear->GetState());
    P->ToggleGear();
    UE_LOG(LogTemp,Display,TEXT("[GearInput] Ship='%hs' State=%d -> %d (0 UP, 1 LOWERING, 2 DOWN, 3 RAISING)"),
        P->GetName(),Before,int32(Gear->GetState()));
}


void UMissionBriefingDlg::OnMissionThrottleStep(const FInputActionValue& Value)
{
    const float Axis = Value.Get<float>();
    if (!FMath::IsFinite(Axis) || FMath::Abs(Axis) < 0.1f) return;
    ApplyMissionThrottle(Axis > 0.0f ? 5.0 : -5.0, true);
}

void UMissionBriefingDlg::OnMissionThrottleZero()
{
    ApplyMissionThrottle(0.0, false);
}

void UMissionBriefingDlg::OnMissionThrottleFull()
{
    ApplyMissionThrottle(100.0, false);
}

void UMissionBriefingDlg::ApplyMissionThrottle(double Amount, bool bRelative)
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    Ship* PlayerShip = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!PlayerShip || !Region || !Region->GetShips().contains(PlayerShip) ||
        PlayerShip->IsDead() || PlayerShip->IsDying()) return;

    // Accumulate against the request so rapid presses survive engine spool-up.
    const double Requested = FMath::Clamp(
        bRelative ? PlayerShip->GetThrottleRequest() + Amount : Amount, 0.0, 100.0);
    PlayerShip->SetManualThrottle(Requested);
    if (!bRelative && Amount==0.0) PlayerShip->StopManualFlight();
    UE_LOG(LogTemp, Display, TEXT("[ThrottleInput] Requested=%.0f%% Actual=%.1f%%"),
        Requested, PlayerShip->GetThrottle());
}


void UMissionBriefingDlg::ApplyMissionRotation(int32 Axis, float Value)
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    APlayerController* PC = GetOwningPlayer();
    if (PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
    {
        Player->SetManualRotationAxis(Axis, 0.0f);
        return;
    }
    Player->SetManualRotationAxis(Axis, Value);
}

void UMissionBriefingDlg::ClearMissionRotationInput()
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (Player && Region && Region->GetShips().contains(Player)) Player->ClearManualRotationInput();
}

void UMissionBriefingDlg::OnMissionPitch(const FInputActionValue& Value) { ApplyMissionRotation(0, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionPitch() { ApplyMissionRotation(0, 0.0f); }

void UMissionBriefingDlg::OnMissionYaw(const FInputActionValue& Value) { ApplyMissionRotation(1, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionYaw() { ApplyMissionRotation(1, 0.0f); }

void UMissionBriefingDlg::OnMissionRoll(const FInputActionValue& Value) { ApplyMissionRotation(2, Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionRoll() { ApplyMissionRotation(2, 0.0f); }


void UMissionBriefingDlg::OnMissionShieldsUp() { ApplyMissionShields(1); }
void UMissionBriefingDlg::OnMissionShieldsDown() { ApplyMissionShields(-1); }
void UMissionBriefingDlg::OnMissionShieldsFull() { ApplyMissionShields(2); }
void UMissionBriefingDlg::OnMissionShieldsZero() { ApplyMissionShields(-2); }

void UMissionBriefingDlg::ApplyMissionShields(int32 Command)
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    // Plain S/X can also fire while the Shift chord is active; give full/zero priority.
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))
        {
            if (Command == 1) Command = 2;
            if (Command == -1) Command = -2;
        }
    }
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead() || Player->IsDying()) return;
    Shield* Shields = Player->GetShield();
    if (!Shields) return;
    // Match legacy ShipCtrl: step from the current power level, not shield health.
    const double Level = Shields->GetPowerLevel();
    double Requested = 0.0;
    if (Command == 2) Requested = 100.0;
    else if (Command == -2) Requested = 0.0;
    else if (Command > 0)
        Requested = Level < 25.0 ? 25.0 : Level < 50.0 ? 50.0 : Level < 75.0 ? 75.0 : 100.0;
    else
        Requested = Level > 75.0 ? 75.0 : Level > 50.0 ? 50.0 : Level > 25.0 ? 25.0 : 0.0;
    Shields->SetPowerLevel(Requested);
    HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
    UE_LOG(LogTemp, Display, TEXT("[ShieldInput] Requested=%.0f%% Current=%.1f%%"), Requested, Level);
}

void UMissionBriefingDlg::ApplyMissionTranslation(int32 Axis, float Value)
{
    Sim* Simulation=Sim::GetSim();
    Ship* Player=Simulation?Simulation->GetPlayerShip():nullptr;
    SimRegion* Region=Simulation?Simulation->GetActiveRegion():nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    Player->SetManualTranslationAxis(Axis, Value);
}

void UMissionBriefingDlg::ApplyMissionAugmenter(bool Enabled)
{
    Sim* Simulation=Sim::GetSim();
    Ship* Player=Simulation?Simulation->GetPlayerShip():nullptr;
    SimRegion* Region=Simulation?Simulation->GetActiveRegion():nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player)) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused())
    { Player->ClearManualRotationInput(); return; }
    Player->SetManualAugmenter(Enabled);
}

void UMissionBriefingDlg::OnMissionStrafe(const FInputActionValue& Value) { ApplyMissionTranslation(0,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionStrafe() { ApplyMissionTranslation(0,0); }

void UMissionBriefingDlg::OnMissionForwardThrust(const FInputActionValue& Value) { ApplyMissionTranslation(1,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionForwardThrust() { ApplyMissionTranslation(1,0); }

void UMissionBriefingDlg::OnMissionVerticalThrust(const FInputActionValue& Value) { ApplyMissionTranslation(2,Value.Get<float>()); }
void UMissionBriefingDlg::ReleaseMissionVerticalThrust() { ApplyMissionTranslation(2,0); }

void UMissionBriefingDlg::OnMissionAugmenter()
{
    APlayerController* PC = GetOwningPlayer();
    const bool Shift = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
    ApplyMissionAugmenter(!Shift);
}
void UMissionBriefingDlg::ReleaseMissionAugmenter() { ApplyMissionAugmenter(false); }


void UMissionBriefingDlg::ToggleMissionCameraView()
{
    OnMissionCameraCommand(FInputActionValue(true), 7);
}

void UMissionBriefingDlg::UpdateMissionTargetCamera()
{
    UWorld* World = GetWorld();
    APlayerController* PC = GetOwningPlayer();
    if (!World || !PC || !bLiveMissionStarted || !IsValid(MissionPreviewCamera)) return;
    if (bMissionControlsOpen || EngineeringPopup.IsValid() || (WeaponsPopup.IsValid() && !bContactsOnlyPanel) || ObjectivesPopup.IsValid() || FlightOpsPopup.IsValid() || NavigationPopup.IsValid() || (MissionQuitMenu && MissionQuitMenu->IsMenuShown()) || World->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead())
    {
        if (MissionCameraRig) MissionCameraRig->RestoreVisibility(PC);
        return;
    }
    SyncMissionNavigationTarget();
    AShipActor* Visual = Cast<AShipActor>(MissionPlayerCameraActor.Get());
    const double Now = World->GetTimeSeconds();
    if (!IsValid(Visual) || Visual->IsActorBeingDestroyed() || Visual->GetRuntimeShip()!=Player)
    {
        MissionPlayerCameraActor.Reset();
        if (Now < MissionPlayerCameraSearchTime) return;
        MissionPlayerCameraSearchTime = Now + 0.5;
        Visual = nullptr;
        for (TActorIterator<AShipActor> It(World); It; ++It)
        {
            if (!It->IsActorBeingDestroyed() && It->GetRuntimeShip()==Player)
            {
                Visual=*It;
                MissionPlayerCameraActor=Visual;
                // Hull/visual bounds helper excludes exhaust particle bounds.
                MissionPlayerLocalBounds=MissionTargetLocalBounds(Visual);
                MissionPlayerCameraDistance=0;
                break;
            }
        }
        if (!Visual) return;
    }
    const double Delta=FMath::Clamp(Now-MissionTargetLastUpdate,0.0,0.1);
    MissionTargetLastUpdate=Now;
    if (!MissionCameraRig) MissionCameraRig = MakeShared<FMissionCameraRig>();
    MissionCameraRig->Tick(PC, MissionPreviewCamera.Get(), Visual, Player, Region,
        Cast<AShipActor>(SelectedMissionTarget.Get()), Delta, MissionTargetZoomDistance);
}

void UMissionBriefingDlg::BindMissionCameraInput()
{
    APlayerController* PC = GetOwningPlayer();
    ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Input = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Input || !MissionMenuInput) return;
    MissionCameraRig = MakeShared<FMissionCameraRig>();
    MissionCameraContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
    MissionCameraActions.Reset();
    UInputAction* Shift = NewObject<UInputAction>(this, NAME_None, RF_Transient);
    Shift->ValueType = EInputActionValueType::Boolean;
    Shift->bConsumeInput = false;
    MissionCameraActions.Add(Shift);
    MissionCameraContext->MapKey(Shift, EKeys::LeftShift);
    MissionCameraContext->MapKey(Shift, EKeys::RightShift);
    const TCHAR* Names[] = {
        TEXT("IA_CameraCockpit"), TEXT("IA_CameraVirtual"), TEXT("IA_CameraChase"),
        TEXT("IA_CameraDrop"), TEXT("IA_CameraOrbit"), TEXT("IA_CameraTarget"),
        TEXT("IA_CameraThreat"), TEXT("IA_ViewTarget"), TEXT("IA_CameraNextObject"),
        TEXT("IA_CameraWide"), TEXT("IA_CameraLookX"), TEXT("IA_CameraLookY"), TEXT("IA_CameraRange")
    };
    const FKey Keys[] = {EKeys::F1, EKeys::F1, EKeys::F2, EKeys::F2, EKeys::F3,
        EKeys::F4, EKeys::Invalid, EKeys::V, EKeys::Invalid, EKeys::K,
        EKeys::Right, EKeys::Up, EKeys::Add};
    auto IsMapped = [this](UInputAction* Action)
    {
        const UInputMappingContext* Contexts[] = {MissionMenuContext.Get(), FighterInputContext.Get()};
        for (const UInputMappingContext* Context : Contexts)
            if (Context)
                for (const auto& Mapping : Context->GetMappings())
                    if (Mapping.Action == Action) return true;
        return false;
    };
    for (int32 Command = 0; Command < UE_ARRAY_COUNT(Names); ++Command)
    {
        const FString Path = FString::Printf(TEXT("/Game/Input/%s.%s"), Names[Command], Names[Command]);
        UInputAction* Action = LoadObject<UInputAction>(nullptr, *Path);
        const EInputActionValueType Type = Command >= 10 ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
        if (Action && Action->ValueType != Type)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MissionCamera] %s has the wrong value type; using the default binding."), Names[Command]);
            Action = nullptr;
        }
        if (!Action)
        {
            Action = NewObject<UInputAction>(this, NAME_None, RF_Transient);
            Action->ValueType = Type;
        }
        MissionCameraActions.Add(Action);
        if (!IsMapped(Action) && Keys[Command].IsValid())
        {
            auto Map = [this, Action, Shift, Command](FKey Key, bool bNegative)
            {
                auto& Mapping = MissionCameraContext->MapKey(Action, Key);
                if (Command == 1 || Command == 3 || Command == 8 || Command == 10 || Command == 11)
                {
                    auto* Chord = NewObject<UInputTriggerChordAction>(MissionCameraContext.Get());
                    Chord->ChordAction = Shift;
                    Mapping.Triggers.Add(Chord);
                }
                if (bNegative) Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MissionCameraContext.Get()));
            };
            Map(Keys[Command], false);
            if (Command == 10) Map(EKeys::Left, true);
            if (Command == 11) Map(EKeys::Down, true);
            if (Command == 12) Map(EKeys::Subtract, true);
        }
        MissionMenuInput->BindAction(Action, Command >= 10 ? ETriggerEvent::Triggered : ETriggerEvent::Started,
            this, &UMissionBriefingDlg::OnMissionCameraCommand, Command);
        if (Command >= 10)
        {
            MissionMenuInput->BindAction(Action, ETriggerEvent::Completed, this, &UMissionBriefingDlg::ReleaseMissionCameraCommand, Command);
            MissionMenuInput->BindAction(Action, ETriggerEvent::Canceled, this, &UMissionBriefingDlg::ReleaseMissionCameraCommand, Command);
        }
    }
    Input->AddMappingContext(MissionCameraContext.Get(), 1002);
}

void UMissionBriefingDlg::ReleaseMissionCameraCommand(const FInputActionValue&, int32 Command)
{
    if (!MissionCameraRig) return;
    if (Command == 10) MissionCameraRig->LookX = 0.0;
    if (Command == 11) MissionCameraRig->LookY = 0.0;
    if (Command == 12) MissionCameraRig->RangeInput = 0.0;
}

void UMissionBriefingDlg::OnMissionCameraCommand(const FInputActionValue& Value, int32 Command)
{
    if (!CanUseFighterHUD() || !MissionCameraRig || !GetWorld() || GetWorld()->IsPaused()) return;
    APlayerController* PC = GetOwningPlayer();
    const bool Shift = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
    // Guard the unmodified F1/F2 actions when a chord fires in another IMC.
    if ((Command == 0 || Command == 2) && Shift) return;
    if (Command >= 10)
    {
        const float Axis = Value.Get<float>();
        if (!FMath::IsFinite(Axis)) return;
        const double Amount = FMath::Clamp(double(Axis), -1.0, 1.0);
        if (Command == 10) MissionCameraRig->LookX = Amount;
        if (Command == 11) MissionCameraRig->LookY = Amount;
        if (Command == 12) MissionCameraRig->RangeInput = Amount;
        return;
    }
    switch (Command)
    {
    case 0: MissionCameraRig->SetMode(FMissionCameraRig::Cockpit); break;
    case 1: MissionCameraRig->SetMode(FMissionCameraRig::Virtual); break;
    case 2: MissionCameraRig->SetMode(FMissionCameraRig::Chase); break;
    case 3: MissionCameraRig->SetMode(FMissionCameraRig::Drop); break;
    case 4:
        MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
        MissionCameraRig->ViewObject.Reset();
        break;
    case 5: MissionCameraRig->SetMode(FMissionCameraRig::Target); break;
    case 6: MissionCameraRig->SetMode(FMissionCameraRig::Threat); break;
    case 7:
        if (MissionCameraRig->Mode == FMissionCameraRig::Orbit &&
            MissionCameraRig->ViewObject.Get() == SelectedMissionTarget.Get())
            MissionCameraRig->SetMode(FMissionCameraRig::Chase);
        else if (AShipActor* Selected = Cast<AShipActor>(SelectedMissionTarget.Get()))
        {
            Sim* Simulation = Sim::GetSim();
            SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
            Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
            Ship* Data = Selected->GetRuntimeShip();
            if (!Region || !Pilot || !Region->GetShips().contains(Pilot) || !Data || !Region->GetShips().contains(Data)) return;
            SimContact* Contact = Pilot->FindContact(Data);
            if (Data->GetIFF() != Pilot->GetIFF() && (!Contact || !Contact->ActLock())) return;
            MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
            MissionCameraRig->ViewObject = Selected;
        }
        break;
    case 8: CycleMissionViewObject(); break;
    case 9: MissionCameraRig->bWide = !MissionCameraRig->bWide; break;
    default: return;
    }
    bMissionTargetInspection = false; // View-object and weapon-target selection are independent.
    MissionTargetZoomDistance = 1.0;
    UpdateMissionTargetCamera();
}

void UMissionBriefingDlg::CycleMissionViewObject()
{
    Sim* Simulation = Sim::GetSim();
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!MissionCameraRig || !Region || !Pilot || !Region->GetShips().contains(Pilot)) return;
    TArray<AShipActor*> Candidates;
    for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
    {
        Ship* Data = It->GetRuntimeShip();
        if (!Data || !Region->GetShips().contains(Data) || It->IsActorBeingDestroyed() || Data->IsDead() || Data->IsDying()) continue;
        SimContact* Contact = Pilot->FindContact(Data);
        if (Data == Pilot || (Contact && Contact->ActLock())) Candidates.Add(*It);
    }
    Candidates.Sort([](const AShipActor& A, const AShipActor& B) { return A.GetName() < B.GetName(); });
    if (Candidates.IsEmpty()) return;
    AShipActor* Current = MissionCameraRig->ViewObject.IsValid() ? MissionCameraRig->ViewObject.Get()
        : Cast<AShipActor>(MissionPlayerCameraActor.Get());
    const int32 Index = Candidates.IndexOfByKey(Current);
    MissionCameraRig->SetMode(FMissionCameraRig::Orbit);
    MissionCameraRig->ViewObject = Candidates[(Index + 1) % Candidates.Num()];
}

void UMissionBriefingDlg::ToggleEngineering()
{
    CloseNavigationPopup();
    CloseFlightOpsPopup();
    CloseObjectivesPopup();
    CloseWeaponsPopup();
    if (EngineeringPopup.IsValid()) { CloseEngineering(); return; }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    bEngineeringPreviousCursor=PC->bShowMouseCursor;
    UClass* PanelClass=LoadClass<UEngineeringDlg>(nullptr,TEXT("/Game/Screens/inGame/WBP_Engineering.WBP_Engineering_C"));
    EngineeringPanelWidget=CreateWidget<UEngineeringDlg>(PC,PanelClass?PanelClass:UEngineeringDlg::StaticClass());
    if(!EngineeringPanelWidget)return;
    EngineeringPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseEngineering();});
    EngineeringPanelWidget->SetVisibility(ESlateVisibility::Visible);
    EngineeringPopup=EngineeringPanelWidget->TakeWidget();
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(EngineeringPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(EngineeringPopup);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor=true;
}

void UMissionBriefingDlg::CloseEngineering()
{
    if (!EngineeringPopup.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(EngineeringPopup.ToSharedRef());
    EngineeringPopup.Reset();
    if(EngineeringPanelWidget)EngineeringPanelWidget->RemoveFromParent();
    EngineeringPanelWidget=nullptr;
    if (APlayerController* PC=GetOwningPlayer()) {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=bEngineeringPreviousCursor;
    }
}

void UMissionBriefingDlg::ToggleObjectivesPopup()
{
    CloseFlightOpsPopup();
    CloseNavigationPopup();
    CloseEngineering();
    CloseWeaponsPopup();
    if (ObjectivesPopup.IsValid()) { CloseObjectivesPopup(); return; }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    bObjectivesPreviousCursor=PC->bShowMouseCursor;
    UClass* PanelClass = LoadClass<UObjectivesDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_Objectives.WBP_Objectives_C"));
    ObjectivesPanelWidget = CreateWidget<UObjectivesDlg>(PC, PanelClass ? PanelClass : UObjectivesDlg::StaticClass());
    if (!ObjectivesPanelWidget) return;
    ObjectivesPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this, [this]() { CloseObjectivesPopup(); });
    ObjectivesPanelWidget->SetVisibility(ESlateVisibility::Visible);
    ObjectivesPopup = ObjectivesPanelWidget->TakeWidget();
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(ObjectivesPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(ObjectivesPopup);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor=true;
}

void UMissionBriefingDlg::CloseObjectivesPopup()
{
    if (!ObjectivesPopup.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(ObjectivesPopup.ToSharedRef());
    ObjectivesPopup.Reset();
    if (ObjectivesPanelWidget) ObjectivesPanelWidget->RemoveFromParent();
    ObjectivesPanelWidget = nullptr;
    if (APlayerController* PC=GetOwningPlayer()) {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=bObjectivesPreviousCursor;
    }
}



void UMissionBriefingDlg::ToggleFlightOpsPopup()
{
    CloseNavigationPopup();
    CloseEngineering();
    CloseWeaponsPopup();
    if (FlightOpsPopup.IsValid()) { CloseFlightOpsPopup(); return; }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    bFlightOpsPreviousCursor=PC->bShowMouseCursor;
    UClass* PanelClass = LoadClass<UFlightOpsDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_FlightOps.WBP_FlightOps_C"));
    FlightOpsPanelWidget = CreateWidget<UFlightOpsDlg>(PC, PanelClass ? PanelClass : UFlightOpsDlg::StaticClass());
    if (!FlightOpsPanelWidget) return;
    FlightOpsPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this, [this]() { CloseFlightOpsPopup(); });
    FlightOpsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    FlightOpsPopup = FlightOpsPanelWidget->TakeWidget();
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(FlightOpsPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(FlightOpsPopup);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor=true;
}

void UMissionBriefingDlg::CloseFlightOpsPopup()
{
    if (!FlightOpsPopup.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(FlightOpsPopup.ToSharedRef());
    FlightOpsPopup.Reset();
    if (FlightOpsPanelWidget) FlightOpsPanelWidget->RemoveFromParent();
    FlightOpsPanelWidget = nullptr;
    if (APlayerController* PC=GetOwningPlayer()) {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=bFlightOpsPreviousCursor;
    }
}



void UMissionBriefingDlg::ToggleNavigationPopup()
{
    UE_LOG(LogTemp, Display, TEXT("[NavigationInput] Open/close requested"));
    CloseFlightOpsPopup();
    CloseObjectivesPopup();
    CloseEngineering();
    CloseWeaponsPopup();
    if (NavigationPopup.IsValid()) { CloseNavigationPopup(); return; }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport())
    {
        UE_LOG(LogTemp, Warning, TEXT("[NavigationInput] Blocked: mission=%d cover=%d controls=%d"), bLiveMissionStarted, MissionSceneCover.IsValid(), bMissionControlsOpen);
        return;
    }
    APlayerController* PC=GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    bNavigationPreviousCursor=PC->bShowMouseCursor;
    UClass* PanelClass=LoadClass<UNavigationDlg>(nullptr,
        TEXT("/Game/Screens/inGame/WBP_Navigation.WBP_Navigation_C"));
    NavigationPanelWidget=CreateWidget<UNavigationDlg>(PC,PanelClass?PanelClass:UNavigationDlg::StaticClass());
    if(!NavigationPanelWidget) { UE_LOG(LogTemp, Error, TEXT("[NavigationInput] Failed to create Navigation widget")); return; }
    UE_LOG(LogTemp, Display, TEXT("[NavigationInput] Created %s"), *NavigationPanelWidget->GetClass()->GetName());
    NavigationPanelWidget->OnPanelClosed=FSimpleDelegate::CreateWeakLambda(this,[this](){CloseNavigationPopup();});
    NavigationPanelWidget->SetVisibility(ESlateVisibility::Visible);
    NavigationPopup=NavigationPanelWidget->TakeWidget();
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(NavigationPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(NavigationPopup);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor=true;
}

void UMissionBriefingDlg::CloseNavigationPopup()
{
    if (!NavigationPopup.IsValid()) return;
    if (GetWorld() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(NavigationPopup.ToSharedRef());
    NavigationPopup.Reset();
    if(NavigationPanelWidget)NavigationPanelWidget->RemoveFromParent();
    NavigationPanelWidget=nullptr;
    if (APlayerController* PC=GetOwningPlayer()) {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor=bNavigationPreviousCursor;
    }
}



void UMissionBriefingDlg::FireMissionPrimary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FirePrimary();
}

void UMissionBriefingDlg::FireMissionSecondary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FireSecondary();
}

void UMissionBriefingDlg::FireMissionDecoy()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->FireDecoy();
}

void UMissionBriefingDlg::LaunchMissionProbe()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->LaunchProbe();
}

void UMissionBriefingDlg::CycleMissionPrimary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->CyclePrimary();
}

void UMissionBriefingDlg::CycleMissionSecondary()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->CycleSecondary();
}

void UMissionBriefingDlg::CycleMissionPattern()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    if (auto* G=P->GetSecondaryGroup()) G->SetControlMode(static_cast<WeaponsControl>((static_cast<int32>(G->GetControlMode())+1)%3));
}

void UMissionBriefingDlg::CycleMissionSweep()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    if (auto* G=P->GetPrimaryGroup()) G->SetSweep(static_cast<WeaponsSweep>((static_cast<int32>(G->GetSweep())+1)%3));
}

void UMissionBriefingDlg::DropMissionWeaponTarget()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim();
    Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead() || P->IsDying()) return;
    P->DropTarget();
}

void UMissionBriefingDlg::ToggleContactsPanel()
{
    CloseNavigationPopup();
    CloseFlightOpsPopup();
    CloseObjectivesPopup();
    CloseEngineering();
    if (WeaponsPopup.IsValid()) {
        const bool WasContacts = bContactsOnlyPanel;
        CloseWeaponsPopup();
        if (WasContacts) return;
    }
    if (!CanUseFighterHUD() || !GetWorld() || !GetWorld()->GetGameViewport()) return;
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    ClearMissionRotationInput();
    if (MissionCameraRig) MissionCameraRig->LookX=MissionCameraRig->LookY=MissionCameraRig->RangeInput=0;
    if (FighterHUDDetails.IsValid()) FighterHUDDetails->CloseRadio();
    UpdateFighterRadioInput();
    UClass* PanelClass = LoadClass<UContactsDlg>(nullptr,
        TEXT("/Game/Screens/inGame/BP_ContactsList.BP_ContactsList_C"));
    WeaponsPanelWidget = CreateWidget<UContactsDlg>(PC, PanelClass ? PanelClass : UContactsDlg::StaticClass());
    if (!WeaponsPanelWidget) return;
    bWeaponsPreviousCursor = PC->bShowMouseCursor;
    bContactsOnlyPanel = true;
    CastChecked<UContactsDlg>(WeaponsPanelWidget)->SetMissionHost(this);
    WeaponsPanelWidget->OnPanelClosed = FSimpleDelegate::CreateWeakLambda(this,[this](){CloseWeaponsPopup();});
    WeaponsPanelWidget->SetVisibility(ESlateVisibility::Visible);
    const TSharedRef<SWidget> Panel = WeaponsPanelWidget->TakeWidget();
    WeaponsPopup = SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(12)
        [SNew(SBox)
            .WidthOverride_Lambda([PC](){ int32 X=0,Y=0;PC->GetViewportSize(X,Y);return FOptionalSize(FMath::Min(640.0f,X*0.38f)); })
            .HeightOverride_Lambda([PC](){ int32 X=0,Y=0;PC->GetViewportSize(X,Y);return FOptionalSize(Y*0.9f); })
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                [SNew(SBox).WidthOverride(640).HeightOverride(900)[Panel]]]];
    GetWorld()->GetGameViewport()->AddViewportWidgetContent(WeaponsPopup.ToSharedRef(),1100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(Panel);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
}

void UMissionBriefingDlg::CycleMissionSensor()
{
    APlayerController* SensorPC = GetOwningPlayer();
    if (SensorPC && (SensorPC->IsInputKeyDown(EKeys::LeftShift) || SensorPC->IsInputKeyDown(EKeys::RightShift) || SensorPC->IsInputKeyDown(EKeys::LeftControl) || SensorPC->IsInputKeyDown(EKeys::RightControl))) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) { if (S->GetMode()>=ESensorMode::PST) P->DropTarget(); else P->SetSensorMode(S->GetMode()==ESensorMode::GM?ESensorMode::PAS:static_cast<ESensorMode>(static_cast<int32>(S->GetMode())+1)); }
}

void UMissionBriefingDlg::MissionSensorGround()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if(P->IsAirborne() && P->GetSensor() && P->GetSensorMode()<ESensorMode::PST) P->SetSensorMode(ESensorMode::GM);
}

void UMissionBriefingDlg::ReleaseSensorRangeUp() { bSensorRangeUpHeld = false; }
void UMissionBriefingDlg::ReleaseSensorRangeDown() { bSensorRangeDownHeld = false; }

void UMissionBriefingDlg::MissionSensorRangeUp()
{
    if (bSensorRangeUpHeld) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) {
        bSensorRangeUpHeld = true;
        const double Before = S->GetBeamRange();
        S->IncreaseRange();
        UE_LOG(LogTemp, Display, TEXT("[SensorRange] Up %.0f -> %.0f metres"), Before, S->GetBeamRange());
    }
}

void UMissionBriefingDlg::MissionSensorRangeDown()
{
    if (bSensorRangeDownHeld) return;
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    if (auto* S=P->GetSensor()) {
        bSensorRangeDownHeld = true;
        const double Before = S->GetBeamRange();
        S->DecreaseRange();
        UE_LOG(LogTemp, Display, TEXT("[SensorRange] Down %.0f -> %.0f metres"), Before, S->GetBeamRange());
    }
}

void UMissionBriefingDlg::MissionEMCONUp()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->SetEMCON(P->GetEMCON()+1);
}

void UMissionBriefingDlg::MissionEMCONDown()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->SetEMCON(P->GetEMCON()-1);
}

void UMissionBriefingDlg::LockMissionClosest()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHIP,true,false);
}

void UMissionBriefingDlg::LockMissionHostile()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHIP,true,true);
}

void UMissionBriefingDlg::LockMissionThreat()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->LockTarget(ESimObject::SHOT,true,true);
}

void UMissionBriefingDlg::CycleMissionSubsystem()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation=Sim::GetSim(); Ship* P=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!P || P->IsDead()) return;
    P->CycleSubTarget(1);
}

void UMissionBriefingDlg::CycleMissionSubsystemPrevious()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();Ship* P=S?S->GetPlayerShip():nullptr;
    if(P && !P->IsDead()){P->CycleSubTarget(-1);}
}

void UMissionBriefingDlg::LockMissionHostileThreat()
{
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* S=Sim::GetSim();Ship* P=S?S->GetPlayerShip():nullptr;
    if(P && !P->IsDead()){P->LockTarget(ESimObject::SHOT,false,true);}
}

void UMissionBriefingDlg::ToggleMissionAutoNav()
{
    UE_LOG(LogTemp, Display, TEXT("[AutoNavInput] Started; controls=%d paused=%d"),
        CanUseFighterHUD() ? 1 : 0, GetWorld() && GetWorld()->IsPaused() ? 1 : 0);
    if (!CanUseFighterHUD() || !GetWorld() || GetWorld()->IsPaused()) return;
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!Player || !Region || !Region->GetShips().contains(Player) || Player->IsDead()) return;
    if (LastAutoNavInputFrame == GFrameCounter) return;
    LastAutoNavInputFrame = GFrameCounter;
    if (Player->GetInbound())
    {
        Player->SetAutoLand(!Player->IsAutoLandEngaged());
        SyncMissionNavigationTarget();
        return;
    }
    // Explicit autonav input commits a selected route waypoint. Selection alone
    // never changes the flight plan. B on the current destination remains a toggle.
    if (AWaypointActor* Chosen = Cast<AWaypointActor>(SelectedMissionTarget.Get())) {
        SimElement* Flight = Player->GetElement();
        if (Flight && Chosen->ElementIdentity == Flight->GetIdentity()) {
            List<Instruction>& Plan = Player->GetFlightPlan();
            const int RouteOffset = Chosen->WaypointIndex - 1;
            if (RouteOffset >= 0 && RouteOffset < Plan.size()) {
                Instruction* Destination = Plan.at(RouteOffset);
                if (Destination && Destination->GetStatus() <= INSTRUCTION_STATUS::ACTIVE && Destination != Player->GetNextNavPoint()) {
                    // Skipping is not completion: do not emit arrival mission events.
                    for (int RoutePrior = 0; RoutePrior < RouteOffset; ++RoutePrior) {
                        Instruction* Prior = Plan.at(RoutePrior);
                        if (Prior && (Prior->GetStatus() <= INSTRUCTION_STATUS::ACTIVE ||
                            (Prior->GetStatus() == INSTRUCTION_STATUS::COMPLETE && Prior->GetHoldTime() > 0)))
                            Prior->SetStatus(INSTRUCTION_STATUS::SKIPPED);
                    }
                    Flight->SetHoldTime(0);
                    Destination->SetStatus(INSTRUCTION_STATUS::PENDING);
                    bAutoNavSelectionActive = false;
                    Player->SetAutoNav(true);
                    UE_LOG(LogTemp, Display, TEXT("[AutoNavInput] Selected waypoint=%d Engaged=%d"),
                        Chosen->WaypointIndex, Player->IsAutoNavEngaged() ? 1 : 0);
                    return;
                }
            }
        }
    }
    Player->SetAutoNav(!Player->IsAutoNavEngaged());
}


void UMissionBriefingDlg::SyncMissionNavigationTarget()
{
    if (!bLiveMissionStarted || !GetWorld()) return;
    Sim* LiveSim = Sim::GetSim();
    Ship* Pilot = LiveSim ? LiveSim->GetPlayerShip() : nullptr;
    SimRegion* ActiveRegion = LiveSim ? LiveSim->GetActiveRegion() : nullptr;
    if (!Pilot || !ActiveRegion || !ActiveRegion->GetShips().contains(Pilot) || !Pilot->GetElement()) return;
    InboundSlot* Arrival=Pilot->GetInbound();
    FlightDeck* RecoveryDeck=Arrival?Arrival->GetDeck():nullptr;
    const Ship* RecoveryCarrier=RecoveryDeck?RecoveryDeck->GetCarrier():nullptr;
    if(Arrival && Arrival->Cleared() && RecoveryCarrier)
    {
        if(!bRecoveryTargetSelected || Pilot->IsAutoLandEngaged())
            for(TActorIterator<AShipActor> It(GetWorld());It;++It)
                if(It->GetRuntimeShip()==RecoveryCarrier)
                {
                    if(SelectedMissionTarget.Get()!=*It) SelectMissionContact(*It);
                    bRecoveryTargetSelected=SelectedMissionTarget.Get()==*It;
                    break;
                }
        return;
    }
    bRecoveryTargetSelected=false;
    Instruction* RoutePoint = Pilot->GetNextNavPoint();
    const bool Engaged = Pilot->IsAutoNavEngaged();
    const bool AutoChanged = Engaged && (!bAutoNavSelectionActive || AutoNavSelectedRoutePoint != RoutePoint);
    if (!Engaged) { bAutoNavSelectionActive = false; AutoNavSelectedRoutePoint = nullptr; }
    if (!RoutePoint) return;
    const int32 RouteIndex = Pilot->GetNavIndex(RoutePoint);
    AWaypointActor* SelectedWaypoint = Cast<AWaypointActor>(SelectedMissionTarget.Get());
    const bool OwnWaypoint = SelectedWaypoint && SelectedWaypoint->ElementIdentity == Pilot->GetElement()->GetIdentity();
    bool CompletedSelection = false;
    if (OwnWaypoint && SelectedWaypoint->WaypointIndex != RouteIndex) {
        List<Instruction>& Plan = Pilot->GetFlightPlan();
        const int32 PreviousIndex = SelectedWaypoint->WaypointIndex - 1;
        if (PreviousIndex >= 0 && PreviousIndex < Plan.size()) {
            Instruction* Previous = Plan.at(PreviousIndex);
            CompletedSelection = Previous && Previous->GetStatus() > INSTRUCTION_STATUS::ACTIVE;
        }
    }
    // A completed selected waypoint follows the route even in manual flight.
    // Updating target display never engages autonav or modifies the route.
    if (!AutoChanged && !CompletedSelection) return;
    const TArray<AActor*> Candidates = GetSelectableMissionContacts();
    AActor* Destination = nullptr;
    if (!OwnWaypoint) {
        if (SimObject* Object = RoutePoint->GetTarget()) {
            for (TActorIterator<AShipActor> It(GetWorld()); It; ++It)
                if (It->GetRuntimeShip() == Object && Candidates.Contains(*It)) { Destination = *It; break; }
        }
    }
    if (!Destination) {
        for (TActorIterator<AWaypointActor> It(GetWorld()); It; ++It)
            if (It->ElementIdentity == Pilot->GetElement()->GetIdentity() &&
                It->WaypointIndex == RouteIndex && Candidates.Contains(*It)) { Destination = *It; break; }
    }
    if (!Destination) return;
    SelectMissionContact(Destination);
    if (SelectedMissionTarget.Get() == Destination) {
        AutoNavSelectedRoutePoint = RoutePoint;
        bAutoNavSelectionActive = Engaged;
        UE_LOG(LogTemp, Display, TEXT("[NavTargetSync] Waypoint=%d Actor=%s Auto=%d"), RouteIndex, *Destination->GetName(), Engaged ? 1 : 0);
    }
}

