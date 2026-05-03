/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionSelectDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Full Unreal conversion of legacy MsnSelectDlg.

    Notes:
    - We bind to widgets strictly via BaseScreen FORM id mapping (GetButton/GetList/GetText/GetCombo).
    - We use UMissionListItem for mission list entries in UListView.
    - No lambdas (per project rule).
    - No sender params for UButton: we bind distinct handlers per button id.
*/

#include "MissionSelectDlg.h"

#include "MissionListItem.h"

// Unreal:
#include "Engine/World.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"

// Slate input (ExecFrame Enter handling is already in BaseScreen via HandleAccept/Cancel):
#include "InputCoreTypes.h"

// Legacy core:
#include "Starshatter.h"
#include "Campaign.h"
#include "Mission.h"
#include "Game.h"
#include "FormatUtil.h"
#include "GameStructs.h"
#include "SSWRuntimeSubsystem.h"


// +--------------------------------------------------------------------+

Mission* UMissionSelectDlg::EditMission = nullptr;

// +--------------------------------------------------------------------+

UMissionSelectDlg::UMissionSelectDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Stars = nullptr;
    CampaignPtr = nullptr;

    SelectedMissionIndex = -1;
    MissionId = 0;
    bEditable = false;
}

// +--------------------------------------------------------------------+
// UBaseScreen binding
// +--------------------------------------------------------------------+

void UMissionSelectDlg::BindFormWidgets()
{
    // Buttons
    BindButton(1, BtnAccept);
    BindButton(2, BtnCancel);

    BindButton(301, BtnNew);
    BindButton(302, BtnEdit);
    BindButton(303, BtnDelete);

    // Lists
    BindList(202, MissionsList);
    BindList(203, CampaignsList);

    // Campaign selector (optional in legacy: combo OR list)
    BindCombo(201, CampaignCombo);

    // Description text (RichTextBlock)
    BindText(200, DescriptionText);
}
// +--------------------------------------------------------------------+
// UUserWidget lifecycle
// +--------------------------------------------------------------------+

void UMissionSelectDlg::NativeConstruct()
{
    Super::NativeConstruct();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    RegisterControls();
    WireEvents();
}

void UMissionSelectDlg::NativeDestruct()
{
    Super::NativeDestruct();
}

// +--------------------------------------------------------------------+
// Legacy RegisterControls / WireEvents
// +--------------------------------------------------------------------+

void UMissionSelectDlg::RegisterControls()
{
    // Match legacy: accept disabled until mission selected
    if (UButton* Accept = GetButton(1))
    {
        Accept->SetIsEnabled(false);
    }

    // New/Edit/Delete default states are set in Show() after we know campaign type
}

void UMissionSelectDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionSelectDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UMissionSelectDlg::WireEvents()
{
    if (bWired)
        return;

    // Accept / Cancel:
    if (UButton* Accept = GetButton(1))
    {
        if (!Accept->OnClicked.IsAlreadyBound(this, &UMissionSelectDlg::OnAcceptClicked))
            Accept->OnClicked.AddDynamic(this, &UMissionSelectDlg::OnAcceptClicked);
    }

    if (UButton* Cancel = GetButton(2))
    {
        if (!Cancel->OnClicked.IsAlreadyBound(this, &UMissionSelectDlg::OnCancelClicked))
            Cancel->OnClicked.AddDynamic(this, &UMissionSelectDlg::OnCancelClicked);
    }

    // Optional buttons:
    if (UButton* NewBtn = GetButton(301))
    {
        if (!NewBtn->OnClicked.IsAlreadyBound(this, &UMissionSelectDlg::OnNewClicked))
            NewBtn->OnClicked.AddDynamic(this, &UMissionSelectDlg::OnNewClicked);
    }

    if (UButton* EditBtn = GetButton(302))
    {
        if (!EditBtn->OnClicked.IsAlreadyBound(this, &UMissionSelectDlg::OnEditClicked))
            EditBtn->OnClicked.AddDynamic(this, &UMissionSelectDlg::OnEditClicked);
    }

    if (UButton* DelBtn = GetButton(303))
    {
        if (!DelBtn->OnClicked.IsAlreadyBound(this, &UMissionSelectDlg::OnDeleteClicked))
            DelBtn->OnClicked.AddDynamic(this, &UMissionSelectDlg::OnDeleteClicked);
    }

    // Campaign selection (combo OR list):
    CampaignCombo = GetCombo(201);
    if (CampaignCombo)
    {
        CampaignCombo->OnSelectionChanged.RemoveAll(this);
        CampaignCombo->OnSelectionChanged.AddDynamic(
            this,
            &UMissionSelectDlg::OnCampaignComboChanged
        );
    }
    if (UListView* CampaignList = GetList(203))
    {
        // Remove any existing binding(s) from this object to avoid duplicates:
        CampaignList->OnItemSelectionChanged().RemoveAll(this);

        // Bind:
        CampaignList->OnItemSelectionChanged().AddUObject(
            this,
            &UMissionSelectDlg::OnCampaignListSelectionChanged
        );
    }

    // Mission selection:
    if (UListView* Missions = GetList(202))
    {
        Missions->OnItemSelectionChanged().RemoveAll(this);
        Missions->OnItemSelectionChanged().AddUObject(this, &UMissionSelectDlg::OnMissionListSelectionChanged);
    }

    bWired = true;
}

// +--------------------------------------------------------------------+
// Show / ExecFrame (legacy)
// +--------------------------------------------------------------------+

void UMissionSelectDlg::Show()
{
    Super::Show();

    CampaignPtr = Campaign::GetCampaign();

    PopulateCampaigns();

    // Determine editability based on campaign id (legacy rule):
    if (CampaignPtr)
    {
        const int32 Id = CampaignPtr->GetCampaignId();
        bEditable = (Id >= Campaign::MULTIPLAYER_MISSIONS && Id <= Campaign::CUSTOM_MISSIONS);

        if (UButton* NewBtn = GetButton(301))  NewBtn->SetIsEnabled(bEditable);
        if (UButton* EditBtn = GetButton(302)) EditBtn->SetIsEnabled(false);
        if (UButton* DelBtn = GetButton(303))  DelBtn->SetIsEnabled(false);
    }

    // Default description:
    if (URichTextBlock* Desc = GetText(200))
    {
        Desc->SetText(FText::FromString(FString(Game::GetText("MsnSelectDlg.choose").data())));
    }

    PopulateMissions();

    // Legacy behavior:
    // - OnMissionSelect(0) after populating
    // We replicate by recomputing selection state and updating description/buttons:
    UpdateDescriptionAndButtons();

    EditMission = nullptr;
}

void UMissionSelectDlg::ExecFrame(double DeltaTime)
{
    Super::ExecFrame(DeltaTime);

    // Legacy handled Enter in ExecFrame; we already handle Enter/Escape in UBaseScreen.
}

// +--------------------------------------------------------------------+
// Accept / Cancel (Enter/Escape)
// +--------------------------------------------------------------------+

void UMissionSelectDlg::HandleAccept()
{
    Super::HandleAccept();
    OnAcceptClicked();
}

void UMissionSelectDlg::HandleCancel()
{
    Super::HandleCancel();
    OnCancelClicked();
}

// +--------------------------------------------------------------------+
// Populate Campaigns / Missions
// +--------------------------------------------------------------------+

void UMissionSelectDlg::PopulateCampaigns()
{
    // Preferred UE path: Campaign combo box (ID 201)
    CampaignCombo = GetCombo(201);
    if (!CampaignCombo)
        return;

    CampaignCombo->ClearOptions();

    int32 Index = 0;
    ListIter<Campaign> Iter = Campaign::GetAllCampaigns();
    while (++Iter)
    {
        Campaign* C = Iter.value();
        if (!C)
            continue;

        // Legacy rule: show only single/custom mission campaigns
        if (C->GetCampaignId() >= Campaign::SINGLE_MISSIONS)
        {
            const FString CampaignName = FString(C->GetName());
            CampaignCombo->AddOption(CampaignName);

            // Legacy auto-select logic
            if (CampaignPtr && CampaignPtr->GetCampaignId() < Campaign::SINGLE_MISSIONS)
            {
                CampaignPtr = Campaign::SelectCampaign(C->GetName());
                CampaignCombo->SetSelectedIndex(Index);
            }
            else if (CampaignPtr && CampaignPtr->GetCampaignId() == C->GetCampaignId())
            {
                CampaignCombo->SetSelectedIndex(Index);
            }

            ++Index;
        }
    }
}

void UMissionSelectDlg::PopulateMissions()
{
    UListView* Missions = GetList(202);
    if (!Missions)
        return;

    Missions->ClearListItems();
    MissionItems.Reset();

    if (!CampaignPtr)
        return;

    ListIter<MissionInfo> Iter = CampaignPtr->GetMissionList();
    int32 Index = 0;

    while (++Iter)
    {
        MissionInfo* Info = Iter.value();
        if (!Info)
        {
            ++Index;
            continue;
        }

        UMissionListItem* Item = NewObject<UMissionListItem>(this);
        Item->InitFromMissionInfo(CampaignPtr, Info, Index);

        MissionItems.Add(Item);
        Missions->AddItem(Item);

        // Legacy: if this mission matches edit_mission, select it:
        if (Info->mission && Info->mission == EditMission)
        {
            Missions->SetSelectedItem(Item);
        }

        ++Index;
    }

    // Legacy: if selected_mission persisted and nothing selected, select it:
    if (SelectedMissionIndex >= 0 && Missions && Missions->GetNumItems() > 0)
    {
        TArray<UObject*> Selected;
        Missions->GetSelectedItems(Selected);

        if (Selected.Num() == 0)
        {
            if (SelectedMissionIndex < MissionItems.Num() && MissionItems[SelectedMissionIndex])
            {
                Missions->SetSelectedItem(MissionItems[SelectedMissionIndex]);
            }
        }
    }
}

// +--------------------------------------------------------------------+
// Selection update (legacy OnMissionSelect)
// +--------------------------------------------------------------------+

void UMissionSelectDlg::UpdateDescriptionAndButtons()
{
    SelectedMissionIndex = -1;
    MissionId = 0;

    UListView* Missions = GetList(202);
    URichTextBlock* Desc = GetText(200);
    UButton* Accept = GetButton(1);
    UButton* EditBtn = GetButton(302);
    UButton* DelBtn = GetButton(303);

    if (!Missions || !Desc || !CampaignPtr)
    {
        if (Accept) Accept->SetIsEnabled(false);
        if (EditBtn) EditBtn->SetIsEnabled(false);
        if (DelBtn) DelBtn->SetIsEnabled(false);
        return;
    }

    TArray<UObject*> Selected;
    Missions->GetSelectedItems(Selected);

    UMissionListItem* SelectedItem =
        (Selected.Num() > 0)
        ? Cast<UMissionListItem>(Selected[0])
        : nullptr;

    if (!SelectedItem)
    {
        Desc->SetText(FText::FromString(FString(Game::GetText("MsnSelectDlg.choose").data())));
        if (Accept) Accept->SetIsEnabled(false);
        if (EditBtn) EditBtn->SetIsEnabled(false);
        if (DelBtn) DelBtn->SetIsEnabled(false);
        return;
    }

    SelectedMissionIndex = SelectedItem->GetIndex();
    MissionId = SelectedItem->GetMissionId();

    // Build legacy description rich text:
    // Matches the original tag style:
    //   <font Limerick12><color ffffff> ...
    //   etc.
    char TimeBuf[32];
    FMemory::Memzero(TimeBuf, sizeof(TimeBuf));
    FormatDayTime(TimeBuf, SelectedItem->GetStartTime());

    FString D;
    D += TEXT("<font Limerick12><color ffffff>");
    D += SelectedItem->GetDisplayName();

    D += TEXT("<font Verdana>\n\n<color ffff80>");
    D += FString(Game::GetText("MsnSelectDlg.mission-type").data());
    D += TEXT("<color ffffff>\n\t");
    D += FString(Mission::GetRoleName(SelectedItem->GetMissionType()));

    D += TEXT("\n\n<color ffff80>");
    D += FString(Game::GetText("MsnSelectDlg.scenario").data());
    D += TEXT("<color ffffff>\n\t");
    D += SelectedItem->GetDescription();

    D += TEXT("\n\n<color ffff80>");
    D += FString(Game::GetText("MsnSelectDlg.location").data());
    D += TEXT("<color ffffff>\n\t");
    D += SelectedItem->GetRegion();
    D += TEXT(" ");
    D += FString(Game::GetText("MsnSelectDlg.sector").data());
    D += TEXT(" / ");
    D += SelectedItem->GetSystem();
    D += TEXT(" ");
    D += FString(Game::GetText("MsnSelectDlg.system").data());

    D += TEXT("\n\n<color ffff80>");
    D += FString(Game::GetText("MsnSelectDlg.start-time").data());
    D += TEXT("<color ffffff>\n\t");
    D += FString(TimeBuf);

    Desc->SetText(FText::FromString(D));

    if (Accept) Accept->SetIsEnabled(true);
    if (EditBtn) EditBtn->SetIsEnabled(bEditable);
    if (DelBtn) DelBtn->SetIsEnabled(bEditable);
}

// +--------------------------------------------------------------------+
// UI Handlers
// +--------------------------------------------------------------------+

void UMissionSelectDlg::OnCampaignComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    const char* SelectedCampaign = TCHAR_TO_ANSI(*SelectedItem);
    Campaign* C = Campaign::SelectCampaign(SelectedCampaign);

    if (C)
    {
        CampaignPtr = C;

        // Reset accept + description
        if (UButton* Accept = GetButton(1)) Accept->SetIsEnabled(false);
        if (URichTextBlock* Desc = GetText(200))
            Desc->SetText(FText::FromString(FString(Game::GetText("MsnSelectDlg.choose").data())));

        // Update editability:
        const int32 Id = C->GetCampaignId();
        bEditable = (Id >= Campaign::MULTIPLAYER_MISSIONS && Id <= Campaign::CUSTOM_MISSIONS);

        if (UButton* NewBtn = GetButton(301))  NewBtn->SetIsEnabled(bEditable);
        if (UButton* EditBtn = GetButton(302)) EditBtn->SetIsEnabled(false);
        if (UButton* DelBtn = GetButton(303))  DelBtn->SetIsEnabled(false);

        // Rebuild mission list:
        PopulateMissions();
        UpdateDescriptionAndButtons();
    }
}

void UMissionSelectDlg::OnCampaignListSelectionChanged(UObject* SelectedItem)
{
    // If you later implement a campaign row model, resolve it here.
    (void)SelectedItem;
}

void UMissionSelectDlg::OnMissionListSelectionChanged(UObject* SelectedItem)
{
    (void)SelectedItem;
    UpdateDescriptionAndButtons();
}

void UMissionSelectDlg::OnAcceptClicked()
{
    if (SelectedMissionIndex >= 0 && CampaignPtr)
    {
        if (MissionId != 0)
        {
            CampaignPtr->SetMissionId(MissionId);
            CampaignPtr->ReloadMission(MissionId);

            if (UGameInstance* GI = GetGameInstance())
            {
                if (USSWRuntimeSubsystem* RuntimeSS =
                    GI->GetSubsystem<USSWRuntimeSubsystem>())
                {
                    RuntimeSS->SetGameMode(EGameMode::PREP);
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("MissionSelectDlg: Runtime subsystem not found."));
                }
            }

            OnAccepted.Broadcast(MissionId);
        }
    }
}

void UMissionSelectDlg::OnCancelClicked()
{
    OnCancelled.Broadcast();
    Hide();
}

void UMissionSelectDlg::OnNewClicked()
{
    // Legacy editor flow referenced MenuScreen + MsnEditDlg.
    // If you have an equivalent UE editor, call it here.
    // This function is still a real handler (no stub call sites).
}

void UMissionSelectDlg::OnEditClicked()
{
    // Legacy editor flow referenced MenuScreen + MsnEditDlg.
}

void UMissionSelectDlg::OnDeleteClicked()
{
    // Legacy confirm flow referenced ConfirmDlg.
    // If you have a confirm dialog screen, invoke it here.
}
