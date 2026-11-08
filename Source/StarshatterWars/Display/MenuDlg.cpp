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
#include "FactionInsignia.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Brushes/SlateColorBrush.h"
#include "MissionUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "StarshatterGameDataSubsystem.h"

// Legacy
#include "Starshatter.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "SSWGameInstance.h"
#include "MenuScreen.h"
#include "MultiplayerDlg.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "StarshatterPlayerSubsystem.h"
#include "CampaignPresentationRules.h"
#include "StarshatterUIStyleSubsystem.h"
#include "TimerSubsystem.h"
#include "CampaignSave.h"
#include "Misc/DateTime.h"

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
    auto Existing=[this](const TCHAR* Name){return Cast<UButton>(WidgetTree->FindWidget(Name));};
    UButton* Start=Existing(TEXT("BtnStart"));
    UButton* Tactical=Existing(TEXT("BtnTac"));
    UButton* Exit=Existing(TEXT("BtnQuit"));
    bDesignerGameButton=Start!=nullptr;bDesignerTacButton=Tactical!=nullptr;bDesignerExitButton=Exit!=nullptr;
    for(UButton* Button:{Start,Tactical,Exit})if(Button)Button->RemoveFromParent();
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("SSWMenu_ModernRoot"));
    WidgetTree->RootWidget=Root;
    auto* Backdrop=WidgetTree->ConstructWidget<UImage>();
    if(auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/SciFiUIComponents/Textures/Bg/T_SciFi_Bg001.T_SciFi_Bg001")))Backdrop->SetBrushFromTexture(Texture);
    Backdrop->SetColorAndOpacity(FLinearColor(.58f,.7f,.85f,1));Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BackgroundSlot=Root->AddChildToCanvas(Backdrop);BackgroundSlot->SetAnchors(FAnchors(0,0,1,1));BackgroundSlot->SetOffsets(FMargin(0));
    auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);
    auto* FitSlot=Root->AddChildToCanvas(Fit);FitSlot->SetAnchors(FAnchors(0,0,1,1));FitSlot->SetOffsets(FMargin(0));
    auto* Frame=WidgetTree->ConstructWidget<USizeBox>();Frame->SetWidthOverride(1920);Frame->SetHeightOverride(1080);Fit->SetContent(Frame);
    auto* Page=WidgetTree->ConstructWidget<UCanvasPanel>();Frame->SetContent(Page);
    auto Place=[Page](UWidget* Widget,float X,float Y,float W,float H){auto* Slot=Page->AddChildToCanvas(Widget);Slot->SetPosition(FVector2D(X,Y));Slot->SetSize(FVector2D(W,H));return Slot;};
    const FLinearColor White(.9f,.96f,1,1),Muted(.4f,.56f,.65f,1),Cyan(.3f,.82f,.95f,1);
    auto Label=[&](const TCHAR* Value,int Size,FLinearColor Color,float X,float Y,float W,float H,bool Header=false){
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Value));
        Text->SetFont(Header?MissionUIStyle::GetHeaderFont(Size):MissionUIStyle::GetInfoLabelFont(Size));
        Text->SetColorAndOpacity(Color);Text->SetAutoWrapText(false);Text->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Text,X,Y,W,H);return Text;
    };
    auto Panel=[&](float X,float Y,float W,float H,FLinearColor Color){
        auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrush(FSlateColorBrush(FLinearColor::White));Border->SetBrushColor(Color);
        Border->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Border,X,Y,W,H);return Border;
    };
    // Quiet contrast behind navigation; the space scene stays visible at left.
    Panel(1052,92,772,760,FLinearColor(.006f,.015f,.026f,.88f));
    Panel(1052,92,2,760,FLinearColor(.13f,.45f,.6f,.65f));
    Panel(96,905,1728,1,FLinearColor(.2f,.48f,.6f,.6f));
    Label(TEXT("FRACTAL DEV STUDIOS"),13,Muted,104,64,600,28);
    Label(TEXT("TACTICAL AND STRATEGIC OPERATIONS SIMULATOR"),14,Cyan,104,376,750,28);
    Label(TEXT("STARSHATTER"),64,White,96,420,960,90,true);
    Label(TEXT("WARS"),64,White,96,499,850,124,true);
    Panel(104,646,78,3,Cyan);
    Label(TEXT("COMMAND YOUR SHIP. LEAD YOUR FLEET."),17,Muted,104,673,850,44);
    auto* Logo=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("SSWMenu_CircularLogo"));
    if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/Materials/M_UISSWCircularLogo.M_UISSWCircularLogo")))Logo->SetBrushFromMaterial(Material);
    else if(auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/SSW_Circular.SSW_Circular")))Logo->SetBrushFromTexture(Texture);
    Logo->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Logo,104,128,214,214);
    Label(TEXT("MAIN MENU"),13,Cyan,1100,130,500,28);
    Label(TEXT("OPERATIONS"),30,White,1100,162,664,48,true);
    auto NewButton=[this](const TCHAR* Name){return WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),Name);};
    BtnStart=Start?Start:NewButton(TEXT("BtnStart"));BtnCampaign=NewButton(TEXT("BtnCampaign"));BtnMission=NewButton(TEXT("BtnMission"));
    BtnPlayer=NewButton(TEXT("BtnPlayer"));BtnMulti=NewButton(TEXT("BtnMulti"));BtnOptions=NewButton(TEXT("BtnOptions"));
    BtnTac=Tactical?Tactical:NewButton(TEXT("BtnTac"));BtnQuit=Exit?Exit:NewButton(TEXT("BtnQuit"));
    auto Card=[&](UButton* Button,const TCHAR* Heading,const TCHAR* Subtitle,float X,float Y,float W,float H,bool Primary=false){
        Button->ClearChildren();Button->SetToolTipText(FText::FromString(Subtitle));Place(Button,X,Y,W,H);
        // Separate hit-test-transparent labels protect against stale BP content.
        const float LabelInset=Button==BtnQuit?66.f:50.f;
        Label(Heading,Primary?26:18,White,X+LabelInset,Y+(Primary?24:18),W-LabelInset-32,36,true);
        Label(Subtitle,11,Muted,X+LabelInset,Y+(Primary?70:58),W-LabelInset-32,26);
    };
    Card(BtnStart,TEXT("CONTINUE"),TEXT("Return to your active campaign"),1100,234,664,116,true);
    Card(BtnCampaign,TEXT("CAMPAIGNS"),TEXT("Choose your operation"),1100,376,324,104);
    Card(BtnMission,TEXT("MISSIONS"),TEXT("Campaign, single and custom"),1440,376,324,104);
    Card(BtnPlayer,TEXT("PLAYER"),TEXT("Personnel and logbook"),1100,498,324,104);
    Card(BtnMulti,TEXT("MULTIPLAYER"),TEXT("Create or join a session"),1440,498,324,104);
    Card(BtnTac,TEXT("REFERENCE"),TEXT("Ships, weapons and tactics"),1100,620,324,104);
    Card(BtnOptions,TEXT("OPTIONS"),TEXT("Display, sound and controls"),1440,620,324,104);
    Card(BtnQuit,TEXT("EXIT TO WINDOWS"),TEXT("Close Starshatter Wars"),1100,750,664,96);
    // Compact, persistent campaign/pilot context; no opaque full-width slab.
    Label(TEXT("ACTIVE CAMPAIGN"),12,Cyan,104,930,560,25);
    CampaignIdentityText=Label(TEXT("NO ACTIVE CAMPAIGN"),19,White,104,960,580,32);
    CampaignTimeText=Label(TEXT("Day 01 00:00:00"),12,Muted,104,1000,580,25);
    Label(TEXT("UNIVERSE TIME"),12,Cyan,720,930,480,25)->SetJustification(ETextJustify::Center);
    UniverseTimeText=Label(TEXT("--"),17,White,720,965,480,32);UniverseTimeText->SetJustification(ETextJustify::Center);
    Label(TEXT("PILOT"),12,Cyan,1330,930,434,25)->SetJustification(ETextJustify::Right);
    PlayerIdentityText=Label(TEXT("PLAYER NOT LOADED"),19,White,1330,960,434,32);PlayerIdentityText->SetJustification(ETextJustify::Right);
    UnitText=Label(TEXT("UNASSIGNED"),12,Muted,1330,1000,434,25);UnitText->SetJustification(ETextJustify::Right);
    DescriptionText=Label(TEXT(""),13,Muted,104,852,1650,35);
    VersionText=Label(TEXT(""),11,Muted,104,1040,700,25);
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
                Name = Info.Name.IsEmpty() ? TEXT("UNNAMED PLAYER") : Info.GetRankedDisplayName();
                if(auto* Logo=Cast<UImage>(GetWidgetFromName(TEXT("SSWMenu_CircularLogo"))))
                {
                    if(Info.Empire==static_cast<int32>(EEMPIRE_NAME::Marakan))FactionInsignia::Set(Logo,TEXT("Marakan"),214.f);
                    else if(auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/Materials/M_UISSWCircularLogo.M_UISSWCircularLogo")))
                        if(Logo->GetBrush().GetResourceObject()!=Material)Logo->SetBrushFromMaterial(Material);
                }
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
    PlayerIdentityText->SetJustification(ETextJustify::Right);
    CampaignIdentityText->SetText(FText::FromString(CampaignName));
    CampaignIdentityText->SetJustification(ETextJustify::Left);
    if (UnitText)
    {
        FString UnitName = TEXT("UNASSIGNED");
        if (Campaign* Live = Campaign::GetCampaign())
            if (CombatGroup* Group = Live->GetPlayerGroup())
                UnitName = Group->GetDescription();
        UnitText->SetText(FText::FromString(UnitName));
        UnitText->SetJustification(ETextJustify::Right);
    }
    RefreshClockLabels();
}

void UMenuDlg::BindMenuClock()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;
    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;
    Timer->OnUniverseSecond.RemoveAll(this);
    Timer->OnUniverseSecond.AddUObject(this, &UMenuDlg::HandleMenuClock);
    RefreshClockLabels();
}

void UMenuDlg::HandleMenuClock(uint64)
{
    RefreshClockLabels();
}

void UMenuDlg::RefreshClockLabels()
{
    if (!UniverseTimeText && !CampaignTimeText) return;
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;
    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer) return;

    const uint64 Now = Timer->GetUniverseTimeSeconds();
    int64 BaseUnixSeconds = Timer->UniverseBaseUnixSeconds;
    if (BaseUnixSeconds <= 0)
        BaseUnixSeconds = FDateTime(2228, 1, 1).ToUnixTimestamp();

    if (UniverseTimeText)
    {
        const FDateTime UniverseDateTime = FDateTime::FromUnixTimestamp(BaseUnixSeconds + static_cast<int64>(Now));
        UniverseTimeText->SetText(FText::FromString(UniverseDateTime.ToString(TEXT("%Y-%m-%d %H:%M:%S"))));
    }

    if (CampaignTimeText)
    {
        uint64 TPlusSeconds = Timer->GetCampaignTPlusSeconds();
        UCampaignSave* Save = Timer->GetCampaignSave();
        if (!Save)
            if (USSWGameInstance* SSW = Cast<USSWGameInstance>(GI))
                Save = SSW->CampaignSave;
        if (Save)
            TPlusSeconds = Save->GetTPlusSeconds(Now);

        const uint64 Days = TPlusSeconds / 86400ULL;
        const uint64 Rest = TPlusSeconds % 86400ULL;
        CampaignTimeText->SetText(FText::FromString(FString::Printf(
            TEXT("Day %02llu  %02llu:%02llu:%02llu"),
            (unsigned long long)(Days + 1ULL),
            (unsigned long long)(Rest / 3600ULL),
            (unsigned long long)((Rest % 3600ULL) / 60ULL),
            (unsigned long long)(Rest % 60ULL))));
        CampaignTimeText->SetJustification(ETextJustify::Left);
    }
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
            StyleSS->ApplyMenuButtonStyle(BtnStart);
            StyleSS->ApplyMenuButtonStyle(BtnCampaign);
            StyleSS->ApplyMenuButtonStyle(BtnMission);
            StyleSS->ApplyMenuButtonStyle(BtnPlayer);
            StyleSS->ApplyMenuButtonStyle(BtnMulti);
            StyleSS->ApplyMenuButtonStyle(BtnOptions);
            StyleSS->ApplyMenuButtonStyle(BtnTac);
            StyleSS->ApplyMenuButtonStyle(BtnQuit);
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
    UTexture2D* CyanNormal = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Cyan_n.T_Btn02Cyan_n"));
    UTexture2D* CyanPressed = LoadButtonTexture(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn02Cyan_p.T_Btn02Cyan_p"));

    const TArray<UButton*> MenuButtons = {
        BtnStart, BtnCampaign, BtnMission, BtnMulti,
        BtnPlayer, BtnOptions, BtnTac, BtnQuit
    };
    for (UButton* StyledButton : MenuButtons)
    {
        if (!StyledButton) continue;
        // Apply the color family even to Blueprint drop-in buttons. Their widget
        // instances/positioning remain designer-authored; visual states use the
        // exact game-supplied textures so an old metallic brush cannot leak through.
        const bool bExit = StyledButton == BtnQuit;
        const bool bTactical = StyledButton == BtnTac;
        const bool bContinue = StyledButton == BtnStart;
        UTexture2D* NormalTexture = bExit ? RedNormal : (bTactical ? CyanNormal : (bContinue ? GreenNormal : BlueNormal));
        UTexture2D* PressedTexture = bExit ? RedPressed : (bTactical ? CyanPressed : (bContinue ? GreenPressed : BluePressed));
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
            ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(.58f,.70f,.82f,.90f));
            ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(.95f, 1.f, 1.f, 1.f));
            ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::White);
            ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(0.45f, 0.48f, 0.50f, 0.65f));
        }
        else
        {
            // Missing assets: retain distinct, functional native UI states.
            const FLinearColor BaseColor = bExit ? FLinearColor(0.43f, 0.10f, 0.14f, 1.f)
                : (bTactical ? FLinearColor(0.08f, 0.36f, 0.44f, 1.f) : (bContinue ? FLinearColor(0.10f, 0.34f, 0.26f, 1.f) : FLinearColor(0.16f, 0.27f, 0.36f, 1.f)));
            const FLinearColor HighlightColor = bExit ? FLinearColor(0.75f, 0.20f, 0.24f, 1.f)
                : (bTactical ? FLinearColor(0.18f, 0.64f, 0.72f, 1.f) : (bContinue ? FLinearColor(0.19f, 0.57f, 0.39f, 1.f) : FLinearColor(0.30f, 0.49f, 0.62f, 1.f)));
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
    BindMenuClock();
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
    BindMenuClock();
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
    UE_LOG(LogMenuDlg, Log, TEXT("[MainMenu] Continue: restoring campaign and opening Operations Missions"));

    // Continue is a campaign action, not an unconditional switch to PLAY mode.
    // Restore the saved campaign before Operations rebuilds its live mission list.
    USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWGI)
    {
        SSWGI->RestoreSavedActiveCampaign();
    }

    Campaign* ActiveCampaign = Campaign::GetCampaign();
    if (!ActiveCampaign)
    {
        UE_LOG(LogMenuDlg, Warning,
            TEXT("[MainMenu] Continue: no active campaign; opening Campaign Selection"));
        if (Manager) Manager->ShowCampaignSelectDlg();
        return;
    }

    UE_LOG(LogMenuDlg, Log,
        TEXT("[MainMenu] Continue: campaign=%d missions=%d"),
        ActiveCampaign->GetCampaignId(), ActiveCampaign->GetMissionList().size());

    if (Manager)
    {
        // Authoritative Operations roster: UCmdMissionsDlg reads the active
        // Campaign::GetMissionList() and filters player-selectable missions.
        Manager->ShowOperationsMissionsDlg();
    }
    else
    {
        UE_LOG(LogMenuDlg, Error, TEXT("[MainMenu] Continue: MenuScreen manager unavailable"));
    }
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
        SetButtonEnabled(BtnMission, true);

        SetButtonEnabled(BtnOptions, true);

        SetButtonEnabled(BtnTac, true);
        SetButtonEnabled(BtnQuit, true);

        // Multiplayer missions do not depend on an existing campaign selection.
        SetButtonEnabled(BtnMulti, true);
    }

    const UGameInstance* GI = GetGameInstance();
    const auto* Players = GI ? GI->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
    const bool bCampaignUnlocked = Players && CampaignPresentationRules::CanSelectCampaign(Players->GetPlayerInfo());
    SetButtonEnabled(BtnCampaign, bCampaignUnlocked);
    if (BtnCampaign)
        BtnCampaign->SetToolTipText(FText::FromString(bCampaignUnlocked
            ? TEXT("Choose your operation") : TEXT("Complete Flight School or start as an Ensign to unlock campaigns")));
}
