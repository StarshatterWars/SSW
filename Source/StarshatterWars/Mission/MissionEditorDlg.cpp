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
#include "MissionElement.h"
#include "MissionDebriefDlg.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "UObject/UnrealType.h"

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
    SelectedElement = SelectedEvent = INDEX_NONE;
    OriginalMission = mission ? ANSI_TO_TCHAR(mission->Serialize(nullptr, 0, false).data()) : FString();
    OriginalTarget=mission && mission->GetTarget()?ANSI_TO_TCHAR(mission->GetTarget()->GetName()):FString();
    OriginalWard=mission && mission->GetWard()?ANSI_TO_TCHAR(mission->GetWard()->GetName()):FString();
    bEditing = mission != nullptr;
    current_tab = 0;
}

void UMissionEditorDlg::SetMissionInfo(MissionInfo* InMissionInfo)
{
    mission_info = InMissionInfo;
    if (mission_info)
    {
        OriginalName = ANSI_TO_TCHAR(mission_info->name.data());
        OriginalDescription = ANSI_TO_TCHAR(mission_info->description.data());
        OriginalSystem = ANSI_TO_TCHAR(mission_info->system.data());
        OriginalRegion = ANSI_TO_TCHAR(mission_info->region.data());
        OriginalType = mission_info->type;
    }
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

    auto BindPackageList = [this](UListView*& List, const TCHAR* Name)
    {
        if (!List && WidgetTree)
        {
            if (UVerticalBox* Box = EnsureAutoVerticalBox())
            {
                auto* NativeList = WidgetTree->ConstructWidget<UDebriefListView>(UDebriefListView::StaticClass(), Name);
                NativeList->SetEntry(UDebriefRowWidget::StaticClass());
                Box->AddChildToVerticalBox(NativeList);
                List = NativeList;
            }
        }
        if (List)
        {
            // FORM and BP list widgets share the native row data contract.
            if (FClassProperty* Entry = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
                Entry->SetObjectPropertyValue_InContainer(List, UDebriefRowWidget::StaticClass());
            List->SetSelectionMode(ESelectionMode::Single);
        }
    };
    BindPackageList(LstElem, TEXT("MissionElements"));
    BindPackageList(LstEvent, TEXT("MissionEvents"));
    if (LstElem)
    {
        LstElem->OnItemSelectionChanged().RemoveAll(this);
        LstElem->OnItemSelectionChanged().AddWeakLambda(this, [this](UObject* Item){ ElementSelected(Item, Item != nullptr); });
        LstElem->OnItemDoubleClicked().RemoveAll(this);
        LstElem->OnItemDoubleClicked().AddUObject(this, &UMissionEditorDlg::ElementDoubleClicked);
    }
    if (LstEvent)
    {
        LstEvent->OnItemSelectionChanged().RemoveAll(this);
        LstEvent->OnItemSelectionChanged().AddWeakLambda(this, [this](UObject* Item){ EventSelected(Item, Item != nullptr); });
        LstEvent->OnItemDoubleClicked().RemoveAll(this);
        LstEvent->OnItemDoubleClicked().AddUObject(this, &UMissionEditorDlg::EventDoubleClicked);
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

    bPopulating = true;
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
            CmbType->SetSelectedIndex(mission->GetMissionType());
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
            regions.append(sys->GetAllRegions());
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

    bPopulating = false;
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
        mission->SetMissionType(NewType);

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
        Manager->EditMissionMap(mission, mission_info);
    }
}

void UMissionEditorDlg::DrawPackages()
{
    bPopulating = true;
    if (LstElem) LstElem->ClearListItems();
    if (LstEvent) LstEvent->ClearListItems();
    if (mission)
    {
        auto& Elements = mission->GetElements();
        auto& Events = mission->GetEvents();
        SelectedElement = Elements.size() ? FMath::Clamp(SelectedElement, 0, Elements.size()-1) : INDEX_NONE;
        SelectedEvent = Events.size() ? FMath::Clamp(SelectedEvent, 0, Events.size()-1) : INDEX_NONE;
        if (LstElem) for (int32 I=0; I<Elements.size(); ++I)
        {
            auto* E = Elements[I];
            if (E) LstElem->AddItem(UDebriefListItem::Make(this, ANSI_TO_TCHAR(E->GetName().data()),
                FString::Printf(TEXT("x%d"), E->Count()), FString(), FString(), I));
        }
        if (LstEvent) for (int32 I=0; I<Events.size(); ++I)
        {
            auto* E = Events[I];
            if (E) LstEvent->AddItem(UDebriefListItem::Make(this, FString::FromInt(E->EventID()),
                ANSI_TO_TCHAR(E->EventName()), FString(), FString(), I));
        }
        if (LstElem && SelectedElement != INDEX_NONE) LstElem->SetSelectedIndex(SelectedElement);
        if (LstEvent && SelectedEvent != INDEX_NONE) LstEvent->SetSelectedIndex(SelectedEvent);
    }
    bPopulating = false;
    UpdateSelection();
}

void UMissionEditorDlg::UpdateSelection()
{
    const int32 NE = mission ? mission->GetElements().size() : 0;
    const int32 NV = mission ? mission->GetEvents().size() : 0;
    const bool E = SelectedElement >= 0 && SelectedElement < NE;
    const bool V = SelectedEvent >= 0 && SelectedEvent < NV;
    if (BtnElemDel) BtnElemDel->SetIsEnabled(E);
    if (BtnElemEdit) BtnElemEdit->SetIsEnabled(E);
    if (BtnElemInc) BtnElemInc->SetIsEnabled(E && SelectedElement > 0);
    if (BtnElemDec) BtnElemDec->SetIsEnabled(E && SelectedElement < NE-1);
    if (BtnEventDel) BtnEventDel->SetIsEnabled(V);
    if (BtnEventEdit) BtnEventEdit->SetIsEnabled(V);
    if (BtnEventInc) BtnEventInc->SetIsEnabled(V && SelectedEvent > 0);
    if (BtnEventDec) BtnEventDec->SetIsEnabled(V && SelectedEvent < NV-1);
}

void UMissionEditorDlg::ElementSelected(UObject* Item, bool bSelected)
{
    if (bPopulating) return;
    auto* Row = Cast<UDebriefListItem>(Item);
    SelectedElement = bSelected && Row ? Row->DataIndex : INDEX_NONE;
    UpdateSelection();
}
void UMissionEditorDlg::EventSelected(UObject* Item, bool bSelected)
{
    if (bPopulating) return;
    auto* Row = Cast<UDebriefListItem>(Item);
    SelectedEvent = bSelected && Row ? Row->DataIndex : INDEX_NONE;
    UpdateSelection();
}
void UMissionEditorDlg::ElementDoubleClicked(UObject* Item) { ElementSelected(Item, true); OnElemEditClicked(); }
void UMissionEditorDlg::EventDoubleClicked(UObject* Item) { EventSelected(Item, true); OnEventEditClicked(); }

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
    if (bPopulating) return;
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
        regions.append(sys->GetAllRegions());
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

void UMissionEditorDlg::OnAcceptClicked() { CommitEditing(); }
void UMissionEditorDlg::OnCancelClicked() { CancelEditing(); }

void UMissionEditorDlg::CommitEditing(bool bScrape)
{
    if (mission)
    {
        if (bScrape) ScrapeForm();
        if (!mission->Save()) return;
        if (mission_info) mission_info->name = mission->GetName();
    }
    bEditing = false;
    if (Manager) Manager->ShowMissionSelectDlg();
}

void UMissionEditorDlg::CancelEditing()
{
    if (mission && bEditing && !OriginalMission.IsEmpty())
    {
        mission->SetTarget(nullptr);
        mission->SetWard(nullptr);
        mission->GetElements().destroy();
        mission->GetEvents().destroy();
        mission->ClearSystemList();
        mission->ParseMission(TCHAR_TO_ANSI(*OriginalMission));
        mission->SetTarget(OriginalTarget.IsEmpty()?nullptr:mission->FindElement(TCHAR_TO_ANSI(*OriginalTarget)));
        mission->SetWard(OriginalWard.IsEmpty()?nullptr:mission->FindElement(TCHAR_TO_ANSI(*OriginalWard)));
        if (mission_info)
        {
            mission_info->name = TCHAR_TO_ANSI(*OriginalName);
            mission_info->description = TCHAR_TO_ANSI(*OriginalDescription);
            mission_info->system = TCHAR_TO_ANSI(*OriginalSystem);
            mission_info->region = TCHAR_TO_ANSI(*OriginalRegion);
            mission_info->type = OriginalType;
        }
    }
    bEditing = false;
    DrawPackages();
    if (Manager) Manager->ShowMissionSelectDlg();
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
    SelectedElement = elements.size()-1;

    DrawPackages();

    if (Manager)
    {
        OnElemEditClicked();
    }
}

void UMissionEditorDlg::OnElemDelClicked()
{
    if (!mission || SelectedElement<0 || SelectedElement>=mission->GetElements().size()) return;
    delete mission->GetElements().removeIndex(SelectedElement);
    DrawPackages();
}

void UMissionEditorDlg::OnElemEditClicked()
{
    if (!mission || SelectedElement<0 || SelectedElement>=mission->GetElements().size()) return;
    if (Manager) { ScrapeForm();Manager->EditMissionElement(mission, mission->GetElements()[SelectedElement]); }
    DrawPackages();
}

void UMissionEditorDlg::OnElemIncClicked()
{
    if (!mission || SelectedElement<0 || SelectedElement>=mission->GetElements().size()) return;
    mission->IncreaseElemPriority(SelectedElement);
    SelectedElement = FMath::Max(0, SelectedElement-1);
    DrawPackages();
}

void UMissionEditorDlg::OnElemDecClicked()
{
    if (!mission || SelectedElement<0 || SelectedElement>=mission->GetElements().size()) return;
    mission->DecreaseElemPriority(SelectedElement);
    SelectedElement = FMath::Min(mission->GetElements().size()-1, SelectedElement+1);
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
    SelectedEvent = events.size()-1;

    DrawPackages();

    if (Manager)
    {
        OnEventEditClicked();
    }
}

void UMissionEditorDlg::OnEventDelClicked()
{
    if (!mission || SelectedEvent<0 || SelectedEvent>=mission->GetEvents().size()) return;
    delete mission->GetEvents().removeIndex(SelectedEvent);
    DrawPackages();
}

void UMissionEditorDlg::OnEventEditClicked()
{
    if (!mission || SelectedEvent<0 || SelectedEvent>=mission->GetEvents().size()) return;
    if (Manager) { ScrapeForm();Manager->EditMissionEvent(mission, mission->GetEvents()[SelectedEvent]); }
    DrawPackages();
}

void UMissionEditorDlg::OnEventIncClicked()
{
    if (!mission || SelectedEvent<0 || SelectedEvent>=mission->GetEvents().size()) return;
    mission->IncreaseEventPriority(SelectedEvent);
    SelectedEvent = FMath::Max(0, SelectedEvent-1);
    DrawPackages();
}

void UMissionEditorDlg::OnEventDecClicked()
{
    if (!mission || SelectedEvent<0 || SelectedEvent>=mission->GetEvents().size()) return;
    mission->DecreaseEventPriority(SelectedEvent);
    SelectedEvent = FMath::Min(mission->GetEvents().size()-1, SelectedEvent+1);
    DrawPackages();
}
