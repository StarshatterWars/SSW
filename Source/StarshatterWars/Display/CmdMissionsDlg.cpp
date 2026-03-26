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
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Image.h"

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

static bool ShouldShowMissionInCmdMissionsDlg(const MissionInfo* Info)
{
    if (!Info)
    {
        return false;
    }

    return Info->DisplayType == EMissionDisplayType::PlayerMission;
}

UCmdMissionsDlg::UCmdMissionsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdMissionsDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] NativeConstruct"));

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

    if (btn_accept)
    {
        btn_accept->OnClicked.RemoveDynamic(this, &UCmdMissionsDlg::OnAcceptClicked);
        btn_accept->OnClicked.AddDynamic(this, &UCmdMissionsDlg::OnAcceptClicked);
        btn_accept->SetIsEnabled(false);
    }

    if (btn_save)
    {
        btn_save->OnClicked.RemoveDynamic(this, &UCmdMissionsDlg::OnSaveClicked);
        btn_save->OnClicked.AddDynamic(this, &UCmdMissionsDlg::OnSaveClicked);
    }

    if (btn_exit)
    {
        btn_exit->OnClicked.RemoveDynamic(this, &UCmdMissionsDlg::OnExitClicked);
        btn_exit->OnClicked.AddDynamic(this, &UCmdMissionsDlg::OnExitClicked);
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
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmdMissionsDlg] ShowMissionsDlg: Campaign='%s' missions=%d"),
            ANSI_TO_TCHAR(CampaignPtr->Name()),
            CampaignPtr->GetMissionList().size());
    }


    SetVisibility(ESlateVisibility::Visible);

    RebuildMissionList();
    LoadFirstMissionData();
    UpdateAcceptEnabled();
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
    UpdateAcceptEnabled();
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

    List<MissionInfo>& Missions = CampaignPtr->GetMissionList();
    const int32 Existing = MissionList->GetNumItems();
    const int32 Total = Missions.size();

    if (Total < Existing)
    {
        const int32 PrevSelectedId = GetSelectedMissionId();
        RebuildMissionList();

        if (PrevSelectedId > 0)
        {
            SetSelectedMissionId(PrevSelectedId);
        }
        else
        {
            LoadFirstMissionData();
        }
        return;
    }

    if (Total == Existing)
    {
        return;
    }

    for (int32 i = Existing; i < Total; ++i)
    {
        MissionInfo* Info = Missions[i];
        if (!Info)
        {
            continue;
        }

        AddMissionInfoToList(Info);
    }

    MissionList->RequestRefresh();

    if (!SelectedMissionItem)
    {
        LoadFirstMissionData();
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

        if (btn_accept)
        {
            btn_accept->SetIsEnabled(false);
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
        UpdateAcceptEnabled();
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

    UpdateAcceptEnabled();
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
    return (Info != nullptr);
}

void UCmdMissionsDlg::UpdateAcceptEnabled()
{
    if (!btn_accept)
    {
        return;
    }

    MissionInfo* Info = SelectedMissionItem ? SelectedMissionItem->MissionInfoPtr : nullptr;
    btn_accept->SetIsEnabled(CanAcceptMission(Info));
}

void UCmdMissionsDlg::LoadFirstMissionData()
{
    if (!MissionList || MissionList->GetNumItems() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] LoadFirstMissionData: no mission items"));
        return;
    }

    UObject* FirstObj = MissionList->GetItemAt(0);
    UMissionListObject* FirstItem = Cast<UMissionListObject>(FirstObj);
    if (!FirstItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] LoadFirstMissionData: first item invalid"));
        return;
    }

    SelectedMissionItem = FirstItem;

    MissionInfo* Info = FirstItem->MissionInfoPtr;
    if (Info)
    {
        SelectedMission = Info->mission;
        SetDescriptionForMissionInfo(Info);
    }
    else
    {
        SelectedMission = nullptr;
    }

    UpdateMissionDetailPanel(FirstItem);
    UpdateAcceptEnabled();

    UE_LOG(LogTemp, Warning,
        TEXT("[CmdMissionsDlg] LoadFirstMissionData: loaded first mission '%s'"),
        *FirstItem->MissionName);
}

void UCmdMissionsDlg::OnAcceptClicked()
{
    CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr || !SelectedMissionItem || !Stars)
    {
        return;
    }

    Mission* MissionToStart = SelectedMission;
    if (!MissionToStart && SelectedMissionItem->MissionId >= 0)
    {
        MissionToStart = CampaignPtr->GetMission(SelectedMissionItem->MissionId);
    }

    if (!MissionToStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmdMissionsDlg] OnAcceptClicked: mission could not be resolved"));
        return;
    }

    Mouse::Show(false);
    CampaignPtr->SetMissionId(MissionToStart->GetIdentity());
    CampaignPtr->StartMission();
    Stars->SetGameMode(EGameMode::PREP);
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
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
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

        if (LegacyPlayer && M->GetType() == (int)EMISSIONTYPE::TRAINING)
        {
            bTrained = LegacyPlayer->HasTrained(M->GetIdentity());
        }

        Item->MissionType = (M->GetType() == (int)EMISSIONTYPE::TRAINING && bTrained)
            ? TEXT("Training")
            : UTF8_TO_TCHAR(M->GetTypeName());

        Item->MissionStatus = TEXT("Active");
    }
    else
    {
        switch (Info->type)
        {
        case (int)EMISSIONTYPE::PATROL:
            Item->MissionType = TEXT("Patrol");
            break;
        case (int)EMISSIONTYPE::STRIKE:
            Item->MissionType = TEXT("Strike");
            break;
        case (int)EMISSIONTYPE::ESCORT:
            Item->MissionType = TEXT("Escort");
            break;
        case (int)EMISSIONTYPE::DEFEND:
            Item->MissionType = TEXT("Defend");
            break;
        case (int)EMISSIONTYPE::TRAINING:
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