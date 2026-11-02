/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionDebriefDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Debriefing dialog (Unreal) - UBaseScreen/UMG port
*/

#include "MissionDebriefDlg.h"

#include "MissionPlanner.h"   // your renamed PlanScreen (do not rename)
#include "Starshatter.h"
#include "Campaign.h"
#include "Mission.h"
#include "Sim.h"
#include "Ship.h"
#include "StarSystem.h"
#include "SimElement.h"
#include "Instruction.h"
#include "FormatUtil.h"
#include "PlayerCharacter.h"
#include "SimEvent.h"
#include "ShipDesign.h"
#include "GameStructs.h"
#include "SSWRuntimeSubsystem.h"
#include "MissionUIStyle.h"
#include "MenuScreen.h"
#include "CampaignSave.h"
#include "SSWGameInstance.h"
#include "StarshatterPlayerSubsystem.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"


// Legacy helpers:
#include "Game.h"
#include "Mouse.h"

// Unreal:
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/ButtonSlot.h"

namespace
{
    FSlateChildSize DebriefFill(float Weight)
    {
        FSlateChildSize Size(ESlateSizeRule::Fill); Size.Value=Weight; return Size;
    }
}

UDebriefListItem* UDebriefListItem::Make(UObject* Outer,
    const FString& In0, const FString& In1, const FString& In2, const FString& In3, int32 InDataIndex)
{
    UDebriefListItem* Item = NewObject<UDebriefListItem>(Outer);
    Item->Col0 = In0;
    Item->Col1 = In1;
    Item->Col2 = In2;
    Item->Col3 = In3;
    Item->DataIndex = InDataIndex;
    return Item;
}

TSharedRef<SWidget> UDebriefRowWidget::RebuildWidget()
{
    EnsureRow();
    return Super::RebuildWidget();
}

void UDebriefRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureRow();
}

void UDebriefRowWidget::EnsureRow()
{
    if (RowText || !WidgetTree) return;
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
    for (int32 I=0; I<4; ++I)
    {
        auto* Column=WidgetTree->ConstructWidget<UTextBlock>();
        Column->SetFont(MissionUIStyle::GetLimerickFont(14));
        Column->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Column->SetAutoWrapText(true);
        auto* LayoutSlot=Row->AddChildToHorizontalBox(Column);
        LayoutSlot->SetSize(DebriefFill(1.f)); LayoutSlot->SetPadding(FMargin(6,4));
        RowColumns.Add(Column);
    }
    RowText=RowColumns[0];
    WidgetTree->RootWidget = Row;
}

void UDebriefRowWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    EnsureRow();
    const UDebriefListItem* Item = Cast<UDebriefListItem>(ListItemObject);
    if (!RowText) return;
    if (!Item)
    {
        RowText->SetText(FText::GetEmpty());
        return;
    }
    const FString Values[]={Item->Col0,Item->Col1,Item->Col2,Item->Col3};
    const int32 Count=Item->Col3.IsEmpty() ? (Item->Col2.IsEmpty() ? 2 : 3) : 4;
    const float Weights[3][4]={{0.65f,0.35f,0,0},{0.16f,0.46f,0.38f,0},{0.1f,0.35f,0.3f,0.25f}};
    for (int32 I=0; I<RowColumns.Num(); ++I)
    {
        RowColumns[I]->SetText(FText::FromString(Values[I]));
        RowColumns[I]->SetVisibility(I<Count ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (auto* LayoutSlot=Cast<UHorizontalBoxSlot>(RowColumns[I]->Slot)) LayoutSlot->SetSize(DebriefFill(Weights[Count-2][I]));
    }
}

void UMissionDebriefDlg::EnsureUi()
{
    if (bModernUiBuilt) return;
    UVerticalBox* Box = EnsureAutoVerticalBoxWithOffsets(FMargin(24.f));
    if (!Box || !WidgetTree) return;
    // Cover the legacy Blueprint layout without replacing an already-built Slate root.
    if (RootCanvas)
        for (UWidget* Child : RootCanvas->GetAllChildren())
            if (Child != Box) Child->SetVisibility(ESlateVisibility::Collapsed);
    Box->ClearChildren();
    auto Label = [this](const TCHAR* Text, int32 Size, bool Header=false)
    {
        auto* W = WidgetTree->ConstructWidget<UTextBlock>();
        W->SetText(FText::FromString(Text));
        W->SetFont(Header ? MissionUIStyle::GetHeaderFont(Size) : MissionUIStyle::GetLimerickFont(Size));
        W->SetColorAndOpacity(FSlateColor(MissionUIStyle::InfoValueText));
        return W;
    };
    auto Panel = [this](UWidget* Content)
    {
        auto* B = WidgetTree->ConstructWidget<UBorder>();
        B->SetBrushColor(MissionUIStyle::PanelBG);
        B->SetPadding(FMargin(16.f,12.f)); B->SetContent(Content); return B;
    };
    auto* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* Caption = Label(TEXT("MISSION DEBRIEFING"),28,true);
    Header->AddChildToHorizontalBox(Caption)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CloseButtonWidget = WidgetTree->ConstructWidget<UButton>();
    Header->AddChildToHorizontalBox(CloseButtonWidget)->SetVerticalAlignment(VAlign_Center);
    Box->AddChildToVerticalBox(Panel(Header))->SetPadding(FMargin(0,0,0,12));
    auto* Info = WidgetTree->ConstructWidget<UVerticalBox>();
    MissionNameWidget = Label(TEXT(""),24,true); Info->AddChildToVerticalBox(MissionNameWidget);
    auto* Meta = WidgetTree->ConstructWidget<UHorizontalBox>();
    for (auto Pair : {TPair<const TCHAR*,TObjectPtr<UTextBlock>*>(TEXT("SYSTEM: "),&MissionSystemWidget),
        {TEXT("SECTOR: "),&MissionSectorWidget},{TEXT("START: "),&MissionTimeStartWidget}})
    {
        Meta->AddChildToHorizontalBox(Label(Pair.Key,14))->SetPadding(FMargin(0,6,6,6));
        *Pair.Value = Label(TEXT(""),14);
        Meta->AddChildToHorizontalBox(*Pair.Value)->SetPadding(FMargin(0,6,30,6));
    }
    Info->AddChildToVerticalBox(Meta);
    auto* ResultRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    OutcomeWidget = Label(TEXT(""),22,true);
    ResultRow->AddChildToHorizontalBox(OutcomeWidget)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    MissionScoreWidget = Label(TEXT(""),20);
    ResultRow->AddChildToHorizontalBox(MissionScoreWidget);
    Info->AddChildToVerticalBox(ResultRow);
    EvidenceWidget = Label(TEXT(""),14); EvidenceWidget->SetAutoWrapText(true);
    Info->AddChildToVerticalBox(EvidenceWidget)->SetPadding(FMargin(0,4,0,6));
    ObjectivesWidget = WidgetTree->ConstructWidget<URichTextBlock>();
    FTextBlockStyle TextStyle; TextStyle.SetFont(MissionUIStyle::GetLimerickFont(14));
    TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ObjectivesWidget->SetDefaultTextStyle(TextStyle); ObjectivesWidget->SetAutoWrapText(true);
    Info->AddChildToVerticalBox(Label(TEXT("OBJECTIVES"),16,true));
    Info->AddChildToVerticalBox(ObjectivesWidget);
    SituationWidget = nullptr; // Mission situation is already covered in briefing.
    Box->AddChildToVerticalBox(Panel(Info))->SetPadding(FMargin(0,0,0,12));
    auto* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* Left = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* Right = WidgetTree->ConstructWidget<UVerticalBox>();
    auto AddList = [this,Label](UVerticalBox* Parent,const TCHAR* Caption,const TCHAR* Columns,float Weight)
    {
        Parent->AddChildToVerticalBox(Label(Caption,18,true))->SetPadding(FMargin(0,0,0,6));
        const FString Heading(Columns);
        TArray<FString> Captions;
        if (Heading.StartsWith(TEXT("PKG"))) Captions={TEXT("PKG"),TEXT("CALLSIGN"),TEXT("ROLE"),TEXT("TYPE")};
        else if (Heading.StartsWith(TEXT("STATISTIC"))) Captions={TEXT("STATISTICS"),TEXT("VALUE")};
        else Captions={TEXT("TIME"),TEXT("EVENT"),TEXT("TARGET / DETAILS")};
        const float Weights[3][4]={{0.65f,0.35f,0,0},{0.16f,0.46f,0.38f,0},{0.1f,0.35f,0.3f,0.25f}};
        auto* Headings=WidgetTree->ConstructWidget<UHorizontalBox>();
        for (int32 I=0; I<Captions.Num(); ++I)
        {
            auto* HeadingSlot=Headings->AddChildToHorizontalBox(Label(*Captions[I],12));
            HeadingSlot->SetSize(DebriefFill(Weights[Captions.Num()-2][I]));
            HeadingSlot->SetPadding(FMargin(6,0));
        }
        Parent->AddChildToVerticalBox(Headings)->SetPadding(FMargin(0,0,0,6));
        auto* List = WidgetTree->ConstructWidget<UDebriefListView>();
        UClass* RowClass=LoadClass<UDebriefRowWidget>(nullptr,TEXT("/Game/Screens/Mission/Rows/WBP_DebriefRow.WBP_DebriefRow_C"));
        List->SetEntry(RowClass ? RowClass : UDebriefRowWidget::StaticClass());
        auto* LayoutSlot = Parent->AddChildToVerticalBox(List);
        LayoutSlot->SetSize(DebriefFill(Weight)); LayoutSlot->SetPadding(FMargin(0,0,0,12));
        return List;
    };
    UnitListWidget=AddList(Left,TEXT("ACTIVE UNITS"),TEXT("PKG     CALLSIGN                  ROLE                  TYPE"),0.4f);
    SummaryListWidget=AddList(Left,TEXT("MISSION SUMMARY"),TEXT("STATISTICS                                     VALUE"),0.6f);
    EventListWidget=AddList(Right,TEXT("MISSION LOG"),TEXT("TIME               EVENT                                      TARGET / DETAILS"),1.f);
    auto* LSlot=Body->AddChildToHorizontalBox(Panel(Left)); LSlot->SetSize(DebriefFill(0.48f)); LSlot->SetPadding(FMargin(0,0,12,0));
    Body->AddChildToHorizontalBox(Panel(Right))->SetSize(DebriefFill(0.52f));
    Box->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    bModernUiBuilt=true;
    BindFormWidgets();
    CloseButtonWidget->OnClicked.AddUniqueDynamic(this,&UMissionDebriefDlg::OnCloseClicked);
    UnitListWidget->OnItemSelectionChanged().RemoveAll(this);
    UnitListWidget->OnItemSelectionChanged().AddUObject(this,&UMissionDebriefDlg::OnUnitSelectionChanged);
}

UMissionDebriefDlg::UMissionDebriefDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        MissionPtr = CampaignPtr->GetMission();
    }
}

void UMissionDebriefDlg::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureUi();
    BindFormWidgets();

    if (CloseButtonWidget)
        CloseButtonWidget->OnClicked.AddUniqueDynamic(this, &UMissionDebriefDlg::OnCloseClicked);
    if (UnitListWidget)
        UnitListWidget->OnItemSelectionChanged().AddUObject(this, &UMissionDebriefDlg::OnUnitSelectionChanged);
}

void UMissionDebriefDlg::NativeDestruct()
{
    Super::NativeDestruct();
}

void UMissionDebriefDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsShown)
    {
        ExecFrame(InDeltaTime);
    }
}

// --------------------------------------------------------------------
// FORM binding (IDs -> widgets)
// --------------------------------------------------------------------

void UMissionDebriefDlg::BindFormWidgets()
{
    // Clear the map (optional safety):
    // FormMap.Reset(); // FormMap is protected in UBaseScreen; if you keep it private, remove this.

    // IDs come from legacy DebriefDlg.frm mapping:
    if (MissionNameWidget)      BindLabel(200, MissionNameWidget);
    if (MissionSystemWidget)    BindLabel(202, MissionSystemWidget);
    if (MissionSectorWidget)    BindLabel(204, MissionSectorWidget);
    if (MissionTimeStartWidget) BindLabel(206, MissionTimeStartWidget);

    if (MissionScoreWidget)     BindLabel(211, MissionScoreWidget);

    if (ObjectivesWidget)       BindText(210, ObjectivesWidget);
    if (SituationWidget)        BindText(240, SituationWidget);

    if (CloseButtonWidget)
    {
        BindButton(1, CloseButtonWidget);
        MissionUIStyle::ApplyActionButtonStyle(CloseButtonWidget,TEXT("Green"));
        CloseButtonWidget->SetToolTipText(NSLOCTEXT("MissionDebrief","Accept","Accept debrief and return"));
        if (WidgetTree && (!CloseButtonWidget->GetContent()
            || CloseButtonWidget->GetContent()->GetFName() != TEXT("DebriefAcceptIconBox")))
        {
            USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("DebriefAcceptIconBox"));
            IconBox->SetWidthOverride(40.f);
            IconBox->SetHeightOverride(40.f);
            UImage* Icon = WidgetTree->ConstructWidget<UImage>();
            MissionUIStyle::SetActionIcon(Icon,
                TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Check_256x256.T_Check_256x256"),FLinearColor::White);
            IconBox->SetContent(Icon);
            CloseButtonWidget->SetContent(IconBox);
            if (UButtonSlot* IconSlot = Cast<UButtonSlot>(IconBox->Slot))
            {
                IconSlot->SetHorizontalAlignment(HAlign_Center);
                IconSlot->SetVerticalAlignment(VAlign_Center);
                IconSlot->SetPadding(FMargin(8.f));
            }
        }
        if (UVerticalBoxSlot* ButtonSlot = Cast<UVerticalBoxSlot>(CloseButtonWidget->Slot))
        {
            ButtonSlot->SetHorizontalAlignment(HAlign_Right);
            ButtonSlot->SetPadding(FMargin(0.f,8.f,0.f,0.f));
        }
    }

    if (UnitListWidget)         BindList(320, UnitListWidget);
    if (SummaryListWidget)      BindList(330, SummaryListWidget);
    if (EventListWidget)        BindList(340, EventListWidget);
}

// --------------------------------------------------------------------
// Dialog input hooks (Enter/Escape)
// --------------------------------------------------------------------

void UMissionDebriefDlg::HandleAccept()
{
    // Legacy: Enter closes
    OnCloseClicked();
}

void UMissionDebriefDlg::HandleCancel()
{
    // Escape closes as well for debrief:
    OnCloseClicked();
}

// --------------------------------------------------------------------
// Show / Hide (UMG)
// --------------------------------------------------------------------

void UMissionDebriefDlg::Show()
{
    const bool bNeedRefresh = (GetVisibility() != ESlateVisibility::Visible);

    SetVisibility(ESlateVisibility::Visible);
    bIsShown = true;
    EnsureUi();
    if (Sim* Current=Sim::GetSim(); Current && RecordedMission==Current->GetMission()
        && RecordedStartTime==Current->StartTime()) return; // Retain the snapshot after stats are committed.

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USSWRuntimeSubsystem* RuntimeSS = GI->GetSubsystem<USSWRuntimeSubsystem>())
        {
            RuntimeSS->SetTimeCompression(1);
        }
    }

    MissionPtr = nullptr;
    CampaignPtr = Campaign::GetCampaign();
    SimPtr = Sim::GetSim();

    PlayerShip = nullptr;
    if (SimPtr)
    {
        PlayerShip = SimPtr->GetPlayerShip();
    }

    MissionPtr = SimPtr ? SimPtr->GetMission() : nullptr;
    if (!MissionPtr && CampaignPtr) MissionPtr = CampaignPtr->GetMission();

    // Mission title:
    if (UTextBlock* MissionName = GetLabel(200))
    {
        if (MissionPtr)
            MissionName->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetName())));
        else
            MissionName->SetText(FText::FromString(ANSI_TO_TCHAR("DebriefDlg: Mission Name")));
    }

    // System:
    if (UTextBlock* MissionSystem = GetLabel(202))
    {
        MissionSystem->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            StarSystem* Sys = MissionPtr->GetStarSystem();
            if (Sys)
                MissionSystem->SetText(FText::FromString(ANSI_TO_TCHAR(Sys->GetName())));
        }
    }

    // Sector/Region:
    if (UTextBlock* MissionSector = GetLabel(204))
    {
        MissionSector->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            MissionElement* Elem0 = MissionPtr->GetElements().size() ? MissionPtr->GetElements().at(0) : nullptr;
            if (Elem0)
                MissionSector->SetText(FText::FromString(ANSI_TO_TCHAR(Elem0->GetRegion())));
        }
    }

    // Start time:
    if (UTextBlock* MissionTimeStart = GetLabel(206))
    {
        MissionTimeStart->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            char Txt[32] = { 0 };
            FormatDayTime(Txt, MissionPtr->GetStart());
            MissionTimeStart->SetText(FText::FromString(ANSI_TO_TCHAR(Txt)));
        }
    }

    // Objectives:
    if (URichTextBlock* Objectives = GetText(210))
    {
        bool bFoundObjectives = false;

        if (SimPtr && SimPtr->GetPlayerElement())
        {
            Text T;
            SimElement* PlayerElem = SimPtr->GetPlayerElement();

            for (int i = 0; i < PlayerElem->NumObjectives(); i++)
            {
                Instruction* Obj = PlayerElem->GetObjective(i);
                T += Text("* ") + Obj->GetDescription() + Text("\n");
                bFoundObjectives = true;
            }

            Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(T.data())));
        }

        if (!bFoundObjectives)
        {
            if (MissionPtr)
                Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetObjective())));
            else
                Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(Game::GetText("DebriefDlg.unspecified").data())));
        }
    }

    // Situation:
    if (URichTextBlock* Situation = GetText(240))
    {
        if (MissionPtr)
            Situation->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetSituation())));
        else
            Situation->SetText(FText::FromString(ANSI_TO_TCHAR("DebriefDlg: Unknown")));
    }

    int32 EarnedPoints = 0;
    // Score:
    if (UTextBlock* MissionScore = GetLabel(211))
    {
        MissionScore->SetText(FText::FromString(ANSI_TO_TCHAR("DebriefDlg: No Stats")));

        if (PlayerShip)
        {
            for (int i = 0; i < ShipStats::NumStats(); i++)
            {
                ShipStats* Stats = ShipStats::GetStats(i);
                if (Stats && !strcmp(PlayerShip->GetName(), Stats->GetName()))
                {
                    Stats->Summarize();

                    PlayerCharacter* PlayerObj = PlayerCharacter::GetCurrentPlayer();
                    int Points = Stats->GetPoints() + Stats->GetCommandPoints();

                    if (PlayerObj && SimPtr)
                        Points = PlayerObj->GetMissionPoints(Stats, SimPtr->StartTime()) + Stats->GetCommandPoints();

                    EarnedPoints = Points;
                    char ScoreTxt[64] = { 0 };
                    sprintf_s(ScoreTxt, "%d points", Points);
                    MissionScore->SetText(FText::FromString(ANSI_TO_TCHAR(ScoreTxt)));
                    break;
                }
            }
        }
    }

    RefreshOutcomeAndRecord(EarnedPoints);

    // Lists:
    DrawUnits();

    if (bNeedRefresh)
    {
        // Optional: force selection behavior or first row selection.
        // (Handled in ExecFrame)
    }
}

void UMissionDebriefDlg::Hide()
{
    bIsShown = false;
    SetVisibility(ESlateVisibility::Collapsed);
}

// --------------------------------------------------------------------
// Frame
// --------------------------------------------------------------------

void UMissionDebriefDlg::ExecFrame(float /*DeltaSeconds*/)
{
    // Legacy: if list has items but none selected, force select first
    if (UListView* UnitList = GetList(320))
    {
        if (UnitList->GetNumItems() > 0)
        {
            TArray<UObject*> Selected;
            UnitList->GetSelectedItems(Selected);

            if (Selected.Num() < 1)
            {
                UnitList->SetSelectedIndex(0);
                if (UDebriefListItem* Row = Cast<UDebriefListItem>(UnitList->GetItemAt(0)))
                    DrawSelectedUnit(Row->DataIndex);
            }
        }
    }
}

// --------------------------------------------------------------------
// Units + selection
// --------------------------------------------------------------------

void UMissionDebriefDlg::DrawUnits()
{
    if (!MissionPtr)
        return;

    UListView* UnitList = GetList(320);
    if (!UnitList)
        return;

    UnitList->ClearListItems();
    UnitSnapshots.Reset();

    int32 SelectIndex = INDEX_NONE;

    for (int i = 0; i < ShipStats::NumStats(); i++)
    {
        ShipStats* Stats = ShipStats::GetStats(i);
        if (!Stats) continue;

        Stats->Summarize();

        if (Stats->GetIFF() == MissionPtr->GetTeam() &&
            !strcmp(Stats->GetRegion(), MissionPtr->GetRegion()))
        {
            // Col0 unused in original (space), keep for compatibility:
            UDebriefListItem* Row = UDebriefListItem::Make(
                this,
                TEXT(" "),
                ANSI_TO_TCHAR(Stats->GetName()),
                ANSI_TO_TCHAR(Stats->GetRole()),
                ANSI_TO_TCHAR(Stats->GetType()),
                /*DataIndex*/ i
            );

            UnitList->AddItem(Row);
            UnitSnapshots.Add(i,Row);
            DrawSelectedUnit(i); // Capture every unit while ShipStats still exists.

            if (PlayerShip && !strcmp(PlayerShip->GetName(), Stats->GetName()))
            {
                SelectIndex = UnitList->GetNumItems() - 1;
            }
        }
    }

    if (SelectIndex != INDEX_NONE)
    {
        UnitList->SetSelectedIndex(SelectIndex);
        if (UDebriefListItem* Row = Cast<UDebriefListItem>(UnitList->GetItemAt(SelectIndex)))
            DrawSelectedUnit(Row->DataIndex);
    }
    else if (UnitList->GetNumItems() > 0)
    {
        UnitList->SetSelectedIndex(0);
        if (UDebriefListItem* Row = Cast<UDebriefListItem>(UnitList->GetItemAt(0)))
            DrawSelectedUnit(Row->DataIndex);
    }
}

void UMissionDebriefDlg::OnUnitSelectionChanged(UObject* SelectedItem)
{
    const UDebriefListItem* Item = Cast<UDebriefListItem>(SelectedItem);
    if (!Item)
        return;

    DrawSelectedUnit(Item->DataIndex);
}

void UMissionDebriefDlg::DrawSelectedUnit(int32 StatsIndex)
{
    UListView* SummaryList = GetList(330);
    UListView* EventList = GetList(340);

    if (!SummaryList || !EventList)
        return;

    SummaryList->ClearListItems();
    EventList->ClearListItems();

    UDebriefListItem* Snapshot=UnitSnapshots.FindRef(StatsIndex);
    if (Snapshot && Snapshot->bHasSnapshot)
    {
        for (const auto& Row : Snapshot->SummaryRows) SummaryList->AddItem(Row);
        for (const auto& Row : Snapshot->EventRows) EventList->AddItem(Row);
        return;
    }
    ShipStats* Stats = (StatsIndex >= 0) ? ShipStats::GetStats(StatsIndex) : nullptr;
    if (!Stats)
        return;

    Stats->Summarize();

    // Summary:
    auto AddSummary = [&](const char* Label, int Value)
        {
            char Buf[64] = { 0 };
            sprintf_s(Buf, "%d", Value);

            SummaryList->AddItem(UDebriefListItem::Make(
                this,
                ANSI_TO_TCHAR(Label),
                ANSI_TO_TCHAR(Buf)
            ));
        };

    AddSummary("Guns Fired:", Stats->GetGunShots());
    AddSummary("Gun Hits:", Stats->GetGunHits());
    AddSummary("Gun Kills:", Stats->GetGunKills());

    SummaryList->AddItem(UDebriefListItem::Make(this, TEXT(" "))); // spacer

    AddSummary("Missiles Fired:", Stats->GetMissileShots());
    AddSummary("Missile Hits:", Stats->GetMissileHits());
    AddSummary("Missile Kills:", Stats->GetMissileKills());

    SummaryList->AddItem(UDebriefListItem::Make(this, TEXT(" ")));

    AddSummary("Deaths:", Stats->GetDeaths());
    AddSummary("Collisions:", Stats->GetColls());
    AddSummary("Points:", Stats->GetPoints());
    AddSummary("Command Points:", Stats->GetCommandPoints());

    if (MissionPtr)
    {
        SummaryList->AddItem(UDebriefListItem::Make(
            this,
            TEXT("Result:"),
            DebriefResult.Outcome
        ));
    }

    EventList->AddItem(UDebriefListItem::Make(this,TEXT("RESULT"),DebriefResult.Outcome,DebriefResult.Reason));
    // Events:
    ListIter<SimEvent> Iter = Stats->GetEvents();
    while (++Iter)
    {
        SimEvent* E = Iter.value();
        if (!E) continue;

        char TimeTxt[64] = { 0 };
        const int Time = E->GetTime();

        if (Time > 24 * 60 * 60) FormatDayTime(TimeTxt, Time);
        else                     FormatTime(TimeTxt, Time);

        const char* Target = E->GetTarget();

        EventList->AddItem(UDebriefListItem::Make(
            this,
            ANSI_TO_TCHAR(TimeTxt),
            ANSI_TO_TCHAR(E->GetEventDesc()),
            FString(Target ? ANSI_TO_TCHAR(Target) : TEXT("")) + (E->GetInfo() && *E->GetInfo() ? FString(TEXT(" ")) + ANSI_TO_TCHAR(E->GetInfo()) : FString())
        ));
    }
    if (Snapshot)
    {
        for (UObject* Item : SummaryList->GetListItems()) Snapshot->SummaryRows.Add(Cast<UDebriefListItem>(Item));
        for (UObject* Item : EventList->GetListItems()) Snapshot->EventRows.Add(Cast<UDebriefListItem>(Item));
        Snapshot->bHasSnapshot=true;
    }

}

void UMissionDebriefDlg::RefreshOutcomeAndRecord(int32 Points)
{
    ShipStats* Stats = PlayerShip ? ShipStats::Find(PlayerShip->GetName()) : nullptr;
    FMissionDebriefEvidence E=CollectMissionDebriefEvidence(MissionPtr,
        SimPtr ? SimPtr->GetPlayerElement() : nullptr,Stats);
    DebriefResult=EvaluateMissionDebrief(E);
    if (OutcomeWidget)
    {
        OutcomeWidget->SetText(FText::FromString(DebriefResult.Outcome));
        OutcomeWidget->SetColorAndOpacity(FSlateColor(DebriefResult.bSuccess
            ? FLinearColor(0.2f,1.f,0.4f) : FLinearColor(1.f,0.75f,0.2f)));
    }
    if (EvidenceWidget) EvidenceWidget->SetText(FText::FromString(DebriefResult.Reason));
    if (MissionPtr)
    {
        MissionPtr->SetActive(false);
        if (DebriefResult.bSuccess) MissionPtr->SetComplete(true);
    }
    if (!SimPtr || !MissionPtr || (RecordedMission==MissionPtr && RecordedStartTime==SimPtr->StartTime())) return;
    // Snapshot before CommitMission clears ShipStats. Save evidence and score separately.
    if (auto* GI=Cast<USSWGameInstance>(GetGameInstance()))
    {
        if (DebriefResult.bSuccess && CampaignPtr && CampaignPtr->OwnsMission(MissionPtr)
            && !CampaignPtr->IsCutsceneMission(MissionPtr->GetIdentity()))
            if (auto* Players=GI->GetSubsystem<UStarshatterPlayerSubsystem>())
            {
                auto& Info=Players->GetMutablePlayerInfo();
                Info.MarkMissionCompleted(CampaignPtr->GetCampaignId(),MissionPtr->GetIdentity());
                if(CampaignPtr->GetCampaignId()==1 && MissionPtr->GetIdentity()==103
                    && MissionPtr->GetMissionType()==int(EMissionType::TRAINING))
                {
                    Info.bTrainingGraduate=true;
                    if(auto* Pilot=PlayerCharacter::GetCurrentPlayer()){Pilot->CompleteFlightTraining();Info.Rank=Pilot->GetRank();}
                }
                Players->MarkDirty();
                if (!Players->SavePlayer(true))
                    UE_LOG(LogTemp,Warning,TEXT("[MissionDebrief] Could not save player mission completion"));
            }
        if (UCampaignSave* Save=GI->CampaignSave; Save && CampaignPtr
            && Save->CampaignIndex==CampaignPtr->GetCampaignId() && CampaignPtr->OwnsMission(MissionPtr))
        {
            FMissionDebriefRecord Record;
            Record.MissionId=MissionPtr->GetIdentity(); Record.MissionName=ANSI_TO_TCHAR(MissionPtr->GetName());
            if (auto* Players=GI->GetSubsystem<UStarshatterPlayerSubsystem>()) Record.Pilot=Players->GetPlayerInfo().Name;
            Record.Outcome=DebriefResult.Outcome; Record.Evidence=DebriefResult.Reason;
            Record.Points=Points; Record.RecordedAt=FDateTime::UtcNow();
            Save->MissionDebriefs.Add(Record);
            if (DebriefResult.bSuccess)
            {
                Save->RecordCompletedMission(CampaignPtr,MissionPtr);
                if(CampaignPtr->GetCampaignId()==1 && MissionPtr->GetIdentity()==103
                    && MissionPtr->GetMissionType()==int(EMissionType::TRAINING)) Save->bTrainingGraduate=true;
            }
            if (!GI->SaveCampaign())
                UE_LOG(LogTemp,Warning,TEXT("[MissionDebrief] Could not save mission result for %d"),Record.MissionId);
        }
    }
    RecordedMission=MissionPtr; RecordedStartTime=SimPtr->StartTime();
    UE_LOG(LogTemp,Display,TEXT("[MissionDebrief] Mission=%d Outcome=%s Points=%d Evidence=%s"),
        MissionPtr->GetIdentity(),*DebriefResult.Outcome,Points,*DebriefResult.Reason);
}

// --------------------------------------------------------------------
// Close
// --------------------------------------------------------------------

void UMissionDebriefDlg::OnCloseClicked()
{
    Sim* LocalSim = Sim::GetSim();
    if (!LocalSim)
        return;

    LocalSim->CommitMission();
    bIsShown = false;
    PlayerShip = nullptr;
    MissionPtr = nullptr;
    SimPtr = nullptr;
    for (int ID : {320, 330, 340})
        if (UListView* List = GetList(ID)) List->ClearListItems();
    LocalSim->UnloadMission();

    PlayerCharacter* PlayerObj = PlayerCharacter::GetCurrentPlayer();
    if (PlayerObj && PlayerObj->ShowAward())
    {
        if (Manager)
        {
            Manager->ShowAwardDlg();
        }
        else if (GetMenuManager()) GetMenuManager()->ShowAwardDlg();
        return;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error,
            TEXT("MissionDebriefDlg::OnCloseClicked() - GameInstance not found"));

        return;
    }

    USSWRuntimeSubsystem* RuntimeSS =
        GI->GetSubsystem<USSWRuntimeSubsystem>();

    if (!RuntimeSS)
    {
        UE_LOG(LogTemp, Error,
            TEXT("MissionDebriefDlg::OnCloseClicked() - Runtime subsystem not found"));

        return;
    }

    Mouse::Show(false);

    Campaign* Camp = Campaign::GetCampaign();
    if (Camp && Camp->GetCampaignId() < Campaign::SINGLE_MISSIONS)
    {
        RuntimeSS->SetGameMode(EGameMode::CMPN);
    }
    else
    {
        RuntimeSS->SetGameMode(EGameMode::MENU);
    }
    if (GetMenuManager())
    {
        Hide();
        GetMenuManager()->ReturnFromMissionBriefing();
    }
}
