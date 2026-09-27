/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSaveGame.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignSaveGame contains the logic needed to save and load
    campaign games in progress.
*/

#pragma once

#include "Types.h"
#include "term.h"
#include "List.h"
#include "Text.h"

// Minimal Unreal includes (per port rules):
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // FMath

// +--------------------------------------------------------------------+

class Campaign;
class CampaignPlan;
class Combatant;
class CombatGroup;
class CombatZone;
class DataLoader;
class Mission;
class PlayerCharacter;
class StarSystem;

// +--------------------------------------------------------------------+

class CampaignSaveGame
{
public:
    static const char* TYPENAME() { return "CampaignSaveGame"; }

    CampaignSaveGame(Campaign* c = 0);
    virtual ~CampaignSaveGame();

    virtual Campaign* GetCampaign() { return campaign; }

    virtual void      Load(const char* name);
    virtual void      Save(const char* name);
    static  void      Delete(const char* name);
    static  void      RemovePlayer(PlayerCharacter* p);

    virtual void      LoadAuto();
    virtual void      SaveAuto();

    static  Text      GetResumeFile();
    static  int       GetSaveGameList(List<Text>& save_list);

private:
    static  Text      GetSaveDirectory();
    static  Text      GetSaveDirectory(PlayerCharacter* p);
    static  void      CreateSaveDirectory();

    Campaign* campaign;
};
