/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdMissionsDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdMissionsDlg implementation
*/

#include "CmdMissionsDlg.h"

#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"

#include "MissionListObject.h"

#include "Starshatter.h"
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "PlayerCharacter.h"
#include "Game.h"
#include "Mouse.h"
#include "FormatUtil.h"
#include "CombatGroup.h"
#include "CmpnScreen.h"
#include "SSWRuntimeSubsystem.h"


static bool ShouldShowMissionInCmdMissionsDlg(const MissionInfo* Info)
{
    if (!Info)
    {
        return false;
    }

    // Always show normal player missions.
    if (Info->DisplayType == EMissionDisplayType::PlayerMission)
        return true;

    // Training campaign missions (type Training) must appear in operations
    // even if the DataTable tagged them StoryOnly/CutsceneOnly.
    if (Info->type == (int)EMissionType::TRAINING)
        return true;

    // Training campaign: show everything that is not explicitly hidden.
    Campaign* C = Campaign::GetCampaign();
    if (C && C->IsTraining() && Info->DisplayType != EMissionDisplayType::Hidden)
        return true;

    return false;
}

UCmdMissionsDlg::UCmdMissionsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdMissionsDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] NativeConstruct"));

    BuildModernMissionsLayout();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    if (MissionList)
    {
        MissionList->OnItemClicked().RemoveAll(this);
        MissionList->OnItemSelectionChanged().RemoveAll(this);

        MissionList->OnItemClicked().AddUObject(this, &UCmdMissionsDlg::OnMissionItemClicked);
        MissionList->OnItemSelectionChanged().AddUObject(this, &UCmdMissionsDlg::OnMissionSelectionChanged);

        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] MissionList bindings set"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdMissionsDlg] MissionList is NULL"));
    }

    SelectedMission = nullptr;
    SelectedMissionItem = nullptr;

    ClearDescription();
}

void UCmdMissionsDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

void UCmdMissionsDlg::SetManager(UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdMissionsDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

void UCmdMissionsDlg::ShowMissionsDlg()
{
    Mode = ECOMMAND_MODE::MODE_MISSIONS;

    CampaignPtr = Campaign::GetCampaign();
    Stars = Starshatter::GetInstance();

    if (!CampaignPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdMissionsDlg] ShowMissionsDlg: CampaignPtr NULL"));
        ClearDescription();
        SelectedMission = nullptr;
        SelectedMissionItem = nullptr;

        if (MissionList)
        {
            MissionList->ClearSelection();
        }

        if (ParentCmdDlg)
        {
            ParentCmdDlg->UpdateMissionButton();
        }

        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdMissionsDlg] ShowMissionsDlg: Campaign='%s' missions=%d"),
        ANSI_TO_TCHAR(CampaignPtr->GetName()),
        CampaignPtr->GetMissionList().size());

    SetVisibility(ESlateVisibility::Visible);

    RebuildMissionList();

    SelectedMission = nullptr;
    SelectedMissionItem = nullptr;

    if (MissionList && MissionList->GetNumItems() > 0)
    {
        UObject* FirstObj = MissionList->GetItemAt(0);
        UMissionListObject* FirstItem = Cast<UMissionListObject>(FirstObj);

        if (FirstItem)
        {
            // Visual selection
            MissionList->SetSelectedItem(FirstObj);

            // Force internal selection state
            SelectedMissionItem = FirstItem;
            SelectedMission = FirstItem->MissionInfoPtr
                ? FirstItem->MissionInfoPtr->mission
                : nullptr;

            UpdateMissionDetailPanel(FirstItem);

            if (FirstItem->MissionInfoPtr)
            {
                SetDescriptionForMissionInfo(FirstItem->MissionInfoPtr);
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[CmdMissionsDlg] FORCE SELECT: id=%d name=%s"),
                FirstItem->MissionId,
                *FirstItem->MissionName);
        }
        else
        {
            ClearDescription();

            if (MissionList)
            {
                MissionList->ClearSelection();
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] ShowMissionsDlg: no mission items after rebuild"));
        ClearDescription();

        if (MissionList)
        {
            MissionList->ClearSelection();
        }
    }

    SyncModernSelection();
    if (ParentCmdDlg)
    {
        ParentCmdDlg->UpdateMissionButton();
    }
}

void UCmdMissionsDlg::ExecFrame()
{
    if (!CampaignPtr)
    {
        CampaignPtr = Campaign::GetCampaign();
    }

    if (!CampaignPtr || !MissionList)
    {
        return;
    }

    AppendNewMissionsIfAny();
    ValidateSelectionStillExists();
}

void UCmdMissionsDlg::RebuildMissionList()
{
    if (!MissionList)
    {
        return;
    }

    CampaignPtr = Campaign::GetCampaign();
    if (!CampaignPtr)
    {
        return;
    }

    MissionList->ClearListItems();

    SelectedMission = nullptr;
    SelectedMissionItem = nullptr;

    List<MissionInfo>& Missions = CampaignPtr->GetMissionList();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdMissionsDlg] RebuildMissionList: missions=%d"),
        Missions.size());

    for (int32 i = 0; i < Missions.size(); ++i)
    {
        MissionInfo* Info = Missions[i];
        if (!Info)
        {
            continue;
        }

        if (!ShouldShowMissionInCmdMissionsDlg(Info))
        {
            UE_LOG(LogTemp, Log,
                TEXT("[CmdMissionsDlg] Skipping story/cutscene mission id=%d"),
                Info->id);
            continue;
        }

        AddMissionInfoToList(Info);
    }

    MissionList->RequestRefresh();
    RefreshModernMissionRows();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdMissionsDlg] RebuildMissionList complete: total UI items=%d"),
        MissionList->GetNumItems());
}

void UCmdMissionsDlg::AppendNewMissionsIfAny()
{
    if (!MissionList)
    {
        return;
    }

    CampaignPtr = Campaign::GetCampaign();
    if (!CampaignPtr)
    {
        return;
    }

    const int32 ExistingVisible = MissionList->GetNumItems();
    const int32 TotalVisible = GetVisibleMissionCount();

    if (TotalVisible < ExistingVisible)
    {
        const int32 PrevSelectedId = GetSelectedMissionId();
        RebuildMissionList();

        if (PrevSelectedId > 0)
        {
            SetSelectedMissionId(PrevSelectedId);
        }
        else
        {
            LoadFirstMissionPreview();

            if (MissionList)
            {
                MissionList->ClearSelection();
            }

            SelectedMission = nullptr;
            SelectedMissionItem = nullptr;
        }

        if (ParentCmdDlg)
        {
            ParentCmdDlg->UpdateMissionButton();
        }

        return;
    }

    if (TotalVisible == ExistingVisible)
    {
        return;
    }

    List<MissionInfo>& Missions = CampaignPtr->GetMissionList();

    for (int32 i = 0; i < Missions.size(); ++i)
    {
        MissionInfo* Info = Missions[i];
        if (!Info || !ShouldShowMissionInCmdMissionsDlg(Info))
        {
            continue;
        }

        bool bAlreadyInList = false;

        for (int32 j = 0; j < MissionList->GetNumItems(); ++j)
        {
            UMissionListObject* ExistingItem = Cast<UMissionListObject>(MissionList->GetItemAt(j));
            if (ExistingItem && ExistingItem->MissionId == Info->id)
            {
                bAlreadyInList = true;
                break;
            }
        }

        if (!bAlreadyInList)
        {
            AddMissionInfoToList(Info);
        }
    }

    MissionList->RequestRefresh();
    RefreshModernMissionRows();

    if (!SelectedMissionItem)
    {
        LoadFirstMissionPreview();

        if (MissionList)
        {
            MissionList->ClearSelection();
        }

        SelectedMission = nullptr;
        SelectedMissionItem = nullptr;
    }

    if (ParentCmdDlg)
    {
        ParentCmdDlg->UpdateMissionButton();
    }
}

void UCmdMissionsDlg::ValidateSelectionStillExists()
{
    CampaignPtr = Campaign::GetCampaign();
    if (!CampaignPtr)
    {
        return;
    }

    if (!SelectedMissionItem || !SelectedMissionItem->MissionInfoPtr)
    {
        return;
    }

    const int32 SelectedId = SelectedMissionItem->MissionId;
    bool bFound = false;

    List<MissionInfo>& Missions = CampaignPtr->GetMissionList();
    for (int32 i = 0; i < Missions.size(); ++i)
    {
        MissionInfo* Info = Missions[i];
        if (Info && Info->id == SelectedId)
        {
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        SelectedMission = nullptr;
        SelectedMissionItem = nullptr;
        ClearDescription();

        if (MissionList)
        {
            MissionList->ClearSelection();
        }
    }
}

int32 UCmdMissionsDlg::GetSelectedMissionId() const
{
    if (!MissionList)
    {
        return 0;
    }

    UObject* Selected = MissionList->GetSelectedItem();
    UMissionListObject* Item = Cast<UMissionListObject>(Selected);
    return Item ? Item->MissionId : 0;
}

void UCmdMissionsDlg::SetSelectedMissionId(int32 MissionId)
{
    if (!MissionList || MissionId <= 0)
    {
        return;
    }

    const int32 Num = MissionList->GetNumItems();
    for (int32 i = 0; i < Num; ++i)
    {
        UObject* Obj = MissionList->GetItemAt(i);
        UMissionListObject* Item = Cast<UMissionListObject>(Obj);
        if (Item && Item->MissionId == MissionId)
        {
            MissionList->SetSelectedItem(Obj);
            HandleMissionSelection(Obj);
            break;
        }
    }
}

void UCmdMissionsDlg::OnMissionItemClicked(UObject* Item)
{
    HandleMissionSelection(Item);
}

void UCmdMissionsDlg::OnMissionSelectionChanged(UObject* Item)
{
    HandleMissionSelection(Item);
}

void UCmdMissionsDlg::HandleMissionSelection(UObject* ItemObj)
{
    if (!MissionList)
    {
        return;
    }

    UMissionListObject* Item = Cast<UMissionListObject>(ItemObj);
    if (!Item)
    {
        SelectedMission = nullptr;
        SelectedMissionItem = nullptr;
        ClearDescription();

        if (ParentCmdDlg)
        {
            ParentCmdDlg->UpdateMissionButton();
        }
        return;
    }

    MissionInfo* Info = Item->MissionInfoPtr;

    if (Info)
    {
        SetDescriptionForMissionInfo(Info);
        UpdateMissionDetailPanel(Item);
        SelectedMission = Info->mission;
        SelectedMissionItem = Item;
    }
    else
    {
        SelectedMission = nullptr;
        SelectedMissionItem = nullptr;
        ClearDescription();
    }

    SyncModernSelection();
    if (ParentCmdDlg)
    {
        ParentCmdDlg->UpdateMissionButton();
    }
}

void UCmdMissionsDlg::UpdateMissionDetailPanel(UMissionListObject* Item)
{
    if (!Item)
    {
        if (MissionNameText)      MissionNameText->SetText(FText::GetEmpty());
        if (MissionTypeText)      MissionTypeText->SetText(FText::GetEmpty());
        if (MissionStatusText)    MissionStatusText->SetText(FText::GetEmpty());
        if (MissionSystemText)    MissionSystemText->SetText(FText::GetEmpty());
        if (MissionRegionText)    MissionRegionText->SetText(FText::GetEmpty());
        if (MissionObjectiveText) MissionObjectiveText->SetText(FText::GetEmpty());
        if (MissionSitrepText)    MissionSitrepText->SetText(FText::GetEmpty());
        if (MissionStartText)     MissionStartText->SetText(FText::GetEmpty());
        if (MissionImage)         MissionImage->SetBrush(FSlateBrush());
        return;
    }

    if (MissionNameText)      MissionNameText->SetText(FText::FromString(Item->MissionName));
    if (MissionTypeText)      MissionTypeText->SetText(FText::FromString(Item->MissionType));
    if (MissionStatusText)    MissionStatusText->SetText(FText::FromString(Item->MissionStatus));
    if (MissionSystemText)    MissionSystemText->SetText(FText::FromString(Item->MissionSystem));
    if (MissionRegionText)    MissionRegionText->SetText(FText::FromString(Item->MissionRegion));
    if (MissionObjectiveText) MissionObjectiveText->SetText(FText::FromString(Item->MissionObjective));
    if (MissionSitrepText)    MissionSitrepText->SetText(FText::FromString(Item->MissionSitrep));
    if (MissionStartText)     MissionStartText->SetText(FText::FromString(Item->MissionTime));
}

void UCmdMissionsDlg::SetDescriptionForMissionInfo(MissionInfo* Info)
{
    if (!Info)
    {
        return;
    }

    if (MissionSitrepText)
    {
        MissionSitrepText->SetText(FText::FromString(UTF8_TO_TCHAR(Info->description)));
    }
}

void UCmdMissionsDlg::ClearDescription()
{
    UpdateMissionDetailPanel(nullptr);
}

bool UCmdMissionsDlg::CanAcceptMission(MissionInfo* Info) const
{
    if (!Info)
    {
        return false;
    }

    if (Info->mission)
    {
        return Info->mission->IsOK();
    }

    return true;
}

bool UCmdMissionsDlg::CanAcceptSelectedMission() const
{
    if (!SelectedMissionItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] CanAcceptSelectedMission: SelectedMissionItem is NULL"));
        return false;
    }

    return CanAcceptMission(SelectedMissionItem->MissionInfoPtr);
}

void UCmdMissionsDlg::LoadFirstMissionPreview()
{
    if (!MissionList || MissionList->GetNumItems() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] LoadFirstMissionPreview: no mission items"));
        ClearDescription();
        return;
    }

    UObject* FirstObj = MissionList->GetItemAt(0);
    UMissionListObject* FirstItem = Cast<UMissionListObject>(FirstObj);
    if (!FirstItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] LoadFirstMissionPreview: first item invalid"));
        ClearDescription();
        return;
    }

    // Preview only. This is not a real selection.
    UpdateMissionDetailPanel(FirstItem);

    MissionInfo* Info = FirstItem->MissionInfoPtr;
    if (Info)
    {
        SetDescriptionForMissionInfo(Info);
    }

    SelectedMission = nullptr;
    SelectedMissionItem = nullptr;

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdMissionsDlg] LoadFirstMissionPreview: previewing first mission '%s' without selecting"),
        *FirstItem->MissionName);
}

int32 UCmdMissionsDlg::GetVisibleMissionCount() const
{
    Campaign* LocalCampaign = Campaign::GetCampaign();
    if (!LocalCampaign)
    {
        return 0;
    }

    int32 Count = 0;
    List<MissionInfo>& Missions = LocalCampaign->GetMissionList();

    for (int32 i = 0; i < Missions.size(); ++i)
    {
        if (ShouldShowMissionInCmdMissionsDlg(Missions[i]))
        {
            ++Count;
        }
    }

    return Count;
}

void UCmdMissionsDlg::OnSaveClicked()
{
    if (Manager)
    {
        Manager->ShowCmpFileDlg();
    }
}

void UCmdMissionsDlg::OnExitClicked()
{
    Mouse::Show(false);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USSWRuntimeSubsystem* RuntimeSS =
            GI->GetSubsystem<USSWRuntimeSubsystem>())
        {
            RuntimeSS->SetGameMode(EGameMode::MENU);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("CmdMissionsDlg: Runtime subsystem not found."));
        }
    }
}

void UCmdMissionsDlg::AddMissionInfoToList(MissionInfo* Info)
{
    if (!MissionList || !Info)
    {
        return;
    }

    PlayerCharacter* LegacyPlayer = PlayerCharacter::GetCurrentPlayer();

    UMissionListObject* Item = NewObject<UMissionListObject>(this);
    if (!Item)
    {
        return;
    }

    Item->MissionInfoPtr = Info;
    Item->MissionId = Info->id;
    Item->MissionName = UTF8_TO_TCHAR(Info->name.data());
    Item->MissionRegion = UTF8_TO_TCHAR(Info->region.data());
    Item->MissionSystem = UTF8_TO_TCHAR(Info->system.data());
    Item->MissionDesc = UTF8_TO_TCHAR(Info->description.data());
    Item->MissionObjective = UTF8_TO_TCHAR(Info->player_info.data());
    Item->MissionSitrep = UTF8_TO_TCHAR(Info->description.data());

    Mission* M = Info->mission;

    if (M)
    {
        bool bTrained = false;

        if (LegacyPlayer && M->GetMissionType() == (int)EMissionType::TRAINING)
        {
            bTrained = LegacyPlayer->HasTrained(M->GetIdentity());
        }

        Item->MissionType = (M->GetMissionType() == (int)EMissionType::TRAINING && bTrained)
            ? TEXT("Training")
            : UTF8_TO_TCHAR(M->GetTypeName());

        Item->MissionStatus = TEXT("Active");
    }
    else
    {
        switch (Info->type)
        {
        case (int)EMissionType::PATROL:
            Item->MissionType = TEXT("Patrol");
            break;
        case (int)EMissionType::STRIKE:
            Item->MissionType = TEXT("Strike");
            break;
        case (int)EMissionType::ESCORT:
            Item->MissionType = TEXT("Escort");
            break;
        case (int)EMissionType::DEFEND:
            Item->MissionType = TEXT("Defend");
            break;
        case (int)EMissionType::TRAINING:
            Item->MissionType = TEXT("Training");
            break;
        default:
            Item->MissionType = FString::Printf(TEXT("Type %d"), Info->type);
            break;
        }

        Item->MissionStatus = TEXT("Available");
    }

    char StartTime[64] = { 0 };
    FormatDayTime(StartTime, Info->start);
    Item->MissionTime = UTF8_TO_TCHAR(StartTime);

    MissionList->AddItem(Item);
}

namespace
{
    const FLinearColor CmdText(0.94f, 0.97f, 1.f, 1.f);
    const FLinearColor CmdMuted(0.71f, 0.81f, 0.88f, 1.f);
    const FLinearColor CmdAccent(0.29f, 0.68f, 0.89f, 1.f);
    UTextBlock* MissionText(UWidgetTree* Tree, const FString& Initial, const FLinearColor& Color, const FName Name = NAME_None)
    {
        UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Text->SetText(FText::FromString(Initial));
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetAutoWrapText(true);
        Text->SetJustification(ETextJustify::Left);
        return Text;
    }
    void MissionLine(UVerticalBox* Parent, UWidget* Child, float Bottom = 10.f)
    {
        UVerticalBoxSlot* LineSlot = Parent->AddChildToVerticalBox(Child);
        LineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
        LineSlot->SetHorizontalAlignment(HAlign_Fill);
    }
    UVerticalBox* MissionCard(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* Header)
    {
        UBorder* Card = Tree->ConstructWidget<UBorder>();
        Card->SetBrushColor(FLinearColor(0.07f, 0.10f, 0.15f, 0.9f));
        Card->SetPadding(FMargin(13.f));
        MissionLine(Parent, Card, 12.f);
        UVerticalBox* Contents = Tree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Contents);
        MissionLine(Contents, MissionText(Tree, FString(Header), CmdAccent), 8.f);
        return Contents;
    }
}

void UCmdMissionRowProxy::Activate()
{
    if (UCmdMissionsDlg* Dialog = Owner.Get())
    {
        Dialog->SetSelectedMissionId(MissionId);
    }
}

void UCmdMissionsDlg::BuildModernMissionsLayout()
{
    if (!WidgetTree) return;
    UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!CanvasRoot) CanvasRoot = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CanvasRoot")));
    if (!CanvasRoot)
    {
        UE_LOG(LogTemp, Error, TEXT("[CmdMissionsDlg] Blueprint requires a Canvas Panel root named CanvasRoot"));
        return;
    }
    if (WidgetTree->FindWidget(TEXT("Missions_ModernRoot"))) return;

    // Preserve any existing bound ListView's data/selection events. A new
    // collapsed ListView is used when the Blueprint provides only CanvasRoot.
    if (!MissionList)
    {
        MissionList = WidgetTree->ConstructWidget<UListView>(UListView::StaticClass(), TEXT("Missions_NativeListModel"));
        UCanvasPanelSlot* ModelSlot = CanvasRoot->AddChildToCanvas(MissionList);
        ModelSlot->SetAnchors(FAnchors(0.f));
        ModelSlot->SetOffsets(FMargin(0.f));
        MissionList->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        MissionList->SetVisibility(ESlateVisibility::Collapsed);
    }

    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Missions_PanelBackground"));
    if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
    {
        Background->SetBrushFromTexture(Texture, true);
    }
    else UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] Missing /Game/UI/Panel.Panel texture"));
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot* BgSlot = CanvasRoot->AddChildToCanvas(Background);
    BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    BgSlot->SetOffsets(FMargin(0.f));
    BgSlot->SetZOrder(-1);

    // Static page header; the columns below scroll independently.
    UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Missions_ModernRoot"));
    UCanvasPanelSlot* MainSlot = CanvasRoot->AddChildToCanvas(Page);
    MainSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    MainSlot->SetOffsets(FMargin(0.f));
    MainSlot->SetZOrder(0);
    MissionLine(Page, MissionText(WidgetTree, TEXT("OPERATIONS  /  MISSION SELECTION"), CmdAccent), 8.f);
    NativeMissionCount = MissionText(WidgetTree, TEXT("AVAILABLE MISSIONS"), CmdMuted);
    MissionLine(Page, NativeMissionCount, 14.f);

    UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
    UVerticalBoxSlot* ColumnsSlot = Page->AddChildToVerticalBox(Columns);
    ColumnsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ColumnsSlot->SetHorizontalAlignment(HAlign_Fill);
    ColumnsSlot->SetVerticalAlignment(VAlign_Fill);

    // Left: independent roster scrollbox. The bound ListView is a hidden model;
    // the visible rows and click handlers still use the existing model.
    UBorder* RosterCard = WidgetTree->ConstructWidget<UBorder>();
    RosterCard->SetBrushColor(FLinearColor(0.06f, 0.09f, 0.14f, 0.85f));
    RosterCard->SetPadding(FMargin(14.f));
    UHorizontalBoxSlot* RosterSlot = Columns->AddChildToHorizontalBox(RosterCard);
    RosterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    RosterSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    RosterSlot->SetVerticalAlignment(VAlign_Fill);

    UScrollBox* RosterScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Missions_RosterScroll"));
    RosterCard->SetContent(RosterScroll);
    UVerticalBox* RosterContents = WidgetTree->ConstructWidget<UVerticalBox>();
    RosterScroll->AddChild(RosterContents);
    MissionLine(RosterContents, MissionText(WidgetTree, TEXT("AVAILABLE ASSIGNMENTS"), CmdAccent));
    NativeMissionRows = WidgetTree->ConstructWidget<UVerticalBox>();
    MissionLine(RosterContents, NativeMissionRows, 0.f);

    // Right: independent detail scrollbox; changing missions never scrolls the roster.
    UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Missions_DetailScroll"));
    UHorizontalBoxSlot* DetailSlot = Columns->AddChildToHorizontalBox(DetailScroll);
    DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DetailSlot->SetVerticalAlignment(VAlign_Fill);
    UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>();
    DetailScroll->AddChild(Details);

    UVerticalBox* Identity = MissionCard(WidgetTree, Details, TEXT("MISSION DETAILS"));
    MissionNameText = MissionText(WidgetTree, TEXT("SELECT A MISSION"), CmdText); MissionLine(Identity, MissionNameText);
    MissionTypeText = MissionText(WidgetTree, TEXT(""), CmdMuted); MissionLine(Identity, MissionTypeText);
    MissionStatusText = MissionText(WidgetTree, TEXT(""), CmdMuted); MissionLine(Identity, MissionStatusText);
    MissionStartText = MissionText(WidgetTree, TEXT(""), CmdMuted); MissionLine(Identity, MissionStartText, 0.f);

    UVerticalBox* Location = MissionCard(WidgetTree, Details, TEXT("LOCATION"));
    MissionSystemText = MissionText(WidgetTree, TEXT(""), CmdText); MissionLine(Location, MissionSystemText);
    MissionRegionText = MissionText(WidgetTree, TEXT(""), CmdMuted); MissionLine(Location, MissionRegionText, 0.f);

    UVerticalBox* Objectives = MissionCard(WidgetTree, Details, TEXT("TASKFOCE"));
    MissionObjectiveText = MissionText(WidgetTree, TEXT(""), CmdText); MissionLine(Objectives, MissionObjectiveText, 0.f);

    UVerticalBox* Sitrep = MissionCard(WidgetTree, Details, TEXT("SITUATION REPORT"));
    MissionSitrepText = MissionText(WidgetTree, TEXT(""), CmdText); MissionLine(Sitrep, MissionSitrepText, 0.f);
    UE_LOG(LogTemp, Log, TEXT("[CmdMissionsDlg] Modern Missions layout attached to CanvasRoot (independent roster/detail scrollboxes)"));
}

void UCmdMissionsDlg::RefreshModernMissionRows()
{
    if (!NativeMissionRows || !MissionList || !WidgetTree) return;
    NativeMissionRows->ClearChildren();
    RowCallbacks.Empty(); RowButtons.Empty(); RowLabels.Empty(); RowMissionIds.Empty();
    const int32 Count = MissionList->GetNumItems();
    if (NativeMissionCount)
        NativeMissionCount->SetText(FText::FromString(FString::Printf(TEXT("%d AVAILABLE MISSIONS"), Count)));
    if (!Count)
    {
        MissionLine(NativeMissionRows, MissionText(WidgetTree, TEXT("No missions available"), CmdMuted));
        return;
    }
    for (int32 Index = 0; Index < Count; ++Index)
    {
        UMissionListObject* Item = Cast<UMissionListObject>(MissionList->GetItemAt(Index));
        if (!Item) continue;
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        Button->SetBackgroundColor(FLinearColor(0.16f, 0.23f, 0.31f, 1.f));
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        if (UButtonSlot* ButtonContentSlot = Cast<UButtonSlot>(Button->AddChild(Content)))
        {
            ButtonContentSlot->SetPadding(FMargin(12.f, 8.f));
            ButtonContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ButtonContentSlot->SetVerticalAlignment(VAlign_Center);
        }
        MissionLine(Content, MissionText(WidgetTree,
            FString::Printf(TEXT("%03d  %s"), Item->MissionId, *Item->MissionName), CmdText), 4.f);
        MissionLine(Content, MissionText(WidgetTree,
            FString::Printf(TEXT("%s"), *Item->MissionType), CmdMuted), 0.f);
        MissionLine(NativeMissionRows, Button, 8.f);
        UCmdMissionRowProxy* Proxy = NewObject<UCmdMissionRowProxy>(this);
        Proxy->Owner = this;
        Proxy->MissionId = Item->MissionId;
        Button->OnClicked.AddDynamic(Proxy, &UCmdMissionRowProxy::Activate);
        RowCallbacks.Add(Proxy); RowButtons.Add(Button);
        RowMissionIds.Add(Item->MissionId);
    }
    SyncModernSelection();
}

void UCmdMissionsDlg::SyncModernSelection()
{
    const int32 SelectedId = GetSelectedMissionId();
    for (int32 Index = 0; Index < RowButtons.Num(); ++Index)
    {
        if (RowButtons[Index])
            RowButtons[Index]->SetBackgroundColor(RowMissionIds.IsValidIndex(Index) && RowMissionIds[Index] == SelectedId
                ? FLinearColor(0.15f, 0.41f, 0.57f, 1.f)
                : FLinearColor(0.16f, 0.23f, 0.31f, 1.f));
    }
}
