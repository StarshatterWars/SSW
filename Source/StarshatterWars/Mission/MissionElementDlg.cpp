/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionElementDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionElementDlg
    - Unreal UUserWidget replacement for legacy MsnElemDlg
    - Mirrors legacy UI behavior, but uses UMG widgets + bindings
    - Maintains use of legacy List<> / Text for sim data
*/

#include "MissionElementDlg.h"

#include "MenuScreen.h"

// UMG:
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"

#include "Engine/Engine.h"

// Legacy sim/editor includes:
#include "Mission.h"
#include "MissionElement.h"
//#include "MissionLoad.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "StarSystem.h"
#include "Galaxy.h"
#include "Instruction.h"
#include "Skin.h"
#include "ShipLoad.h"
#include "Game.h"
#include "ParseUtil.h"
#include "GameStructs.h"
#include "ShipDesignRegistry.h"
#include "MissionLoad.h"

// +--------------------------------------------------------------------+
// Local helpers (no Unreal containers; keep it simple)

static bool ParseIntBox(UEditableTextBox* Box, int32& OutVal)
{
    OutVal = 0;
    if (!Box) return false;

    const FString S = Box->GetText().ToString().TrimStartAndEnd();
    if (S.IsEmpty()) return false;

    const TCHAR C0 = S[0];
    if (!FChar::IsDigit(C0) && C0 != TEXT('-') && C0 != TEXT('+'))
        return false;

    OutVal = FCString::Atoi(*S);
    return true;
}

static bool ParseDoubleBox(UEditableTextBox* Box, double& OutVal)
{
    OutVal = 0.0;
    if (!Box) return false;

    const FString S = Box->GetText().ToString().TrimStartAndEnd();
    if (S.IsEmpty()) return false;

    const TCHAR C0 = S[0];
    if (!FChar::IsDigit(C0) && C0 != TEXT('-') && C0 != TEXT('+'))
        return false;

    OutVal = FCString::Atod(*S);
    return true;
}

// Converts legacy C-string / Text into Combo selection by string match:
static void SetComboSelectionByString(UComboBoxString* Combo, const FString& Wanted)
{
    if (!Combo) return;

    // UComboBoxString doesn’t expose item array directly, so:
    // We'll re-select by setting SelectedOption if it exists in the list.
    // If not present, keep current.
    Combo->SetSelectedOption(Wanted);
}

// +--------------------------------------------------------------------+

UMissionElementDlg::UMissionElementDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionElementDlg::InitializeDlg(UMenuScreen* InManager)
{
    Manager = InManager;
}

void UMissionElementDlg::SetMission(Mission* InMission)
{
    MissionPtr = InMission;
}

void UMissionElementDlg::SetMissionElement(MissionElement* InElem)
{
    ElemPtr = InElem;
}

void UMissionElementDlg::ShowDlg()
{
    // Mirror legacy Show(): populate UI from model
    RebuildFromModel();

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);

    // Optional focus:
    if (NameEdit)
    {
        NameEdit->SetKeyboardFocus();
    }
}

void UMissionElementDlg::BindFormWidgets()
{
    // Buttons:
    if (AcceptButton)
    {
        AcceptButton->OnClicked.Clear();
        AcceptButton->OnClicked.AddDynamic(this, &UMissionElementDlg::OnAcceptClicked);
    }

    if (CancelBtn)
    {
        CancelBtn->OnClicked.Clear();
        CancelBtn->OnClicked.AddDynamic(this, &UMissionElementDlg::OnCancelClicked);
    }

    // Combo events:
    if (ClassCombo)
    {
        ClassCombo->OnSelectionChanged.Clear();
        ClassCombo->OnSelectionChanged.AddDynamic(this, &UMissionElementDlg::OnClassChanged);
    }

    if (DesignCombo)
    {
        DesignCombo->OnSelectionChanged.Clear();
        DesignCombo->OnSelectionChanged.AddDynamic(this, &UMissionElementDlg::OnDesignChanged);
    }

    if (ObjectiveCombo)
    {
        ObjectiveCombo->OnSelectionChanged.Clear();
        ObjectiveCombo->OnSelectionChanged.AddDynamic(this, &UMissionElementDlg::OnObjectiveChanged);
    }

    // IFF commit:
    if (IFFEdit)
    {
        IFFEdit->OnTextCommitted.Clear();
        IFFEdit->OnTextCommitted.AddDynamic(this, &UMissionElementDlg::OnIFFCommitted);
    }
}

FString UMissionElementDlg::GetLegacyFormText() const
{
    return LegacyFormText;
}

void UMissionElementDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindFormWidgets();

    // Start hidden like a modal
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMissionElementDlg::RebuildFromModel()
{
    if (!ElemPtr)
        return;

    // ---------------------------
    // Class combo (legacy order)
    // ---------------------------
    if (ClassCombo)
    {
        ClassCombo->ClearOptions();

        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::DRONE)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::FIGHTER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::ATTACK)));

        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::LCA)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::COURIER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::CARGO)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::CORVETTE)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::FREIGHTER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::FRIGATE)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::DESTROYER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::CRUISER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::BATTLESHIP)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::CARRIER)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::SWACS)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::DREADNAUGHT)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::STATION)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::FARCASTER)));

        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::MINE)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::COMSAT)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::DEFSAT)));

        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::BUILDING)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::FACTORY)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::SAM)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::EWR)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::C3I)));
        ClassCombo->AddOption(ANSI_TO_TCHAR(Ship::GetShipClassName(CLASSIFICATION::STARBASE)));

        // Select class based on current design:
        const FShipDesign* Design = ElemPtr->GetShipDesign();
        if (Design)
        {
            const char* DesiredClassName = Ship::GetShipClassName(Design->ShipType);
            if (DesiredClassName)
                ClassCombo->SetSelectedOption(ANSI_TO_TCHAR(DesiredClassName));
        }
        else
        {
            // Default first option
            // (UMG will keep empty selection if not set)
            if (ClassCombo->GetOptionCount() > 0)
                ClassCombo->SetSelectedIndex(0);
        }
    }

    // Trigger dependent lists:
    RebuildDesignListFromClass();
    RebuildSkinAndLoadoutFromDesign();

    // Role combo:
    if (RoleCombo)
    {
        RoleCombo->ClearOptions();

        for (int i = (int)EMissionType::PATROL; i <= (int)EMissionType::OTHER; i++)
        {
            RoleCombo->AddOption(ANSI_TO_TCHAR(Mission::GetRoleName(i)));

            if (i == 0)
            {
                RoleCombo->SetSelectedIndex(0);
            }
            else if (ElemPtr->MissionRole() == i)
            {
                RoleCombo->SetSelectedOption(ANSI_TO_TCHAR(Mission::GetRoleName(i)));
            }
        }
    }

    // Region combo:
    if (RegionCombo)
    {
        RegionCombo->ClearOptions();

        if (MissionPtr)
        {
            StarSystem* Sys = MissionPtr->GetStarSystem();
            if (Sys)
            {
                List<OrbitalRegion> Regions;
                Regions.append(Sys->GetAllRegions());
                Regions.sort();

                ListIter<OrbitalRegion> Iter = Regions;
                while (++Iter)
                {
                    OrbitalRegion* R = Iter.value();
                    if (!R) continue;

                    RegionCombo->AddOption(ANSI_TO_TCHAR(R->GetName()));

                    if (!strcmp(ElemPtr->GetRegion(), R->GetName()))
                        RegionCombo->SetSelectedOption(ANSI_TO_TCHAR(R->GetName()));
                }
            }
        }
    }

    // Heading combo (legacy fixed options):
    if (HeadingCombo)
    {
        HeadingCombo->ClearOptions();
        HeadingCombo->AddOption(TEXT("North"));
        HeadingCombo->AddOption(TEXT("East"));
        HeadingCombo->AddOption(TEXT("South"));
        HeadingCombo->AddOption(TEXT("West"));

        double Heading = ElemPtr->GetHeading();
        while (Heading > 2 * PI) Heading -= 2 * PI;
        while (Heading < 0)      Heading += 2 * PI;

        if (Heading >= PI / 4 && Heading < 3 * PI / 4)       HeadingCombo->SetSelectedIndex(1);
        else if (Heading >= 3 * PI / 4 && Heading < 5 * PI / 4) HeadingCombo->SetSelectedIndex(2);
        else if (Heading >= 5 * PI / 4 && Heading < 7 * PI / 4) HeadingCombo->SetSelectedIndex(3);
        else                                                   HeadingCombo->SetSelectedIndex(0);
    }

    // Intel combo:
    if (IntelCombo)
    {
        IntelCombo->ClearOptions();
        for (int32 i = static_cast<int32>(EIntel::RESERVE);
            i <= static_cast<int32>(EIntel::TRACKED);
            ++i)
        {
            const EIntel IntelLevel =
                static_cast<EIntel>(i);

            const char* IntelName =
                Intel::GetNameFromIntel(IntelLevel);

            IntelCombo->AddOption(
                ANSI_TO_TCHAR(IntelName));

            if (i == static_cast<int32>(EIntel::RESERVE))
            {
                IntelCombo->SetSelectedIndex(0);
            }
            else if (static_cast<int32>(ElemPtr->GetIntelLevel()) == i)
            {
                IntelCombo->SetSelectedOption(
                    ANSI_TO_TCHAR(IntelName));
            }
        }
    }

    // Objective + Target:
    if (ObjectiveCombo)
    {
        ObjectiveCombo->ClearOptions();
        ObjectiveCombo->AddOption(TEXT(""));
        ObjectiveCombo->SetSelectedIndex(0);
        ObjectiveCombo->ClearOptions();

        Instruction* Instr = nullptr;
        if (ElemPtr->GetObjectives().size() > 0)
            Instr = ElemPtr->GetObjectives().at(0);

        const UEnum* Enum = StaticEnum<INSTRUCTION_ACTION>();
        if (!Enum || !ObjectiveCombo)
        {
            return;
        }


        for (int32 i = 0; i < 16; ++i)
        {
            const int64 Value = (int64)i;

            const FString Display =
                Enum->GetDisplayNameTextByValue(Value).ToString();

            ObjectiveCombo->AddOption(Display);

            if (Instr && (int64)Instr->GetAction() == Value)
            {
                ObjectiveCombo->SetSelectedOption(Display);
            }
        }
    }

    BuildObjectiveTargets();

    // Team combos:
    UpdateTeamInfo();

    // Text fields:
    if (NameEdit) NameEdit->SetText(FText::FromString(ANSI_TO_TCHAR(ElemPtr->GetName())));

    if (SizeEdit)      SizeEdit->SetText(FText::AsNumber(ElemPtr->Count()));
    if (IFFEdit)       IFFEdit->SetText(FText::AsNumber(ElemPtr->GetIFF()));

    // Legacy uses km in UI; internal is meters:
    const FVector Loc = ElemPtr->GetLocation();

    if (LocXEdit) LocXEdit->SetText(FText::AsNumber((int32)(Loc.X / 1000)));
    if (LocYEdit) LocYEdit->SetText(FText::AsNumber((int32)(Loc.Y / 1000)));
    if (LocZEdit) LocZEdit->SetText(FText::AsNumber((int32)(Loc.Z / 1000)));

    if (RespawnsEdit)  RespawnsEdit->SetText(FText::AsNumber(ElemPtr->RespawnCount()));
    if (HoldTimeEdit)  HoldTimeEdit->SetText(FText::AsNumber(ElemPtr->HoldTime()));

    bExitLatch = true;
}

void UMissionElementDlg::RebuildDesignListFromClass()
{
    if (!ClassCombo || !DesignCombo)
        return;

    const FString ClassName = ClassCombo->GetSelectedOption();
    const int ClassId = Ship::ClassForName(TCHAR_TO_ANSI(*ClassName));

    DesignCombo->ClearOptions();

    List<Text> Designs;
    ShipDesign::GetDesignList(ClassId, Designs);

    if (Designs.size() > 0)
    {
        const FShipDesign* Current = (ElemPtr ? ElemPtr->GetShipDesign() : nullptr);
        bool bFound = false;

        for (int i = 0; i < Designs.size(); i++)
        {
            const char* Dsn = Designs[i]->data();
            if (!Dsn) continue;

            const FString Opt = ANSI_TO_TCHAR(Dsn);
            DesignCombo->AddOption(Opt);

            if (Current && !_stricmp(Dsn, TCHAR_TO_ANSI(*Current->ShipName)))
            {
                DesignCombo->SetSelectedOption(Opt);
                bFound = true;
            }
        }

        if (!bFound)
            DesignCombo->SetSelectedIndex(0);
    }
    else
    {
        DesignCombo->AddOption(TEXT(""));
        DesignCombo->SetSelectedIndex(0);
    }
}

void UMissionElementDlg::RebuildSkinAndLoadoutFromDesign()
{
    // Mirrors legacy OnDesignSelect
    if (!DesignCombo)
        return;

    ShipDesign* Design = nullptr;
    const FString DesignName = DesignCombo->GetSelectedOption();
    if (!DesignName.IsEmpty())
        Design = ShipDesign::Get(TCHAR_TO_ANSI(*DesignName));

    // Loadout:
    if (LoadoutCombo)
    {
        LoadoutCombo->ClearOptions();

        int LoadIndex = 0;

        if (Design)
        {
            MissionLoad* MLoad = nullptr;
            if (ElemPtr && ElemPtr->GetLoadouts().size() > 0)
                MLoad = ElemPtr->GetLoadouts().at(0);

            const List<ShipLoad>& Loadouts = Design->loadouts;

            if (Loadouts.size() > 0)
            {
                for (int i = 0; i < Loadouts.size(); i++)
                {
                    const ShipLoad* L = Loadouts[i];
                    if (L && L->GetName()[0])
                    {
                        const FString Opt = ANSI_TO_TCHAR(L->GetName());
                        LoadoutCombo->AddOption(Opt);

                        if (MLoad && (MLoad->GetName() == L->GetName()))
                            LoadIndex = LoadoutCombo->GetOptionCount() - 1;
                    }
                }
            }
        }

        if (LoadoutCombo->GetOptionCount() < 1)
            LoadoutCombo->AddOption(TEXT(""));

        LoadoutCombo->SetSelectedIndex(LoadIndex);
    }

    // Skin:
    if (SkinCombo)
    {
        SkinCombo->ClearOptions();

        if (Design)
        {
            const FString DefaultSkin = TEXT("Default Skin");
            SkinCombo->AddOption(DefaultSkin);
            SkinCombo->SetSelectedIndex(0);

            ListIter<Skin> Iter = Design->skins;
            while (++Iter)
            {
                Skin* S = Iter.value();
                if (!S) continue;

                const FString Opt = ANSI_TO_TCHAR(S->Name());
                SkinCombo->AddOption(Opt);

                if (ElemPtr && ElemPtr->GetSkin() && !strcmp(S->Name(), ElemPtr->GetSkin()->Name()))
                    SkinCombo->SetSelectedOption(Opt);
            }
        }
        else
        {
            SkinCombo->AddOption(TEXT(""));
            SkinCombo->SetSelectedIndex(0);
        }
    }
}

void UMissionElementDlg::BuildObjectiveTargets()
{
    if (!TargetCombo || !ObjectiveCombo || !MissionPtr || !ElemPtr)
        return;

    TargetCombo->ClearOptions();
    TargetCombo->AddOption(TEXT(""));
    TargetCombo->SetSelectedIndex(0);

    Instruction* Instr = nullptr;
    if (ElemPtr->GetObjectives().size() > 0)
        Instr = ElemPtr->GetObjectives().at(0);

    // Legacy: objid = selectedIndex - 1, but here the first option is "".
    // We stored options as "" + actions by name.
    // We'll map selection by name back to action id:
    const FString ObjSel = ObjectiveCombo->GetSelectedOption();

    int32 ObjId = -1;

    if (!ObjSel.IsEmpty())
    {
        const UEnum* Enum = StaticEnum<INSTRUCTION_ACTION>();
        if (Enum)
        {
            // Convert DisplayName back to enum value
            const int64 Value =
                Enum->GetValueByNameString(ObjSel);

            if (Value != INDEX_NONE)
            {
                ObjId = (int32)Value;
            }
        }
    }

    if (MissionPtr)
    {
        ListIter<MissionElement> Iter = MissionPtr->GetElements();
        while (++Iter)
        {
            MissionElement* E = Iter.value();
            if (!E || E == ElemPtr) continue;

            bool bAdd = false;

            if (ObjId < (int) INSTRUCTION_ACTION::PATROL)
                bAdd = (E->GetIFF() == 0) || (E->GetIFF() == ElemPtr->GetIFF());
            else
                bAdd = (E->GetIFF() != ElemPtr->GetIFF());

            if (bAdd)
            {
                const FString Opt = ANSI_TO_TCHAR(E->GetName());
                TargetCombo->AddOption(Opt);

                if (Instr && !_stricmp(Instr->GetTargetName(), E->GetName()))
                    TargetCombo->SetSelectedOption(Opt);
            }
        }
    }
}

void UMissionElementDlg::UpdateTeamInfo()
{
    if (!MissionPtr || !ElemPtr)
        return;

    // Commander:
    if (CommanderCombo)
    {
        CommanderCombo->ClearOptions();
        CommanderCombo->AddOption(TEXT(""));
        CommanderCombo->SetSelectedIndex(0);

        ListIter<MissionElement> Iter = MissionPtr->GetElements();
        while (++Iter)
        {
            MissionElement* E = Iter.value();
            if (!E)
                continue;

            if (CanCommand(E, ElemPtr))
            {
                const FString Opt = ANSI_TO_TCHAR(E->GetName());
                CommanderCombo->AddOption(Opt);

                if (ElemPtr->GetCommander() == E->GetName())
                {
                    CommanderCombo->SetSelectedOption(Opt);
                }
            }
        }
    }

    // Squadron:
    if (SquadronCombo)
    {
        SquadronCombo->ClearOptions();
        SquadronCombo->AddOption(TEXT(""));
        SquadronCombo->SetSelectedIndex(0);

        ListIter<MissionElement> Iter = MissionPtr->GetElements();
        while (++Iter)
        {
            MissionElement* E = Iter.value();
            if (!E)
                continue;

            if (E->GetIFF() == ElemPtr->GetIFF() &&
                E != ElemPtr &&
                E->IsSquadron())
            {
                const FString Opt = ANSI_TO_TCHAR(E->GetName());
                SquadronCombo->AddOption(Opt);

                if (ElemPtr->GetSquadron() == E->GetName())
                {
                    SquadronCombo->SetSelectedOption(Opt);
                }
            }
        }
    }

    // Carrier:
    if (CarrierCombo)
    {
        CarrierCombo->ClearOptions();
        CarrierCombo->AddOption(TEXT(""));
        CarrierCombo->SetSelectedIndex(0);

        ListIter<MissionElement> Iter = MissionPtr->GetElements();
        while (++Iter)
        {
            MissionElement* E = Iter.value();
            if (!E)
                continue;

            const FShipDesign* Design = E->GetShipDesign();

            if (E->GetIFF() == ElemPtr->GetIFF() &&
                E != ElemPtr &&
                Design &&
                Design->FlightDeck.Num() > 0)
            {
                const FString Opt = ANSI_TO_TCHAR(E->GetName());
                CarrierCombo->AddOption(Opt);

                if (ElemPtr->GetCarrier() == E->GetName())
                {
                    CarrierCombo->SetSelectedOption(Opt);
                }
            }
        }
    }
}

bool UMissionElementDlg::CanCommand(const MissionElement* Commander, const MissionElement* Subordinate) const
{
    if (MissionPtr && Commander && Subordinate && Commander != Subordinate)
    {
        if (Commander->GetIFF() != Subordinate->GetIFF())
            return false;

        if (Commander->IsSquadron())
            return false;

        if (Commander->GetCommander().length() == 0)
            return true;

        if (Subordinate->GetName() == Commander->GetCommander())
            return false;

        MissionElement* E = MissionPtr->FindElement(Commander->GetCommander());

        if (E)
            return CanCommand(E, Subordinate);
    }

    return false;
}

// +--------------------------------------------------------------------+
// UI events

void UMissionElementDlg::OnClassChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!ElemPtr) return;

    RebuildDesignListFromClass();
    RebuildSkinAndLoadoutFromDesign();
}

void UMissionElementDlg::OnDesignChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!ElemPtr) return;

    RebuildSkinAndLoadoutFromDesign();
}

void UMissionElementDlg::OnObjectiveChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    BuildObjectiveTargets();
}

void UMissionElementDlg::OnIFFCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (!IFFEdit || !ElemPtr)
        return;

    int32 ElemIff = 0;
    if (!ParseIntBox(IFFEdit, ElemIff))
        return;

    if (ElemPtr->GetIFF() == ElemIff)
        return;

    ElemPtr->SetIFF(ElemIff);

    UpdateTeamInfo();
    BuildObjectiveTargets();
}

void UMissionElementDlg::OnAcceptClicked()
{
    if (!MissionPtr || !ElemPtr)
        return;

    // Name:
    if (NameEdit)
    {
        const FString N = NameEdit->GetText().ToString();
        ElemPtr->SetName(TCHAR_TO_ANSI(*N));
    }

    // Size:
    if (SizeEdit)
    {
        int32 V = 1;
        if (!ParseIntBox(SizeEdit, V)) V = 1;
        if (V < 1) V = 1;
        ElemPtr->SetCount(V);
    }

    // IFF:
    if (IFFEdit)
    {
        int32 V = 1;
        if (!ParseIntBox(IFFEdit, V)) V = 1;
        ElemPtr->SetIFF(V);
    }

    // Location (km -> meters):
    if (LocXEdit && LocYEdit && LocZEdit)
    {
        int32 Xkm = 0, Ykm = 0, Zkm = 0;
        ParseIntBox(LocXEdit, Xkm);
        ParseIntBox(LocYEdit, Ykm);
        ParseIntBox(LocZEdit, Zkm);

        FVector Loc;
        Loc.X = Xkm * 1000;
        Loc.Y = Ykm * 1000;
        Loc.Z = Zkm * 1000;

        ElemPtr->SetLocation(Loc);
    }

    // Respawns:
    if (RespawnsEdit)
    {
        int32 V = 0;
        if (!ParseIntBox(RespawnsEdit, V)) V = 0;
        ElemPtr->SetRespawnCount(V);
    }

    // Hold time:
    if (HoldTimeEdit)
    {
        int32 V = 0;
        if (!ParseIntBox(HoldTimeEdit, V)) V = 0;
        ElemPtr->SetHoldTime(V);
    }

    // Design + skin:
    if (DesignCombo)
    {
        const FString DName = DesignCombo->GetSelectedOption();
        const FShipDesign* D = DName.IsEmpty() ? nullptr : ShipDesignRegistry::Find(DName);

        if (D)
        {
            ElemPtr->SetShipDesign(D);

            if (SkinCombo)
            {
                // Temporary: skin resolution is still legacy-driven.
                // Leave null for now until skins are migrated to FShipDesign.
                ElemPtr->SetSkin(nullptr);
            }
        }
    }

    // Role:
    if (RoleCombo)
    {
        // Here, RoleCombo options are Mission::RoleName(i) from PATROL..OTHER
        // Legacy uses selected index directly. If yours matches, keep:
        ElemPtr->SetMissionRole(RoleCombo->GetSelectedIndex());
    }

    // Region:
    if (RegionCombo)
    {
        const FString R = RegionCombo->GetSelectedOption();
        if (!R.IsEmpty())
        {
            ElemPtr->SetRegion(TCHAR_TO_ANSI(*R));

            if (ElemPtr->IsPlayer())
                MissionPtr->SetRegion(TCHAR_TO_ANSI(*R));
        }
    }

    // Heading:
    if (HeadingCombo)
    {
        switch (HeadingCombo->GetSelectedIndex())
        {
        default:
        case 0:  ElemPtr->SetHeading(0);        break;
        case 1:  ElemPtr->SetHeading(PI / 2);   break;
        case 2:  ElemPtr->SetHeading(PI);       break;
        case 3:  ElemPtr->SetHeading(3 * PI / 2); break;
        }
    }

    // Commander / Squadron / Carrier:
    if (CommanderCombo)
    {
        const FString S = CommanderCombo->GetSelectedOption();
        ElemPtr->SetCommander(TCHAR_TO_ANSI(*S));
    }

    if (SquadronCombo)
    {
        const FString S = SquadronCombo->GetSelectedOption();
        ElemPtr->SetSquadron(TCHAR_TO_ANSI(*S));
    }

    if (CarrierCombo)
    {
        const FString S = CarrierCombo->GetSelectedOption();
        ElemPtr->SetCarrier(TCHAR_TO_ANSI(*S));
    }

    // Intel:
    if (IntelCombo)
    {
        const FString S = IntelCombo->GetSelectedOption();
        ElemPtr->SetIntelLevel(Intel::GetIntelFromName(TCHAR_TO_ANSI(*S)));
    }

    // Loadout:
    if (LoadoutCombo && LoadoutCombo->GetOptionCount() > 0)
    {
        ElemPtr->GetLoadouts().destroy();

        const FString LoadName = LoadoutCombo->GetSelectedOption();
        if (!LoadName.IsEmpty())
        {
            MissionLoad* MLoad = new MissionLoad(-1, TCHAR_TO_ANSI(*LoadName));
            ElemPtr->GetLoadouts().append(MLoad);
        }
    }

    // Objective + Target:
    if (ObjectiveCombo && TargetCombo)
    {
        List<Instruction>& Objectives = ElemPtr->GetObjectives();
        Objectives.destroy();

        const FString ObjSel = ObjectiveCombo->GetSelectedOption();
        const FString TgtSel = TargetCombo->GetSelectedOption();

        INSTRUCTION_ACTION Action = INSTRUCTION_ACTION::NONE;

        if (!ObjSel.IsEmpty())
        {
            const UEnum* Enum = StaticEnum<INSTRUCTION_ACTION>();
            if (Enum)
            {
                const int64 Value = Enum->GetValueByNameString(ObjSel);
                if (Value != INDEX_NONE)
                {
                    Action = static_cast<INSTRUCTION_ACTION>(Value);
                }
            }
        }

        if (Action >= INSTRUCTION_ACTION::VECTOR)
        {
            Instruction* Obj = new Instruction(Action, TCHAR_TO_ANSI(*TgtSel));
            Objectives.append(Obj);
        }
    }

    // If player, sync mission team:
    if (ElemPtr->IsPlayer())
        MissionPtr->SetTeam(ElemPtr->GetIFF());

    // Return to mission editor:
    if (Manager)
        Manager->ShowMissionEditorDlg();

    SetVisibility(ESlateVisibility::Collapsed);
}

void UMissionElementDlg::OnCancelClicked()
{
    if (Manager)
        Manager->ShowMissionEditorDlg();

    SetVisibility(ESlateVisibility::Collapsed);
}
