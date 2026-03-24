// MissionBriefingDlg.cpp

/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionBriefingDlg.cpp
    AUTHOR:       Carlos Bott
*/

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

// Legacy sim/campaign includes (match your ported headers/paths):
#include "Campaign.h"
#include "Mission.h"
#include "StarSystem.h"
#include "FormatUtil.h"

// UE:
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

#if __has_include("NetLobby.h")
#include "NetLobby.h"
#define SSW_HAS_NETLOBBY 1
#else
#define SSW_HAS_NETLOBBY 0
#endif

UMissionBriefingDlg::UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionBriefingDlg::NativeConstruct()
{
    Super::NativeConstruct();

    CampaignPtr = Campaign::GetCampaign();
    MissionPtr = CampaignPtr ? CampaignPtr->GetMission() : nullptr;

    BindButtons();
    ShowMsnDlg();
}

void UMissionBriefingDlg::NativeDestruct()
{
    UnbindButtons();
    Super::NativeDestruct();
}

void UMissionBriefingDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Legacy: Enter triggers accept
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (PC->WasInputKeyJustPressed(EKeys::Enter))
        {
            OnCommit();
        }
    }
}

FText UMissionBriefingDlg::ToTextFromUtf8(const char* Utf8)
{
    if (!Utf8 || !Utf8[0])
        return FText::GetEmpty();

    // IMPORTANT: no TStringConversion default-ctor usage
    return FText::FromString(FString(UTF8_TO_TCHAR(Utf8)));
}

void UMissionBriefingDlg::BindButtons()
{
    if (AcceptButton) AcceptButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    if (CancelButton) CancelButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);

    if (SitButton) SitButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleSitClicked);
    if (PkgButton) PkgButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandlePkgClicked);
    if (NavButton) NavButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleNavClicked);
    if (WepButton) WepButton->OnClicked.AddUniqueDynamic(this, &UMissionBriefingDlg::HandleWepClicked);
}

void UMissionBriefingDlg::UnbindButtons()
{
    if (AcceptButton) AcceptButton->OnClicked.RemoveAll(this);
    if (CancelButton) CancelButton->OnClicked.RemoveAll(this);

    if (SitButton) SitButton->OnClicked.RemoveAll(this);
    if (PkgButton) PkgButton->OnClicked.RemoveAll(this);
    if (NavButton) NavButton->OnClicked.RemoveAll(this);
    if (WepButton) WepButton->OnClicked.RemoveAll(this);
}

void UMissionBriefingDlg::ShowMsnDlg()
{
    CampaignPtr = Campaign::GetCampaign();
    MissionPtr = CampaignPtr ? CampaignPtr->GetMission() : nullptr;
    PackageIndex = -1;

    // Mission name
    if (MissionNameText)
    {
        if (MissionPtr)
            MissionNameText->SetText(ToTextFromUtf8(MissionPtr->GetName()));
        else
            MissionNameText->SetText(FText::FromString(TEXT("NO MISSION")));
    }

    // System
    if (MissionSystemText)
    {
        MissionSystemText->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            if (StarSystem* Sys = MissionPtr->GetStarSystem())
                MissionSystemText->SetText(ToTextFromUtf8(Sys->Name()));
        }
    }

    // Sector/Region
    if (MissionSectorText)
    {
        MissionSectorText->SetText(FText::GetEmpty());

        if (MissionPtr)
            MissionSectorText->SetText(ToTextFromUtf8(MissionPtr->GetRegion()));
    }

    // Start time
    if (MissionTimeStartText)
    {
        if (MissionPtr)
        {
            char Buf[32] = { 0 };
            FormatDayTime(Buf, MissionPtr->GetStart());        // matches legacy signature FormatDayTime(char*, int)
            MissionTimeStartText->SetText(ToTextFromUtf8(Buf));
        }
        else
        {
            MissionTimeStartText->SetText(FText::GetEmpty());
        }
    }

    // Target time (optional)
    if (MissionTimeTargetText && MissionTimeTargetLabelText)
    {
        if (bShowTimeOnTarget)
        {
            const int32 TimeOnTarget = CalcTimeOnTarget();
            if (TimeOnTarget > 0)
            {
                char Buf[32] = { 0 };
                FormatDayTime(Buf, TimeOnTarget);
                MissionTimeTargetText->SetText(ToTextFromUtf8(Buf));
                MissionTimeTargetLabelText->SetText(FText::FromString(TEXT("TARGET:")));
            }
            else
            {
                MissionTimeTargetText->SetText(FText::GetEmpty());
                MissionTimeTargetLabelText->SetText(FText::GetEmpty());
            }
        }
        else
        {
            MissionTimeTargetText->SetText(FText::GetEmpty());
            MissionTimeTargetLabelText->SetText(FText::GetEmpty());
        }
    }

    const bool bMissionOK = (MissionPtr && MissionPtr->IsOK());

    // Tabs enable gating
    if (bDisableTabsWhenMissionNotOK)
    {
        if (SitButton) SitButton->SetIsEnabled(bMissionOK);
        if (PkgButton) PkgButton->SetIsEnabled(bMissionOK);
        if (NavButton) NavButton->SetIsEnabled(bMissionOK);
        if (WepButton) WepButton->SetIsEnabled(bMissionOK);
    }
    else
    {
        if (SitButton) SitButton->SetIsEnabled(true);
        if (PkgButton) PkgButton->SetIsEnabled(true);
        if (NavButton) NavButton->SetIsEnabled(true);
        if (WepButton) WepButton->SetIsEnabled(true);
    }

    // Optional: disable WEP in net lobby
    if (bDisableWeaponTabInNetLobby && WepButton)
    {
#if SSW_HAS_NETLOBBY
        if (NetLobby::GetInstance())
            WepButton->SetIsEnabled(false);
#endif
    }

    // Accept/Cancel enable gating
    if (AcceptButton) AcceptButton->SetIsEnabled(bMissionOK);
    if (CancelButton) CancelButton->SetIsEnabled(true);
}

int32 UMissionBriefingDlg::CalcTimeOnTarget() const
{
    // If your ported Mission/Instruction navigation types are available,
    // implement this exactly like legacy. If not, safely return 0.
#if __has_include("Instruction.h") && __has_include("Element.h")
#include "Instruction.h"
#include "Element.h"
    if (!MissionPtr) return 0;

    MissionElement* Element = MissionPtr->GetElements()[0];
    if (!Element) return 0;

    // Legacy Point math lives in your port. If it’s not Point anymore,
    // update these 3 lines to your actual vector type.
    Point Loc = Element->Location();
    Loc.SwapYZ();

    int32 MissionTime = MissionPtr->Start();

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        const int32 Action = NavPt->Action();

        const double Dist = Point(Loc - NavPt->Location()).length();
        int32 ETR = 0;
        if (NavPt->Speed() > 0)
            ETR = (int32)(Dist / NavPt->Speed());
        else
            ETR = (int32)(Dist / 500);

        MissionTime += ETR;
        Loc = NavPt->Location();

        if (Action >= Instruction::ESCORT)
            return MissionTime;
    }
#endif

    return 0;
}

void UMissionBriefingDlg::OnTabButton(UButton* Pressed)
{
    if (!Manager || !Pressed)
        return;

    if (Pressed == SitButton)
    {
        Manager->ShowMsnObjDlg();
    }
    else if (Pressed == PkgButton)
    {
        Manager->ShowMsnPkgDlg();
    }
    else if (Pressed == NavButton)
    {
        Manager->ShowNavDlg();
    }
    else if (Pressed == WepButton)
    {
        Manager->ShowMsnWepDlg();
    }
}

void UMissionBriefingDlg::OnCommit()
{
    // Unreal handoff point – keep behavior minimal for now:
    if (Manager)
    {
        Manager->Hide();
    }
}

void UMissionBriefingDlg::OnCancel()
{
    if (Manager)
    {
        Manager->Hide();
    }
}

void UMissionBriefingDlg::HandleAcceptClicked() { OnCommit(); }
void UMissionBriefingDlg::HandleCancelClicked() { OnCancel(); }

void UMissionBriefingDlg::HandleSitClicked() { OnTabButton(SitButton); }
void UMissionBriefingDlg::HandlePkgClicked() { OnTabButton(PkgButton); }
void UMissionBriefingDlg::HandleNavClicked() { OnTabButton(NavButton); }
void UMissionBriefingDlg::HandleWepClicked() { OnTabButton(WepButton); }
