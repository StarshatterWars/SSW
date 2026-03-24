/*=============================================================================
    Project:        Starshatter Wars (Unreal Port)
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      UI / Tactical Reference
    FILE:           TacRefDlg.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Implements UTacRefDlg.

    FILTERING MODEL
    ===============
    - All category buttons re-use the same dropdown (ShipCombo).
    - The dropdown is repopulated from ShipDesignTable each time the category
      changes, filtering rows by:
          DeriveShipCategoryFromClass(FShipDesign::ShipClass)

    NOTES
    =====
    - CancelButton is owned by UBaseScreen; this file binds to it safely.
    - This preserves your existing workflow:
        Show() -> PopulateShipDropdown() -> SelectShipByIndex()
    - Weapon mode currently shows placeholder text (as before).

=============================================================================*/

#include "TacRefDlg.h"

#include "MenuScreen.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/ComboBoxString.h"

DEFINE_LOG_CATEGORY_STATIC(LogTacRefDlg, Log, All);

// -----------------------------------------------------------------------------
// Minimal weapon grouping helpers (optional)
// -----------------------------------------------------------------------------
static FString NormalizeWeaponGroupLabel(const FShipWeapon& W)
{
    if (!W.GroupName.IsEmpty())
        return W.GroupName.ToUpper();

    const FString Type = W.DesignName.ToLower();
    const FString Wep = W.WeaponType.ToLower();

    if (Type.Contains(TEXT("beam")) || Wep.Contains(TEXT("laser")))
        return TEXT("LASER");

    if (Type.Contains(TEXT("missile")) || Wep.Contains(TEXT("mk")) || Wep.Contains(TEXT("nike")))
        return TEXT("MISSILE");

    if (Type.Contains(TEXT("bolt")) || Wep.Contains(TEXT("phalanx")) || Wep.Contains(TEXT("vanguard")))
        return TEXT("PDB");

    return TEXT("WEAPON");
}

// -----------------------------------------------------------------------------

UTacRefDlg::UTacRefDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Mode = 0;
    SelectedShipIndex = INDEX_NONE;
    ActiveCategory = EShipCategory::Unknown;
}

void UTacRefDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UTacRefDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;

    UE_LOG(LogTacRefDlg, Warning, TEXT("[TacRefDlg] InitializeDlg: manager=%s ShipDesignTable=%s ShipCombo=%s"),
        manager ? TEXT("VALID") : TEXT("NULL"),
        ShipDesignTable ? TEXT("VALID") : TEXT("NULL"),
        ShipCombo ? TEXT("VALID") : TEXT("NULL"));
}

void UTacRefDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // CancelButton is inherited from BaseScreen:
    if (CancelButton)     CancelButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleCloseClicked);

    // Category buttons (names match your WBP)
    if (StationButton)    StationButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleStationModeClicked);
    if (ShipButton)       ShipButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleShipModeClicked);
    if (FighterButton)    FighterButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleFighterModeClicked);
    if (TransportButton)  TransportButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleTransportModeClicked);
    if (BuildingButton)   BuildingButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleBuildingModeClicked);

    if (WeaponButton)     WeaponButton->OnClicked.AddDynamic(this, &UTacRefDlg::HandleWeaponModeClicked);

    // Dropdown
    if (ShipCombo)
        ShipCombo->OnSelectionChanged.AddDynamic(this, &UTacRefDlg::HandleShipComboChanged);

    if (EmpireCombo)
    {
        EmpireCombo->OnSelectionChanged.AddDynamic(this, &UTacRefDlg::HandleEmpireComboChanged);
        PopulateEmpireDropdown_AutoDetect(true, true);
    }

    UE_LOG(LogTacRefDlg, Log, TEXT("[TacRefDlg] NativeOnInitialized: Bound buttons/combobox (where available)"));
}

void UTacRefDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

void UTacRefDlg::Show()
{
    UE_LOG(LogTacRefDlg, Warning, TEXT("[TacRefDlg] Show: BEGIN"));

    // Default to SHIP category (capital ships) on open:
    Mode = 0;
    ActiveCategory = EShipCategory::CapitalShip;

    PopulateShipDropdown();

    if (SelectedShipIndex == INDEX_NONE && ShipRowNames.Num() > 0)
        SelectedShipIndex = 0;

    SelectShipByIndex(SelectedShipIndex);

    UE_LOG(LogTacRefDlg, Warning, TEXT("[TacRefDlg] Show: DONE options=%d sel=%d cat=%d"),
        ShipRowNames.Num(), SelectedShipIndex, (int32)ActiveCategory);
}

void UTacRefDlg::ExecFrame()
{
    // Optional per-frame logic
}

bool UTacRefDlg::PassesCategoryFilter(const FShipDesign& Row) const
{
    // Unknown means "no filter"
    if (ActiveCategory == EShipCategory::Unknown)
        return true;

    return (Row.Category == ActiveCategory);
}

// -----------------------------------------------------------------------------
// Dropdown population (filtered by ActiveCategory)
// -----------------------------------------------------------------------------
void UTacRefDlg::PopulateShipDropdown()
{
    ShipRowNames.Reset();

    if (!ShipCombo)
    {
        UE_LOG(LogTacRefDlg, Error, TEXT("[TacRefDlg] PopulateShipDropdown: ShipCombo is NULL (BindWidget failed)"));
        return;
    }

    ShipCombo->ClearOptions();

    if (!ShipDesignTable)
    {
        UE_LOG(LogTacRefDlg, Error, TEXT("[TacRefDlg] PopulateShipDropdown: ShipDesignTable is NULL"));
        return;
    }

    TArray<FName> RowNames = ShipDesignTable->GetRowNames();
    if (RowNames.Num() == 0)
    {
        UE_LOG(LogTacRefDlg, Warning, TEXT("[TacRefDlg] PopulateShipDropdown: no rows"));
        return;
    }

    // Sort by DisplayName (preserves your current behavior)
    RowNames.Sort([this](const FName& A, const FName& B)
        {
            const FShipDesign* RA = ShipDesignTable->FindRow<FShipDesign>(A, TEXT("Sort"), false);
            const FShipDesign* RB = ShipDesignTable->FindRow<FShipDesign>(B, TEXT("Sort"), false);

            const FString SA = RA ? RA->DisplayName : A.ToString();
            const FString SB = RB ? RB->DisplayName : B.ToString();
            return SA < SB;
        });

    // Build filtered options
    for (const FName& RowName : RowNames)
    {
        const FShipDesign* Row = ShipDesignTable->FindRow<FShipDesign>(RowName, TEXT("PopulateShipDropdown"), false);
        if (!Row)
            continue;

        if (!PassesCategoryFilter(*Row))
            continue;

        if (!PassesSecretFilter(*Row))
            continue;

        if (!PassesEmpireFilter(*Row))     // if you already added empire filtering
            continue;

        // Label format: "CR COURAGEOUS" (or just DisplayName)
        const FString Label =
            Row->Abrv.IsEmpty()
            ? Row->DisplayName
            : FString::Printf(TEXT("%s %s"), *Row->Abrv, *Row->DisplayName);

        ShipCombo->AddOption(Label);
        ShipRowNames.Add(RowName);
    }

    // Ensure selection valid
    if (ShipRowNames.Num() > 0)
    {
        if (!ShipRowNames.IsValidIndex(SelectedShipIndex))
            SelectedShipIndex = 0;

        ShipCombo->SetSelectedIndex(SelectedShipIndex);
    }
    else
    {
        SelectedShipIndex = INDEX_NONE;
    }

    UE_LOG(LogTacRefDlg, Log, TEXT("[TacRefDlg] PopulateShipDropdown: cat=%d options=%d"),
        (int32)ActiveCategory, ShipRowNames.Num());
}

void UTacRefDlg::HandleShipComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;

    if (!ShipCombo)
        return;

    // Resolve index from the selected option label
    const FString Current = ShipCombo->GetSelectedOption();
    const int32 Index = ShipCombo->FindOptionIndex(Current);
    if (Index == INDEX_NONE)
        return;

    SelectedShipIndex = Index;
    SelectShipByIndex(Index);
}

void UTacRefDlg::SelectShipByIndex(int32 Index)
{
    if (!ShipDesignTable)
        return;

    if (!ShipRowNames.IsValidIndex(Index))
        return;

    const FName RowName = ShipRowNames[Index];
    const FShipDesign* Row = ShipDesignTable->FindRow<FShipDesign>(RowName, TEXT("SelectShipByIndex"), false);
    if (!Row)
        return;

    FString Caption, Stats, Desc;
    BuildShipTexts(*Row, Caption, Stats, Desc);
    
    FString FullName;

    if (Row->Abrv.IsEmpty())
    {
        FullName = Row->DisplayName;
    }
    else
    {
        FullName = FString::Printf(TEXT("%s-%s"), *Row->Abrv, *Row->DisplayName);
    }
    if (TxtDisplayName) TxtDisplayName->SetText(FText::FromString(FullName));
    if (TxtCaption)     TxtCaption->SetText(FText::FromString(Caption));
    if (TxtStats)       TxtStats->SetText(FText::FromString(Stats));
    if (TxtDescription) TxtDescription->SetText(FText::FromString(Desc));
}

void UTacRefDlg::BuildShipTexts(const FShipDesign& Dsn, FString& OutCaption, FString& OutStats, FString& OutDesc) const
{
    OutCaption = Dsn.Abrv.IsEmpty()
        ? Dsn.DisplayName
        : FString::Printf(TEXT("%s %s"), *Dsn.Abrv, *Dsn.DisplayName);

    OutDesc = !Dsn.Description.IsEmpty()
        ? Dsn.Description
        : TEXT("NO DESCRIPTION AVAILABLE.");

    auto Num0 = [](double V) { return FString::Printf(TEXT("%.0f"), V); };
    auto Num1 = [](double V) { return FString::Printf(TEXT("%.1f"), V); };

    OutCaption = Dsn.Abrv.IsEmpty()
        ? Dsn.DisplayName
        : FString::Printf(TEXT("%s %s"), *Dsn.Abrv, *Dsn.DisplayName);

    // Description panel: keep simple but styled
    OutDesc = !Dsn.Description.IsEmpty()
        ? FString::Printf(TEXT("<Default>%s</>"), *Dsn.Description)
        : TEXT("<HUD_Muted>NO DESCRIPTION AVAILABLE.</>");

    // Stats panel
    FString S;

    // Big header
    //S += FString::Printf(TEXT("<HUD_Title>%s</>\n\n"), *OutCaption);

    // Overview
    S += TEXT("<HUD_Section>OVERVIEW</>\n");
    S += FString::Printf(TEXT("<HUD_Label>CLASS:</>      <HUD_Value>%s</>\n"), *Dsn.ShipClass);
    S += FString::Printf(TEXT("<HUD_Label>MASS:</>       <HUD_Value>%s</>\n"), *Num0(Dsn.Mass));
    S += FString::Printf(TEXT("<HUD_Label>SCALE:</>      <HUD_Value>%s</>\n"), *Num1(Dsn.Scale));
    S += FString::Printf(TEXT("<HUD_Label>V LIMIT:</>    <HUD_Value>%s</>\n"), *Num0(Dsn.Vlimit));
    S += FString::Printf(TEXT("<HUD_Label>AGILITY:</>    <HUD_Value>%s</>\n"), *Num1(Dsn.Agility));
    S += FString::Printf(TEXT("<HUD_Label>DETECT:</>     <HUD_Value>%s</>\n"), *Num0(Dsn.Detet));
    S += FString::Printf(TEXT("<HUD_Label>REPAIR:</>     <HUD_Value>%d TEAMS</>\n"), Dsn.RepairTeams);

    // Weapons summary
    if (Dsn.Weapon.Num() > 0)
    {
        TMap<FString, int32> GroupCounts;
        for (const FShipWeapon& W : Dsn.Weapon)
        {
            const FString G = NormalizeWeaponGroupLabel(W);
            GroupCounts.FindOrAdd(G)++;
        }

        S += TEXT("\n<HUD_Section>WEAPONS</>\n");
        for (const auto& KVP : GroupCounts)
        {
            S += FString::Printf(TEXT("<HUD_Indent>%s:</> <HUD_Value>%d</>\n"), *KVP.Key, KVP.Value);
        }
    }
    else
    {
        S += TEXT("\n<HUD_Muted>NO WEAPONS DATA.</>\n");
    }

    OutStats = MoveTemp(S);
}

// -----------------------------------------------------------------------------
// Mode / Category buttons
// -----------------------------------------------------------------------------
void UTacRefDlg::HandleStationModeClicked()
{
    Mode = 0;
    ActiveCategory = EShipCategory::Station;
    PopulateEmpireDropdown_AutoDetect(true, true);
    PopulateShipDropdown();
    SelectShipByIndex(SelectedShipIndex);
}

void UTacRefDlg::HandleShipModeClicked()
{
    Mode = 0;
    ActiveCategory = EShipCategory::CapitalShip;
    PopulateShipDropdown();
    PopulateEmpireDropdown_AutoDetect(true, true);
    SelectShipByIndex(SelectedShipIndex);
}

void UTacRefDlg::HandleFighterModeClicked()
{
    Mode = 0;
    ActiveCategory = EShipCategory::Fighter;
    PopulateShipDropdown();
    PopulateEmpireDropdown_AutoDetect(true, true);
    SelectShipByIndex(SelectedShipIndex);
}

void UTacRefDlg::HandleTransportModeClicked()
{
    Mode = 0;
    ActiveCategory = EShipCategory::Transport;
    PopulateShipDropdown();
    PopulateEmpireDropdown_AutoDetect(true, true);
    SelectShipByIndex(SelectedShipIndex);
}

void UTacRefDlg::HandleBuildingModeClicked()
{
    Mode = 0;
    ActiveCategory = EShipCategory::Building;
    PopulateShipDropdown();
    PopulateEmpireDropdown_AutoDetect(true, true);
    SelectShipByIndex(SelectedShipIndex);
}

void UTacRefDlg::HandleWeaponModeClicked()
{
    Mode = 1;

    if (TxtCaption)     TxtCaption->SetText(FText::FromString(TEXT("WEAPON REFERENCE")));
    if (TxtStats)       TxtStats->SetText(FText::FromString(TEXT("WEAPON MODE NOT WIRED YET.")));
    if (TxtDescription) TxtDescription->SetText(FText::GetEmpty());
}

// -----------------------------------------------------------------------------
// Close
// -----------------------------------------------------------------------------
void UTacRefDlg::HandleCloseClicked()
{
    if (manager)
    {
        manager->ShowMenuDlg();
        return;
    }

    RemoveFromParent();
}

bool UTacRefDlg::PassesSecretFilter(const FShipDesign& Row) const
{
    // Non-secret rows always show
    if (!Row.Secret)          // <-- use your actual bool name
        return true;

    // For now: hide all secret designs unless debug toggle enabled
    return bShowSecretDesigns;

    // Later:
    // return bShowSecretDesigns || IsShipClassDiscovered(Row);
}

// -----------------------------------------------------------------------------
// Empire Filtering
// -----------------------------------------------------------------------------
bool UTacRefDlg::PassesEmpireFilter(const FShipDesign& Row) const
{
    if (ActiveEmpire == EShipEmpire::NONE)
        return true;

    // Ships marked as All are visible to every empire filter
    if (Row.ShipEmpire == EShipEmpire::Civilian || Row.ShipEmpire == EShipEmpire::Neutral)
        return true;

    if (Row.ShipEmpire == EShipEmpire::All)
        return true;

    return (Row.ShipEmpire == ActiveEmpire);
}

// -----------------------------------------------------------------------------
// Empire Combo Population
// -----------------------------------------------------------------------------
void UTacRefDlg::PopulateEmpireDropdown()
{
    if (!EmpireCombo)
        return;

    EmpireCombo->ClearOptions();

    // First entry = ALL (no filter)
    EmpireCombo->AddOption(TEXT("ALL"));
    ActiveEmpire = EShipEmpire::NONE;

    // Pull enum values dynamically
    const UEnum* EnumPtr = StaticEnum<EShipEmpire>();
    if (!EnumPtr)
        return;

    const int32 NumEnums = EnumPtr->NumEnums();

    for (int32 i = 0; i < NumEnums - 1; ++i) // -1 skips _MAX
    {
        const EShipEmpire Value = static_cast<EShipEmpire>(EnumPtr->GetValueByIndex(i));

        if (Value == EShipEmpire::NONE)
            continue;

        const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();
        EmpireCombo->AddOption(DisplayName);
    }

    EmpireCombo->SetSelectedIndex(0);
}

void UTacRefDlg::HandleEmpireComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;

    if (!EmpireCombo)
        return;

    const int32 Index = EmpireCombo->FindOptionIndex(EmpireCombo->GetSelectedOption());
    if (!EmpireValues.IsValidIndex(Index))
        return;

    ActiveEmpire = EmpireValues[Index];

    PopulateShipDropdown();       // ship list respects ActiveEmpire via PassesEmpireFilter
    // Optionally: SelectShipByIndex(SelectedShipIndex);
}

// -----------------------------------------------------------------------------
// Empire Combo (Auto-Detect)
// -----------------------------------------------------------------------------
void UTacRefDlg::PopulateEmpireDropdown_AutoDetect(bool bRespectCategory, bool bRespectSecret)
{
    if (!EmpireCombo || !ShipDesignTable)
        return;

    EmpireCombo->ClearOptions();
    EmpireValues.Reset();

    // Always provide ALL
    EmpireCombo->AddOption(TEXT("ALL"));
    EmpireValues.Add(EShipEmpire::NONE);

    // Gather empires present in the table (optionally respecting other filters)
    TSet<EShipEmpire> Present;

    const TArray<FName> RowNames = ShipDesignTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        const FShipDesign* Row = ShipDesignTable->FindRow<FShipDesign>(RowName, TEXT("EmpireDetect"), false);
        if (!Row)
            continue;

        // Respect secret/visibility if your struct has it:
        if (bRespectSecret)
        {
            // If your field name differs, change this line:
            if (Row->Secret) // legacy "secret" flag
                continue;
        }

        // Respect category filter if requested:
        if (bRespectCategory)
        {
            if (!PassesCategoryFilter(*Row))
                continue;
        }

        // Pull the empire (assumes you added a real field in FShipDesign)
        // If your field name differs, change this line:
        const EShipEmpire Emp = Row->ShipEmpire;

        if (Emp != EShipEmpire::NONE)
            Present.Add(Emp);
    }

    // Sort by display name
    const UEnum* EnumPtr = StaticEnum<EShipEmpire>();
    TArray<EShipEmpire> Sorted = Present.Array();

    Sorted.Sort([EnumPtr](const EShipEmpire A, const EShipEmpire B)
        {
            const int64 VA = (int64)A;
            const int64 VB = (int64)B;

            const FString SA = EnumPtr ? EnumPtr->GetDisplayNameTextByValue(VA).ToString() : FString::FromInt((int32)A);
            const FString SB = EnumPtr ? EnumPtr->GetDisplayNameTextByValue(VB).ToString() : FString::FromInt((int32)B);
            return SA < SB;
        });

    // Add options
    for (const EShipEmpire Emp : Sorted)
    {
        const FString Label = EnumPtr
            ? EnumPtr->GetDisplayNameTextByValue((int64)Emp).ToString()
            : FString::Printf(TEXT("Empire %d"), (int32)Emp);

        EmpireCombo->AddOption(Label);
        EmpireValues.Add(Emp);
    }

    // Preserve selection if still valid, otherwise ALL
    int32 SelectIndex = 0;
    for (int32 i = 0; i < EmpireValues.Num(); ++i)
    {
        if (EmpireValues[i] == ActiveEmpire)
        {
            SelectIndex = i;
            break;
        }
    }

    EmpireCombo->SetSelectedIndex(SelectIndex);
}

