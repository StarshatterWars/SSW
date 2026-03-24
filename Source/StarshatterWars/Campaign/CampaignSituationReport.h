/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSituationReport.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignSituationReport generates the situation report
    portion of the briefing for a dynamically generated
    mission in a dynamic campaign.
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "List.h"
#include "Text.h"

// Unreal minimal math (per your standard for .h files):
#include "Math/Vector.h"             // FVector
#include "Math/Color.h"              // FColor
#include "Math/UnrealMathUtility.h"  // FMath

// +--------------------------------------------------------------------+

class Campaign;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;

// +--------------------------------------------------------------------+

class CampaignSituationReport
{
public:
    static const char* TYPENAME() { return "CampaignSituationReport"; }

    CampaignSituationReport(Campaign* c, Mission* m);
    virtual ~CampaignSituationReport();

    virtual void      GenerateSituationReport();

protected:
    virtual void      GlobalSituation();
    virtual void      MissionSituation();
    virtual MissionElement* FindEscort(MissionElement* elem);
    virtual Text      GetThreatInfo();

    Campaign* campaign;
    Mission* mission;
    Text              sitrep;
};
