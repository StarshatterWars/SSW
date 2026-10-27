/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           MenuDlg.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Main Menu dialog (legacy MenuDlg) for Unreal UMG.

    CHANGE NOTE (OPTIONS HUB)
    =========================
    Audio/Video/Controls/Keyboard/Joystick/Mods are now managed exclusively by
    UOptionsScreen via a WidgetSwitcher. Therefore:

      - MenuDlg no longer binds or routes any "Video", "Controls", or "Mod" buttons.
      - MenuDlg exposes ONLY one "Options" entry point: ShowOptionsScreen().
      - Sub-option navigation is handled entirely within OptionsScreen.

=============================================================================*/

#include "MenuDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "MissionUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "StarshatterGameDataSubsystem.h"

// Legacy
#include "Starshatter.h"
#include "Campaign.h"
#include "MenuScreen.h"
#include "MultiplayerDlg.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMenuDlg, Log, All);

UMenuDlg::UMenuDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UMenuDlg::RebuildWidget()
{
    if (!WidgetTree)
        WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
    BuildModernMenu();
    return Super::RebuildWidget();
}

void UMenuDlg::BuildModernMenu()
{
    if (!WidgetTree || WidgetTree->FindWidget(TEXT("SSWMenu_ModernRoot"))) return;

    // Reuse designer-authored buttons if present. Preserve their child content,
    // normal/hover/pressed brushes, dimensions and Blueprint-selected typography.
    UButton* AuthoredGameButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnStart")));
    UButton* AuthoredTacButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnTac")));
    UButton* AuthoredExitButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnQuit")));
    bDesignerGameButton = (AuthoredGameButton != nullptr);
    bDesignerTacButton = (AuthoredTacButton != nullptr);
    bDesignerExitButton = (AuthoredExitButton != nullptr);

    // The designer buttons are moved into the native dashboard; they are not
    // recreated, so Widget Blueprint styling stays authoritative.
    if (AuthoredGameButton) AuthoredGameButton->RemoveFromParent();
    if (AuthoredTacButton) AuthoredTacButton->RemoveFromParent();
    if (AuthoredExitButton) AuthoredExitButton->RemoveFromParent();
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SSWMenu_ModernRoot"));
    WidgetTree->RootWidget = Root;

    auto Text = [this](const FString& Value, int32 Size, FLinearColor Color, bool Header = false) -> UTextBlock*
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Value));
        Label->SetFont(Header ? MissionUIStyle::GetHeaderFont(Size) : FCoreStyle::GetDefaultFontStyle("Regular", Size));
        Label->SetColorAndOpacity(FSlateColor(Color));
        Label->SetAutoWrapText(true);
        return Label;
    };
    const FLinearColor Primary(0.92f, 0.97f, 0.99f, 1.f);
    const FLinearColor Accent(0.35f, 0.74f, 0.86f, 1.f);
    const FLinearColor Muted(0.65f, 0.74f, 0.79f, 1.f);

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(FLinearColor(0.045f, 0.085f, 0.12f, 1.f));
    UCanvasPanelSlot* BG = Root->AddChildToCanvas(Backdrop);
    BG->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); BG->SetOffsets(FMargin(0.f));

    UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot* Main = Root->AddChildToCanvas(Page);
    Main->SetAnchors(FAnchors(0.07f, 0.06f, 0.93f, 0.94f)); Main->SetOffsets(FMargin(0.f));

    UBorder* Header = WidgetTree->ConstructWidget<UBorder>();
    Header->SetBrushColor(FLinearColor(0.105f, 0.113f, 0.125f, 1.f));
    Header->SetPadding(FMargin(24.f, 14.f));
    Page->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
    UHorizontalBox* HeaderContent = WidgetTree->ConstructWidget<UHorizontalBox>();
    Header->SetContent(HeaderContent);
    UVerticalBox* Brand = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* BrandSlot = HeaderContent->AddChildToHorizontalBox(Brand);
    BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Brand->AddChildToVerticalBox(Text(TEXT("FRACTAL DEV STUDIOS  /  COMMAND INTERFACE"), 13, Accent));
    Brand->AddChildToVerticalBox(Text(TEXT("STARSHATTER WARS"), 38, Primary, true));

    UVerticalBox* Identity = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* IdentitySlot = HeaderContent->AddChildToHorizontalBox(Identity);
    IdentitySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Identity->AddChildToVerticalBox(Text(TEXT("ACTIVE PLAYER"), 12, Accent));
    PlayerIdentityText = Text(TEXT("PLAYER NOT LOADED"), 20, Primary);
    Identity->AddChildToVerticalBox(PlayerIdentityText);
    Identity->AddChildToVerticalBox(Text(TEXT("CURRENT CAMPAIGN"), 12, Accent))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
    CampaignIdentityText = Text(TEXT("NO ACTIVE CAMPAIGN"), 18, Primary);
    Identity->AddChildToVerticalBox(CampaignIdentityText);

    UVerticalBox* MenuArea = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBoxSlot* AreaSlot = Page->AddChildToVerticalBox(MenuArea);
    AreaSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // Button layout is rebuilt uniformly for native and designer-authored buttons.
    // The designer's brush/style remains authoritative for drop-in buttons.
    auto Tile = [this](UButton*& OutButton, const TCHAR* Caption, const TCHAR* Subcaption, int32 Column)
    {
        (void)Caption;
        (void)Subcaption;
        (void)Column;
        OutButton = WidgetTree->ConstructWidget<UButton>();
    };
    UButton *BStart = nullptr, *BCampaign = nullptr, *BMission = nullptr, *BMulti = nullptr;
    UButton *BPlayer = nullptr, *BOptions = nullptr, *BTac = nullptr, *BQuit = nullptr;
    if (AuthoredGameButton) BStart = AuthoredGameButton;
    else Tile(BStart, TEXT("CONTINUE"), TEXT("Resume current operation"), 0);
    Tile(BCampaign, TEXT("CAMPAIGNS"), TEXT("Select or restart a campaign"), 1);
    Tile(BMission, TEXT("MISSIONS"), TEXT("Campaign, single and custom missions"), 0);
    Tile(BMulti, TEXT("MULTIPLAYER"), TEXT("Network operations"), 1);
    Tile(BPlayer, TEXT("PLAYER"), TEXT("Personnel and logbook"), 0);
    Tile(BOptions, TEXT("OPTIONS"), TEXT("Settings and controls"), 1);
    if (AuthoredTacButton) BTac = AuthoredTacButton;
    else Tile(BTac, TEXT("TACTICAL REFERENCE"), TEXT("Ships, weapons and mission roles"), 0);
    if (AuthoredExitButton) BQuit = AuthoredExitButton;
    else Tile(BQuit, TEXT("EXIT TO WINDOWS"), TEXT("Close Starshatter Wars"), 1);
    // All button content shares the same left alignment and exact 16 px inset.
    // Large Serpentine headings are tinted for each sci-fi color family;
    // the subtitle uses the standard, smaller UI face at 16 px.
    auto FormatButtonContent = [this, &Text](UButton* TargetButton,
        const TCHAR* Caption, const TCHAR* Subtitle, const FLinearColor& CaptionColor,
        const FLinearColor& SubtitleColor)
    {
        if (!TargetButton) return;
        while (TargetButton->GetChildrenCount() > 0)
            TargetButton->RemoveChildAt(0);
        UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>();
        UTextBlock* MainLabel = Text(Caption, 32, CaptionColor, true);
        MainLabel->SetJustification(ETextJustify::Left);
        MainLabel->SetAutoWrapText(true);
        UTextBlock* DetailLabel = Text(Subtitle, 16, SubtitleColor, false);
        DetailLabel->SetJustification(ETextJustify::Left);
        DetailLabel->SetAutoWrapText(true);
        Words->AddChildToVerticalBox(MainLabel);
        Words->AddChildToVerticalBox(DetailLabel)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(TargetButton->AddChild(Words)))
        {
            ContentSlot->SetPadding(FMargin(16.f));
            ContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ContentSlot->SetVerticalAlignment(VAlign_Center);
        }
    };
    const FLinearColor BlueTitle(0.82f, 0.93f, 1.f, 1.f);
    const FLinearColor BlueSubtitle(0.69f, 0.83f, 0.92f, 1.f);
    const FLinearColor GreenTitle(0.80f, 1.f, 0.87f, 1.f);
    const FLinearColor GreenSubtitle(0.69f, 0.90f, 0.78f, 1.f);
    const FLinearColor RedTitle(1.f, 0.83f, 0.85f, 1.f);
    const FLinearColor RedSubtitle(0.96f, 0.73f, 0.76f, 1.f);
    FormatButtonContent(BStart, TEXT("CONTINUE"), TEXT("Resume current operation"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BCampaign, TEXT("CAMPAIGNS"), TEXT("Select or restart a campaign"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BMission, TEXT("MISSIONS"), TEXT("Campaign, single and custom missions"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BMulti, TEXT("MULTIPLAYER"), TEXT("Network operations"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BPlayer, TEXT("PLAYER"), TEXT("Personnel and logbook"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BOptions, TEXT("OPTIONS"), TEXT("Settings and controls"), BlueTitle, BlueSubtitle);
    FormatButtonContent(BTac, TEXT("TACTICAL REFERENCE"), TEXT("Ships, weapons and mission roles"), GreenTitle, GreenSubtitle);
    FormatButtonContent(BQuit, TEXT("EXIT TO WINDOWS"), TEXT("Close Starshatter Wars"), RedTitle, RedSubtitle);
    UButton* Buttons[8] = { BStart, BCampaign, BMission, BMulti, BPlayer, BOptions, BTac, BQuit };
    for (int32 RowIndex = 0; RowIndex < 4; ++RowIndex)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        UVerticalBoxSlot* RowSlot = MenuArea->AddChildToVerticalBox(Row);
        RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
        for (int32 ColIndex = 0; ColIndex < 2; ++ColIndex)
        {
            UHorizontalBoxSlot* TileSlot = Row->AddChildToHorizontalBox(Buttons[RowIndex * 2 + ColIndex]);
            TileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TileSlot->SetPadding(FMargin(0.f, 0.f, ColIndex == 0 ? 10.f : 0.f, 0.f));
        }
    }

    DescriptionText = Text(TEXT(""), 13, Muted);
    Page->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(6.f, 0.f, 6.f, 8.f));
    VersionText = Text(TEXT(""), 12, Muted);
    Page->AddChildToVerticalBox(VersionText);

    BtnStart = BStart; BtnCampaign = BCampaign; BtnMission = BMission;
    BtnMulti = BMulti; BtnPlayer = BPlayer; BtnOptions = BOptions;
    BtnTac = BTac; BtnQuit = BQuit;
}

void UMenuDlg::RefreshIdentityLabels()
{
    if (!PlayerIdentityText || !CampaignIdentityText) return;
    FString Name = TEXT("NO PLAYER SAVE");
    FString CampaignName = TEXT("NO ACTIVE CAMPAIGN");
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            if (!PlayerSS->HasLoaded()) PlayerSS->LoadPlayer();
            if (PlayerSS->HadExistingSaveOnLoad())
            {
                const FS_PlayerGameInfo& Info = PlayerSS->GetPlayerInfo();
                Name = Info.Name.IsEmpty() ? TEXT("UNNAMED PLAYER") : Info.Name;
                if (Info.Campaign > 0)
                {
                    if (UStarshatterGameDataSubsystem* Data = GI->GetSubsystem<UStarshatterGameDataSubsystem>())
                    {
                        if (const FS_Campaign* Row = Data->GetCampaignByIndex1Based(Info.Campaign))
                            CampaignName = Row->Name;
                    }
                }
            }
        }
    }
    PlayerIdentityText->SetText(FText::FromString(Name));
    CampaignIdentityText->SetText(FText::FromString(CampaignName));
}

void UMenuDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Static strings (only need to set once)
    AltStart = TEXT("Start a new game, or resume your current game");
    AltCampaign = TEXT("Start a new dynamic campaign, or load a saved game");
    AltMission = TEXT("Play or create a scripted mission exercise");
    AltMulti = TEXT("Start or join a multiplayer scenario");
    AltPlayer = TEXT("Manage your logbook and player preferences");

    // IMPORTANT: all sub-option routing is now inside OptionsScreen:
    AltOptions = TEXT("Audio, Video, Gameplay, Control, and Mod configuration options");

    AltTac = TEXT("View ship and weapon stats and mission roles");
    AltQuit = TEXT("Exit Starshatter and return to Windows");

    BindUMGDelegates();
}

void UMenuDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildModernMenu();
    BindUMGDelegates();
    // APPLY UI THEME (OPTION A: already loaded from Project Settings during BOOT)
    if (UGameInstance* GI = GetGameInstance())
    {
        if (auto* StyleSS = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
        {
            if (!bDesignerGameButton) StyleSS->ApplyMenuButtonStyle(BtnStart);
            StyleSS->ApplyMenuButtonStyle(BtnCampaign);
            StyleSS->ApplyMenuButtonStyle(BtnMission);
            StyleSS->ApplyMenuButtonStyle(BtnPlayer);
            StyleSS->ApplyMenuButtonStyle(BtnMulti);
            StyleSS->ApplyMenuButtonStyle(BtnOptions);
            if (!bDesignerTacButton) StyleSS->ApplyMenuButtonStyle(BtnTac);
            if (!bDesignerExitButton) StyleSS->ApplyMenuButtonStyle(BtnQuit);
        }
    }

    // Use the exact texture assets supplied for the normal and pressed states.
    // Hover uses the pressed variant, providing an unambiguous highlight.
    auto LoadButtonTexture = [](const TCHAR* AssetPath) -> UTexture2D*
    {
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, AssetPath);
        if (!Texture)
            UE_LOG(LogMenuDlg, Warning, TEXT("[MainMenu] Missing button texture: %s"), AssetPath);
        return Texture;
    };
    UTexture2D* BlueNormal = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Blue_n.T_Btn02Blue_n"));
    UTexture2D* BluePressed = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Blue_p.T_Btn02Blue_p"));
    UTexture2D* RedNormal = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Red_n.T_Btn02Red_n"));
    UTexture2D* RedPressed = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Red_p.T_Btn02Red_p"));
    UTexture2D* GreenNormal = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Green_n.T_Btn02Green_n"));
    UTexture2D* GreenPressed = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Green_p.T_Btn02Green_p"));

    const TArray<UButton*> MenuButtons = {
        BtnStart, BtnCampaign, BtnMission, BtnMulti,
        BtnPlayer, BtnOptions, BtnTac, BtnQuit
    };
    for (UButton* StyledButton : MenuButtons)
    {
        if (!StyledButton) continue;
        if ((StyledButton == BtnStart && bDesignerGameButton) ||
            (StyledButton == BtnTac && bDesignerTacButton) ||
            (StyledButton == BtnQuit && bDesignerExitButton))
            continue; // Blueprint button appearance is authoritative.
        const bool bExit = StyledButton == BtnQuit;
        const bool bTactical = StyledButton == BtnTac;
        UTexture2D* NormalTexture = bExit ? RedNormal : (bTactical ? GreenNormal : BlueNormal);
        UTexture2D* PressedTexture = bExit ? RedPressed : (bTactical ? GreenPressed : BluePressed);
        if (!PressedTexture) PressedTexture = NormalTexture;

        FButtonStyle ButtonStyle = StyledButton->GetStyle();
        if (NormalTexture)
        {
            ButtonStyle.Normal.SetResourceObject(NormalTexture);
            ButtonStyle.Hovered.SetResourceObject(PressedTexture);
            ButtonStyle.Pressed.SetResourceObject(PressedTexture);
            ButtonStyle.Disabled.SetResourceObject(NormalTexture);
            ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::Image;
            ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::Image;
            ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::Image;
            ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::Image;
            // Preserve the authored texture colors; only brighten interaction states.
            ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::White);
            ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(1.08f, 1.08f, 1.08f, 1.f));
            ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::White);
            ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(0.45f, 0.48f, 0.50f, 0.65f));
        }
        else
        {
            // Missing assets: retain distinct, functional native UI states.
            const FLinearColor BaseColor = bExit ? FLinearColor(0.43f, 0.10f, 0.14f, 1.f)
                : (bTactical ? FLinearColor(0.10f, 0.34f, 0.26f, 1.f) : FLinearColor(0.16f, 0.27f, 0.36f, 1.f));
            const FLinearColor HighlightColor = bExit ? FLinearColor(0.75f, 0.20f, 0.24f, 1.f)
                : (bTactical ? FLinearColor(0.19f, 0.57f, 0.39f, 1.f) : FLinearColor(0.30f, 0.49f, 0.62f, 1.f));
            ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
            ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
            ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
            ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::Box;
            ButtonStyle.Normal.Margin = FMargin(0.12f);
            ButtonStyle.Hovered.Margin = FMargin(0.12f);
            ButtonStyle.Pressed.Margin = FMargin(0.12f);
            ButtonStyle.Disabled.Margin = FMargin(0.12f);
            ButtonStyle.Normal.TintColor = FSlateColor(BaseColor);
            ButtonStyle.Hovered.TintColor = FSlateColor(HighlightColor);
            ButtonStyle.Pressed.TintColor = FSlateColor(HighlightColor);
            ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(0.13f, 0.17f, 0.21f, 0.65f));
        }
        ButtonStyle.NormalPadding = FMargin(0.f);
        ButtonStyle.PressedPadding = FMargin(1.f, 2.f, 0.f, 0.f);
        StyledButton->SetStyle(ButtonStyle);
    }
    // Cache legacy singletons (fine to do here)
    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    // Version text
    if (VersionText)
    {
        VersionText->SetText(FText::FromString(Game::VersionInfo));
    }
    else
    {
        UE_LOG(LogMenuDlg, Warning, TEXT("VersionText is not bound (BindWidgetOptional)."));
    }

    // Widget bindings can become available after NativeOnInitialized on some
    // Blueprint-derived menu instances. Rebind idempotently at construction.
    if (BtnMulti)
    {
        BtnMulti->OnClicked.RemoveDynamic(this, &UMenuDlg::OnMultiplayer);
        BtnMulti->OnClicked.AddDynamic(this, &UMenuDlg::OnMultiplayer);
        UE_LOG(LogMenuDlg, Warning, TEXT("[MultiplayerNav] Construct: BtnMulti bound, enabled=%d"), BtnMulti->GetIsEnabled() ? 1 : 0);
    }
    else
    {
        UE_LOG(LogMenuDlg, Error, TEXT("[MultiplayerNav] Construct: BtnMulti missing. Check WBP_MenuDlg BindWidget variable name."));
    }

    RefreshFromPlayerState();
    RefreshIdentityLabels();
    ApplyMenuGating();

    // First run routing AFTER gating
    if (bFirstRun_NoPlayerSave)
    {
        if (Manager)
        {
            Manager->ShowFirstTimeDlg();
        }
        else
        {
            UE_LOG(LogMenuDlg, Warning, TEXT("First run detected, but Manager is null."));
        }
    }

    Show();
}

void UMenuDlg::BindUMGDelegates()
{
    // Idempotent AddUniqueDynamic permits rebinding native buttons after widget reconstruction.

    // Click bindings:
    if (BtnStart)    BtnStart->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnStart);
    if (BtnCampaign) BtnCampaign->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnCampaign);
    if (BtnMission)  BtnMission->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnMission);
    if (BtnPlayer)   BtnPlayer->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnPlayer);
    if (BtnMulti)    BtnMulti->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnMultiplayer);

    // SINGLE entry point for all options:
    if (BtnOptions)  BtnOptions->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnOptions);

    if (BtnTac)      BtnTac->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnTacReference);
    if (BtnQuit)     BtnQuit->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnQuit);

    // Hover bindings:
    if (BtnStart) { BtnStart->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Start);       BtnStart->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Start); }
    if (BtnCampaign) { BtnCampaign->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Campaign); BtnCampaign->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Campaign); }
    if (BtnMission) { BtnMission->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Mission);   BtnMission->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Mission); }
    if (BtnPlayer) { BtnPlayer->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Player);    BtnPlayer->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Player); }
    if (BtnMulti) { BtnMulti->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Multi);      BtnMulti->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Multi); }
    if (BtnOptions) { BtnOptions->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Options);   BtnOptions->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Options); }
    if (BtnTac) { BtnTac->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Tac);          BtnTac->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Tac); }
    if (BtnQuit) { BtnQuit->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Quit);        BtnQuit->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Quit); }

    const bool bAny =
        (BtnStart || BtnCampaign || BtnMission || BtnPlayer || BtnMulti ||
            BtnOptions || BtnTac || BtnQuit);

    if (!bAny)
    {
        UE_LOG(LogMenuDlg, Error, TEXT("NO BUTTON WIDGETS ARE BOUND. Check BP widget names match C++ and are variables."));
    }

    bDelegatesBound = true;
}

void UMenuDlg::RefreshFromPlayerState()
{
    // Default assumptions
    bFirstRun_NoPlayerSave = false;
    bHasCampaignSelected = true;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            if (!PlayerSS->HasLoaded())
            {
                PlayerSS->LoadFromBoot();
            }

            const bool bHasPlayerSave = PlayerSS->HadExistingSaveOnLoad();
            bFirstRun_NoPlayerSave = !bHasPlayerSave;

            if (bHasPlayerSave)
            {
                const FS_PlayerGameInfo& Info = PlayerSS->GetPlayerInfo();
                bHasCampaignSelected = (Info.Campaign > 0) && (!Info.CampaignRowName.IsNone());
            }
            else
            {
                bHasCampaignSelected = false;
            }
        }
    }
}

void UMenuDlg::Show()
{
    RefreshFromPlayerState();
    RefreshIdentityLabels();
    ApplyMenuGating();
    // Multiplayer mission selection is available independently of the legacy filesystem mode.
    if (BtnMulti) BtnMulti->SetIsEnabled(true);

    ClearDescription();
}

void UMenuDlg::ExecFrame(double DeltaTime)
{
    (void)DeltaTime;
}

// --------------------------------------------------------------------
// Click handlers
// --------------------------------------------------------------------

void UMenuDlg::OnStart()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnStart()"));

    if (Stars)
        Stars->StartOrResumeGame();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Stars is null in OnStart()."));
}

void UMenuDlg::OnCampaign()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnCampaign()"));

    if (Manager)
        Manager->ShowCampaignSelectDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnCampaign()."));
}

void UMenuDlg::OnMission()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnMission()"));

    if (Manager)
        Manager->ShowMissionSelectDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnMission()."));
}

void UMenuDlg::OnPlayer()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnPlayer()"));

    if (Manager)
        Manager->ShowPlayerDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnPlayer()."));
}

void UMenuDlg::OnMultiplayer()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Warning, TEXT("[MultiplayerNav] CLICK: OnMultiplayer; Manager=%s; enabled=%d"),
        *GetNameSafe(Manager.Get()), BtnMulti && BtnMulti->GetIsEnabled() ? 1 : 0);

    if (!Manager)
    {
        UE_LOG(LogMenuDlg, Error, TEXT("[MultiplayerNav] FAILED: MenuDlg.Manager is null"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PC = Manager->GetOwningPlayer();
        UE_LOG(LogMenuDlg, Warning, TEXT("[MultiplayerNav] Menu owner missing, manager owner=%s"), *GetNameSafe(PC));
    }
    if (!PC)
    {
        UE_LOG(LogMenuDlg, Error, TEXT("[MultiplayerNav] FAILED: neither menu nor manager has an owning player"));
        return;
    }

    if (!MultiplayerScreen)
    {
        MultiplayerScreen = CreateWidget<UMultiplayerDlg>(PC, UMultiplayerDlg::StaticClass());
        if (!MultiplayerScreen)
        {
            UE_LOG(LogMenuDlg, Error, TEXT("[MultiplayerNav] FAILED: CreateWidget<UMultiplayerDlg> returned null"));
            return;
        }
        MultiplayerScreen->SetMenuManager(Manager);
        MultiplayerScreen->InitializeDlg(Manager);
        UE_LOG(LogMenuDlg, Warning, TEXT("[MultiplayerNav] CREATED widget=%s class=%s"),
            *GetNameSafe(MultiplayerScreen.Get()), *GetNameSafe(MultiplayerScreen->GetClass()));
    }

    // Build/refresh while this main menu is still visible, then transfer focus.
    MultiplayerScreen->Show();
    if (!MultiplayerScreen->IsInViewport())
    {
        MultiplayerScreen->AddToViewport(300);
    }
    MultiplayerScreen->SetVisibility(ESlateVisibility::Visible);
    MultiplayerScreen->SetIsEnabled(true);
    MultiplayerScreen->SetIsFocusable(true);
    MultiplayerScreen->SetDialogInputEnabled(true);

    // Previously the menu stayed on screen and retained focus. It is now
    // hidden only after the multiplayer widget has been created successfully.
    SetVisibility(ESlateVisibility::Hidden);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(MultiplayerScreen->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;

    UE_LOG(LogMenuDlg, Warning,
        TEXT("[MultiplayerNav] OPEN: viewport=%d visible=%d widget=%s"),
        MultiplayerScreen->IsInViewport() ? 1 : 0,
        MultiplayerScreen->GetVisibility() == ESlateVisibility::Visible ? 1 : 0,
        *GetNameSafe(MultiplayerScreen.Get()));
}

void UMenuDlg::OnOptions()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnOptions()"));

    if (Manager)
        Manager->ShowOptionsScreen();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnOptions()."));
}

void UMenuDlg::OnTacReference()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnTacReference()"));

    if (Manager)
        Manager->ShowTacRefDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnTacReference()."));
}

void UMenuDlg::OnQuit()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnQuit()"));

    if (Manager)
        Manager->ShowExitDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnQuit()."));
}

// --------------------------------------------------------------------
// Hover handlers
// --------------------------------------------------------------------

void UMenuDlg::OnButtonEnter_Start() { SetDescription(AltStart); }
void UMenuDlg::OnButtonExit_Start() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Campaign() { SetDescription(AltCampaign); }
void UMenuDlg::OnButtonExit_Campaign() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Mission() { SetDescription(AltMission); }
void UMenuDlg::OnButtonExit_Mission() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Player() { SetDescription(AltPlayer); }
void UMenuDlg::OnButtonExit_Player() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Multi() { SetDescription(AltMulti); }
void UMenuDlg::OnButtonExit_Multi() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Options() { SetDescription(AltOptions); }
void UMenuDlg::OnButtonExit_Options() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Tac() { SetDescription(AltTac); }
void UMenuDlg::OnButtonExit_Tac() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Quit() { SetDescription(AltQuit); }
void UMenuDlg::OnButtonExit_Quit() { ClearDescription(); }

// --------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------

void UMenuDlg::ClearDescription()
{
    if (DescriptionText)
        DescriptionText->SetText(FText::GetEmpty());
}

void UMenuDlg::SetDescription(const FString& Text)
{
    if (DescriptionText)
        DescriptionText->SetText(FText::FromString(Text));
}

void UMenuDlg::SetButtonEnabled(UButton* Button, bool bEnable)
{
    if (Button)
        Button->SetIsEnabled(bEnable);
}

void UMenuDlg::EnableMenuButtons(bool bEnable)
{
    SetButtonEnabled(BtnStart, bEnable);
    SetButtonEnabled(BtnCampaign, bEnable);
    SetButtonEnabled(BtnMission, bEnable);
    SetButtonEnabled(BtnPlayer, bEnable);

    // No legacy filesystem gate.
    SetButtonEnabled(BtnMulti, bEnable);

    // Options is a top-level menu entry now (single entry point to all sub-options):
    SetButtonEnabled(BtnOptions, bEnable);

    SetButtonEnabled(BtnTac, bEnable);

    // Quit is always allowed in your legacy logic
    SetButtonEnabled(BtnQuit, true);
}

/* --------------------------------------------------------------------
   ApplyMenuGating
   - MenuDlg gating is based on state, not "first run overlay"
   - First run overlay disabling is handled by MenuScreen
   -------------------------------------------------------------------- */
void UMenuDlg::ApplyMenuGating()
{
    // Default: enable everything (except multiplayer rule inside EnableMenuButtons)
    EnableMenuButtons(true);

    // NO CAMPAIGN SELECTED:
    // Enable: Campaign, Mission, Options, Tac, Quit
    // Disable: Start; multiplayer remains independently available
    if (!bHasCampaignSelected)
    {
        EnableMenuButtons(false);

        SetButtonEnabled(BtnPlayer, true);
        SetButtonEnabled(BtnCampaign, true);
        SetButtonEnabled(BtnMission, true);

        SetButtonEnabled(BtnOptions, true);

        SetButtonEnabled(BtnTac, true);
        SetButtonEnabled(BtnQuit, true);

        // Multiplayer missions do not depend on an existing campaign selection.
        SetButtonEnabled(BtnMulti, true);
    }
}
