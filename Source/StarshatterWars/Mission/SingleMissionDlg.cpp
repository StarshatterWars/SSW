/* Starshatter Wars - modern single-mission roster.
   Uses legacy Single Missions campaign and original PREP-mode handoff.
   The existing MissionSelectDlg is intentionally not modified. */
#include "SingleMissionDlg.h"
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
#include "Campaign.h"
#include "MissionInfo.h"
#include "ConfirmDlg.h"
#include "Mission.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "FormatUtil.h"
#include "MenuScreen.h"
#include "StarshatterUIStyleSubsystem.h"
#include "MissionUIStyle.h"

namespace SingleMissionColors
{
    const FLinearColor Backdrop(0.012f, 0.025f, 0.045f, 1.0f);
    const FLinearColor Panel(0.025f, 0.055f, 0.083f, 0.97f);
    const FLinearColor Row(0.68f, 0.80f, 0.85f, 1.0f);
    const FLinearColor Selected(0.23f, 0.73f, 0.88f, 1.0f);
    const FLinearColor Accent(0.16f, 0.77f, 0.91f, 1.0f);
    const FLinearColor Primary(0.92f, 0.97f, 1.0f, 1.0f);
    const FLinearColor Muted(0.61f, 0.72f, 0.81f, 1.0f);
}

USingleMissionDlg::USingleMissionDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void USingleMissionDlg::NativeConstruct()
{
    Super::NativeConstruct();
    BuildSingleMissionScreen();
}

UTextBlock* USingleMissionDlg::MakeLabel(const FString& Value, int32 Size,
    const FLinearColor& Tint, bool bWrap)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Value));
    // Match Campaign Selection's Serpentine header styling for titles.
    Label->SetFont(Size >= 22
        ? MissionUIStyle::GetHeaderFont(Size)
        : FCoreStyle::GetDefaultFontStyle("Regular", Size));
    Label->SetColorAndOpacity(FSlateColor(Tint));
    Label->SetAutoWrapText(bWrap);
    return Label;
}

UBorder* USingleMissionDlg::MakePanel(const FLinearColor& Tint)
{
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(Tint);
    Panel->SetPadding(FMargin(18.f));
    return Panel;
}

UButton* USingleMissionDlg::MakeActionButton(const FString& Label, bool bEnabled, bool bRosterButton)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(SingleMissionColors::Row);
    Style.Hovered.TintColor = FSlateColor(SingleMissionColors::Selected);
    Style.Pressed.TintColor = FSlateColor(SingleMissionColors::Accent);
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

void USingleMissionDlg::BuildSingleMissionScreen()
{
    if (!WidgetTree) return;
    if (WidgetTree->FindWidget(TEXT("SingleMission_Root"))) return;

    // Self-contained layout; WBP may remain the host class with no designer edits.
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SingleMission_Root"));
    WidgetTree->RootWidget = Root;
    UBorder* Background = MakePanel(SingleMissionColors::Backdrop);
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
    // Dark-grey title band, shared visually with the Campaign Selection header.
    UBorder* SingleTitleBand = MakePanel(FLinearColor(0.105f, 0.113f, 0.125f, 1.0f));
    SingleTitleBand->SetPadding(FMargin(18.f, 11.f, 18.f, 11.f));
    SingleTitleBand->SetContent(MakeLabel(TEXT("SINGLE MISSIONS"), 38, SingleMissionColors::Primary));
    Heading->AddChildToVerticalBox(SingleTitleBand)->SetPadding(FMargin(0.f, 7.f, 0.f, 7.f));
    UBorder* HeaderRule = MakePanel(SingleMissionColors::Accent);
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

    UBorder* LeftPanel = MakePanel(SingleMissionColors::Panel);
    UHorizontalBoxSlot* Left = Columns->AddChildToHorizontalBox(LeftPanel);
    FSlateChildSize LeftColumnSize(ESlateSizeRule::Fill);
    LeftColumnSize.Value = 0.88f;
    Left->SetSize(LeftColumnSize);
    Left->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    UVerticalBox* LeftContent = WidgetTree->ConstructWidget<UVerticalBox>();
    LeftPanel->SetContent(LeftContent);
    LeftContent->AddChildToVerticalBox(MakeLabel(TEXT("MISSIONS"), 18, SingleMissionColors::Accent));
    MissionCountLabel = MakeLabel(TEXT(""), 13, SingleMissionColors::Muted);
    LeftContent->AddChildToVerticalBox(MissionCountLabel)->SetPadding(FMargin(0.f, 5.f, 0.f, 12.f));
    UScrollBox* RosterScroll = WidgetTree->ConstructWidget<UScrollBox>();
    LeftContent->AddChildToVerticalBox(RosterScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    MissionRoster = WidgetTree->ConstructWidget<UVerticalBox>();
    RosterScroll->AddChild(MissionRoster);

    UBorder* RightPanel = MakePanel(SingleMissionColors::Panel);
    UHorizontalBoxSlot* Right = Columns->AddChildToHorizontalBox(RightPanel);
    FSlateChildSize RightColumnSize(ESlateSizeRule::Fill);
    RightColumnSize.Value = 1.12f;
    Right->SetSize(RightColumnSize);
    // Constrain mission intelligence to the right column and scroll independently
    // of the roster and the action bar. Long SITREPs/objectives remain accessible.
    MissionDetailsScroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("SingleMission_DetailsScroll"));
    MissionDetailsScroll->SetOrientation(EOrientation::Orient_Vertical);
    MissionDetailsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    RightPanel->SetContent(MissionDetailsScroll);
    UVerticalBox* RightContent = WidgetTree->ConstructWidget<UVerticalBox>();
    MissionDetailsScroll->AddChild(RightContent);
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("BRIEFING"), 18, SingleMissionColors::Accent));
    MissionTitle = MakeLabel(TEXT("SELECT A MISSION"), 26, SingleMissionColors::Primary, true);
    UBorder* MissionTitleBand = MakePanel(FLinearColor(0.105f, 0.113f, 0.125f, 1.0f));
    MissionTitleBand->SetPadding(FMargin(14.f, 10.f, 14.f, 10.f));
    MissionTitleBand->SetContent(MissionTitle);
    RightContent->AddChildToVerticalBox(MissionTitleBand)->SetPadding(FMargin(0.f, 14.f, 0.f, 14.f));
    MissionRole = MakeLabel(TEXT(""), 15, SingleMissionColors::Accent);
    MissionLocation = MakeLabel(TEXT(""), 15, SingleMissionColors::Muted);
    MissionTime = MakeLabel(TEXT(""), 15, SingleMissionColors::Muted);
    RightContent->AddChildToVerticalBox(MissionRole)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    RightContent->AddChildToVerticalBox(MissionLocation)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    RightContent->AddChildToVerticalBox(MissionTime)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("SITUATION"), 17, SingleMissionColors::Accent));
    MissionSummary = MakeLabel(TEXT(""), 17, SingleMissionColors::Primary, true);
    RightContent->AddChildToVerticalBox(MissionSummary)->SetPadding(FMargin(0.f, 8.f, 0.f, 20.f));
    RightContent->AddChildToVerticalBox(MakeLabel(TEXT("OBJECTIVES"), 17, SingleMissionColors::Accent));
    MissionObjectives = MakeLabel(TEXT(""), 15, SingleMissionColors::Primary, true);
    RightContent->AddChildToVerticalBox(MissionObjectives)->SetPadding(FMargin(0.f, 8.f));

    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Full->AddChildToVerticalBox(Actions)->SetPadding(FMargin(5.f, 2.f, 5.f, 0.f));
    UButton* BackButton = MakeActionButton(TEXT("BACK"));
    Actions->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    BackButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnBackPressed);

    NewMissionButton = MakeActionButton(TEXT("NEW MISSION"), false);
    EditMissionButton = MakeActionButton(TEXT("EDIT MISSION"), false);
    DeleteMissionButton = MakeActionButton(TEXT("DELETE MISSION"), false);
    Actions->AddChildToHorizontalBox(NewMissionButton)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    Actions->AddChildToHorizontalBox(EditMissionButton)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    Actions->AddChildToHorizontalBox(DeleteMissionButton)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    NewMissionButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnCreateMission);
    EditMissionButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnEditMission);
    DeleteMissionButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnDeleteMission);

    LaunchButton = MakeActionButton(TEXT("BRIEFING"), false);
    Actions->AddChildToHorizontalBox(LaunchButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LaunchButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnLaunchPressed);
}

void USingleMissionDlg::Show()
{
    // Do not call UMissionSelectDlg::Show(): it switches among single/custom/multiplayer.
    UBaseScreen::Show();
    if (!MissionRoster) BuildSingleMissionScreen();
    RefreshMissionRoster();
}

void USingleMissionDlg::RefreshMissionRoster()
{
    if (!MissionRoster) return;
    MissionRoster->ClearChildren();
    MissionButtons.Reset();
    MissionIds.Reset();
    ScenarioRows.Reset();
    CustomMissionInfos.Reset();
    CustomCampaign = nullptr;
    ChosenIndex = INDEX_NONE;
    SelectedMissionIndex = INDEX_NONE;
    MissionId = 0;
    bEditable = false;

    UStarshatterGameDataSubsystem* GameData = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>() : nullptr;
    UDataTable* DataTable = GameData ? GameData->GetSingleMissionsDataTable() : nullptr;
    if (!DataTable || DataTable->GetRowStruct() != FS_CampaignMission::StaticStruct())
    {
        MissionCountLabel->SetText(FText::FromString(TEXT("DT_SingleMissions unavailable or incorrect row structure")));
        RefreshMissionDetails();
        return;
    }

    TArray<FName> RowNames = DataTable->GetRowNames();
    // Sort by authored mission ID, not by unpredictable DataTable iteration order.
    TArray<const FS_CampaignMission*> SortedRows;
    for (const FName& RowName : RowNames)
    {
        const FS_CampaignMission* Scenario = DataTable->FindRow<FS_CampaignMission>(RowName, TEXT("SingleMissionRoster"), false);
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
        case 0: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission01Pressed); break;
        case 1: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission02Pressed); break;
        case 2: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission03Pressed); break;
        case 3: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission04Pressed); break;
        case 4: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission05Pressed); break;
        case 5: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission06Pressed); break;
        case 6: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission07Pressed); break;
        case 7: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission08Pressed); break;
        case 8: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission09Pressed); break;
        case 9: RowButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnMission10Pressed); break;
        default: break;
        }
        MissionRoster->AddChildToVerticalBox(RowButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));
        MissionIds.Add(Scenario->Id);
        ScenarioRows.Add(Scenario);
        CustomMissionInfos.Add(nullptr);
        MissionButtons.Add(RowButton);
    }
    // Custom missions reside in the writable legacy mission campaign, not the
    // cooked DT_SingleMissions asset. Never allow editing of DataTable entries.
    ListIter<Campaign> CustomCampaignIter = Campaign::GetAllCampaigns();
    while (++CustomCampaignIter)
    {
        Campaign* CandidateCampaign = CustomCampaignIter.value();
        if (CandidateCampaign && CandidateCampaign->GetCampaignId() == Campaign::CUSTOM_MISSIONS)
        {
            CustomCampaign = CandidateCampaign;
            break;
        }
    }
    if (CustomCampaign)
    {
        ListIter<MissionInfo> CustomIter = CustomCampaign->GetMissionList();
        while (++CustomIter)
        {
            MissionInfo* CustomInfo = CustomIter.value();
            if (!CustomInfo || CustomInfo->id <= 0) continue;
            UButton* CustomButton = MakeActionButton(
                FString::Printf(TEXT("CUSTOM: %s"), ANSI_TO_TCHAR(CustomInfo->name.data())), true, true);
            CustomButton->OnClicked.AddDynamic(this, &USingleMissionDlg::OnCustomMissionChosen);
            MissionRoster->AddChildToVerticalBox(CustomButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));
            MissionButtons.Add(CustomButton);
            MissionIds.Add(CustomInfo->id);
            ScenarioRows.Add(nullptr);
            CustomMissionInfos.Add(CustomInfo);
        }
    }
    if (NewMissionButton) NewMissionButton->SetIsEnabled(CustomCampaign != nullptr && manager != nullptr);
    MissionCountLabel->SetText(FText::FromString(FString::Printf(
        TEXT("%d missions"), MissionIds.Num())));
    if (!MissionIds.IsEmpty()) SelectMissionAt(0);
    else
    {
        if (EditMissionButton) EditMissionButton->SetIsEnabled(false);
        if (DeleteMissionButton) DeleteMissionButton->SetIsEnabled(false);
        RefreshMissionDetails();
    }
}

void USingleMissionDlg::SelectMissionAt(int32 Index)
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
            Style.Normal.TintColor = FSlateColor(i == Index ? SingleMissionColors::Selected : SingleMissionColors::Row);
            Button->SetStyle(Style);
        }
    }
    bEditable = CustomMissionInfos.IsValidIndex(Index) && CustomMissionInfos[Index] != nullptr;
    if (EditMissionButton) EditMissionButton->SetIsEnabled(bEditable && manager != nullptr);
    if (DeleteMissionButton) DeleteMissionButton->SetIsEnabled(bEditable && manager != nullptr);
    RefreshMissionDetails();
    // Each selected operation opens at the top of its intelligence report.
    if (MissionDetailsScroll) MissionDetailsScroll->ScrollToStart();
}

void USingleMissionDlg::RefreshMissionDetails()
{
    const FS_CampaignMission* Scenario = ScenarioRows.IsValidIndex(ChosenIndex)
        ? ScenarioRows[ChosenIndex] : nullptr;
    MissionInfo* CustomInfo = CustomMissionInfos.IsValidIndex(ChosenIndex) ? CustomMissionInfos[ChosenIndex] : nullptr;
    if (CustomInfo)
    {
        if (MissionTitle) MissionTitle->SetText(FText::FromString(ANSI_TO_TCHAR(CustomInfo->name.data())));
        if (MissionRole) MissionRole->SetText(FText::FromString(TEXT("CUSTOM MISSION")));
        if (MissionLocation) MissionLocation->SetText(FText::FromString(
            FString::Printf(TEXT("%s / %s"), ANSI_TO_TCHAR(CustomInfo->system.data()), ANSI_TO_TCHAR(CustomInfo->region.data()))));
        if (MissionTime) MissionTime->SetText(FText::GetEmpty());
        if (MissionSummary) MissionSummary->SetText(FText::FromString(ANSI_TO_TCHAR(CustomInfo->description.data())));
        if (MissionObjectives) MissionObjectives->SetText(FText::GetEmpty());
        if (LaunchButton) LaunchButton->SetIsEnabled(true);
        return;
    }
    if (MissionTitle) MissionTitle->SetText(FText::FromString(Scenario
        ? Scenario->MissionName : TEXT("NO MISSION SELECTED")));
    if (MissionRole) MissionRole->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("TYPE: %s"),
            *FString(Mission::GetRoleName(static_cast<int32>(Scenario->MissionType)))) : TEXT("")));
    if (MissionLocation) MissionLocation->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("%s / %s"),
            *Scenario->MissionSystem, *Scenario->MissionRegion) : TEXT("")));
    if (MissionTime) MissionTime->SetText(FText::FromString(Scenario
        ? FString::Printf(TEXT("%s  /  %d elements"),
            *Scenario->StartTime, Scenario->Element.Num()) : TEXT("")));
    const FString Situation = Scenario
        ? (!Scenario->Sitrep.IsEmpty() ? Scenario->Sitrep : Scenario->Desc) : FString();
    if (MissionSummary) MissionSummary->SetText(FText::FromString(Scenario
        ? Situation.Replace(TEXT("\\n"), TEXT("\n")) : TEXT("")));
    if (MissionObjectives) MissionObjectives->SetText(FText::FromString(Scenario && !Scenario->Objective.IsEmpty()
        ? Scenario->Objective.Replace(TEXT("\\n"), TEXT("\n"))
        : TEXT("Full objectives and loadout are available in Mission Briefing.")));
    if (LaunchButton) LaunchButton->SetIsEnabled(Scenario != nullptr);
}

void USingleMissionDlg::LaunchSelected()
{
    if (!ScenarioRows.IsValidIndex(ChosenIndex) || MissionId <= 0) return;
    if (CustomMissionInfos.IsValidIndex(ChosenIndex) && CustomMissionInfos[ChosenIndex])
    {
        if (CustomCampaign)
        {
            // CustomCampaign was resolved from Campaign::GetAllCampaigns() during roster refresh.
    // Do not perform a redundant name-based lookup: it can dereference invalid
    // legacy ANSI name storage inside Campaign::SelectCampaign().
    CampaignPtr = CustomCampaign;
            CampaignPtr = CustomCampaign;
            OnAcceptClicked();
        }
        return;
    }

    // The existing mission briefing consumes a legacy Mission object. Use the
    // preserved PREP-mode handoff only when the built-in campaign is hydrated.
    // Do not claim successful loading solely because a DataTable row exists.
    Campaign* SingleCampaign = nullptr;
    ListIter<Campaign> Iter = Campaign::GetAllCampaigns();
    while (++Iter)
    {
        Campaign* Candidate = Iter.value();
        if (Candidate && Candidate->GetCampaignId() == Campaign::SINGLE_MISSIONS)
        {
            SingleCampaign = Campaign::SelectCampaign(Candidate->GetName());
            break;
        }
    }
    if (!SingleCampaign)
    {
        UE_LOG(LogTemp, Error, TEXT("[SingleMissions] Cannot start %d: Single Missions campaign not initialized"), MissionId);
        return;
    }
    CampaignPtr = SingleCampaign;
    if (CampaignPtr->GetMissionList().isEmpty()) CampaignPtr->Load();
    if (!CampaignPtr->GetMissionInfo(MissionId))
    {
        UE_LOG(LogTemp, Error, TEXT("[SingleMissions] DT mission %d has no legacy MissionInfo; briefing bridge not ready"), MissionId);
        return;
    }
    OnAcceptClicked();
}
void USingleMissionDlg::HandleAccept() { LaunchSelected(); }
void USingleMissionDlg::HandleCancel() { OnBackPressed(); }
void USingleMissionDlg::OnLaunchPressed() { LaunchSelected(); }
void USingleMissionDlg::OnBackPressed()
{
    // The inherited OnCancelClicked only hides this widget and broadcasts a
    // delegate. The main menu route must be invoked explicitly here.
    if (manager)
    {
        manager->ShowMenuDlg();
    }
    else
    {
        // Fallback for an uninitialized menu manager.
        OnCancelClicked();
    }
}
void USingleMissionDlg::OnMission01Pressed() { SelectMissionAt(0); }
void USingleMissionDlg::OnMission02Pressed() { SelectMissionAt(1); }
void USingleMissionDlg::OnMission03Pressed() { SelectMissionAt(2); }
void USingleMissionDlg::OnMission04Pressed() { SelectMissionAt(3); }
void USingleMissionDlg::OnMission05Pressed() { SelectMissionAt(4); }
void USingleMissionDlg::OnMission06Pressed() { SelectMissionAt(5); }
void USingleMissionDlg::OnMission07Pressed() { SelectMissionAt(6); }
void USingleMissionDlg::OnMission08Pressed() { SelectMissionAt(7); }
void USingleMissionDlg::OnMission09Pressed() { SelectMissionAt(8); }
void USingleMissionDlg::OnMission10Pressed() { SelectMissionAt(9); }

void USingleMissionDlg::OnCustomMissionChosen()
{
    // Unlike built-in missions, the number of custom entries is unrestricted.
    for (int32 Index = 0; Index < MissionButtons.Num(); ++Index)
    {
        UButton* EntryButton = MissionButtons[Index];
        if (EntryButton && CustomMissionInfos.IsValidIndex(Index) && CustomMissionInfos[Index]
            && (EntryButton->IsHovered() || EntryButton->HasKeyboardFocus()))
        {
            SelectMissionAt(Index);
            return;
        }
    }
}

void USingleMissionDlg::OnCreateMission()
{
    if (!manager || !CustomCampaign) return;
    // CustomCampaign was resolved from Campaign::GetAllCampaigns() during roster refresh.
    // Do not perform a redundant name-based lookup: it can dereference invalid
    // legacy ANSI name storage inside Campaign::SelectCampaign().
    CampaignPtr = CustomCampaign;
    MissionInfo* Info = CustomCampaign->CreateNewMission();
    if (!Info || !Info->mission) return;
    manager->EditMission(Info->mission, Info);
}

void USingleMissionDlg::OnEditMission()
{
    if (!manager || !CustomCampaign || !CustomMissionInfos.IsValidIndex(ChosenIndex)) return;
    MissionInfo* Info = CustomMissionInfos[ChosenIndex];
    if (!Info) return; // Built-in missions are read-only.
    // CustomCampaign was resolved from Campaign::GetAllCampaigns() during roster refresh.
    // Do not perform a redundant name-based lookup: it can dereference invalid
    // legacy ANSI name storage inside Campaign::SelectCampaign().
    CampaignPtr = CustomCampaign;
    Mission* Model = CustomCampaign->GetMission(Info->id);
    if (!Model) return;
    manager->EditMission(Model, Info);
}

void USingleMissionDlg::OnDeleteMission()
{
    if (!manager || !CustomCampaign || !CustomMissionInfos.IsValidIndex(ChosenIndex)) return;
    MissionInfo* Info = CustomMissionInfos[ChosenIndex];
    if (!Info) return;
    UConfirmDlg* Dialog = manager->GetConfirmDlg();
    if (!Dialog) return;
    PendingCustomDeleteId = Info->id;
    Dialog->OnConfirmed.RemoveDynamic(this, &USingleMissionDlg::OnCustomDeleteConfirmed);
    Dialog->OnCanceled.RemoveDynamic(this, &USingleMissionDlg::OnCustomDeleteCanceled);
    Dialog->OnConfirmed.AddDynamic(this, &USingleMissionDlg::OnCustomDeleteConfirmed);
    Dialog->OnCanceled.AddDynamic(this, &USingleMissionDlg::OnCustomDeleteCanceled);
    Dialog->SetTitle(TEXT("Delete mission"));
    Dialog->SetMessage(FString::Printf(TEXT("Delete '%s'?"), ANSI_TO_TCHAR(Info->name.data())));
    manager->ShowConfirmDlg();
}

void USingleMissionDlg::OnCustomDeleteConfirmed()
{
    const int32 IdToDelete = PendingCustomDeleteId;
    OnCustomDeleteCanceled();
    if (!CustomCampaign || IdToDelete <= 0 || !CustomCampaign->GetMissionInfo(IdToDelete)) return;
    CustomCampaign->DeleteMission(IdToDelete);
    RefreshMissionRoster();
    if (manager) manager->ShowMissionSelectDlg(); // Existing manager route; pending routing integration.
}

void USingleMissionDlg::OnCustomDeleteCanceled()
{
    PendingCustomDeleteId = INDEX_NONE;
    if (manager)
    {
        if (UConfirmDlg* Dialog = manager->GetConfirmDlg())
        {
            Dialog->OnConfirmed.RemoveDynamic(this, &USingleMissionDlg::OnCustomDeleteConfirmed);
            Dialog->OnCanceled.RemoveDynamic(this, &USingleMissionDlg::OnCustomDeleteCanceled);
        }
    }
}
