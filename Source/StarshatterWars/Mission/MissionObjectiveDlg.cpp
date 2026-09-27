#include "MissionObjectiveDlg.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "ShipDesignRegistry.h"

#include "Mission.h"
#include "MissionElement.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "GameStructs.h"

UMissionObjectiveDlg::UMissionObjectiveDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionObjectiveDlg::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] NativeConstruct"));
}

void UMissionObjectiveDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] SetParentDlg: %s"),
        ParentDlg ? TEXT("VALID") : TEXT("NULL"));
}

Mission* UMissionObjectiveDlg::ResolveMission() const
{
    if (!ParentDlg)
    {
        return nullptr;
    }

    return ParentDlg->GetMissionPtr();
}

void UMissionObjectiveDlg::RefreshFromMission()
{
    Mission* MissionPtr = ResolveMission();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] RefreshFromMission: Mission=%p Name='%s' Sit='%s' Obj='%s' Scripted=%s"),
        MissionPtr,
        (MissionPtr && MissionPtr->GetName()) ? ANSI_TO_TCHAR(MissionPtr->GetName()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->GetSituation()) ? UTF8_TO_TCHAR(MissionPtr->GetSituation()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->GetObjective()) ? UTF8_TO_TCHAR(MissionPtr->GetObjective()) : TEXT("NULL"),
        (MissionPtr && MissionPtr->IsScripted()) ? TEXT("true") : TEXT("false"));

    RefreshSituationAndObjectives(MissionPtr);
    RefreshPlayerCaption(MissionPtr);
    RefreshPreviewPlaceholder(MissionPtr);
}

void UMissionObjectiveDlg::RefreshSituationAndObjectives(Mission* MissionPtr)
{
    const FString SitStr = (MissionPtr && MissionPtr->GetSituation())
        ? FString(UTF8_TO_TCHAR(MissionPtr->GetSituation()))
        : FString();

    const FString ObjStr = (MissionPtr && MissionPtr->GetObjective())
        ? FString(UTF8_TO_TCHAR(MissionPtr->GetObjective()))
        : FString();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] RefreshSituationAndObjectives: Mission=%p IsOK=%s Sit='%s' Obj='%s'"),
        MissionPtr,
        (MissionPtr && MissionPtr->IsOK()) ? TEXT("true") : TEXT("false"),
        *SitStr,
        *ObjStr);

    if (ObjectivesText)
    {
        if (MissionPtr)
        {
            ObjectivesText->SetText(
                ObjStr.IsEmpty()
                ? FText::FromString(TEXT("NO OBJECTIVES"))
                : FText::FromString(ObjStr));
        }
        else
        {
            ObjectivesText->SetText(FText::FromString(TEXT("NO MISSION")));
        }
    }

    if (SituationText)
    {
        if (MissionPtr)
        {
            SituationText->SetText(
                SitStr.IsEmpty()
                ? FText::FromString(TEXT("NO SITUATION"))
                : FText::FromString(SitStr));
        }
        else
        {
            SituationText->SetText(FText::FromString(TEXT("NO MISSION")));
        }
    }
}

void UMissionObjectiveDlg::RefreshPlayerCaption(Mission* MissionPtr)
{
    if (!PlayerCaptionText)
    {
        return;
    }

    PlayerCaptionText->SetText(FText::GetEmpty());

    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] RefreshPlayerCaption: MissionPtr is null"));
        return;
    }

    MissionElement* PlayerElem = MissionPtr->GetPlayer();
    if (!PlayerElem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] No player element"));
        return;
    }

    const FShipDesign* Design = PlayerElem->GetShipDesign();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] Player='%s' Design=%s"),
        ANSI_TO_TCHAR(PlayerElem->GetName().data()),
        Design ? TEXT("VALID") : TEXT("NULL"));

    if (Design)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Design ship='%s' display='%s' abrv='%s' type=%d"),
            *Design->ShipName,
            *Design->DisplayName,
            *Design->Abrv,
            Design->ShipType);
    }

    FString Caption;

    if (Design)
    {
        const FString Abbrev =
            !Design->Abrv.IsEmpty() ? Design->Abrv : TEXT("UNIT");

        const FString DisplayName =
            !Design->DisplayName.IsEmpty() ? Design->DisplayName : Design->ShipName;

        const FString ElemName = ANSI_TO_TCHAR(PlayerElem->GetName().data());

        if (Design->ShipType <= (int32)CLASSIFICATION::ATTACK)
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                *Abbrev,
                DisplayName.IsEmpty() ? TEXT("UNKNOWN") : *DisplayName);
        }
        else
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                *Abbrev,
                ElemName.IsEmpty() ? TEXT("UNKNOWN") : *ElemName);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Caption from FShipDesign: '%s'"),
            *Caption);
    }
    else
    {
        Caption = ANSI_TO_TCHAR(PlayerElem->GetName().data());

        if (Caption.IsEmpty())
        {
            Caption = TEXT("UNKNOWN UNIT");
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Caption fallback from player element name: '%s'"),
            *Caption);
    }

    PlayerCaptionText->SetText(FText::FromString(Caption));
}

void UMissionObjectiveDlg::RefreshPreviewPlaceholder(Mission* MissionPtr)
{
    if (!PreviewImage)
    {
        return;
    }

    PreviewImage->SetVisibility(ESlateVisibility::Visible);

    if (MissionPtr && MissionPtr->IsOK() && MissionPtr->GetPlayer())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Preview: player available"));
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Preview: no valid player"));
    }
}