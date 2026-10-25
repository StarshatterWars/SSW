/* Starshatter Wars - modern single-mission roster.
   Uses legacy Multiplayer Missions campaign and original PREP-mode handoff.
   The existing MissionSelectDlg is intentionally not modified. */
#include "MultiplayerDlg.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "MissionUIStyle.h"
#include "Campaign.h"
#include "MissionInfo.h"
#include "Mission.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "FormatUtil.h"
#include "MenuScreen.h"
#include "StarshatterUIStyleSubsystem.h"

namespace MultiplayerColors
{
    const FLinearColor Backdrop(0.012f, 0.025f, 0.045f, 1.0f);
    const FLinearColor Panel(0.025f, 0.055f, 0.083f, 0.97f);
    const FLinearColor Row(0.68f, 0.80f, 0.85f, 1.0f);
    const FLinearColor Selected(0.23f, 0.73f, 0.88f, 1.0f);
    const FLinearColor Accent(0.16f, 0.77f, 0.91f, 1.0f);
    const FLinearColor Primary(0.92f, 0.97f, 1.0f, 1.0f);
    const FLinearColor Muted(0.61f, 0.72f, 0.81f, 1.0f);
}

UMultiplayerDlg::UMultiplayerDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UMultiplayerDlg::NativeConstruct()
{
    Super::NativeConstruct();
    BuildMultiplayerScreen();
}

UTextBlock* UMultiplayerDlg::MakeLabel(const FString& Value, int32 Size,
    const FLinearColor& Tint, bool bWrap)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Value));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle(Size >= 22 ? "Bold" : "Regular", Size));
    Label->SetColorAndOpacity(FSlateColor(Tint));
    Label->SetAutoWrapText(bWrap);
    return Label;
}

UBorder* UMultiplayerDlg::MakePanel(const FLinearColor& Tint)
{
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(Tint);
    Panel->SetPadding(FMargin(18.f));
    return Panel;
}

UButton* UMultiplayerDlg::MakeActionButton(const FString& Label, bool bEnabled, bool bRosterButton)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(MultiplayerColors::Row);
    Style.Hovered.TintColor = FSlateColor(MultiplayerColors::Selected);
    Style.Pressed.TintColor = FSlateColor(MultiplayerColors::Accent);
    Button->SetStyle(Style);
    // Shared menu action styling matches Campaign Selection and other menus.
    // Roster entries retain distinct selected-state highlighting.
    if (!bRosterButton)
    {
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UStarshatterUIStyleSubsystem* Styles = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
            {
                Styles->ApplyMenuButtonStyle(Button);
            }
        }
    }
    Button->SetIsEnabled(bEnabled);
    UTextBlock* Text = MakeLabel(Label, bRosterButton ? 17 : 15, FLinearColor::Black);
    if (UButtonSlot* ButtonContent = Cast<UButtonSlot>(Button->AddChild(Text)))
    {
        ButtonContent->SetPadding(FMargin(bRosterButton ? 16.f : 12.f, bRosterButton ? 11.f : 9.f));
        ButtonContent->SetHorizontalAlignment(HAlign_Left);
        ButtonContent->SetVerticalAlignment(VAlign_Center);
    }
    return Button;
}

void UMultiplayerDlg::BuildMultiplayerScreen()
{
    if (!WidgetTree) return;
    if (WidgetTree->FindWidget(TEXT("Multiplayer_Root"))) return;

    // Self-contained layout; WBP may remain the host class with no designer edits.
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Multiplayer_Root"));
    WidgetTree->RootWidget = Root;
    UBorder* Background = MakePanel(MultiplayerColors::Backdrop);
    if (UCanvasPanelSlot* BackgroundCanvas = Root->AddChildToCanvas(Background))
    {
        BackgroundCanvas->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        BackgroundCanvas->SetOffsets(FMargin(0.f));
    }
    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::DownOnly);
    Background->SetContent(Scale);
    USizeBox* FrameSize = WidgetTree->ConstructWidget<USizeBox>();
    FrameSize->SetWidthOverride(1600.f);
    FrameSize->SetHeightOverride(900.f);
    Scale->SetContent(FrameSize);
    UVerticalBox* Full = WidgetTree->ConstructWidget<UVerticalBox>();
    FrameSize->SetContent(Full);

    UVerticalBox* Heading = WidgetTree->ConstructWidget<UVerticalBox>();
    Full->AddChildToVerticalBox(Heading)->SetPadding(FMargin(5.f, 4.f, 5.f, 18.f));
    Heading->AddChildToVerticalBox(MakeLabel(TEXT("STARSHATTER WARS  /  TACTICAL OPERATIONS  /  MULTIPLAYER"), 15, MultiplayerColors::Accent));
    // Dark-grey title band, shared visually with the Campaign Selection header.
    UBorder* SingleTitleBand = MakePanel(FLinearColor(0.105f, 0.113f, 0.125f, 1.0f));
    SingleTitleBand->SetPadding(FMargin(18.f, 11.f, 18.f, 11.f));
    UTextBlock* MultiplayerHeaderText = MakeLabel(TEXT("MULTIPLAYER MISSIONS"), 38, MultiplayerColors::Primary);
    MultiplayerHeaderText->SetFont(MissionUIStyle::GetHeaderFont(38));
    SingleTitleBand->SetContent(MultiplayerHeaderText);
    Heading->AddChildToVerticalBox(SingleTitleBand)->SetPadding(FMargin(0.f, 7.f, 0.f, 7.f));
    Heading->AddChildToVerticalBox(MakeLabel(TEXT("MULTIPLAYER SCENARIOS  /  NETWORK OPERATIONS"), 14, MultiplayerColors::Muted));
    UBorder* HeaderRule = MakePanel(MultiplayerColors::Accent);
    HeaderRule->SetPadding(FMargin(0.f));
    UVerticalBoxSlot* RuleSlot = Heading->AddChildToVerticalBox(HeaderRule);
    RuleSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
    USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>();
    RuleSize->SetHeightOverride(2.f);
    HeaderRule->SetContent(RuleSize);

    UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
    UVerticalBoxSlot* ColumnArea = Full->AddChildToVerticalBox(Columns);
    ColumnArea->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ColumnArea->SetPadding(FMargin(5.f, 0.f, 5.f, 12.f));

    UBorder* LeftPanel = MakePanel(MultiplayerColors::Panel);
    UHorizontalBoxSlot* Left = Columns->AddChildToHorizontalBox(LeftPanel);
    FSlateChildSize LeftColumnSize(ESlateSizeRule::Fill);
    LeftColumnSize.Value = 0.88f;
    Left->SetSize(LeftColumnSize);
    Left->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    UVerticalBox* LeftContent = WidgetTree->ConstructWidget<UVerticalBox>();
    LeftPanel->SetContent(LeftContent);
    LeftContent->AddChildToVerticalBox(MakeLabel(TEXT("AVAILABLE OPERATIONS"), 18, MultiplayerColors::Accent));
    MissionCountLabel = MakeLabel(TEXT(""), 13, MultiplayerColors::Muted);
    LeftContent->AddChildToVerticalBox(MissionCountLabel)->SetPadding(FMargin(0.f, 5.f, 0.f, 12.f));
    UScrollBox* RosterScroll = WidgetTree->ConstructWidget<UScrollBox>();
    LeftContent->AddChildToVerticalBox(RosterScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    MissionRoster = WidgetTree->ConstructWidget<UVerticalBox>();
    RosterScroll->AddChild(MissionRoster);

    UBorder* RightPanel = MakePanel(MultiplayerColors::Panel);
    UHorizontalBoxSlot* Right = Columns->AddChildToHorizontalBox(RightPanel);
    FSlateChildSize RightColumnSize(ESlateSizeRule::Fill);
    RightColumnSize.Value = 1.12f;
    Right->SetSize(RightColumnSize);
    // Constrain mission intelligence to the right column and scroll independently
    // of the roster and the action bar. Long SITREPs/objectives remain accessible.
    MissionDetailsScroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("Multiplayer_DetailsScroll"));
    MissionDetailsScroll->SetOrientation(EOrientation::Orient_Vertical);
    MissionDetailsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    RightPanel->SetContent(MissionDetailsScroll);
    UVerticalBox* RightContent = WidgetTree->ConstructWidget<UVerticalBox>();
    MissionDetailsScroll->AddChild(RightContent);
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("MISSION INTELLIGENCE  /  AUTHORED SCENARIO"), 18, MultiplayerColors::Accent));
    MissionTitle = MakeLabel(TEXT("SELECT A MISSION"), 26, MultiplayerColors::Primary, true);
    MissionTitle->SetFont(MissionUIStyle::GetHeaderFont(26));
    UBorder* MissionTitleBand = MakePanel(FLinearColor(0.105f, 0.113f, 0.125f, 1.0f));
    MissionTitleBand->SetPadding(FMargin(14.f, 10.f, 14.f, 10.f));
    MissionTitleBand->SetContent(MissionTitle);
    RightContent->AddChildToVerticalBox(MissionTitleBand)->SetPadding(FMargin(0.f, 14.f, 0.f, 14.f));
    MissionRole = MakeLabel(TEXT(""), 15, MultiplayerColors::Accent);
    MissionLocation = MakeLabel(TEXT(""), 15, MultiplayerColors::Muted);
    MissionTime = MakeLabel(TEXT(""), 15, MultiplayerColors::Muted);
    RightContent->AddChildToVerticalBox(MissionRole)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    RightContent->AddChildToVerticalBox(MissionLocation)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    RightContent->AddChildToVerticalBox(MissionTime)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("TACTICAL SITUATION"), 17, MultiplayerColors::Accent));
    MissionSummary = MakeLabel(TEXT("Choose a mission from the roster."), 17, MultiplayerColors::Primary, true);
    RightContent->AddChildToVerticalBox(MissionSummary)->SetPadding(FMargin(0.f, 8.f, 0.f, 20.f));
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("MISSION OBJECTIVE"), 17, MultiplayerColors::Accent));
    MissionObjectives = MakeLabel(TEXT("Full objectives and loadout are available in Multiplayer Briefing."), 15, MultiplayerColors::Primary, true);
    RightContent->AddChildToVerticalBox(MissionObjectives)->SetPadding(FMargin(0.f, 8.f));

    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Full->AddChildToVerticalBox(Actions)->SetPadding(FMargin(5.f, 2.f, 5.f, 0.f));
    UButton* BackButton = MakeActionButton(TEXT("BACK TO MAIN MENU"));
    Actions->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    BackButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnBackPressed);

    // Keep legacy editing affordances visible, but disabled for built-in missions.
    Actions->AddChildToHorizontalBox(MakeActionButton(TEXT("NEW MISSION"), false))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    Actions->AddChildToHorizontalBox(MakeActionButton(TEXT("EDIT MISSION"), false))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    Actions->AddChildToHorizontalBox(MakeActionButton(TEXT("DELETE MISSION"), false))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

    LaunchButton = MakeActionButton(TEXT("OPEN MULTIPLAYER BRIEFING"), false);
    Actions->AddChildToHorizontalBox(LaunchButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LaunchButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnLaunchPressed);
}

void UMultiplayerDlg::Show()
{
    // Do not call UMissionSelectDlg::Show(): it switches among single/custom/multiplayer.
    UBaseScreen::Show();
    if (!MissionRoster) BuildMultiplayerScreen();
    RefreshMissionRoster();
}

void UMultiplayerDlg::RefreshMissionRoster()
{
    if (!MissionRoster) return;
    MissionRoster->ClearChildren();
    MissionButtons.Reset();
    MissionIds.Reset();
    ScenarioRows.Reset();
    ChosenIndex = INDEX_NONE;
    SelectedMissionIndex = INDEX_NONE;
    MissionId = 0;
    bEditable = false;

    UStarshatterGameDataSubsystem* GameData = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>() : nullptr;
    UDataTable* DataTable = GameData ? GameData->GetMultiplayerMissionsDataTable() : nullptr;
    if (!DataTable || DataTable->GetRowStruct() != FS_CampaignMission::StaticStruct())
    {
        MissionCountLabel->SetText(FText::FromString(TEXT("DT_MultiplayerMissions unavailable or incorrect row structure")));
        RefreshMissionDetails();
        return;
    }

    TArray<FName> RowNames = DataTable->GetRowNames();
    // Sort by authored mission ID, not by unpredictable DataTable iteration order.
    TArray<const FS_CampaignMission*> SortedRows;
    for (const FName& RowName : RowNames)
    {
        const FS_CampaignMission* Scenario = DataTable->FindRow<FS_CampaignMission>(RowName, TEXT("MultiplayerRoster"), false);
        if (Scenario && Scenario->Id > 0) SortedRows.Add(Scenario);
    }
    SortedRows.Sort([](const FS_CampaignMission& A, const FS_CampaignMission& B) { return A.Id < B.Id; });

    for (const FS_CampaignMission* Scenario : SortedRows)
    {
        if (!Scenario || MissionIds.Num() >= 10) continue;
        const int32 DisplayIndex = MissionIds.Num();
        const FString MissionLabel = Scenario->MissionName.IsEmpty()
            ? FString::Printf(TEXT("%02d  /  UNNAMED OPERATION"), Scenario->Id)
            : Scenario->MissionName;
        UButton* RowButton = MakeActionButton(MissionLabel, true, true);
        switch (DisplayIndex)
        {
        case 0: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission01Pressed); break;
        case 1: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission02Pressed); break;
        case 2: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission03Pressed); break;
        case 3: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission04Pressed); break;
        case 4: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission05Pressed); break;
        case 5: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission06Pressed); break;
        case 6: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission07Pressed); break;
        case 7: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission08Pressed); break;
        case 8: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission09Pressed); break;
        case 9: RowButton->OnClicked.AddDynamic(this, &UMultiplayerDlg::OnMission10Pressed); break;
        default: break;
        }
        MissionRoster->AddChildToVerticalBox(RowButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));
        MissionIds.Add(Scenario->Id);
        ScenarioRows.Add(Scenario);
        MissionButtons.Add(RowButton);
    }
    MissionCountLabel->SetText(FText::FromString(FString::Printf(
        TEXT("%d missions  /  sourced from DT_MultiplayerMissions"), MissionIds.Num())));
    if (!MissionIds.IsEmpty()) SelectMissionAt(0);
    else RefreshMissionDetails();
}

void UMultiplayerDlg::SelectMissionAt(int32 Index)
{
    if (!MissionIds.IsValidIndex(Index)) return;
    ChosenIndex = Index;
    SelectedMissionIndex = Index;
    MissionId = MissionIds[Index];
    for (int32 i = 0; i < MissionButtons.Num(); ++i)
    {
        if (UButton* Button = MissionButtons[i])
        {
            FButtonStyle Style = Button->GetStyle();
            Style.Normal.TintColor = FSlateColor(i == Index ? MultiplayerColors::Selected : MultiplayerColors::Row);
            Button->SetStyle(Style);
        }
    }
    RefreshMissionDetails();
    // Each selected operation opens at the top of its intelligence report.
    if (MissionDetailsScroll) MissionDetailsScroll->ScrollToStart();
}

void UMultiplayerDlg::RefreshMissionDetails()
{
    const FS_CampaignMission* Scenario = ScenarioRows.IsValidIndex(ChosenIndex)
        ? ScenarioRows[ChosenIndex] : nullptr;
    if (MissionTitle) MissionTitle->SetText(FText::FromString(Scenario
        ? Scenario->MissionName : TEXT("NO MISSION SELECTED")));
    if (MissionRole) MissionRole->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("OPERATION TYPE  /  %s"),
            *FString(Mission::GetRoleName(static_cast<int32>(Scenario->MissionType)))) : TEXT("")));
    if (MissionLocation) MissionLocation->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("SYSTEM  /  %s     REGION  /  %s"),
            *Scenario->MissionSystem, *Scenario->MissionRegion) : TEXT("")));
    if (MissionTime) MissionTime->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("START TIME  /  %s     FORCES  /  %d ELEMENTS"),
            *Scenario->StartTime, Scenario->Element.Num()) : TEXT("")));
    const FString Situation = Scenario
        ? (!Scenario->Sitrep.IsEmpty() ? Scenario->Sitrep : Scenario->Desc) : FString();
    if (MissionSummary) MissionSummary->SetText(FText::FromString(Scenario
        ? Situation.Replace(TEXT("\\n"), TEXT("\n")) : TEXT("Choose a mission from the roster.")));
    if (MissionObjectives) MissionObjectives->SetText(FText::FromString(Scenario && !Scenario->Objective.IsEmpty()
        ? Scenario->Objective.Replace(TEXT("\\n"), TEXT("\n"))
        : TEXT("Full objectives and loadout are available in Multiplayer Briefing.")));
    if (LaunchButton) LaunchButton->SetIsEnabled(Scenario != nullptr);
}

void UMultiplayerDlg::LaunchSelected()
{
    if (!ScenarioRows.IsValidIndex(ChosenIndex) || MissionId <= 0) return;

    // The existing mission briefing consumes a legacy Mission object. Use the
    // preserved PREP-mode handoff only when the built-in campaign is hydrated.
    // Do not claim successful loading solely because a DataTable row exists.
    Campaign* SingleCampaign = nullptr;
    ListIter<Campaign> Iter = Campaign::GetAllCampaigns();
    while (++Iter)
    {
        Campaign* Candidate = Iter.value();
        if (Candidate && Candidate->GetCampaignId() == Campaign::MULTIPLAYER_MISSIONS)
        {
            SingleCampaign = Campaign::SelectCampaign(Candidate->GetName());
            break;
        }
    }
    if (!SingleCampaign)
    {
        UE_LOG(LogTemp, Error, TEXT("[MultiplayerMissions] Cannot start %d: Multiplayer Missions campaign not initialized"), MissionId);
        return;
    }
    CampaignPtr = SingleCampaign;
    if (CampaignPtr->GetMissionList().isEmpty()) CampaignPtr->Load();
    if (!CampaignPtr->GetMissionInfo(MissionId))
    {
        UE_LOG(LogTemp, Error, TEXT("[MultiplayerMissions] DT multiplayer mission %d has no legacy MissionInfo; multiplayer briefing bridge not ready"), MissionId);
        return;
    }
    OnAcceptClicked();
}
void UMultiplayerDlg::HandleAccept() { LaunchSelected(); }
void UMultiplayerDlg::HandleCancel() { OnBackPressed(); }
void UMultiplayerDlg::OnLaunchPressed() { LaunchSelected(); }
void UMultiplayerDlg::OnBackPressed()
{
    // The inherited OnCancelClicked only hides this widget and broadcasts a
    // delegate. The main menu route must be invoked explicitly here.
    // This dialog is created directly by MenuDlg; remove it from the viewport
    // so it cannot remain above the main menu after Back.
    RemoveFromParent();
    if (manager) manager->ShowMenuDlg();
    else OnCancelClicked();
}
void UMultiplayerDlg::OnMission01Pressed() { SelectMissionAt(0); }
void UMultiplayerDlg::OnMission02Pressed() { SelectMissionAt(1); }
void UMultiplayerDlg::OnMission03Pressed() { SelectMissionAt(2); }
void UMultiplayerDlg::OnMission04Pressed() { SelectMissionAt(3); }
void UMultiplayerDlg::OnMission05Pressed() { SelectMissionAt(4); }
void UMultiplayerDlg::OnMission06Pressed() { SelectMissionAt(5); }
void UMultiplayerDlg::OnMission07Pressed() { SelectMissionAt(6); }
void UMultiplayerDlg::OnMission08Pressed() { SelectMissionAt(7); }
void UMultiplayerDlg::OnMission09Pressed() { SelectMissionAt(8); }
void UMultiplayerDlg::OnMission10Pressed() { SelectMissionAt(9); }
