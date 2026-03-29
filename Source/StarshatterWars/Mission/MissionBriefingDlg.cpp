/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe
    FILE:         MissionBriefingDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Briefing Screen (Unreal Port)

    - Uses CmdDlg-style MenuButton system (SIT / PKG / NAV / WEP)
    - Drives UI state via WidgetSwitcher (MissionSwitcher)
    - Hosts child mission subpanels
    - Displays mission header (name, system, sector, time)
    - Handles Accept / Cancel flow
    - Calculates Time-On-Target using navigation instructions
*/

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

#include "MissionObjectiveDlg.h"
#include "MissionPackageDlg.h"
#include "MissionNavDlg.h"
#include "MissionWeaponDlg.h"

// Legacy sim/campaign:
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "StarSystem.h"
#include "FormatUtil.h"
#include "Instruction.h"
#include "MissionElement.h"

// UI:
#include "MenuButton.h"
#include "SelectableButtonGroup.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"

// Input:
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

// Optional sound:
#include "SSWGameInstance.h"

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

    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
        MissionButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleAcceptClicked);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
        ReturnButton->OnClicked.AddDynamic(this, &UMissionBriefingDlg::HandleCancelClicked);
    }

    BuildMenuButtons();
    InitializeSubPanels();
    ShowMsnDlg();
}

void UMissionBriefingDlg::NativeDestruct()
{
    if (MissionButton)
    {
        MissionButton->OnClicked.RemoveAll(this);
    }

    if (ReturnButton)
    {
        ReturnButton->OnClicked.RemoveAll(this);
    }

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->OnSelected.RemoveAll(this);
            Button->OnHovered.RemoveAll(this);
        }
    }

    Super::NativeDestruct();
}

void UMissionBriefingDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

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
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString(UTF8_TO_TCHAR(Utf8)));
}

void UMissionBriefingDlg::BuildMenuButtons()
{
    if (!MenuButtonClass || !MenuToggleGroup || !MenuButtonContainer)
    {
        return;
    }

    MenuButtonContainer->ClearChildren();
    AllMenuButtons.Empty();

    for (const FString& Item : MenuItems)
    {
        UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
        if (!NewButton)
        {
            continue;
        }

        if (UTextBlock* Label = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
        {
            Label->SetText(FText::FromString(Item));
        }

        NewButton->MenuOption = Item;

        MenuButtonContainer->AddChild(NewButton);
        MenuToggleGroup->RegisterButton(NewButton);

        NewButton->OnSelected.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);
        NewButton->OnSelected.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleSelected);

        NewButton->OnHovered.RemoveDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);
        NewButton->OnHovered.AddDynamic(this, &UMissionBriefingDlg::OnMenuToggleHovered);

        AllMenuButtons.Add(NewButton);
    }
}

void UMissionBriefingDlg::InitializeSubPanels()
{
    if (SitPanel)
    {
        //SitPanel->SetParentBriefingDlg(this);
        //SitPanel->SetManager(Manager);
    }

    if (PkgPanel)
    {
       //PkgPanel->SetParentBriefingDlg(this);
        //PkgPanel->SetManager(Manager);
    }

    if (NavPanel)
    {
        //NavPanel->SetParentBriefingDlg(this);
        //NavPanel->SetManager(Manager);
    }

    if (WepPanel)
    {
        //WepPanel->SetParentBriefingDlg(this);
        //WepPanel->SetManager(Manager);
    }
}

void UMissionBriefingDlg::ShowMsnDlg()
{
    CampaignPtr = Campaign::GetCampaign();
    MissionPtr = CampaignPtr ? CampaignPtr->GetMission() : nullptr;
    PackageIndex = -1;

    RefreshHeader();

    const bool bMissionOK = (MissionPtr && MissionPtr->IsOK());

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (!Button)
        {
            continue;
        }

        bool bEnable = true;

        if (bDisableTabsWhenMissionNotOK)
        {
            bEnable = bMissionOK;
        }

        if (bDisableWeaponTabInNetLobby && Button->MenuOption == TEXT("WEP"))
        {
#if SSW_HAS_NETLOBBY
            if (NetLobby::GetInstance())
            {
                bEnable = false;
            }
#endif
        }

        Button->SetIsEnabled(bEnable);
    }

    if (MissionButton)
    {
        MissionButton->SetIsEnabled(bMissionOK);
    }

    if (ReturnButton)
    {
        ReturnButton->SetIsEnabled(true);
    }

    if (MissionButtonText)
    {
        MissionButtonText->SetText(FText::FromString(TEXT("ACCEPT")));
    }

    if (ReturnButtonText)
    {
        ReturnButtonText->SetText(FText::FromString(TEXT("CANCEL")));
    }

    if (SitPanel)
    {
        //SitPanel->RefreshFromMission();
    }

    if (PkgPanel)
    {
        //PkgPanel->RefreshFromMission();
    }

    if (NavPanel)
    {
       // NavPanel->RefreshFromMission();
    }

    if (WepPanel)
    {
        //WepPanel->RefreshFromMission();
    }

    SetMode(EMissionBriefingMode::SIT);
}

void UMissionBriefingDlg::RefreshHeader()
{
    if (MissionNameText)
    {
        MissionNameText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetName())
            : FText::FromString(TEXT("NO MISSION")));
    }

    if (MissionSystemText)
    {
        MissionSystemText->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            if (StarSystem* Sys = MissionPtr->GetStarSystem())
            {
                MissionSystemText->SetText(ToTextFromUtf8(Sys->GetName()));
            }
        }
    }

    if (MissionSectorText)
    {
        MissionSectorText->SetText(
            MissionPtr ? ToTextFromUtf8(MissionPtr->GetRegion())
            : FText::GetEmpty());
    }

    if (MissionTimeStartText)
    {
        if (MissionPtr)
        {
            char Buf[32] = { 0 };
            FormatDayTime(Buf, MissionPtr->GetStart());
            MissionTimeStartText->SetText(ToTextFromUtf8(Buf));
        }
        else
        {
            MissionTimeStartText->SetText(FText::GetEmpty());
        }
    }

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
}

void UMissionBriefingDlg::SetMode(EMissionBriefingMode NewMode)
{
    CurrentMode = NewMode;

    if (MissionSwitcher)
    {
        MissionSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentMode));
    }

    RefreshMenuSelection();

    switch (CurrentMode)
    {
    case EMissionBriefingMode::SIT:
        if (SitPanel)
        {
            //SitPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::PKG:
        if (PkgPanel)
        {
            //PkgPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::NAV:
        if (NavPanel)
        {
            //NavPanel->RefreshFromMission();
        }
        break;

    case EMissionBriefingMode::WEP:
        if (WepPanel)
        {
            //WepPanel->RefreshFromMission();
        }
        break;

    default:
        break;
    }
}

void UMissionBriefingDlg::RefreshMenuSelection()
{
    const int32 ModeIndex = static_cast<int32>(CurrentMode);
    const FString ActiveLabel = MenuItems.IsValidIndex(ModeIndex)
        ? MenuItems[ModeIndex]
        : FString();

    for (UMenuButton* Button : AllMenuButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }
}

void UMissionBriefingDlg::OnMenuToggleSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    if (USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        GI->PlayAcceptSound(this);
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("SIT"))
    {
        SetMode(EMissionBriefingMode::SIT);
    }
    else if (Option == TEXT("PKG"))
    {
        SetMode(EMissionBriefingMode::PKG);
    }
    else if (Option == TEXT("NAV"))
    {
        SetMode(EMissionBriefingMode::NAV);
    }
    else if (Option == TEXT("WEP"))
    {
        SetMode(EMissionBriefingMode::WEP);
    }
}

void UMissionBriefingDlg::OnMenuToggleHovered(UMenuButton* HoveredButton)
{
    if (!HoveredButton)
    {
        return;
    }

    if (USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        GI->PlayHoverSound(this);
    }
}

int32 UMissionBriefingDlg::CalcTimeOnTarget() const
{
    if (!MissionPtr)
    {
        return 0;
    }

    const auto& Elements = MissionPtr->GetElements();
    if (Elements.size() == 0)
    {
        return 0;
    }

    MissionElement* Element = Elements[0];
    if (!Element)
    {
        return 0;
    }

    FVector Loc(
        Element->Location().X,
        Element->Location().Y,
        Element->Location().Z
    );

    Swap(Loc.Y, Loc.Z);

    int32 MissionTime = MissionPtr->GetStart();

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        const int32 Action = static_cast<int32>(NavPt->GetAction());

        FVector NavLoc(
            NavPt->Location().X,
            NavPt->Location().Y,
            NavPt->Location().Z
        );

        const double Dist = FVector::Dist(Loc, NavLoc);

        const double Speed = NavPt->Speed();
        const int32 ETR = (Speed > 0.0)
            ? static_cast<int32>(Dist / Speed)
            : static_cast<int32>(Dist / 500.0);

        MissionTime += ETR;
        Loc = NavLoc;

        if (Action >= static_cast<int32>(INSTRUCTION_ACTION::ESCORT))
        {
            return MissionTime;
        }
    }

    return 0;
}

void UMissionBriefingDlg::OnCommit()
{
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

void UMissionBriefingDlg::HandleAcceptClicked()
{
    OnCommit();
}

void UMissionBriefingDlg::HandleCancelClicked()
{
    OnCancel();
}