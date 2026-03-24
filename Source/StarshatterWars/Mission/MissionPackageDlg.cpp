/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPackageDlg.cpp
    AUTHOR:       Carlos Bott
*/

#include "MissionPackageDlg.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

UMissionPackageDlg::UMissionPackageDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionPackageDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(this, &UMissionPackageDlg::OnClickedAccept);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UMissionPackageDlg::OnClickedCancel);
    }

    // Initial draw (legacy Show() did this)
    DrawPackages();
    DrawNavPlan();
    DrawThreats();
}

void UMissionPackageDlg::ExecFrame(float DeltaSeconds)
{
    // Legacy: VK_RETURN commits. BaseScreen already maps Enter->HandleAccept.
}

void UMissionPackageDlg::HandleAccept()
{
    OnClickedAccept();
}

void UMissionPackageDlg::HandleCancel()
{
    OnClickedCancel();
}

void UMissionPackageDlg::BindFormWidgets()
{
    // BindList(ID_PKG_LIST, PackageList);
    // BindList(ID_NAV_LIST, NavList);
    //
    // BindLabel(ID_THREAT_0, Threat0); etc...
}

FString UMissionPackageDlg::GetLegacyFormText() const
{
    // This is the FORM block you pasted for MsnPkgDlg.
    // Kept verbatim; BaseScreen currently ignores "column:" parsing (unless you extend it),
    // but it will still apply label/font/colors where supported.

    return TEXT(R"frm(
form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100,  20, 100, 100, 10),
      x_weights:  ( 0, 0.2, 0.4, 0.2, 0.2,  0),

      y_mins:     (28, 30,  10,  90, 24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  0,  1,  0)
   },

   // (snip) — keep the rest exactly as provided in your paste.
}
)frm");
}

// --------------------------------------------------------------------
// Populate (wire to your mission data once Mission/Campaign are Unreal-available)
// --------------------------------------------------------------------

void UMissionPackageDlg::DrawPackages()
{
    // TODO: create UObject row items and feed PackageList->SetListItems(...)
    // Legacy logic filtered MissionElements by IFF/region/type and built columns.
}

void UMissionPackageDlg::DrawNavPlan()
{
    // TODO: read selected package element, fill nav list items
}

void UMissionPackageDlg::DrawThreats()
{
    if (Threat0) Threat0->SetText(FText::GetEmpty());
    if (Threat1) Threat1->SetText(FText::GetEmpty());
    if (Threat2) Threat2->SetText(FText::GetEmpty());
    if (Threat3) Threat3->SetText(FText::GetEmpty());
    if (Threat4) Threat4->SetText(FText::GetEmpty());

    // TODO: compute threats similar to legacy DrawThreats()
}

// --------------------------------------------------------------------
// UI callbacks
// --------------------------------------------------------------------

void UMissionPackageDlg::OnClickedAccept()
{
    // Hook to mission commit logic
}

void UMissionPackageDlg::OnClickedCancel()
{
    RemoveFromParent();
}

void UMissionPackageDlg::OnClickedTabSit()
{
    // Switch back to objective dialog via PlanScreen/UI router
}

void UMissionPackageDlg::OnClickedTabPkg()
{
    // Already on package dialog
}
