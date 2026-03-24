/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionEditorNavDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEditorNavDlg
    - UMG replacement for legacy MsnEditNavDlg (Navigation tab)
    - Inherits UBaseScreen (FORM ID binding + Enter/Escape policy)
    - Preserves legacy control IDs:
        1   Accept
        2   Cancel
        301 Situation tab
        302 Package tab
        303 Navigation tab (this)
        201 Mission name
        202 Mission type
        203 Star system
        204 Sector/region
*/


#include "MissionEditorNavDlg.h"

#include "MenuScreen.h"
#include "MissionEditorDlg.h"

#include "Galaxy.h"
#include "StarSystem.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "GameStructs.h"

UMissionEditorNavDlg::UMissionEditorNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
    bIsFocusable = true;
}

void UMissionEditorNavDlg::BindFormWidgets()
{
    // Accept/Cancel (base optional widgets):
    if (ApplyButton)  BindButton(1, ApplyButton);
    if (CancelButton) BindButton(2, CancelButton);

    // Tabs:
    if (BtnSit) BindButton(301, BtnSit);
    if (BtnPkg) BindButton(302, BtnPkg);
    if (BtnMap) BindButton(303, BtnMap);

    // Fields:
    if (TxtName)   BindEdit(201, TxtName);
    if (CmbType)   BindCombo(202, CmbType);
    if (CmbSystem) BindCombo(203, CmbSystem);
    if (CmbRegion) BindCombo(204, CmbRegion);
}

FString UMissionEditorNavDlg::GetLegacyFormText() const
{
    // Optional: keep FRM text for debugging/consistency.
    // Not required at runtime.
    return FString();
}

void UMissionEditorNavDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindFormWidgets();
    RegisterControls();
    PopulateTypeOptionsIfEmpty();
}

void UMissionEditorNavDlg::RegisterControls()
{
    // Accept/Cancel:
    if (UButton* Btn = GetButton(1))
    {
        Btn->OnClicked.RemoveAll(this);
        Btn->OnClicked.AddDynamic(this, &UMissionEditorNavDlg::OnCommitClicked);
    }

    if (UButton* Btn = GetButton(2))
    {
        Btn->OnClicked.RemoveAll(this);
        Btn->OnClicked.AddDynamic(this, &UMissionEditorNavDlg::OnCancelClicked);
    }

    // Tabs:
    if (UButton* Btn = GetButton(301))
    {
        Btn->OnClicked.RemoveAll(this);
        Btn->OnClicked.AddDynamic(this, &UMissionEditorNavDlg::OnTabClicked_Sit);
    }

    if (UButton* Btn = GetButton(302))
    {
        Btn->OnClicked.RemoveAll(this);
        Btn->OnClicked.AddDynamic(this, &UMissionEditorNavDlg::OnTabClicked_Pkg);
    }

    if (UButton* Btn = GetButton(303))
    {
        Btn->OnClicked.RemoveAll(this);
        Btn->OnClicked.AddDynamic(this, &UMissionEditorNavDlg::OnTabClicked_Map);
    }

    // System selection:
    if (UComboBoxString* Cmb = GetCombo(203))
    {
        Cmb->OnSelectionChanged.RemoveAll(this);
        Cmb->OnSelectionChanged.AddDynamic(this, &UMissionEditorNavDlg::OnSystemSelect);
    }
}

void UMissionEditorNavDlg::Show()
{
    const bool bNeedTabUpdate = (GetVisibility() != ESlateVisibility::Visible);

    SetVisibility(ESlateVisibility::Visible);
    bLocalExitLatch = true;

    // Clear fields:
    if (UEditableTextBox* NameEdit = GetEdit(201))
    {
        NameEdit->SetText(FText::GetEmpty());
    }

    PopulateTypeOptionsIfEmpty();
    PopulateSystems();

    // Apply mission values:
    if (!mission)
    {
        if (bNeedTabUpdate)
            ShowTab(2);
        return;
    }

    // Name:
    if (UEditableTextBox* NameEdit = GetEdit(201))
    {
        NameEdit->SetText(
            FText::FromString(UTF8_TO_TCHAR(mission->GetName()))
        );
    }

    // Type:
    if (UComboBoxString* TypeCmb = GetCombo(202))
    {
        const int32 TypeIndex = mission->GetType();
        if (TypeIndex >= 0 && TypeIndex < TypeCmb->GetOptionCount())
        {
            TypeCmb->SetSelectedIndex(TypeIndex);
        }
    }

    // System + Regions:
    StarSystem* Sys = mission->GetStarSystem();
    if (Sys)
    {
        const FString SysName = FString(Sys->Name());

        if (UComboBoxString* SysCmb = GetCombo(203))
        {
            SysCmb->SetSelectedOption(SysName);
        }

        PopulateRegionsForSystem(Sys);

        if (UComboBoxString* RgnCmb = GetCombo(204))
        {
            const FString MissionRegion = FString(mission->GetRegion());
            RgnCmb->SetSelectedOption(MissionRegion);
        }
    }
    else
    {
        PopulateRegionsForSystem(nullptr);
    }

    if (bNeedTabUpdate)
        ShowTab(2);
}

void UMissionEditorNavDlg::PopulateTypeOptionsIfEmpty()
{
    UComboBoxString* TypeCmb = GetCombo(202);
    if (!TypeCmb)
        return;

    if (TypeCmb->GetOptionCount() > 0)
        return;

    // Order must match legacy mission->Type() index values:
    static const TCHAR* Types[] =
    {
        TEXT("Patrol"),
        TEXT("Sweep"),
        TEXT("Intercept"),
        TEXT("Airborne Patrol"),
        TEXT("Airborne Sweep"),
        TEXT("Airborne Intercept"),
        TEXT("Strike"),
        TEXT("Assault"),
        TEXT("Defend"),
        TEXT("Escort"),
        TEXT("Freight Escort"),
        TEXT("Shuttle Escort"),
        TEXT("Strike Escort"),
        TEXT("Intel"),
        TEXT("Scout"),
        TEXT("Recon"),
        TEXT("Blockade"),
        TEXT("Fleet"),
        TEXT("Attack"),
        TEXT("Flight Ops"),
        TEXT("Transport"),
        TEXT("Cargo"),
        TEXT("Training"),
        TEXT("Misc"),
    };

    TypeCmb->ClearOptions();
    for (const TCHAR* S : Types)
        TypeCmb->AddOption(S);

    TypeCmb->SetSelectedIndex(0);
}

void UMissionEditorNavDlg::PopulateSystems()
{
    UComboBoxString* SysCmb = GetCombo(203);
    if (!SysCmb)
        return;

    SysCmb->ClearOptions();

    Galaxy* galaxy = Galaxy::GetInstance();
    if (!galaxy)
        return;

    ListIter<StarSystem> iter = galaxy->GetSystemList();
    while (++iter)
    {
        StarSystem* s = iter.value();
        if (s)
            SysCmb->AddOption(FString(s->Name()));
    }

    if (SysCmb->GetOptionCount() > 0 && SysCmb->GetSelectedOption().IsEmpty())
        SysCmb->SetSelectedIndex(0);
}

StarSystem* UMissionEditorNavDlg::FindSystemByName(const FString& Name) const
{
    Galaxy* galaxy = Galaxy::GetInstance();
    if (!galaxy)
        return nullptr;

    ListIter<StarSystem> iter = galaxy->GetSystemList();
    while (++iter)
    {
        StarSystem* s = iter.value();
        if (s && Name == FString(s->Name()))
            return s;
    }

    return nullptr;
}

void UMissionEditorNavDlg::PopulateRegionsForSystem(StarSystem* Sys)
{
    UComboBoxString* RgnCmb = GetCombo(204);
    if (!RgnCmb)
        return;

    RgnCmb->ClearOptions();

    if (!Sys)
        return;

    List<OrbitalRegion> regions;
    regions.append(Sys->AllRegions());
    regions.sort();

    ListIter<OrbitalRegion> iter = regions;
    while (++iter)
    {
        OrbitalRegion* r = iter.value();
        if (r)
            RgnCmb->AddOption(FString(r->Name()));
    }

    if (RgnCmb->GetOptionCount() > 0)
        RgnCmb->SetSelectedIndex(0);
}

void UMissionEditorNavDlg::ScrapeForm()
{
    if (!mission)
        return;

    // Name:
    if (UEditableTextBox* NameEdit = GetEdit(201))
    {
        const FString NewName = NameEdit->GetText().ToString();
        mission->SetName(TCHAR_TO_ANSI(*NewName));

        if (mission_info)
            mission_info->name = TCHAR_TO_ANSI(*NewName);
    }

    // Type:
    if (UComboBoxString* TypeCmb = GetCombo(202))
    {
        const int32 TypeIndex = TypeCmb->GetSelectedIndex();
        if (TypeIndex >= 0)
        {
            mission->SetType(TypeIndex);
            if (mission_info)
                mission_info->type = TypeIndex;
        }
    }

    // System:
    StarSystem* system = nullptr;
    if (UComboBoxString* SysCmb = GetCombo(203))
    {
        const FString SysName = SysCmb->GetSelectedOption();
        system = FindSystemByName(SysName);
    }

    if (system)
    {
        mission->ClearSystemList();
        mission->SetStarSystem(system);

        if (mission_info)
            mission_info->system = system->Name();
    }

    // Region:
    if (UComboBoxString* RgnCmb = GetCombo(204))
    {
        const FString RegionName = RgnCmb->GetSelectedOption();
        mission->SetRegion(TCHAR_TO_ANSI(*RegionName));

        if (mission_info)
            mission_info->region = TCHAR_TO_ANSI(*RegionName);
    }
}

void UMissionEditorNavDlg::ShowTab(int32 TabIndex)
{
    if (TabIndex < 0 || TabIndex > 2)
        TabIndex = 0;

    // Legacy behavior: if leaving nav tab, show the main mission editor dlg tab:
    if (TabIndex != 2 && Manager)
    {
        UMissionEditorDlg* Dlg = Manager->GetMsnEditDlg();
        if (Dlg)
            Dlg->ShowTab(TabIndex);

        Manager->ShowMissionEditorDlg();
    }
}

void UMissionEditorNavDlg::HandleAccept()
{
    Super::HandleAccept();
    OnCommitClicked();
}

void UMissionEditorNavDlg::HandleCancel()
{
    // Match the legacy exit latch behavior:
    if (bLocalExitLatch)
    {
        bLocalExitLatch = false;
        return;
    }

    Super::HandleCancel();
    OnCancelClicked();
}

// ------------------------------------------------------------
// Handlers
// ------------------------------------------------------------

void UMissionEditorNavDlg::OnSystemSelect(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
    StarSystem* Sys = FindSystemByName(SelectedItem);
    PopulateRegionsForSystem(Sys);
    ScrapeForm();
}

void UMissionEditorNavDlg::OnCommitClicked()
{
    if (mission)
    {
        ScrapeForm();

        if (mission_info)
            mission_info->name = mission->GetName();

        mission->Save();
    }

    if (Manager)
        Manager->ShowMissionSelectDlg();
}

void UMissionEditorNavDlg::OnCancelClicked()
{
    if (mission)
        mission->Load();

    if (Manager)
        Manager->ShowMissionSelectDlg();
}

void UMissionEditorNavDlg::OnTabClicked_Sit()
{
    ShowTab(0);
}

void UMissionEditorNavDlg::OnTabClicked_Pkg()
{
    ShowTab(1);
}

void UMissionEditorNavDlg::OnTabClicked_Map()
{
    ShowTab(2);
}
