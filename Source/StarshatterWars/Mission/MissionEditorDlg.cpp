/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionEditorDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Mission Editor dialog (Unreal)
    - UUserWidget-based replacement for legacy MsnEditDlg
    - Uses UBaseScreen FORM binding helpers (BindButton/BindEdit/BindCombo/BindList)
    - Edits mission header fields, SIT/PKG/MAP tab logic, and adds/removes elements/events
*/

#include "MissionEditorDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"

// Starshatter
#include "MenuScreen.h"
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "MissionEvent.h"
#include "Galaxy.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "GameStructs.h"

// Unreal
#include "Math/UnrealMathUtility.h"

UMissionEditorDlg::UMissionEditorDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Do not bind UMG events here (widgets not ready). Keep ctor lightweight.
}

void UMissionEditorDlg::InitializeMissionEditor(UMenuScreen* InManager)
{
    Manager = InManager;
}

void UMissionEditorDlg::SetMission(Mission* InMission)
{
    mission = InMission;
    current_tab = 0;
}

void UMissionEditorDlg::SetMissionInfo(MissionInfo* InMissionInfo)
{
    mission_info = InMissionInfo;
    current_tab = 0;
}

void UMissionEditorDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // BaseScreen Enter/Escape routing (assumes UBaseScreen exposes these pointers)
    ApplyButton = BtnAccept;
    CancelButton = BtnCancel;

    // Prevent accidental double-binding if widget reconstructs:
    if (BtnAccept)
    {
        BtnAccept->OnClicked.RemoveAll(this);
        BtnAccept->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnAcceptClicked);
    }

    if (BtnCancel)
    {
        BtnCancel->OnClicked.RemoveAll(this);
        BtnCancel->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnCancelClicked);
    }

    if (BtnSit)
    {
        BtnSit->OnClicked.RemoveAll(this);
        BtnSit->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnTabSitClicked);
    }

    if (BtnPkg)
    {
        BtnPkg->OnClicked.RemoveAll(this);
        BtnPkg->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnTabPkgClicked);
    }

    if (BtnMap)
    {
        BtnMap->OnClicked.RemoveAll(this);
        BtnMap->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnTabMapClicked);
    }

    if (BtnElemAdd)
    {
        BtnElemAdd->OnClicked.RemoveAll(this);
        BtnElemAdd->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnElemAddClicked);
    }

    if (BtnElemEdit)
    {
        BtnElemEdit->OnClicked.RemoveAll(this);
        BtnElemEdit->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnElemEditClicked);
    }

    if (BtnElemDel)
    {
        BtnElemDel->OnClicked.RemoveAll(this);
        BtnElemDel->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnElemDelClicked);
    }

    if (BtnElemInc)
    {
        BtnElemInc->OnClicked.RemoveAll(this);
        BtnElemInc->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnElemIncClicked);
    }

    if (BtnElemDec)
    {
        BtnElemDec->OnClicked.RemoveAll(this);
        BtnElemDec->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnElemDecClicked);
    }

    if (BtnEventAdd)
    {
        BtnEventAdd->OnClicked.RemoveAll(this);
        BtnEventAdd->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnEventAddClicked);
    }

    if (BtnEventEdit)
    {
        BtnEventEdit->OnClicked.RemoveAll(this);
        BtnEventEdit->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnEventEditClicked);
    }

    if (BtnEventDel)
    {
        BtnEventDel->OnClicked.RemoveAll(this);
        BtnEventDel->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnEventDelClicked);
    }

    if (BtnEventInc)
    {
        BtnEventInc->OnClicked.RemoveAll(this);
        BtnEventInc->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnEventIncClicked);
    }

    if (BtnEventDec)
    {
        BtnEventDec->OnClicked.RemoveAll(this);
        BtnEventDec->OnClicked.AddDynamic(this, &UMissionEditorDlg::OnEventDecClicked);
    }

    if (CmbSystem)
    {
        CmbSystem->OnSelectionChanged.RemoveAll(this);
        CmbSystem->OnSelectionChanged.AddDynamic(this, &UMissionEditorDlg::OnSystemSelectChanged);
    }

    Show();
}

FString UMissionEditorDlg::GetLegacyFormText() const
{
    return LegacyFormText;
}

void UMissionEditorDlg::BindFormWidgets()
{
    // Tabs
    BindButton(301, BtnSit);
    BindButton(302, BtnPkg);
    BindButton(303, BtnMap);

    // OK/Cancel
    BindButton(1, BtnAccept);
    BindButton(2, BtnCancel);

    // Fields
    BindEdit(201, TxtName);
    BindCombo(202, CmbType);
    BindCombo(203, CmbSystem);
    BindCombo(204, CmbRegion);

    BindEdit(410, TxtDescription);
    BindEdit(411, TxtSituation);
    BindEdit(412, TxtObjective);

    // Lists
    BindList(510, LstElem);
    BindList(520, LstEvent);

    // Element buttons
    BindButton(501, BtnElemAdd);
    BindButton(505, BtnElemEdit);
    BindButton(502, BtnElemDel);
    BindButton(503, BtnElemInc);
    BindButton(504, BtnElemDec);

    // Event buttons
    BindButton(511, BtnEventAdd);
    BindButton(515, BtnEventEdit);
    BindButton(512, BtnEventDel);
    BindButton(513, BtnEventInc);
    BindButton(514, BtnEventDec);
}

void UMissionEditorDlg::Show()
{
    if (!TxtName || !CmbType)
        return;

    // Clear fields first
    TxtName->SetText(FText::GetEmpty());

    // Populate system combo from Galaxy
    if (CmbSystem)
    {
        CmbSystem->ClearOptions();

        Galaxy* galaxy = Galaxy::GetInstance();
        if (galaxy)
        {
            ListIter<StarSystem> iter = galaxy->GetSystemList();
            while (++iter)
            {
                StarSystem* sys = iter.value();
                if (sys && sys->GetName())
                {
                    CmbSystem->AddOption(ANSI_TO_TCHAR(sys->GetName()));
                }
            }
        }
    }

    // Populate from mission
    if (mission)
    {
        TxtName->SetText(FText::FromString(ANSI_TO_TCHAR(mission->GetName())));

        if (CmbType)
        {
            CmbType->SetSelectedIndex(mission->GetType());
        }

        StarSystem* sys = mission->GetStarSystem();
        if (sys && CmbSystem && CmbRegion)
        {
            // Select system by name
            const FString SysName = ANSI_TO_TCHAR(sys->GetName());
            CmbSystem->SetSelectedOption(SysName);

            // Fill regions
            CmbRegion->ClearOptions();

            // NOTE: depending on your List<T> semantics, copying OrbitalRegion by value may be heavy.
            // Keeping as-is to match legacy code patterns:
            List<OrbitalRegion> regions;
            regions.append(sys->AllRegions());
            regions.sort();

            int32 sel_rgn = 0;
            int32 idx = 0;

            ListIter<OrbitalRegion> riter = regions;
            while (++riter)
            {
                OrbitalRegion* region = riter.value();
                if (!region || !region->GetName())
                    continue;

                CmbRegion->AddOption(ANSI_TO_TCHAR(region->GetName()));

                if (!FCStringAnsi::Strcmp(mission->GetRegion(), region->GetName()))
                {
                    sel_rgn = idx;
                }

                idx++;
            }

            CmbRegion->SetSelectedIndex(sel_rgn);
        }

        if (TxtDescription && mission_info)
        {
            TxtDescription->SetText(FText::FromString(ANSI_TO_TCHAR(mission_info->description.data())));
        }

        if (TxtSituation)
        {
            TxtSituation->SetText(FText::FromString(ANSI_TO_TCHAR(mission->GetSituation())));
        }

        if (TxtObjective)
        {
            TxtObjective->SetText(FText::FromString(ANSI_TO_TCHAR(mission->GetObjective())));
        }

        DrawPackages();
    }

    ShowTab(current_tab);
}

void UMissionEditorDlg::ScrapeForm()
{
    if (!mission)
        return;

    if (TxtName)
    {
        const FString NameStr = TxtName->GetText().ToString();
        mission->SetName(TCHAR_TO_ANSI(*NameStr));
    }

    if (CmbType)
    {
        const int32 NewType = CmbType->GetSelectedIndex();
        mission->SetType(NewType);

        if (mission_info)
        {
            mission_info->type = NewType;
        }
    }

    // System
    Galaxy* galaxy = Galaxy::GetInstance();
    StarSystem* system = 0;

    if (galaxy && CmbSystem)
    {
        const FString Selected = CmbSystem->GetSelectedOption();
        system = galaxy->GetSystem(TCHAR_TO_ANSI(*Selected));
    }

    if (system)
    {
        mission->ClearSystemList();
        mission->SetStarSystem(system);

        if (mission_info)
        {
            mission_info->system = system->GetName();
        }
    }

    // Region
    if (CmbRegion)
    {
        const FString Region = CmbRegion->GetSelectedOption();
        mission->SetRegion(TCHAR_TO_ANSI(*Region));

        if (mission_info)
        {
            mission_info->region = TCHAR_TO_ANSI(*Region);
        }
    }

    if (TxtDescription && mission_info)
    {
        const FString Desc = TxtDescription->GetText().ToString();

        // Keep Starshatter Text core type:
        mission_info->description = TCHAR_TO_ANSI(*Desc);

        // Also push into mission if you keep a duplicate field there:
        mission->SetDescription(TCHAR_TO_ANSI(*Desc));
    }

    if (TxtSituation)
    {
        const FString Sit = TxtSituation->GetText().ToString();
        mission->SetSituation(TCHAR_TO_ANSI(*Sit));
    }

    if (TxtObjective)
    {
        const FString Obj = TxtObjective->GetText().ToString();
        mission->SetObjective(TCHAR_TO_ANSI(*Obj));
    }
}

void UMissionEditorDlg::ShowTab(int32 Tab)
{
    current_tab = Tab;
    if (current_tab < 0 || current_tab > 2)
        current_tab = 0;

    const bool bSit = (current_tab == 0);
    const bool bPkg = (current_tab == 1);
    const bool bMap = (current_tab == 2);

    // SIT tab
    if (TxtDescription) TxtDescription->SetVisibility(bSit ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (TxtSituation)   TxtSituation->SetVisibility(bSit ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (TxtObjective)   TxtObjective->SetVisibility(bSit ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    // PKG tab
    if (LstElem)  LstElem->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (LstEvent) LstEvent->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    if (BtnElemAdd)  BtnElemAdd->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnElemEdit) BtnElemEdit->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnElemDel)  BtnElemDel->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnElemInc)  BtnElemInc->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnElemDec)  BtnElemDec->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    if (BtnEventAdd)  BtnEventAdd->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnEventEdit) BtnEventEdit->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnEventDel)  BtnEventDel->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnEventInc)  BtnEventInc->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (BtnEventDec)  BtnEventDec->SetVisibility(bPkg ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    // MAP tab: legacy pushes nav dialog
    if (bMap && Manager)
    {
        ScrapeForm();
        // Manager->ShowNavDlg(); // wire to your real API
    }
}

void UMissionEditorDlg::DrawPackages()
{
    bool elem_del = false;
    bool elem_edit = false;
    bool elem_inc = false;
    bool elem_dec = false;

    bool event_del = false;
    bool event_edit = false;
    bool event_inc = false;
    bool event_dec = false;

    // Until you wire UE ListView selection to your legacy data model, keep these disabled.

    if (BtnElemDel)  BtnElemDel->SetIsEnabled(elem_del);
    if (BtnElemEdit) BtnElemEdit->SetIsEnabled(elem_edit);
    if (BtnElemInc)  BtnElemInc->SetIsEnabled(elem_inc);
    if (BtnElemDec)  BtnElemDec->SetIsEnabled(elem_dec);

    if (BtnEventDel)  BtnEventDel->SetIsEnabled(event_del);
    if (BtnEventEdit) BtnEventEdit->SetIsEnabled(event_edit);
    if (BtnEventInc)  BtnEventInc->SetIsEnabled(event_inc);
    if (BtnEventDec)  BtnEventDec->SetIsEnabled(event_dec);
}

void UMissionEditorDlg::OnTabSitClicked()
{
    ShowTab(0);
}

void UMissionEditorDlg::OnTabPkgClicked()
{
    ShowTab(1);
}

void UMissionEditorDlg::OnTabMapClicked()
{
    ShowTab(2);
}

void UMissionEditorDlg::OnSystemSelectChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;

    StarSystem* sys = 0;

    if (CmbSystem)
    {
        const FString Name = CmbSystem->GetSelectedOption();

        Galaxy* galaxy = Galaxy::GetInstance();
        if (galaxy)
        {
            ListIter<StarSystem> iter = galaxy->GetSystemList();
            while (++iter)
            {
                StarSystem* s = iter.value();
                if (s && s->GetName() && Name.Equals(ANSI_TO_TCHAR(s->GetName())))
                {
                    sys = s;
                    break;
                }
            }
        }
    }

    if (sys && CmbRegion)
    {
        CmbRegion->ClearOptions();

        List<OrbitalRegion> regions;
        regions.append(sys->AllRegions());
        regions.sort();

        ListIter<OrbitalRegion> iter = regions;
        while (++iter)
        {
            OrbitalRegion* region = iter.value();
            if (region && region->GetName())
            {
                CmbRegion->AddOption(ANSI_TO_TCHAR(region->GetName()));
            }
        }
    }

    ScrapeForm();
}

void UMissionEditorDlg::OnAcceptClicked()
{
    if (mission)
    {
        ScrapeForm();

        if (mission_info)
        {
            mission_info->name = mission->GetName();
        }

        mission->Save();
    }

    if (Manager)
    {
        // Manager->ShowMsnSelectDlg(); // wire to your real API
    }
}

void UMissionEditorDlg::OnCancelClicked()
{
    if (mission)
    {
        Campaign* ActiveCampaign = Campaign::GetCampaign();

        if (ActiveCampaign)
        {
            const FS_CampaignMission* MissionData =
                ActiveCampaign->FindCampaignMissionById(mission->GetIdentity());

            if (MissionData)
            {
                mission->LoadFromCampaignMissionData(*MissionData);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("OnCancelClicked: No campaign data found for mission %d"),
                    mission->GetIdentity());
            }
        }
    }

    if (Manager)
    {
        // Manager->ShowMsnSelectDlg();
    }
}

void UMissionEditorDlg::OnElemAddClicked()
{
    if (!mission)
        return;

    List<MissionElement>& elements = mission->GetElements();
    MissionElement* elem = new MissionElement;

    if (elements.size() > 0)
    {
        // Use UE random functions (replace legacy RandomPoint()):
        // If MissionElement expects a Point/Vec3, update its signature to accept FVector.
        // Here we assume SetLocation(FVector) exists or will exist.
        const FVector RandLoc(
            FMath::FRandRange(-1.0f, 1.0f),
            FMath::FRandRange(-1.0f, 1.0f),
            FMath::FRandRange(-1.0f, 1.0f)
        );

        elem->SetLocation(RandLoc);
    }

    if (CmbRegion)
    {
        elem->SetRegion(TCHAR_TO_ANSI(*CmbRegion->GetSelectedOption()));
    }

    elements.append(elem);

    DrawPackages();

    if (Manager)
    {
        // Manager->ShowMsnElemDlg();
    }
}

void UMissionEditorDlg::OnElemDelClicked()
{
    if (!mission)
        return;

    // Requires selection wiring for UE ListView.
    DrawPackages();
}

void UMissionEditorDlg::OnElemEditClicked()
{
    if (!mission)
        return;

    // Requires selection wiring for UE ListView.
}

void UMissionEditorDlg::OnElemIncClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
    DrawPackages();
}

void UMissionEditorDlg::OnElemDecClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
    DrawPackages();
}

void UMissionEditorDlg::OnEventAddClicked()
{
    if (!mission)
        return;

    List<MissionEvent>& events = mission->GetEvents();
    MissionEvent* event = new MissionEvent;

    int id = 1;
    for (int i = 0; i < events.size(); i++)
    {
        MissionEvent* e = events[i];
        if (e && e->EventID() >= id)
            id = e->EventID() + 1;
    }

    event->id = id;
    events.append(event);

    DrawPackages();

    if (Manager)
    {
        // Manager->ShowMsnEventDlg();
    }
}

void UMissionEditorDlg::OnEventDelClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
    DrawPackages();
}

void UMissionEditorDlg::OnEventEditClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
}

void UMissionEditorDlg::OnEventIncClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
    DrawPackages();
}

void UMissionEditorDlg::OnEventDecClicked()
{
    if (!mission)
        return;

    // Requires selection wiring.
    DrawPackages();
}
