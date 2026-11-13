/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025�2026.

    DIALOG:      FirstTimeDlg
    FILE:        FirstTimeDlg.cpp
    AUTHOR:      Carlos Bott

    OVERVIEW
    ========
    First-time player setup dialog.

    Authoritative-only implementation:
      - Reads/writes FS_PlayerGameInfo via UStarshatterPlayerSubsystem
      - Saves via UStarshatterPlayerSubsystem::SavePlayer(true)
      - Initializes the runtime pilot after the first successful save
*/

#include "FirstTimeDlg.h"
#include "FactionInsignia.h"
#include "PlayerCharacter.h"
#include "Campaign.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"

// UMG:
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ButtonSlot.h"
#include "MissionUIStyle.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetSystemLibrary.h"

// UE:
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// Subsystem:
#include "StarshatterPlayerSubsystem.h"

// Router:
#include "MenuScreen.h"
#include "MenuDlg.h"
// #include "PlayerDlg.h" // only if you want to directly refresh PlayerDlg here

DEFINE_LOG_CATEGORY_STATIC(LogFirstTimeDlg, Log, All);

UFirstTimeDlg::UFirstTimeDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UFirstTimeDlg::NativeConstruct()
{
    Super::NativeConstruct();

    PopulateDefaultsIfNeeded();

    if (AcceptBtn)
    {
        AcceptBtn->OnClicked.RemoveAll(this);
        AcceptBtn->OnClicked.AddDynamic(this, &UFirstTimeDlg::OnAcceptClicked);
    }
    else
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("AcceptBtn is NULL (BindWidget name mismatch / IsVariable off)."));
    }

    if (CancelBtn)
    {
        CancelBtn->OnClicked.RemoveAll(this);
        CancelBtn->OnClicked.AddDynamic(this, &UFirstTimeDlg::OnCancelClicked);
    }
}

UStarshatterPlayerSubsystem* UFirstTimeDlg::GetPlayerSubsystem() const
{
    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterPlayerSubsystem>();
}

void UFirstTimeDlg::PopulateDefaultsIfNeeded()
{
    // Play Style
    if (PlayStyleCombo)
    {
        if (PlayStyleCombo->GetOptionCount() == 0)
        {
            PlayStyleCombo->ClearOptions();
            PlayStyleCombo->AddOption(TEXT("Arcade Style"));
            PlayStyleCombo->AddOption(TEXT("Standard Model"));
        }

        if (PlayStyleCombo->GetSelectedIndex() < 0)
            PlayStyleCombo->SetSelectedIndex(0);
    }
    else
    {
        UE_LOG(LogFirstTimeDlg, Warning, TEXT("PlayStyleCombo is NULL (BindWidget mismatch)."));
    }

    if (NameEdit)
    {
        const auto* GameInstance = Cast<USSWGameInstance>(GetGameInstance());
        if (GameInstance && !GameInstance->GetPlatformPilotName().IsEmpty())
        {
            NameEdit->SetText(FText::FromString(GameInstance->GetPlatformPilotName()));
            NameEdit->SetIsReadOnly(true);
        }
        else if (NameEdit->GetHintText().IsEmpty())
            NameEdit->SetHintText(FText::FromString(TEXT("Player Name")));
    }

    if (CallsignEdit && CallsignEdit->GetHintText().IsEmpty())
        CallsignEdit->SetHintText(FText::FromString(TEXT("Player Callsign")));

    PopulateEmpireOptionsIfNeeded();
}

void UFirstTimeDlg::PopulateEmpireOptionsIfNeeded()
{
    if (!EmpireCombo)
    {
        UE_LOG(LogFirstTimeDlg, Warning, TEXT("EmpireCombo is NULL (BindWidget mismatch)."));
        return;
    }

    if (EmpireCombo->GetOptionCount() == 0)
    {
        EmpireCombo->ClearOptions();
        EmpireCombo->AddOption(TEXT("Terellian Alliance"));
        EmpireCombo->AddOption(TEXT("Marakan Hegemony"));

    }

    if (EmpireCombo->GetSelectedIndex() < 0)
        EmpireCombo->SetSelectedIndex(0);
}

EEMPIRE_NAME UFirstTimeDlg::EmpireFromComboIndex(int32 Index) const
{
    // First-run playable factions: Alliance and Marakan. Dantari remains campaign-only.
    switch (Index)
    {
    default:
    case 0: return EEMPIRE_NAME::Terellian;
    case 1: return EEMPIRE_NAME::Marakan;

    }
}

void UFirstTimeDlg::EnsureMinimalFirstRunDefaults(FS_PlayerGameInfo& Info) const
{
    // If you want: guarantee non-empty identity fields on first save.
    // Keep it conservative; your save system can handle empty strings if you prefer.
    if (Info.Name.TrimStartAndEnd().IsEmpty())
        Info.Name = TEXT("NEW PILOT");

    if (Info.Callsign.TrimStartAndEnd().IsEmpty())
        Info.Callsign = TEXT("ROOKIE");
}

void UFirstTimeDlg::ApplyUiToPlayerInfo(FS_PlayerGameInfo& Info) const
{
    // -----------------------------
    // Identity
    // -----------------------------
    const auto* GameInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (GameInstance && !GameInstance->GetPlatformPilotName().IsEmpty())
        Info.Name = GameInstance->GetPlatformPilotName();
    else if (NameEdit)
    {
        const FString PlayerName = NameEdit->GetText().ToString().TrimStartAndEnd();
        if (!PlayerName.IsEmpty())
            Info.Name = PlayerName;
    }

    if (CallsignEdit)
    {
        const FString CS = CallsignEdit->GetText().ToString().TrimStartAndEnd();
        if (!CS.IsEmpty())
            Info.Callsign = CS;
    }

    // -----------------------------
    // Empire (authoritative int32)
    // -----------------------------
    if (EmpireCombo)
    {
        int32 Sel = EmpireCombo->GetSelectedIndex();
        if (Sel < 0) Sel = 0;

        const EEMPIRE_NAME EmpireEnum = EmpireFromComboIndex(Sel);
        Info.Empire = static_cast<int32>(EmpireEnum);
    }

    // -----------------------------
    // Play Style -> PlayerInfo options
    // -----------------------------
    int32 PlayStyleIndex = 0;
    if (PlayStyleCombo)
    {
        PlayStyleIndex = PlayStyleCombo->GetSelectedIndex();
        if (PlayStyleIndex < 0) PlayStyleIndex = 0;
    }

    if (PlayStyleIndex == 0)
    {
        // ARCADE
        Info.FlightModel = 2;
        Info.LandingMode = 1;
        Info.HudMode = 0;
        Info.GunSightMode = true;
    }
    else
    {
        // STANDARD
        Info.FlightModel = 0;
        Info.LandingMode = 0;
        Info.HudMode = 0;
        Info.GunSightMode = false;
    }

    Info.Rank = 0;
    Info.bStartedAsEnsign = false;
}

void UFirstTimeDlg::OnCancelClicked()
{
    UE_LOG(LogFirstTimeDlg, Log, TEXT("First-run setup canceled. Exiting to Windows."));
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UFirstTimeDlg::OnAcceptClicked()
{
    UE_LOG(LogFirstTimeDlg, Warning, TEXT("UFirstTimeDlg::OnAcceptClicked()"));

    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem();
    if (!PlayerSS)
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("Player subsystem missing"));
        return;
    }

    // Make sure subsystem has loaded its authoritative info
    PlayerSS->LoadFromBoot();

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();

    ApplyUiToPlayerInfo(Info);
    EnsureMinimalFirstRunDefaults(Info);

    Info.Campaign = Campaign::TRAINING_CAMPAIGN;
    auto* GI = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Data = GI ? GI->GetSubsystem<UStarshatterGameDataSubsystem>() : nullptr;
    const FS_Campaign* InitialCampaign = Data ? Data->GetCampaignByIndex1Based(Info.Campaign) : nullptr;
    Info.CampaignRowName = InitialCampaign ? InitialCampaign->RowName : NAME_None;

    const bool bOk = PlayerSS->SavePlayer(true);
    PlayerSS->DebugDump(TEXT(" AFTER SavePlayer(true)"));
    if (!bOk)
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("SavePlayer(true) failed"));
        return;
    }

    // Bind the new save to the live pilot before campaign prep or simulation.
    PlayerCharacter::Initialize();
    PlayerCharacter::SetWorldContext(GetGameInstance());
    PlayerCharacter::GetCurrentPlayer()->FromPlayerInfo(Info);
    if (GI)
        GI->LoadOrCreateUniverse(true);
    if (GI && GI->RestoreSavedActiveCampaign())
        GI->LoadOrCreateSelectedCampaignSave();

    // Router back to menu
    if (MenuManager)
    {
        MenuManager->ShowMenuDlg();

        // IMPORTANT: anything that displays player data should refresh FROM SUBSYSTEM/PlayerInfo now.
        if (UMenuDlg* MenuDlg = MenuManager->GetMenuDlg())
        {
            MenuDlg->RefreshFromPlayerState();
            MenuDlg->ApplyMenuGating();
            MenuDlg->SetDialogInputEnabled(true);
        }

        // Apply this EXACT pattern anywhere you show player identity (PlayerDlg, HUD, etc.)
        // if (UPlayerDlg* PlayerDlg = MenuManager->GetPlayerDlg())
        // {
        //     PlayerDlg->RefreshFromPlayerInfo();
        // }
    }
}

TSharedRef<SWidget> UFirstTimeDlg::RebuildWidget()
{
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,NAME_None,RF_Transient);
    BuildModernDialog();
    return Super::RebuildWidget();
}

void UFirstTimeDlg::BuildModernDialog()
{
    if (bModernDialogBuilt || !WidgetTree) return;
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto* Dim=WidgetTree->ConstructWidget<UBorder>();Dim->SetBrushColor(FLinearColor(0,0,0,0.7f));
    auto* DimSlot=Root->AddChildToCanvas(Dim);DimSlot->SetAnchors(FAnchors(0,0,1,1));DimSlot->SetOffsets(FMargin(0));
    auto* Frame=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("FirstRunFrame"));
    Frame->SetWidthOverride(512);Frame->SetHeightOverride(512);
    auto* Modal=Root->AddChildToCanvas(Frame);Modal->SetAnchors(FAnchors(0.5f));Modal->SetAlignment(FVector2D(0.5f));
    Modal->SetSize(FVector2D(512));Modal->SetPosition(FVector2D::ZeroVector);Modal->SetZOrder(1);
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(MissionUIStyle::PanelBG);Panel->SetPadding(FMargin(20));Frame->SetContent(Panel);
    auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();Panel->SetContent(Column);
    auto Text=[&](const TCHAR* Value,int32 Size,bool Heading=false){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));
        T->SetFont(Heading?MissionUIStyle::GetHeaderFont(Size):MissionUIStyle::GetRowFont(Size));T->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f,0.96f,1)));return T;};
    auto* Header=WidgetTree->ConstructWidget<UBorder>();Header->SetBrushColor(FLinearColor(0.105f,0.113f,0.125f));Header->SetPadding(FMargin(12,10));
    auto* HeaderRow=WidgetTree->ConstructWidget<UHorizontalBox>();
    FactionLogo=WidgetTree->ConstructWidget<UImage>();FactionInsignia::Set(FactionLogo,TEXT("Terellian"),42.f);
    HeaderRow->AddChildToHorizontalBox(FactionLogo)->SetPadding(FMargin(0,0,12,0));
    HeaderRow->AddChildToHorizontalBox(Text(TEXT("NEW PILOT"),26,true))->SetVerticalAlignment(VAlign_Center);Header->SetContent(HeaderRow);Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,0,0,12));
    auto* Intro=Text(TEXT("Create your pilot profile to begin Flight Training."),16);Column->AddChildToVerticalBox(Intro)->SetPadding(FMargin(0,0,0,12));
    auto Row=[&](const TCHAR* Label,UWidget* Input){auto* H=WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* LabelBox=WidgetTree->ConstructWidget<USizeBox>();LabelBox->SetWidthOverride(140);LabelBox->SetContent(Text(Label,16));
        H->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);
        auto* InputSlot=H->AddChildToHorizontalBox(Input);InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));InputSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(44);Height->SetContent(H);
        Column->AddChildToVerticalBox(Height)->SetPadding(FMargin(0,0,0,10));};
    NameEdit=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("ModernNameEdit"));
    CallsignEdit=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("ModernCallsignEdit"));
    for(auto* Edit:{NameEdit.Get(),CallsignEdit.Get()}){auto Style=Edit->GetWidgetStyle();Style.SetFont(MissionUIStyle::GetRowFont(16));
        Style.SetForegroundColor(FLinearColor::White);Style.SetBackgroundColor(FLinearColor(0.025f,0.045f,0.065f));Style.SetPadding(FMargin(10,5));Edit->SetWidgetStyle(Style);}
    EmpireCombo=WidgetTree->ConstructWidget<UComboBoxString>();PlayStyleCombo=WidgetTree->ConstructWidget<UComboBoxString>();
    for(auto* Combo:{EmpireCombo.Get(),PlayStyleCombo.Get()}){Combo->Font=MissionUIStyle::GetRowFont(15);Combo->ForegroundColor=FSlateColor(FLinearColor::White);
        auto ComboStyle=Combo->GetWidgetStyle();
        ComboStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor=FSlateColor(FLinearColor(0.025f,0.045f,0.065f));
        ComboStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor=FSlateColor(FLinearColor(0.08f,0.16f,0.22f));
        ComboStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor=FSlateColor(FLinearColor(0.08f,0.16f,0.22f));
        Combo->SetWidgetStyle(ComboStyle);}
    EmpireCombo->OnSelectionChanged.AddUniqueDynamic(this,&UFirstTimeDlg::OnEmpireLogoChanged);
    Row(TEXT("Pilot name"),NameEdit);Row(TEXT("Callsign"),CallsignEdit);Row(TEXT("Empire"),EmpireCombo);Row(TEXT("Flight model"),PlayStyleCombo);
    auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();
    auto Action=[&](TObjectPtr<UButton>& Button,const TCHAR* Color,const TCHAR* Icon,const TCHAR* Tooltip){
        auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetWidthOverride(72);Box->SetHeightOverride(48);
        Button=WidgetTree->ConstructWidget<UButton>();MissionUIStyle::ApplyActionButtonStyle(Button,Color);Button->SetToolTipText(FText::FromString(Tooltip));
        auto* Image=WidgetTree->ConstructWidget<UImage>();
        if (auto* Texture=LoadObject<UTexture2D>(nullptr,Icon)) Image->SetBrushFromTexture(Texture,true);
        Image->SetColorAndOpacity(FLinearColor::White);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* IconSize=WidgetTree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(24);IconSize->SetHeightOverride(24);IconSize->SetContent(Image);
        if(auto* IconSlot=Cast<UButtonSlot>(Button->AddChild(IconSize))){IconSlot->SetPadding(FMargin(9,5));IconSlot->SetHorizontalAlignment(HAlign_Center);IconSlot->SetVerticalAlignment(VAlign_Center);}
        Box->SetContent(Button);
        Actions->AddChildToHorizontalBox(Box)->SetPadding(FMargin(6,0));};
    Action(CancelBtn,TEXT("Red"),TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_OnOff_256x256.T_OnOff_256x256"),TEXT("Exit to Windows"));
    Action(AcceptBtn,TEXT("Green"),TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"),TEXT("Create pilot"));
    auto* ActionSlot=Column->AddChildToVerticalBox(Actions);ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ActionSlot->SetVerticalAlignment(VAlign_Bottom);ActionSlot->SetHorizontalAlignment(HAlign_Right);
    bModernDialogBuilt=true;
}

void UFirstTimeDlg::OnEmpireLogoChanged(FString SelectedItem,ESelectInfo::Type)
{ FactionInsignia::Set(FactionLogo,SelectedItem,42.f); }
