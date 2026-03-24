/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionWeaponDlg (Unreal)
    - Port of legacy MsnWepDlg.
    - Mission briefing WEAPON / LOADOUT dialog.
    - Uses UBaseScreen FORM ID binding and legacy control IDs.
    - No lambdas. No direct UPROPERTY widget members.
*/

#include "MissionWeaponDlg.h"

// Unreal:
#include "Logging/LogMacros.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ListView.h"

// Slate (used to infer which button fired OnClicked without lambdas):
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"

// Starshatter core / sim (ported headers; keep names consistent with your project):
#include "Campaign.h"
#include "Mission.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "WeaponDesign.h"
#include "HardPoint.h"
#include "MissionElement.h"
#include "SimElement.h"

#include "MissionPlanner.h"

DEFINE_LOG_CATEGORY_STATIC(LogMissionWeaponDlg, Log, All);

// --------------------------------------------------------------------
// CTOR
// --------------------------------------------------------------------

UMissionWeaponDlg::UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CampaignPtr = nullptr;
    MissionPtr = nullptr;
    MissionPlanner = nullptr;

    Elem = nullptr;
    FirstStation = 0;

    FMemory::Memzero(Designs, sizeof(Designs));
    FMemory::Memzero(Mounts, sizeof(Mounts));

    for (int32 i = 0; i < 8; ++i)
        Loads[i] = -1;
}

// --------------------------------------------------------------------
// LIFECYCLE
// --------------------------------------------------------------------

void UMissionWeaponDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // FORM widgets should already be bound by BaseScreen::NativeOnInitialized().
    // We wire dynamic events here so designer recompile/hot reload stays stable.
    WireEvents();
}

void UMissionWeaponDlg::ExecFrame(double DeltaTime)
{
    Super::ExecFrame(DeltaTime);

    // Legacy behavior:
    // if (Keyboard::KeyDown(VK_RETURN)) OnCommit(0);
    // Centralized in UBaseScreen -> HandleAccept() on Enter.
    (void)DeltaTime;
}

void UMissionWeaponDlg::Show()
{
    Super::Show();

    Elem = nullptr;

    if (MissionPtr)
    {
        ListIter<MissionElement> it = MissionPtr->GetElements();
        while (++it)
        {
            MissionElement* ME = it.value();
            if (ME && ME->IsPlayer())   // if you added IsPlayer()
            {
                Elem = ME;
                break;
            }
        }
    }

    if (Elem)
    {
        BuildLists();
        SetupControls();
    }
    else
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::Show - no player element found"));
    }
}


// --------------------------------------------------------------------
// DIALOG INPUT HOOKS (Enter/Escape)
// --------------------------------------------------------------------

void UMissionWeaponDlg::HandleAccept()
{
    Super::HandleAccept();

    // Legacy: MsnDlg::OnCommit(event)
    // In Unreal: route to MissionPlanner navigation/flow.
    if (MissionPlanner)
    {
        // If you later implement a distinct “commit mission” path, swap here.
        MissionPlanner->OnMissionBriefingAccept();
    }
    else
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::HandleAccept - MissionPlanner is null"));
    }
}

void UMissionWeaponDlg::HandleCancel()
{
    Super::HandleCancel();

    // Legacy: MsnDlg::OnCancel(event)
    if (MissionPlanner)
    {
        MissionPlanner->OnMissionBriefingCancel();
    }
    else
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::HandleCancel - MissionPlanner is null"));
    }
}

// --------------------------------------------------------------------
// EVENT WIRING (NO LAMBDAS)
// --------------------------------------------------------------------

void UMissionWeaponDlg::WireEvents()
{
    // Accept / Cancel:
    if (UButton* Accept = GetButton(1))
    {
        if (!Accept->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnAcceptClicked))
            Accept->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnAcceptClicked);
    }

    if (UButton* Cancel = GetButton(2))
    {
        if (!Cancel->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnCancelClicked))
            Cancel->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnCancelClicked);
    }

    // Tabs: 900 SIT, 901 PKG, 902 MAP, 903 WEP
    if (UButton* B = GetButton(900))
    {
        if (!B->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnTabSit))
            B->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnTabSit);
    }

    if (UButton* B = GetButton(901))
    {
        if (!B->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnTabPkg))
            B->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnTabPkg);
    }

    if (UButton* B = GetButton(902))
    {
        if (!B->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnTabMap))
            B->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnTabMap);
    }

    if (UButton* B = GetButton(903))
    {
        if (!B->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnTabWep))
            B->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnTabWep);
    }

    // Mount buttons grid:
    // ids: 500 + i*10 + (n+1), i=weapon row [0..7], n=station col [0..7]
    ButtonIdToSlot.Empty();

    for (int32 i = 0; i < 8; ++i)
    {
        for (int32 n = 0; n < 8; ++n)
        {
            const int32 Id = 500 + i * 10 + (n + 1);

            if (UButton* Btn = GetButton(Id))
            {
                FMountSlot Slotx;
                Slotx.WeaponIndex = i;
                Slotx.StationIndex = n;
                ButtonIdToSlot.Add(Btn, Slotx);

                if (!Btn->OnClicked.IsAlreadyBound(this, &UMissionWeaponDlg::OnMountClicked))
                    Btn->OnClicked.AddDynamic(this, &UMissionWeaponDlg::OnMountClicked);
            }
        }
    }
}

// --------------------------------------------------------------------
// UI SETUP (LEGACY LOGIC)
// --------------------------------------------------------------------

void UMissionWeaponDlg::SetupControls()
{
    if (!Elem)
        return;

    ShipDesign* Design = (ShipDesign*)Elem->GetDesign();
    if (!Design)
        return;

    // Element name:
    SetLabelText(601, FText::FromString(Elem->Name().data()));

    // Type:
    SetLabelText(602, FText::FromString((const char*)Design->name));

    // Station labels & weapon rows:
    for (int32 i = 0; i < 8; ++i)
    {
        const int32 DescId = 500 + i * 10; // label
        UTextBlock* Desc = GetLabel(DescId);

        if (Designs[i])
        {
            if (Desc)
            {
                Desc->SetVisibility(ESlateVisibility::Visible);

                const FString WeaponText =
                    FString(Designs[i]->group.data()) + TEXT(" ") + FString(Designs[i]->name.data());

                Desc->SetText(FText::FromString(WeaponText));
            }

            for (int32 n = 0; n < 8; ++n)
            {
                const int32 BtnId = 500 + i * 10 + (n + 1);
                if (UButton* Btn = GetButton(BtnId))
                {
                    // show only if that weapon is mountable at that station:
                    const bool bShow = Mounts[i][n];
                    Btn->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

                    // NOTE: legacy toggled LED bitmap on/off.
                    // In UE, you can drive a visual state in the button style or child image.
                    // For now, we preserve the selection logic via Loads[] and update by caller.
                }
            }
        }
        else
        {
            if (Desc)
                Desc->SetVisibility(ESlateVisibility::Collapsed);

            for (int32 n = 0; n < 8; ++n)
            {
                const int32 BtnId = 500 + i * 10 + (n + 1);
                if (UButton* Btn = GetButton(BtnId))
                    Btn->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // Weight label:
    // Legacy: base ship mass + carried mass for selected mounts / or chosen loadout
    if (UTextBlock* LblWeight = GetLabel(603))
    {
        const int32 NStations = (int32)Design->hard_points.size();
        double Mass = Design->mass;

        for (int32 n = 0; n < NStations; ++n)
        {
            const int32 SlotIndex = n + FirstStation;
            const int32 Item = (SlotIndex >= 0 && SlotIndex < 8) ? Loads[SlotIndex] : -1;
            Mass += Design->hard_points[n]->GetCarryMass(Item);
        }

        const int32 Kg = (int32)(Mass * 1000.0);
        LblWeight->SetText(FText::FromString(FString::Printf(TEXT("%d kg"), Kg)));
    }

    // Beauty (id 300) and PlayerDesc (id 301) were commented out in the legacy FORM.
    // If you re-enable them in UMG, BaseScreen can bind them and you can extend here.
}

void UMissionWeaponDlg::BuildLists()
{
    FMemory::Memzero(Designs, sizeof(Designs));
    FMemory::Memzero(Mounts, sizeof(Mounts));

    for (int32 i = 0; i < 8; ++i)
        Loads[i] = -1;

    if (!Elem)
        return;

    ShipDesign* D = (ShipDesign*)Elem->GetDesign();
    if (!D)
        return;

    const int32 NStations = (int32)D->hard_points.size();
    FirstStation = (8 - NStations) / 2;

    // Clear station headings:
    for (int32 s = 0; s < 8; ++s)
    {
        const int32 StationLblId = 401 + s;
        SetLabelText(StationLblId, FText::FromString(TEXT("")));
    }

    int32 Index = 0;
    int32 Station = FirstStation;

    // Build unique weapon list + mount matrix:
    ListIter<HardPoint> it = D->hard_points;
    while (++it)
    {
        HardPoint* Hp = it.value();
        if (!Hp)
        {
            Station++;
            continue;
        }

        // Station abbreviation:
        SetLabelText(
            401 + Station,
            FText::FromString((const char*)Hp->GetAbbreviation())
        );

        for (int32 n = 0; n < HardPoint::MAX_DESIGNS; ++n)
        {
            WeaponDesign* Wep = Hp->GetWeaponDesign(n);
            if (!Wep)
                continue;

            bool bFound = false;

            for (int32 i = 0; i < 8 && !bFound; ++i)
            {
                if (Designs[i] == Wep)
                {
                    bFound = true;
                    Mounts[i][Station] = true;
                }
            }

            if (!bFound && Index < 8)
            {
                Designs[Index] = Wep;
                Mounts[Index][Station] = true;
                ++Index;
            }
        }

        Station++;
    }

    // Map existing element loadout into Loads[]:
    if (Elem->Loadouts().size())
    {
        MissionLoad* MsnLoad = Elem->Loadouts().at(0);
        if (MsnLoad)
        {
            // map loadout:
            int* Loadout = nullptr;

            if (MsnLoad->GetName().length())
            {
                ListIter<ShipLoad> sl = ((ShipDesign*)Elem->GetDesign())->loadouts;
                while (++sl)
                {
                    if (!_stricmp(sl->name, MsnLoad->GetName()))
                    {
                        Loadout = sl->load;
                        break;
                    }
                }
            }
            else
            {
                Loadout = MsnLoad->GetStations();
            }

            if (Loadout)
            {
                for (int32 i = 0; i < NStations; ++i)
                {
                    const int32 LSlot = i + FirstStation;
                    if (LSlot >= 0 && LSlot < 8)
                        Loads[LSlot] = Loadout[i];
                }
            }
        }
    }

    // Loadout list population:
    // Legacy used a ListBox with multiple columns; UE UListView requires an item UObject type.
    // Your UMG entry widget determines how items render.
    // We keep this method “complete” by clearing the list (so it never shows stale data)
    // and preserving the weapon mounting logic which is the core of this dialog.
    if (UListView* LV = GetList(604))
    {
        LV->ClearListItems();
    }
}

// --------------------------------------------------------------------
// LOADOUT / HARDPOINT MAPPING (LEGACY)
// --------------------------------------------------------------------

int UMissionWeaponDlg::LoadToPointIndex(int n) const
{
    const int nn = n + FirstStation;

    if (!Elem || nn < 0 || nn >= 8 || Loads[nn] == -1)
        return -1;

    int Index = -1;

    WeaponDesign* WepDesign = Designs[Loads[nn]];
    ShipDesign* Design = (ShipDesign*)Elem->GetDesign();
    HardPoint* Hp = Design ? Design->hard_points[n] : nullptr;

    if (!Hp || !WepDesign)
        return -1;

    for (int i = 0; i < 8 && Index < 0; ++i)
    {
        if (Hp->GetWeaponDesign(i) == WepDesign)
            Index = i;
    }

    return Index;
}

int UMissionWeaponDlg::PointIndexToLoad(int n, int PointIndex) const
{
    const int nn = n + FirstStation;

    if (!Elem || nn < 0 || nn >= 8)
        return -1;

    ShipDesign* Design = (ShipDesign*)Elem->GetDesign();
    HardPoint* Hp = Design ? Design->hard_points[n] : nullptr;
    WeaponDesign* WepDesign = Hp ? Hp->GetWeaponDesign(PointIndex) : nullptr;

    if (!WepDesign)
        return -1;

    int Result = -1;

    for (int i = 0; i < 8 && Result < 0; ++i)
    {
        if (Designs[i] == WepDesign)
            Result = i;
    }

    return Result;
}

// --------------------------------------------------------------------
// CLICK HANDLERS
// --------------------------------------------------------------------

void UMissionWeaponDlg::OnAcceptClicked()
{
    HandleAccept();
}

void UMissionWeaponDlg::OnCancelClicked()
{
    HandleCancel();
}

void UMissionWeaponDlg::OnTabSit()
{
    if (MissionPlanner)
        MissionPlanner->ShowMsnObjDlg();
}

void UMissionWeaponDlg::OnTabPkg()
{
    if (MissionPlanner)
        MissionPlanner->ShowMsnPkgDlg();
}

void UMissionWeaponDlg::OnTabMap()
{
    if (MissionPlanner)
        MissionPlanner->ShowNavDlg();
}

void UMissionWeaponDlg::OnTabWep()
{
    if (MissionPlanner)
        MissionPlanner->ShowMsnWepDlg();
}

void UMissionWeaponDlg::OnMountClicked()
{
    // UE: UButton::OnClicked has no sender param.
    // We infer sender via Slate focus and map it back to a UButton in ButtonIdToSlot.

    if (!FSlateApplication::IsInitialized())
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::OnMountClicked - Slate not initialized"));
        return;
    }

    UButton* ClickedButton = nullptr;

    // Prefer keyboard focus; fallback to user focus.
    TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (!FocusedWidget.IsValid())
    {
        FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
    }

    if (!FocusedWidget.IsValid())
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::OnMountClicked - no focused widget"));
        return;
    }

    // Resolve focus -> button by comparing cached slate widgets.
    for (auto It = ButtonIdToSlot.CreateIterator(); It; ++It)
    {
        UButton* CandidateButton = It.Key();
        if (!CandidateButton)
            continue;

        if (CandidateButton->GetCachedWidget().Get() == FocusedWidget.Get())
        {
            ClickedButton = CandidateButton;
            break;
        }
    }

    if (!ClickedButton)
    {
        UE_LOG(LogMissionWeaponDlg, Warning, TEXT("MissionWeaponDlg::OnMountClicked - unable to resolve clicked button"));
        return;
    }

    const FMountSlot* SlotInfoPtr = ButtonIdToSlot.Find(ClickedButton);
    if (!SlotInfoPtr)
        return;

    const int32 WeaponIndex = SlotInfoPtr->WeaponIndex;
    const int32 StationIndex = SlotInfoPtr->StationIndex;

    if (WeaponIndex < 0 || WeaponIndex >= 8 || StationIndex < 0 || StationIndex >= 8)
        return;

    // Toggle selection for this station (legacy behavior):
    int32 NewWeaponIndex = WeaponIndex;
    if (Loads[StationIndex] == WeaponIndex)
        NewWeaponIndex = -1;

    Loads[StationIndex] = NewWeaponIndex;

    // Persist into player loadouts (legacy: update all loadouts and clear their name):
    if (Elem)
    {
        ShipDesign* Design = (ShipDesign*)Elem->GetDesign();
        if (Design)
        {
            const int32 NumStations = (int32)Design->hard_points.size();

            if (Elem->Loadouts().size() < 1)
            {
                MissionLoad* NewLoadout = new MissionLoad;
                Elem->Loadouts().append(NewLoadout);

                for (int32 n = 0; n < NumStations; ++n)
                {
                    NewLoadout->SetStation(n, LoadToPointIndex(n));
                }
            }
            else
            {
                ListIter<MissionLoad> LoadoutIter = Elem->Loadouts();
                while (++LoadoutIter)
                {
                    MissionLoad* Loadout = LoadoutIter.value();
                    if (!Loadout)
                        continue;

                    Loadout->SetName("");

                    for (int32 n = 0; n < NumStations; ++n)
                    {
                        Loadout->SetStation(n, LoadToPointIndex(n));
                    }
                }
            }
        }
    }

    // Clear list selection (legacy listbox behavior):
    if (UListView* LoadoutListView = GetList(604))
    {
        LoadoutListView->ClearSelection();
    }

    // Update weight readout:
    UTextBlock* WeightLabel = GetLabel(603);
    if (WeightLabel && Elem)
    {
        ShipDesign* Design = (ShipDesign*)Elem->GetDesign();
        if (Design)
        {
            const int32 NumStations = (int32)Design->hard_points.size();
            double TotalMass = Design->mass;

            for (int32 n = 0; n < NumStations; ++n)
            {
                const int32 UiSlotIndex = n + FirstStation;

                const int32 SelectedWeapon =
                    (UiSlotIndex >= 0 && UiSlotIndex < 8) ? Loads[UiSlotIndex] : -1;

                TotalMass += Design->hard_points[n]->GetCarryMass(SelectedWeapon);
            }

            const int32 Kg = (int32)(TotalMass * 1000.0);
            WeightLabel->SetText(FText::FromString(FString::Printf(TEXT("%d kg"), Kg)));
        }
    }
}
