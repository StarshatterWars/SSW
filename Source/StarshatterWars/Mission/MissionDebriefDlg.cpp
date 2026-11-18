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
#include "MissionAwardDlg.h"
#include "CampaignSave.h"
#include "SSWGameInstance.h"
#include "StarshatterPlayerSubsystem.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Styling/CoreStyle.h"
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
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/NativeWidgetHost.h"
#include "ShipSystemIcon.h"
#include "PanelCloseIcon.h"
#include "FlightPanelStyle.h"

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
        Column->SetFont(MissionUIStyle::GetRowFont(12));
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
    auto* Box = EnsureAutoVerticalBoxWithOffsets(FMargin(0));
    if (!Box || !WidgetTree || !RootCanvas) return;
    for (UWidget* Child : RootCanvas->GetAllChildren())
        if (Child != Box) Child->SetVisibility(ESlateVisibility::Collapsed);
    Box->RemoveFromParent(); Box->ClearChildren();
    auto Label = [this](const TCHAR* Text,int32 Size,bool bHeader=false)
    {
        auto* W=WidgetTree->ConstructWidget<UTextBlock>(); W->SetText(FText::FromString(Text));
        W->SetFont(bHeader?MissionUIStyle::GetHeaderFont(Size):MissionUIStyle::GetRowFont(Size));
        W->SetColorAndOpacity(FSlateColor(MissionUIStyle::InfoValueText)); W->SetAutoWrapText(true); return W;
    };
    auto* Frame=WidgetTree->ConstructWidget<UBorder>(); Frame->SetBrush(FlightPanelStyle::Window());
    Frame->SetBrushColor(FLinearColor::White); Frame->SetPadding(FMargin(12)); Frame->SetContent(Box);
    auto* Fit=WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::DownOnly);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(960); Size->SetHeightOverride(650); Size->SetContent(Frame); Fit->SetContent(Size);
    FloatingFrame=Fit;
    auto* FloatSlot=RootCanvas->AddChildToCanvas(Fit); FloatSlot->SetAnchors(FAnchors(.5f)); FloatSlot->SetAlignment(FVector2D(.5));
    FloatSlot->SetPosition(FVector2D::ZeroVector); FloatSlot->SetSize(FVector2D(960,650));
    auto* Header=WidgetTree->ConstructWidget<UHorizontalBox>();
    Header->AddChildToHorizontalBox(Label(TEXT("AFTER ACTION"),16,true))->SetSize(DebriefFill(1));
    CloseButtonWidget=WidgetTree->ConstructWidget<UButton>(); Header->AddChildToHorizontalBox(CloseButtonWidget);
    DragTitleBar=WidgetTree->ConstructWidget<UBorder>(); DragTitleBar->SetBrush(FlightPanelStyle::TitleBar());
    DragTitleBar->SetBrushColor(FLinearColor::White); DragTitleBar->SetPadding(FMargin(10,6)); DragTitleBar->SetContent(Header);
    Box->AddChildToVerticalBox(DragTitleBar)->SetPadding(FMargin(0,0,0,10));
    MissionNameWidget=Label(TEXT(""),18,true); Box->AddChildToVerticalBox(MissionNameWidget)->SetPadding(FMargin(0,0,0,6));
    auto* Meta=WidgetTree->ConstructWidget<UHorizontalBox>();
    for(auto Pair:{TPair<const TCHAR*,TObjectPtr<UTextBlock>*>(TEXT("SYSTEM "),&MissionSystemWidget),
        {TEXT("SECTOR "),&MissionSectorWidget},{TEXT("START "),&MissionTimeStartWidget}})
    {
        Meta->AddChildToHorizontalBox(Label(Pair.Key,12)); *Pair.Value=Label(TEXT(""),12);
        Meta->AddChildToHorizontalBox(*Pair.Value)->SetPadding(FMargin(0,0,18,0));
    }
    Box->AddChildToVerticalBox(Meta)->SetPadding(FMargin(0,0,0,8));
    auto* Result=WidgetTree->ConstructWidget<UHorizontalBox>(); OutcomeWidget=Label(TEXT(""),15,true);
    Result->AddChildToHorizontalBox(OutcomeWidget)->SetSize(DebriefFill(1)); MissionScoreWidget=Label(TEXT(""),14); Result->AddChildToHorizontalBox(MissionScoreWidget);
    Box->AddChildToVerticalBox(Result)->SetPadding(FMargin(0,0,0,6));
    EvidenceWidget=Label(TEXT(""),12); Box->AddChildToVerticalBox(EvidenceWidget)->SetPadding(FMargin(0,0,0,10));
    auto* Tabs=WidgetTree->ConstructWidget<UHorizontalBox>();
    auto Tab=[this,Tabs](EShipSystemIcon Icon,const FText& Tooltip)
    {
        auto* Button=WidgetTree->ConstructWidget<UButton>(); auto* Host=WidgetTree->ConstructWidget<UNativeWidgetHost>();
        Host->SetContent(SNew(SShipSystemIcon).Icon(Icon)); Button->SetContent(Host); Button->SetToolTipText(Tooltip);
        Tabs->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0,0,8,0)); return Button;
    };
    Tab(EShipSystemIcon::Ship,INVTEXT("Summary and unit statistics"))->OnClicked.AddUniqueDynamic(this,&UMissionDebriefDlg::ShowSummaryPage);
    Tab(EShipSystemIcon::Computer,INVTEXT("Mission objectives"))->OnClicked.AddUniqueDynamic(this,&UMissionDebriefDlg::ShowObjectivesPage);
    Tab(EShipSystemIcon::Intel,INVTEXT("Detailed mission event log"))->OnClicked.AddUniqueDynamic(this,&UMissionDebriefDlg::ShowEventsPage);
    Box->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0,0,0,10));
    DebriefPages=WidgetTree->ConstructWidget<UWidgetSwitcher>();
    Box->AddChildToVerticalBox(DebriefPages)->SetSize(DebriefFill(1));
    auto AddList=[this,Label](UVerticalBox* Parent,const TCHAR* Caption,TArray<FString> Captions)
    {
        Parent->AddChildToVerticalBox(Label(Caption,14,true))->SetPadding(FMargin(0,0,0,6));
        const float Weights[3][4]={{.65f,.35f,0,0},{.16f,.46f,.38f,0},{.1f,.35f,.3f,.25f}};
        auto* Headings=WidgetTree->ConstructWidget<UHorizontalBox>();
        for(int32 I=0;I<Captions.Num();++I)
        {
            auto* Slot=Headings->AddChildToHorizontalBox(Label(*Captions[I],12)); Slot->SetSize(DebriefFill(Weights[Captions.Num()-2][I])); Slot->SetPadding(FMargin(6,0));
        }
        Parent->AddChildToVerticalBox(Headings)->SetPadding(FMargin(0,0,0,6));
        auto* List=WidgetTree->ConstructWidget<UDebriefListView>();
        UClass* Row=LoadClass<UDebriefRowWidget>(nullptr,TEXT("/Game/Screens/Mission/Rows/WBP_DebriefRow.WBP_DebriefRow_C"));
        List->SetEntry(Row?Row:UDebriefRowWidget::StaticClass()); Parent->AddChildToVerticalBox(List)->SetSize(DebriefFill(1)); return List;
    };
    auto* Summary=WidgetTree->ConstructWidget<UHorizontalBox>(); auto* Units=WidgetTree->ConstructWidget<UVerticalBox>(); auto* Stats=WidgetTree->ConstructWidget<UVerticalBox>();
    Summary->AddChildToHorizontalBox(Units)->SetSize(DebriefFill(.52f)); auto* StatsSlot=Summary->AddChildToHorizontalBox(Stats); StatsSlot->SetSize(DebriefFill(.48f)); StatsSlot->SetPadding(FMargin(12,0,0,0));
    UnitListWidget=AddList(Units,TEXT("UNITS"),{TEXT("PKG"),TEXT("CALLSIGN"),TEXT("ROLE"),TEXT("TYPE")});
    SummaryListWidget=AddList(Stats,TEXT("SUMMARY"),{TEXT("STATISTICS"),TEXT("VALUE")}); DebriefPages->AddChild(Summary);
    auto* ObjectivePage=WidgetTree->ConstructWidget<UScrollBox>(); ObjectivesWidget=WidgetTree->ConstructWidget<URichTextBlock>();
    FTextBlockStyle Style; Style.SetFont(MissionUIStyle::GetRowFont(13)); Style.SetColorAndOpacity(FSlateColor(MissionUIStyle::InfoValueText));
    ObjectivesWidget->SetDefaultTextStyle(Style); ObjectivesWidget->SetAutoWrapText(true); ObjectivePage->AddChild(ObjectivesWidget); DebriefPages->AddChild(ObjectivePage);
    auto* Events=WidgetTree->ConstructWidget<UVerticalBox>(); EventListWidget=AddList(Events,TEXT("MISSION LOG"),{TEXT("TIME"),TEXT("EVENT"),TEXT("TARGET / DETAILS")}); DebriefPages->AddChild(Events);
    SituationWidget=nullptr; DebriefPages->SetActiveWidgetIndex(0); bModernUiBuilt=true;
    BindFormWidgets(); CloseButtonWidget->OnClicked.AddUniqueDynamic(this,&UMissionDebriefDlg::OnCloseClicked);
    UnitListWidget->OnItemSelectionChanged().AddUObject(this,&UMissionDebriefDlg::OnUnitSelectionChanged);
}

void UMissionDebriefDlg::ShowSummaryPage(){if(DebriefPages)DebriefPages->SetActiveWidgetIndex(0);}
void UMissionDebriefDlg::ShowObjectivesPage(){if(DebriefPages)DebriefPages->SetActiveWidgetIndex(1);}
void UMissionDebriefDlg::ShowEventsPage(){if(DebriefPages)DebriefPages->SetActiveWidgetIndex(2);}

FReply UMissionDebriefDlg::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if(Event.GetEffectingButton()==EKeys::LeftMouseButton && DragTitleBar && DragTitleBar->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
    {bDraggingDebrief=true;return FReply::Handled().CaptureMouse(TakeWidget());}
    return Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UMissionDebriefDlg::NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if(bDraggingDebrief && RootCanvas && FloatingFrame)
    {
        const auto& View=RootCanvas->GetCachedGeometry(); const FVector2D Size=View.GetLocalSize();
        DebriefDragOffset += View.AbsoluteToLocal(Event.GetScreenSpacePosition())-View.AbsoluteToLocal(Event.GetLastScreenSpacePosition());
        const FVector2D Panel=FloatingFrame->GetCachedGeometry().GetLocalSize();
        const FVector2D Limit=FVector2D(FMath::Max(0.0,(Size.X-Panel.X)*.5),FMath::Max(0.0,(Size.Y-Panel.Y)*.5));
        DebriefDragOffset.X=FMath::Clamp(DebriefDragOffset.X,-Limit.X,Limit.X); DebriefDragOffset.Y=FMath::Clamp(DebriefDragOffset.Y,-Limit.Y,Limit.Y);
        if(auto* FrameSlot=Cast<UCanvasPanelSlot>(FloatingFrame->Slot))FrameSlot->SetPosition(DebriefDragOffset);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(Geometry,Event);
}
FReply UMissionDebriefDlg::NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if(bDraggingDebrief){bDraggingDebrief=false;return FReply::Handled().ReleaseMouseCapture();}
    return Super::NativeOnMouseButtonUp(Geometry,Event);
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
    if(RootCanvas && FloatingFrame)
        if(auto* FrameSlot=Cast<UCanvasPanelSlot>(FloatingFrame->Slot))
        {
            const FVector2D View=RootCanvas->GetCachedGeometry().GetLocalSize();
            if(View.X>48 && View.Y>48) FrameSlot->SetSize(FVector2D(FMath::Min(960.0,View.X-48),FMath::Min(650.0,View.Y-48)));
        }

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
        MissionUIStyle::ApplyActionButtonStyle(CloseButtonWidget,TEXT("Red"));
        CloseButtonWidget->SetToolTipText(INVTEXT("Accept debrief and return"));
        if(WidgetTree && !CloseButtonWidget->GetContent())
        {
            auto* Icon=WidgetTree->ConstructWidget<UTextBlock>(); Icon->SetText(INVTEXT("\u00D7")); Icon->SetFont(MissionUIStyle::GetHeaderFont(18)); Icon->SetColorAndOpacity(FSlateColor(FLinearColor::White)); CloseButtonWidget->SetContent(Icon);
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
    CampaignPtr = TrainingCampaign ? TrainingCampaign : Campaign::GetCampaign();
    TrainingCampaign = nullptr;
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
    SimElement* Flight = PlayerShip ? PlayerShip->GetElement() : nullptr;
    if (!Flight && SimPtr)
        Flight = SimPtr->GetPlayerElement();
    if ((!Flight || Flight->NumObjectives() == 0) && SimPtr && MissionPtr &&
        MissionPtr->GetIdentity() == 102 && MissionPtr->GetMissionType() == int(EMissionType::TRAINING))
    {
        // The trainee's runtime flight retains its evaluated objectives after recovery.
        if (SimElement* Echo = SimPtr->FindElement("Echo"))
            Flight = Echo;
    }
    FMissionDebriefEvidence E = CollectMissionDebriefEvidence(MissionPtr, Flight, Stats);
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
    if (SimPtr->GetMission() == MissionPtr) SimPtr->ResolveAgentAssignment(DebriefResult.bSuccess);
    const bool bStoryMission = MissionPtr->IsStoryMission();
    // Snapshot before CommitMission clears ShipStats. Save evidence and score separately.
    if (auto* GI=Cast<USSWGameInstance>(GetGameInstance()); GI && !GI->IsTrainingTestSession())
    {
        const bool bRegionalAssignment = MissionPtr->IsRegionalAgentMission();
        const bool bTrainingAssignment = MissionPtr->GetMissionType()==int(EMissionType::TRAINING) &&
            MissionPtr->GetIdentity()>=101 && MissionPtr->GetIdentity()<=107;
        const bool bRecordCampaignMission = CampaignPtr && (bStoryMission || CampaignPtr->OwnsMission(MissionPtr))
            && !CampaignPtr->IsCutsceneMission(MissionPtr->GetIdentity());
        if (DebriefResult.bSuccess && (bRecordCampaignMission || bRegionalAssignment || bTrainingAssignment))
            if (auto* Players=GI->GetSubsystem<UStarshatterPlayerSubsystem>())
            {
                auto& Info=Players->GetMutablePlayerInfo();
                const int32 CampaignId = bRegionalAssignment ? Campaign::SINGLE_MISSIONS : bTrainingAssignment ? Campaign::TRAINING_CAMPAIGN : CampaignPtr->GetCampaignId();
                const bool bFirstCompletion = Info.MarkMissionCompleted(CampaignId, MissionPtr->GetIdentity());
                if (bStoryMission && bFirstCompletion)
                {
                    const int32 Bonus = MissionPtr->GetStoryPoints();
                    if (auto* Pilot = PlayerCharacter::GetCurrentPlayer())
                    {
                        Pilot->AddPoints(Bonus);
                        Info.PlayerPoints = Pilot->GetPoints();
                    }
                    Points += Bonus;
                    if (UTextBlock* MissionScore = GetLabel(211))
                        MissionScore->SetText(FText::FromString(FString::Printf(TEXT("%d points (%d story completion)"), Points, Bonus)));
                }
                if(CampaignId==Campaign::TRAINING_CAMPAIGN && MissionPtr->GetIdentity()==103
                    && MissionPtr->GetMissionType()==int(EMissionType::TRAINING))
                {
                    Info.bTrainingGraduate=true;
                    if(auto* Pilot=PlayerCharacter::GetCurrentPlayer()){Pilot->CompleteFlightTraining();Info.Rank=Pilot->GetRank();}
                }
                Players->MarkDirty();
                if (!(bStoryMission ? Players->SaveActivePlayerProgress() : Players->SavePlayer(true)))
                    UE_LOG(LogTemp,Warning,TEXT("[MissionDebrief] Could not save player mission completion"));
            }
        if (UCampaignSave* Save=GI->CampaignSave; Save && CampaignPtr
            && Save->CampaignIndex==CampaignPtr->GetCampaignId() && (bStoryMission || CampaignPtr->OwnsMission(MissionPtr)))
        {
            FMissionDebriefRecord Record;
            Record.MissionId=MissionPtr->GetIdentity(); Record.MissionName=ANSI_TO_TCHAR(MissionPtr->GetName());
            if (auto* Players = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
            {
                Record.Pilot = Players->GetPlayerInfo().Name;
                if (bStoryMission) Record.PilotProfileSlot = Players->GetActiveSavedPlayerSlot();
            }
            Record.bStoryMission = bStoryMission;
            Record.bStoryCompleted = bStoryMission && DebriefResult.bSuccess;
            Record.Outcome=DebriefResult.Outcome; Record.Evidence=DebriefResult.Reason;
            Record.Points=Points; Record.RecordedAt=FDateTime::UtcNow();
            Save->MissionDebriefs.Add(Record);
            if (DebriefResult.bSuccess)
            {
                if (!bStoryMission) Save->RecordCompletedMission(CampaignPtr,MissionPtr);
                if(CampaignPtr->GetCampaignId()==Campaign::TRAINING_CAMPAIGN && MissionPtr->GetIdentity()==103
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
    if (PlayerObj && !(Cast<USSWGameInstance>(GetGameInstance()) && Cast<USSWGameInstance>(GetGameInstance())->IsTrainingTestSession()) && PlayerObj->ShowAward())
    {
        if (Manager && Manager->GetAwardDlg())
        {
            Manager->GetAwardDlg()->SetMenuManager(GetMenuManager());
            Manager->ShowAwardDlg();
            return;
        }
        if (UMenuScreen* Menu = GetMenuManager())
        {
            Menu->ShowAwardDlg();
            return;
        }
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
