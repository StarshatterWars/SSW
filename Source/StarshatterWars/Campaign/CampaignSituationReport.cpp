/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CampaignSituationReport.cpp
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

#include "CampaignSituationReport.h"

#include "GameStructs.h"

#include "Campaign.h"
#include "Combatant.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Callsign.h"
#include "Mission.h"
#include "Instruction.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "StarSystem.h"
#include "Random.h"
#include "PlayerCharacter.h"
#include "GameStructs.h"
#include "ShipDesignRegistry.h"

#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

CampaignSituationReport::CampaignSituationReport(Campaign* c, Mission* m)
	: campaign(c), mission(m)
{
}

CampaignSituationReport::~CampaignSituationReport()
{
}

// +--------------------------------------------------------------------+

void
CampaignSituationReport::GenerateSituationReport()
{
	
	if (!campaign || !mission)
		return;

	sitrep = Text();

	UE_LOG(LogTemp, Warning, TEXT("[CampaignSituationReport] BEGIN"));
	GlobalSituation();
	MissionSituation();
	UE_LOG(LogTemp, Warning, TEXT("[CampaignSituationReport] Final sitrep='%s'"), ANSI_TO_TCHAR(sitrep.data()));
	mission->SetSituation(sitrep);
	UE_LOG(LogTemp, Warning, TEXT("[CampaignSituationReport] Mission->GetSituation()='%s'"), ANSI_TO_TCHAR(mission->GetSituation()));

	mission->SetSituation(sitrep);
}

// +--------------------------------------------------------------------+

static const char* outlooks[4] = { "good", "fluid", "poor", "bleak" };

void
CampaignSituationReport::GlobalSituation()
{
	if (campaign->GetTime() < 40 * 3600)
		sitrep = Text(campaign->GetName())
		+ Text(" is still in its early stages and the situation is ");
	else
		sitrep = Text("The overall outlook for ")
		+ Text(campaign->GetName())
		+ Text(" is ");

	int score = campaign->GetPlayerTeamScore();

	if (score > 1000)
		sitrep += outlooks[0];
	else if (score > -1000)
		sitrep += outlooks[1];
	else if (score > -2000)
		sitrep += outlooks[2];
	else
		sitrep += outlooks[3];

	sitrep += ".  ";

	Text strat_dir;

	CombatGroup* pg = campaign->GetPlayerGroup();

	if (pg)
		strat_dir = pg->GetStrategicDirection();

	if (strat_dir.length())
		sitrep += strat_dir;
	else
		sitrep += Text("Establishing and maintaining military control of the ")
		+ mission->GetStarSystem()->GetName()
		+ Text(" System remains a key priority.");
}

// +--------------------------------------------------------------------+

void
CampaignSituationReport::MissionSituation()
{
	if (mission) {
		MissionElement* player = mission->GetPlayer();
		MissionElement* target = mission->GetTarget();
		MissionElement* ward = mission->GetWard();
		MissionElement* escort = FindEscort(player);
		Text            threat = GetThreatInfo();
		Text            sector = mission->GetRegion();

		(void)escort; // currently unused in original code

		sector += " sector.";

		switch (mission->GetMissionType()) {
		case (int)EMissionType::PATROL:
		case (int)EMissionType::AIR_PATROL:
			sitrep += "\n\nThis mission is a routine patrol of the ";
			sitrep += sector;
			break;

		case (int)EMissionType::SWEEP:
		case (int)EMissionType::AIR_SWEEP:
			sitrep += "\n\nFor this mission, you will be performing a fighter sweep of the ";
			sitrep += sector;
			break;

		case (int)EMissionType::INTERCEPT:
		case (int)EMissionType::AIR_INTERCEPT:
			sitrep += "\n\nWe have detected hostile elements inbound.  ";
			sitrep += "Your mission is to intercept them before they are able to engage their targets.";
			break;

		case (int)EMissionType::STRIKE:
			sitrep += "\n\nThe goal of this mission is to perform a strike on preplanned targets in the ";
			sitrep += sector;

			if (target) {
				sitrep += "  Your package has been assigned to strike the ";

				if (target->GetCombatGroup())
					sitrep += target->GetCombatGroup()->GetDescription();
				else
					sitrep += target->GetName();

				sitrep += ".";
			}
			break;

		case (int)EMissionType::ASSAULT:
			sitrep += "\n\nThis mission is to assault preplanned targets in the ";
			sitrep += sector;

			if (target) {
				sitrep += "  Your package has been assigned to strike the ";

				if (target->GetCombatGroup())
					sitrep += target->GetCombatGroup()->GetDescription();
				else
					sitrep += target->GetName();

				sitrep += ".";
			}
			break;

		case (int)EMissionType::DEFEND:
			if (ward) {
				sitrep += "\n\nFor this mission, you will need to defend ";
				sitrep += ward->GetName();
				sitrep += " in the ";
				sitrep += sector;
			}
			else {
				sitrep += "\n\nThis is a defensive patrol mission in the ";
				sitrep += sector;
			}
			break;

		case (int)EMissionType::ESCORT:
			if (ward) {
				sitrep += "\n\nFor this mission, you will need to escort the ";
				sitrep += ward->GetName();
				sitrep += " in the ";
				sitrep += sector;
			}
			else {
				sitrep += "\n\nThis is an escort mission in the ";
				sitrep += sector;
			}
			break;

		case (int)EMissionType::ESCORT_FREIGHT:
			if (ward) {
				sitrep += "\n\nFor this mission, you will need to escort the freighter ";
				sitrep += ward->GetName();
				sitrep += ".";
			}
			else {
				sitrep += "\n\nThis is a freight escort mission in the ";
				sitrep += sector;
			}
			break;

		case (int)EMissionType::ESCORT_SHUTTLE:
			if (ward) {
				sitrep += "\n\nFor this mission, you will need to escort the shuttle ";
				sitrep += ward->GetName();
				sitrep += ".";
			}
			else {
				sitrep += "\n\nThis is a shuttle escort mission in the ";
				sitrep += sector;
			}
			break;

		case (int)EMissionType::ESCORT_STRIKE:
			if (ward) {
				sitrep += "\n\nFor this mission, you will need to protect the ";
				sitrep += ward->GetName();
				sitrep += " strike package from hostile interceptors.";
			}
			else {
				sitrep += "\n\nFor this mission, you will be responsible for strike escort duty.";
			}
			break;

		case (int)EMissionType::INTEL:
		case (int)EMissionType::SCOUT:
		case (int)EMissionType::RECON:
			sitrep += "\n\nThis is an intelligence gathering mission in the ";
			sitrep += sector;
			break;

		case (int)EMissionType::BLOCKADE:
			sitrep += "\n\nThis mission is part of the blockade operation in the ";
			sitrep += sector;
			break;

		case (int)EMissionType::FLEET:
			sitrep += "\n\nThis mission is a routine fleet patrol of the ";
			sitrep += sector;
			break;

		case (int)EMissionType::BOMBARDMENT:
			sitrep += "\n\nOur goal for this mission is to engage and destroy preplanned targets in the ";
			sitrep += sector;
			break;

		case (int)EMissionType::FLIGHT_OPS:
			sitrep += "\n\nFor this mission, the ";
			if (player)
				sitrep += player->GetName();
			else
				sitrep += "(unknown package)";

			sitrep += " will be conducting combat flight operations in the ";
			sitrep += sector;
			break;

		case (int)EMissionType::TRAINING:
			sitrep += "\n\nThis will be a training mission.";
			break;

		case (int)EMissionType::TRANSPORT:
		case (int)EMissionType::CARGO:
		case (int)EMissionType::OTHER:
		default:
			break;
		}

		if (threat.length()) {
			sitrep += "  ";
			sitrep += threat;
			sitrep += "\n\n";
		}
	}
	else {
		sitrep += "\n\n";
	}

	Text RankText;
	Text NameText;

	PlayerCharacter* PlayerPtr = PlayerCharacter::GetCurrentPlayer();

	if (PlayerPtr)
	{
		if (PlayerPtr->GetRank() > 6)
		{
			RankText = ", Admiral";
		}
		else
		{
			RankText = Text(", ") + PlayerCharacter::RankName(PlayerPtr->GetRank());
		}

		// PlayerPtr->Name() is now FString (UE-side), so convert it to legacy Text:
		NameText = Text(", ") + Text(TCHAR_TO_UTF8(*PlayerPtr->GetName()));
	}

	sitrep += "You have a mission to perform.  ";

	switch (RandomIndex()) {
	case  0: sitrep += "You'd better go get to it!";                     break;
	case  1: sitrep += "And let's be careful out there!";                break;
	case  2: sitrep += "Good luck, sir!";                                break;
	case  3: sitrep += "Let's keep up the good work out there.";         break;
	case  4: sitrep += "Don't lose your focus.";                         break;
	case  5: sitrep += "Good luck out there.";                           break;
	case  6: sitrep += "What are you waiting for, cocktail hour?";       break;
	case  7: sitrep += Text("Godspeed") + RankText + "!";                break;
	case  8: sitrep += Text("Good luck") + NameText + "!";               break;
	case  9: sitrep += Text("Good luck") + NameText + "!";               break;
	case 10: sitrep += "If everything is clear, get your team ready and get underway."; break;
	case 11: sitrep += Text("Go get to it") + RankText + "!";            break;
	case 12: sitrep += "The clock is ticking, so let's move it!";        break;
	case 13: sitrep += "Stay sharp out there!";                          break;
	case 14: sitrep += Text("Go get 'em") + RankText + "!";              break;
	case 15: sitrep += "Now get out of here and get to work!";           break;
	}
}

// +--------------------------------------------------------------------+

MissionElement*
CampaignSituationReport::FindEscort(MissionElement* player)
{
	MissionElement* escort = 0;

	if (!mission || !player)
		return escort;

	ListIter<MissionElement> iter = mission->GetElements();
	while (++iter) {
		MissionElement* elem = iter.value();
		(void)elem;

		// Original source had no implementation here.
		// Keep behavior unchanged (no escort selection).
	}

	return escort;
}

// +--------------------------------------------------------------------+

Text
CampaignSituationReport::GetThreatInfo()
{
	Text threat_info;

	int enemy_fighters = 0;
	int enemy_starships = 0;
	int enemy_sites = 0;

	if (mission && mission->GetPlayer()) {
		MissionElement* player = mission->GetPlayer();
		Text            rgn0 = player->GetRegion();
		Text            rgn1;
		int             iff = player->GetIFF();

		ListIter<Instruction> nav = player->NavList();
		while (++nav) {
			if (rgn0 != nav->GetRegionName())
				rgn1 = nav->GetRegionName();
		}

		ListIter<MissionElement> elem = mission->GetElements();
		while (++elem) {
			MissionElement* e = elem.value();
			if (!e)
				continue;

			if (e->GetIFF() <= 0 || e->GetIFF() == iff || e->GetIntelLevel() <= EIntel::SECRET)
				continue;

			const FShipDesign* Design = e->GetShipDesign();

			if (e->IsGroundUnit()) {
				if (!Design || Design->ShipType != (int)CLASSIFICATION::SAM)
					continue;

				if (e->GetRegion() != rgn0 && e->GetRegion() != rgn1)
					continue;
			}

			int mission_role = e->MissionRole();

			if (mission_role == (int)EMissionType::STRIKE ||
				mission_role == (int)EMissionType::INTEL ||
				mission_role == (int)EMissionType::CARGO ||
				mission_role == (int)EMissionType::TRANSPORT)
				continue;

			if (Design &&
				Design->ShipType >= (int)CLASSIFICATION::MINE &&
				Design->ShipType <= (int)CLASSIFICATION::DEFSAT) {
				enemy_sites += e->Count();
			}
			else if (e->IsDropship()) {
				enemy_fighters += e->Count();
			}
			else if (e->IsStarship()) {
				enemy_starships += e->Count();
			}
			else if (e->IsGroundUnit()) {
				enemy_sites += e->Count();
			}
		}
	}

	if (enemy_starships > 0) {
		threat_info = "We have reports of several enemy starships in the vicinity.";

		if (enemy_fighters > 0) {
			threat_info += "  Also be advised that enemy fighters may be operating nearby.";
		}
		else if (enemy_sites > 0) {
			threat_info += "  And be on the lookout for mines and defense satellites.";
		}
	}
	else if (enemy_fighters > 0) {
		threat_info = "We have reports of several enemy fighters in your operating area.";
	}
	else if (enemy_sites > 0) {
		if (mission->GetMissionType() >= (int)EMissionType::AIR_PATROL &&
			mission->GetMissionType() <= (int)EMissionType::STRIKE)
			threat_info = "Remember to check air-to-ground sensors for SAM and AAA sites.";
		else
			threat_info = "Be on the lookout for mines and defense satellites.";
	}
	else {
		threat_info = "We have no reliable information on threats in your operating area.";
	}

	return threat_info;
}