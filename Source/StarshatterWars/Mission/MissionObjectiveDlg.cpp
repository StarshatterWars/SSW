/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionObjectiveDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionObjectiveDlg

    - Unreal mission briefing situation/objectives panel
    - Simplified replacement for legacy MsnObjDlg
    - Shows situation and objectives on the left
    - Reserves right side for preview image / 3D ship widget later
    - Shows player craft caption on the right
*/

#include "MissionObjectiveDlg.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

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
}

void UMissionObjectiveDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
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

    UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] RefreshFromMission: MissionPtr=%s IsOK=%s Player=%s"),
        MissionPtr ? TEXT("VALID") : TEXT("NULL"),
        (MissionPtr && MissionPtr->IsOK()) ? TEXT("true") : TEXT("false"),
        (MissionPtr && MissionPtr->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"));

    RefreshSituationAndObjectives(MissionPtr);
    RefreshPlayerCaption(MissionPtr);
    RefreshPreviewPlaceholder(MissionPtr);
}

void UMissionObjectiveDlg::RefreshSituationAndObjectives(Mission* MissionPtr)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[MissionObjectiveDlg] Raw Situation='%s' Raw Objective='%s'"),
        MissionPtr ? ANSI_TO_TCHAR(MissionPtr->GetSituation()) : TEXT("NULL"),
        MissionPtr ? ANSI_TO_TCHAR(MissionPtr->GetObjective()) : TEXT("NULL")); 
    
    if (ObjectivesText)
    {
        if (MissionPtr)
        {
            if (MissionPtr->IsOK())
            {
                ObjectivesText->SetText(FText::FromString(UTF8_TO_TCHAR(MissionPtr->GetObjective())));
            }
            else
            {
                ObjectivesText->SetText(FText::GetEmpty());
            }
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
            if (MissionPtr->IsOK())
            {
                SituationText->SetText(FText::FromString(UTF8_TO_TCHAR(MissionPtr->GetSituation())));
            }
            else
            {
                FString ErrorText = TEXT("MISSION ERRORS");
                const char* ErrorMsg = MissionPtr->ErrorMessage();

                if (ErrorMsg && ErrorMsg[0])
                {
                    ErrorText += TEXT("\n\n");
                    ErrorText += UTF8_TO_TCHAR(ErrorMsg);
                }

                SituationText->SetText(FText::FromString(ErrorText));
            }
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

    if (!MissionPtr || !MissionPtr->IsOK())
    {
        return;
    }

    MissionElement* PlayerElem = MissionPtr->GetPlayer();
    if (!PlayerElem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] No player element"));
        return;
    }

    const ShipDesign* Design = PlayerElem->GetDesign();

    FString Caption;

    if (Design)
    {
        const FString Abbrev = ANSI_TO_TCHAR(Design->abrv);
        const FString DisplayName = ANSI_TO_TCHAR(Design->display_name);
        const FString ElemName = ANSI_TO_TCHAR(PlayerElem->GetName().data());

        // Fallback-safe formatting
        if (Design->type <= (int)CLASSIFICATION::ATTACK)
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                Abbrev.IsEmpty() ? TEXT("UNIT") : *Abbrev,
                DisplayName.IsEmpty() ? TEXT("UNKNOWN") : *DisplayName);
        }
        else
        {
            Caption = FString::Printf(
                TEXT("%s %s"),
                Abbrev.IsEmpty() ? TEXT("UNIT") : *Abbrev,
                ElemName.IsEmpty() ? TEXT("UNKNOWN") : *ElemName);
        }
    }
    else
    {
        // HARD fallback when design is missing
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionObjectiveDlg] Player design is NULL for '%s'"),
            ANSI_TO_TCHAR(PlayerElem->GetName().data()));

        Caption = ANSI_TO_TCHAR(PlayerElem->GetName().data());
        if (Caption.IsEmpty())
        {
            Caption = TEXT("UNKNOWN UNIT");
        }
    }

    PlayerCaptionText->SetText(FText::FromString(Caption));
}

void UMissionObjectiveDlg::RefreshPreviewPlaceholder(Mission* MissionPtr)
{
    if (!PreviewImage)
    {
        return;
    }

    // Placeholder for now.
    // Later this can be replaced with:
    // - render target preview
    // - ship actor scene capture
    // - fighter / starship image
    PreviewImage->SetVisibility(ESlateVisibility::Visible);

    if (MissionPtr && MissionPtr->IsOK() && MissionPtr->GetPlayer())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] RefreshPreviewPlaceholder: player preview available"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionObjectiveDlg] RefreshPreviewPlaceholder: no valid player preview yet"));
    }
}