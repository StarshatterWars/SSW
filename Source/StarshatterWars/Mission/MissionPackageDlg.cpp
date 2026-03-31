/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionPackageDlg.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

#include "Mission.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "GameStructs_System.h"

#include "MissionPackageListObject.h"
#include "MissionNavListObject.h"

UMissionPackageDlg::UMissionPackageDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionPackageDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (PackageList)
    {
        PackageList->OnItemSelectionChanged().Clear();
        PackageList->OnItemSelectionChanged().AddUObject(this, &UMissionPackageDlg::OnPackageSelectionChanged);
    }
}

void UMissionPackageDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

Mission* UMissionPackageDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

void UMissionPackageDlg::RefreshFromMission()
{
    PackageIndex = INDEX_NONE;

    DrawPackages();
    DrawNavPlan();
    DrawThreats();
}

MissionElement* UMissionPackageDlg::ResolveSelectedPackageElement() const
{
    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
        return nullptr;

    int32 VisibleIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
            continue;

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
            continue;

        if (Elem->GetIFF() == MissionPtr->GetTeam() &&
            !Elem->IsSquadron() &&
            Elem->GetRegion() == MissionPtr->GetRegion() &&
            Design->ShipType < (int32)CLASSIFICATION::STATION)
        {
            if (VisibleIndex == PackageIndex)
                return Elem;

            ++VisibleIndex;
        }
    }

    return nullptr;
}

void UMissionPackageDlg::DrawPackages()
{
    Mission* MissionPtr = ResolveMission();

    PackageItems.Empty();
    if (PackageList)
        PackageList->ClearListItems();

    if (!MissionPtr || !PackageList)
        return;

    int32 VisibleIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
            continue;

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
            continue;

        if (Elem->GetIFF() == MissionPtr->GetTeam() &&
            !Elem->IsSquadron() &&
            Elem->GetRegion() == MissionPtr->GetRegion() &&
            Design->ShipType < (int32)CLASSIFICATION::STATION)
        {
            UMissionPackageListObject* Item = NewObject<UMissionPackageListObject>(this);
            if (!Item)
                continue;

            Item->InitFromMissionElement(Elem, VisibleIndex, Elem->IsPlayer());

            PackageItems.Add(Item);
            PackageList->AddItem(Item);

            if (Elem->IsPlayer() && PackageIndex == INDEX_NONE)
            {
                PackageIndex = VisibleIndex;
            }

            ++VisibleIndex;
        }
    }

    if (PackageIndex == INDEX_NONE && PackageItems.Num() > 0)
    {
        PackageIndex = 0;
    }

    if (PackageItems.IsValidIndex(PackageIndex))
    {
        PackageList->SetSelectedItem(PackageItems[PackageIndex]);
    }
}

void UMissionPackageDlg::DrawNavPlan()
{
    Mission* MissionPtr = ResolveMission();

    NavItems.Empty();
    if (NavList)
        NavList->ClearListItems();

    if (!MissionPtr || !NavList)
        return;

    MissionElement* Element = ResolveSelectedPackageElement();
    if (!Element)
        return;

    FVector Loc = Element->GetLocation();
    int32 NavIndex = 0;

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        Instruction* Nav = NavPt.value();
        if (!Nav)
            continue;

        const double Dist = FVector::Dist(Loc, Nav->Location());

        UMissionNavListObject* Item = NewObject<UMissionNavListObject>(this);
        if (Item)
        {
            Item->InitFromInstruction(Nav, NavIndex, Dist);
            NavItems.Add(Item);
            NavList->AddItem(Item);
        }

        Loc = Nav->Location();
        NavIndex++;
    }
}

void UMissionPackageDlg::DrawThreats()
{
    auto SetThreat = [](UTextBlock* Block, const FString& Value)
        {
            if (Block)
                Block->SetText(FText::FromString(Value));
        };

    SetThreat(Threat0, TEXT(""));
    SetThreat(Threat1, TEXT(""));
    SetThreat(Threat2, TEXT(""));
    SetThreat(Threat3, TEXT(""));
    SetThreat(Threat4, TEXT(""));

    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
        return;

    MissionElement* Player = MissionPtr->GetPlayer();
    if (!Player)
        return;

    FVector BaseLoc = Player->GetLocation();

    int32 ThreatIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
            continue;

        if (Elem->GetIFF() == 0 ||
            Elem->GetIFF() == Player->GetIFF() ||
            Elem->IntelLevel() <= Intel::SECRET)
            continue;

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
            continue;

        const double Dist = FVector::Dist(BaseLoc, Elem->GetLocation());

        FString Text = FString::Printf(
            TEXT("%d %s - %.0f"),
            Elem->Count(),
            Design->Abrv.IsEmpty() ? TEXT("UNK") : *Design->Abrv,
            Dist);

        if (ThreatIndex == 0) SetThreat(Threat0, Text);
        if (ThreatIndex == 1) SetThreat(Threat1, Text);
        if (ThreatIndex == 2) SetThreat(Threat2, Text);
        if (ThreatIndex == 3) SetThreat(Threat3, Text);
        if (ThreatIndex == 4) SetThreat(Threat4, Text);

        ThreatIndex++;
        if (ThreatIndex >= 5)
            break;
    }
}

void UMissionPackageDlg::OnPackageSelectionChanged(UObject* Item)
{
    UMissionPackageListObject* PackageItem = Cast<UMissionPackageListObject>(Item);
    if (!PackageItem)
        return;

    PackageIndex = PackageItem->GetIndex();
    DrawNavPlan();
}