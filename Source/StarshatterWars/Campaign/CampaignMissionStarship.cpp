/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignMissionStarship.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignMissionStarship generates missions and mission
    info for the player's STARSHIP GROUP as part of a
    dynamic campaign.
*/

#include "CampaignMissionStarship.h"

#include "CampaignMissionRequest.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Callsign.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionTemplate.h"
#include "Instruction.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Starshatter.h"
#include "StarSystem.h"
#include "PlayerCharacter.h"
#include "MissionLoad.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "StarshatterWarsLog.h"
#include "ShipDesignRegistry.h"

// Unreal:
#include "Math/Vector.h"               // FVector
#include "Math/UnrealMathUtility.h"    // FMath
#include "Logging/LogMacros.h"         // UE_LOG

static int pkg_id = 1000;
static int dump_missions;

// +--------------------------------------------------------------------+

CampaignMissionStarship::CampaignMissionStarship(Campaign* c)
    : campaign(c),
    player_group(0),
    player_unit(0),
    mission(0),
    player(0),
    strike_group(0),
    strike_target(0),
    prime_target(0),
    ward(0),
    escort(0),
    ownside(0),
    enemy(-1),
    mission_type(0)
{
    if (!campaign || !campaign->GetPlayerGroup()) {
        UE_LOG(LogStarshatterWars, Error,
            TEXT("ERROR - CMS campaign=%p player_group=%p"),
            campaign,
            campaign ? campaign->GetPlayerGroup() : nullptr);
        return;
    }

    player_group = campaign->GetPlayerGroup();
    player_unit = campaign->GetPlayerUnit();
}

CampaignMissionStarship::~CampaignMissionStarship() {}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateMission(CampaignMissionRequest* req)
{
    if (!campaign || !req)
        return;
    UE_LOG(LogStarshatterWars, Log, TEXT("-----------------------------------------------"));

    const TCHAR* RoleT = ANSI_TO_TCHAR(Mission::GetRoleName(req->Type()));

    if (req->Script().Len() > 0)
    {
        // req->Script() is an FString:
        UE_LOG(LogStarshatterWars, Log, TEXT("CMS CreateMission() request: %s '%s'"),
            RoleT,
            *req->Script());
    }
    else
    {
        // ObjName is a Starshatter Text/C-string:
        const char* ObjNameA = "(no target)";
        if (req->GetObjective())
            ObjNameA = req->GetObjective()->GetName().data();

        UE_LOG(LogStarshatterWars, Log, TEXT("CMS CreateMission() request: %s %s"),
            RoleT,
            ANSI_TO_TCHAR(ObjNameA));
    }

    request = req;

    // Resolve player group/unit from request:
    CombatGroup* Primary = request->GetPrimaryGroup();
    if (Primary && Primary != player_group) {
        player_group = Primary;
        player_unit = nullptr;
    }

    if (!player_group) {
        UE_LOG(LogStarshatterWars, Warning, TEXT("CMS CreateMission(): no player_group (null primary group)."));
        return;
    }

    ownside = player_group->GetIFF();
    mission_info = nullptr;

    // Recompute enemy each time:
    enemy = -1;
    for (int i = 0; i < campaign->GetCombatants().size(); i++) {
        Combatant* c = campaign->GetCombatants().at(i);
        if (!c)
            continue;

        const int iff = c->GetIFF();
        if (iff > 0 && iff != ownside) {
            enemy = iff;
            break;
        }
    }

    static int id_key = 1;

    mission = GenerateMission(id_key++);
    
    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen] After GenerateMission: Name=%s IsOK=%s Player=%s"),
        mission ? ANSI_TO_TCHAR(mission->GetName()) : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"));

    if (!mission) {
        UE_LOG(LogStarshatterWars, Warning, TEXT("CMS CreateMission(): GenerateMission failed."));
        return;
    }

    // Safe to call; sets player if it can locate player element:
    CreatePlayer();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen] After CreatePlayer: IsOK=%s Player=%s"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"));

    DefineMissionObjectives();

    MissionInfo* info = DescribeMission();

    if (info) {
        campaign->GetMissionList().append(info);

        UE_LOG(
            LogStarshatterWars,
            Log,
            TEXT("CMS Created %03d '%s' %s"),
            info->id,
            ANSI_TO_TCHAR(info->name),
            ANSI_TO_TCHAR(Mission::GetRoleName(mission->GetType()))
        );

        if (dump_missions) {
            Text script = mission->Serialize();
            char fname[32] = { 0 };

            sprintf_s(fname, sizeof(fname), "msn%03d.def", info->id);

            FILE* f = nullptr;
            fopen_s(&f, fname, "w");
            if (f) {
                fprintf(f, "%s\n", script.data());
                fclose(f);
            }
        }
    }
    else {
        UE_LOG(LogStarshatterWars, Warning, TEXT("CMS failed to create mission."));
    }
}

// +--------------------------------------------------------------------+

Mission*
CampaignMissionStarship::GenerateMission(int id)
{
    bool found = false;

    SelectType();

    const bool bHasRequest = (request != nullptr);

    // ------------------------------------------------------------
    // CASE 1: Explicit scripted request (still legacy/file-backed)
    // ------------------------------------------------------------
    if (bHasRequest && request->Script().Len() > 0)
    {
        MissionTemplate* mt = new MissionTemplate(
            id,
            TCHAR_TO_ANSI(*request->Script()),
            campaign->Path()
        );

        if (mt)
        {
            mt->SetPlayerSquadron(player_group);
        }

        mission = mt;
        found = (mission != nullptr);
    }

    // ------------------------------------------------------------
    // CASE 2: Use campaign mission data from DT_Campaign
    // ------------------------------------------------------------
    else
    {
        const FS_CampaignMission* mission_data =
            campaign->FindCampaignMissionData(mission_type, player_group);

        found = (mission_data != nullptr);

        if (found)
        {
            mission = new Mission(id);

            if (mission)
            {
                mission->SetType(mission_type);

                if (!mission->LoadFromCampaignMissionData(*mission_data))
                {
                    delete mission;
                    mission = nullptr;
                    found = false;
                }
            }
        }

        if (!found)
        {
            mission = new Mission(id);
            if (mission)
            {
                mission->SetType(mission_type);
            }
        }
    }

    // request is still needed for timing in current logic
    if (!mission || !player_group || !bHasRequest)
    {
        Exit();
        return nullptr;
    }

    char name[64] = { 0 };
    sprintf_s(name, sizeof(name), "Starship Mission %d", id);

    mission->SetName(name);
    mission->SetTeam(player_group->GetIFF());
    mission->SetStart(request->StartTime());

    SelectRegion();
    GenerateStandardElements();

    if (!found)
    {
        GenerateMissionElements();
        mission->SetOK(true);
        mission->Validate();
    }
    else
    {
        CreatePlayer();

        if (mission->IsOK())
        {
            player = mission->GetPlayer();
            prime_target = mission->GetTarget();
            ward = mission->GetWard();
        }
        else
        {
            delete mission;
            mission = new Mission(id);

            if (!mission)
            {
                Exit();
                return nullptr;
            }

            mission->SetType(mission_type);
            mission->SetName(name);
            mission->SetTeam(player_group->GetIFF());
            mission->SetStart(request->StartTime());

            SelectRegion();
            GenerateStandardElements();
            GenerateMissionElements();

            mission->SetOK(true);
            mission->Validate();
        }
    }

    return mission;
}

void
CampaignMissionStarship::SelectType()
{
    if (request)
        mission_type = request->Type();
    else
        mission_type = (int)EMISSIONTYPE::PATROL;

    if (player_unit && player_unit->GetShipClass() == (int)CLASSIFICATION::CARRIER)
        mission_type = (int)EMISSIONTYPE::FLIGHT_OPS;
}

void
CampaignMissionStarship::SelectRegion()
{
    if (!player_group) {
        UE_LOG(LogStarshatterWars, Warning, TEXT("WARNING: CMS - no player group in SelectRegion"));
        return;
    }

    CombatZone* zone = player_group->GetAssignedZone();
    if (!zone)
        zone = player_group->GetCurrentZone();

    if (zone) {
        mission->SetStarSystem(campaign->GetSystem(zone->GetSystem()));

        if (zone->HasRegion(player_group->GetRegion()))
            mission->SetRegion(player_group->GetRegion());
        else
            mission->SetRegion(*zone->GetRegions().at(0));
    }
    else {
        UE_LOG(LogStarshatterWars, Warning, TEXT("WARNING: CMS - No zone for '%s'"),
            ANSI_TO_TCHAR(player_group->GetName().data()));

        StarSystem* s = campaign->GetSystemList()[0];

        mission->SetStarSystem(s);
        mission->SetRegion(s->Regions()[0]->GetName());
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::GenerateStandardElements()
{
    ListIter<CombatZone> z = campaign->GetZones();
    while (++z) {
        ListIter<ZoneForce> iter = z->GetForces();
        while (++iter) {
            ZoneForce* force = iter.value();
            ListIter<CombatGroup> group = force->GetGroups();

            while (++group) {
                CombatGroup* g = group.value();

                switch (g->GetType()) {
                case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
                case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
                case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
                case ECOMBATGROUP_TYPE::LCA_SQUADRON:
                    CreateSquadron(g);
                    break;

                case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
                case ECOMBATGROUP_TYPE::BATTLE_GROUP:
                case ECOMBATGROUP_TYPE::CARRIER_GROUP:
                    CreateElements(g);
                    break;

                case ECOMBATGROUP_TYPE::MINEFIELD:
                case ECOMBATGROUP_TYPE::BATTERY:
                case ECOMBATGROUP_TYPE::MISSILE:
                case ECOMBATGROUP_TYPE::STATION:
                case ECOMBATGROUP_TYPE::STARBASE:
                case ECOMBATGROUP_TYPE::SUPPORT:
                case ECOMBATGROUP_TYPE::COURIER:
                case ECOMBATGROUP_TYPE::MEDICAL:
                case ECOMBATGROUP_TYPE::SUPPLY:
                case ECOMBATGROUP_TYPE::REPAIR:
                    CreateElements(g);
                    break;

                case ECOMBATGROUP_TYPE::CIVILIAN:
                case ECOMBATGROUP_TYPE::WAR_PRODUCTION:
                case ECOMBATGROUP_TYPE::FACTORY:
                case ECOMBATGROUP_TYPE::REFINERY:
                case ECOMBATGROUP_TYPE::RESOURCE:
                case ECOMBATGROUP_TYPE::INFRASTRUCTURE:
                case ECOMBATGROUP_TYPE::TRANSPORT:
                case ECOMBATGROUP_TYPE::NETWORK:
                case ECOMBATGROUP_TYPE::HABITAT:
                case ECOMBATGROUP_TYPE::STORAGE:
                case ECOMBATGROUP_TYPE::NON_COM:
                    CreateElements(g);
                    break;
                }
            }
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::GenerateMissionElements()
{
    CreatePlayer();
    CreateWards();
    CreateTargets();

    if (ward && player) {
        Instruction* obj = new Instruction(INSTRUCTION_ACTION::ESCORT, ward->GetName());

        if (obj) {
            switch (mission->GetType()) {
            case (int)EMISSIONTYPE::ESCORT_FREIGHT:
                obj->SetTargetDesc(Text("the star freighter ") + ward->GetName());
                break;

            case (int)EMISSIONTYPE::ESCORT_SHUTTLE:
                obj->SetTargetDesc(Text("the shuttle ") + ward->GetName());
                break;

            case (int)EMISSIONTYPE::ESCORT_STRIKE:
                obj->SetTargetDesc(Text("the ") + ward->GetName() + Text(" strike package"));
                break;

            default:
                if (ward->GetCombatGroup()) {
                    obj->SetTargetDesc(Text("the ") + ward->GetCombatGroup()->GetDescription());
                }
                else {
                    obj->SetTargetDesc(Text("the ") + ward->GetName());
                }
                break;
            }

            player->AddObjective(obj);
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreatePlayer()
{
    MissionElement* elem = 0;

    if (player_group) {
        ListIter<MissionElement> iter = mission->GetElements();
        while (++iter) {
            MissionElement* e = iter.value();
            if (e->GetCombatGroup() == player_group) {
                player_group_elements.append(e);

                if ((!player_unit && !elem) || (player_unit == e->GetCombatUnit())) {
                    elem = e;
                }
            }
        }
    }

    if (elem) {
        elem->SetPlayer(1);
        elem->SetCommandAI(0);
        player = elem;
    }
    else if (player_group) {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS GenerateMissionElements() could not find player element '%s'"),
            ANSI_TO_TCHAR(player_group->GetName().data()));
    }
    else {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS GenerateMissionElements() could not find player element (no player group)"));
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateElements(CombatGroup* g)
{
    MissionElement* elem = 0;
    List<CombatUnit>& units = g->GetUnits();

    CombatUnit* cmdr = 0;

    for (int i = 0; i < units.size(); i++) {
        elem = CreateSingleElement(g, units[i]);

        if (elem) {
            if (!cmdr) {
                cmdr = units[i];

                if (player_group && player_group->GetIFF() == g->GetIFF()) {
                    PlayerCharacter* playerc = PlayerCharacter::GetCurrentPlayer();
                    if (playerc && playerc->GetRank() >= 10) {
                        elem->SetCommander(player_group->GetName());
                    }
                }
            }
            else {
                elem->SetCommander(cmdr->GetName());

                if (g->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP &&
                    elem->MissionRole() == (int)EMISSIONTYPE::ESCORT) {
                    Instruction* obj = new Instruction(INSTRUCTION_ACTION::ESCORT, cmdr->GetName());
                    if (obj) {
                        obj->SetTargetDesc(Text("the ") + g->GetDescription());
                        elem->AddObjective(obj);
                    }
                }
            }

            mission->AddElement(elem);
        }
    }
}

MissionElement*
CampaignMissionStarship::CreateSingleElement(CombatGroup* g, CombatUnit* u)
{
    if (!g || g->IsReserve())        return nullptr;
    if (!u || u->LiveCount() < 1)    return nullptr;
    if (!mission->GetStarSystem())   return nullptr;

    StarSystem* system = mission->GetStarSystem();
    OrbitalRegion* rgn = system->FindRegion(u->GetRegion());

    if (!rgn || rgn->GetType() == Orbital::TERRAIN)
        return nullptr;

    // ensure this unit isn't already in the mission:
    ListIter<MissionElement> e_iter = mission->GetElements();
    while (++e_iter) {
        MissionElement* elem = e_iter.value();
        if (elem->GetCombatUnit() == u)
            return nullptr;
    }

    // Resolve Unreal design row:
    const FShipDesign* ShipRow = nullptr;

    if (u->GetDesignName().length() > 0)
    {
        ShipRow = ShipDesignRegistry::Find(u->GetDesignName().data());
    }

    if (!ShipRow && u->GetDesign())
    {
        ShipRow = ShipDesignRegistry::Find(u->GetDesign()->name);
    }

    if (!ShipRow)
    {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS CreateSingleElement: missing ship design row for group '%s', unit '%s', design '%s'"),
            ANSI_TO_TCHAR(g->GetName().data()),
            ANSI_TO_TCHAR(u->GetName().data()),
            ANSI_TO_TCHAR(u->GetDesignName().data()));
        return nullptr;
    }

    MissionElement* elem = new MissionElement;
    if (!elem) {
        Exit();
        return nullptr;
    }

    if (u->GetName().length())
        elem->SetName(u->GetName());
    else
        elem->SetName(u->GetDesignName());

    elem->SetElementID(pkg_id++);

    elem->SetShipDesign(ShipRow);
    elem->SetCount(u->LiveCount());
    elem->SetIFF(u->GetIFF());
    elem->SetIntelLevel(g->GetIntelLevel());
    elem->SetRegion(u->GetRegion());
    elem->SetHeading(u->GetHeading());

    const int   unit_index = g->GetUnits().index(u);
    FVector     base_loc = u->Location();
    bool        exact = u->IsStatic();

    if (base_loc.Length() < 1.0f) {
        base_loc = g->GetLocation();
        exact = false;
    }

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    if (unit_index < 0 || (unit_index > 0 && !exact)) {
        FVector loc;

        if (!u->IsStatic()) {
            loc = ScatterInSphere(1.0f);
            loc *= (10000.0f + 9000.0f * (float)unit_index);
        }
        else {
            loc = ScatterInSphere(1.0f);
            loc *= (2000.0f + 2000.0f * (float)unit_index);
        }

        elem->SetLocation(base_loc + loc);
    }
    else {
        elem->SetLocation(base_loc);
    }

    if (g->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP) {
        if (ShipRow->ShipType == (int32)CLASSIFICATION::CARRIER) {
            elem->SetMissionRole((int)EMISSIONTYPE::FLIGHT_OPS);
        }
        else {
            elem->SetMissionRole((int)EMISSIONTYPE::ESCORT);
        }
    }
    else if (ShipRow->ShipType == (int32)CLASSIFICATION::STATION ||
        ShipRow->ShipType == (int32)CLASSIFICATION::FARCASTER) {
        elem->SetMissionRole((int)EMISSIONTYPE::OTHER);

        if (ShipRow->ShipType == (int32)CLASSIFICATION::FARCASTER) {
            Text name = u->GetName();
            int  dash = -1;

            for (int i = 0; i < (int)name.length(); i++)
                if (name[i] == '-')
                    dash = i;

            Text src = name.substring(0, dash);
            Text dst = name.substring(dash + 1, name.length() - (dash + 1));

            Instruction* obj = new Instruction(INSTRUCTION_ACTION::VECTOR, dst + "-" + src);
            if (obj)
                elem->AddObjective(obj);
        }
    }
    else if ((ShipRow->ShipType & (int32)CLASSIFICATION::STARSHIPS) != 0) {
        elem->SetMissionRole((int)EMISSIONTYPE::FLEET);
    }

    elem->SetCombatGroup(g);
    elem->SetCombatUnit(u);

    return elem;
}

CombatUnit*
CampaignMissionStarship::FindCarrier(CombatGroup* g)
{
    CombatGroup* carrier = g->FindCarrier();

    if (carrier && carrier->GetUnits().size()) {
        MissionElement* carrier_elem = mission->FindElement(carrier->GetName());
        if (carrier_elem)
            return carrier->GetUnits().at(0);
    }

    return nullptr;
}

void
CampaignMissionStarship::CreateSquadron(CombatGroup* g)
{
    if (!g || g->IsReserve())
        return;

    CombatUnit* fighter = g->GetUnits().at(0);
    CombatUnit* carrier = FindCarrier(g);

    if (!fighter || !carrier)
        return;

    const FShipDesign* ShipRow = nullptr;

    if (fighter->GetDesignName().length() > 0)
    {
        ShipRow = ShipDesignRegistry::Find(fighter->GetDesignName().data());
    }

    if (!ShipRow && fighter->GetDesign())
    {
        ShipRow = ShipDesignRegistry::Find(fighter->GetDesign()->name);
    }

    if (!ShipRow)
    {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS CreateSquadron: missing ship design row for squadron '%s', unit '%s', design '%s'"),
            ANSI_TO_TCHAR(g->GetName().data()),
            ANSI_TO_TCHAR(fighter->GetName().data()),
            ANSI_TO_TCHAR(fighter->GetDesignName().data()));
        return;
    }

    int live_count = fighter->LiveCount();
    int maint_count = (live_count > 4) ? live_count / 2 : 0;

    MissionElement* elem = new MissionElement;
    if (!elem) {
        Exit();
        return;
    }

    elem->SetName(g->GetName());
    elem->SetElementID(pkg_id++);

    elem->SetShipDesign(ShipRow);
    elem->SetCount(fighter->Count());
    elem->SetDeadCount(fighter->DeadCount());
    elem->SetMaintCount(maint_count);
    elem->SetIFF(fighter->GetIFF());
    elem->SetIntelLevel(g->GetIntelLevel());
    elem->SetRegion(fighter->GetRegion());

    elem->SetCarrier(carrier->GetName());
    elem->SetCommander(carrier->GetName());

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    elem->SetLocation(carrier->Location() + ScatterInSphere(1.0f));

    elem->SetCombatGroup(g);
    elem->SetCombatUnit(fighter);

    mission->AddElement(elem);
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateWards()
{
    switch (mission->GetType()) {
    case (int)EMISSIONTYPE::ESCORT_FREIGHT:
        CreateWardFreight();
            break;
    default:              
        break;
    }
}

void
CampaignMissionStarship::CreateWardFreight()
{
    if (!mission || !mission->GetStarSystem() || !player_group)
        return;

    CombatGroup* freight = 0;

    if (request)
        freight = request->GetObjective();

    if (!freight)
        freight = campaign->FindGroup(ownside, (int) ECOMBATGROUP_TYPE::FREIGHT);

    if (!freight || freight->CalcValue() < 1)
        return;

    CombatUnit* unit = freight->GetNextUnit();
    if (!unit)
        return;

    MissionElement* elem = CreateSingleElement(freight, unit);
    if (!elem)
        return;

    elem->SetMissionRole((int)EMISSIONTYPE::CARGO);
    elem->SetIntelLevel(Intel::KNOWN);
    elem->SetRegion(player_group->GetRegion());

    ward = elem;
    mission->AddElement(elem);

    StarSystem* system = mission->GetStarSystem();
    OrbitalRegion* rgn1 = system->FindRegion(elem->GetRegion());
    if (!rgn1 || !rgn1->Primary())
        return;

    FVector delta = rgn1->Location() - rgn1->Primary()->Location();
    FVector navpt_loc = elem->GetLocation();

    delta.Normalize();
    delta *= 200000.0f;

    navpt_loc += delta;

    Instruction* n = new Instruction(elem->GetRegion(), navpt_loc, INSTRUCTION_ACTION::VECTOR);
    if (n) {
        n->SetSpeed(500);
        elem->AddNavPoint(n);
    }

    Text rgn2 = elem->GetRegion();
    List<CombatZone>& zones = campaign->GetZones();
    if (zones[zones.size() - 1]->HasRegion(rgn2))
        rgn2 = *zones[0]->GetRegions()[0];
    else
        rgn2 = *zones[zones.size() - 1]->GetRegions()[0];

    n = new Instruction(rgn2, FVector(0.0f, 0.0f, 0.0f), INSTRUCTION_ACTION::VECTOR);
    if (n) {
        n->SetSpeed(750);
        elem->AddNavPoint(n);
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateEscorts()
{
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateTargets()
{
    if (player_group && player_group->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP) {
        CreateTargetsCarrier();
    }
    else {
        switch (mission->GetType()) {
        default:
        case (int)EMISSIONTYPE::PATROL:
            CreateTargetsPatrol();   
            break;
        case (int)EMISSIONTYPE::ASSAULT:
        case (int)EMISSIONTYPE::STRIKE:
            CreateTargetsAssault();
            break;
        case (int)EMISSIONTYPE::ESCORT_FREIGHT:
            CreateTargetsFreightEscort(); 
            break;
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateTargetsAssault()
{
    // NOTE: This function is largely unchanged structurally; vector math updated where required.
    if (!player)
        return;

    CombatGroup* assigned = 0;

    if (request)
        assigned = request->GetObjective();

    if (assigned) {
        CreateElements(assigned);

        ListIter<MissionElement> e_iter = mission->GetElements();
        while (++e_iter) {
            MissionElement* elem = e_iter.value();

            if (elem->GetCombatGroup() == assigned) {
                if (!prime_target) {
                    prime_target = elem;

                    MissionElement* player_lead = player_group_elements[0];
                    if (!player_lead)
                        return;

                    Instruction* obj = new Instruction(INSTRUCTION_ACTION::ASSAULT, prime_target->GetName());
                    if (obj) {
                        obj->SetTargetDesc(Text("preplanned target '") + prime_target->GetName() + "'");
                        player_lead->AddObjective(obj);
                    }

                    // create flight plan:
                    RLoc         rloc;
                    RLoc* ref = 0;
                    FVector      dummy(0.0f, 0.0f, 0.0f);
                    Instruction* instr = 0;

                    const FVector loc = player_lead->GetLocation();
                    const FVector tgt = prime_target->GetLocation();
                    FVector mid = loc + (tgt - loc) * 0.35f;

                    rloc.SetReferenceLoc(0);
                    rloc.SetBaseLocation(mid);
                    rloc.SetDistance(50e3);
                    rloc.SetDistanceVar(5e3);
                    rloc.SetAzimuth(90 * DEGREES);
                    rloc.SetAzimuthVar(45 * DEGREES);

                    instr = new Instruction(prime_target->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
                    if (!instr)
                        return;

                    instr->SetSpeed(750);
                    instr->GetRLoc() = rloc;
                    ref = &instr->GetRLoc();

                    player_lead->AddNavPoint(instr);

                    for (int i = 1; i < player_group_elements.size(); i++) {
                        MissionElement* pge = player_group_elements[i];
                        RLoc            rloc2;

                        rloc2.SetReferenceLoc(ref);
                        rloc2.SetDistance(50e3);
                        rloc2.SetDistanceVar(5e3);

                        instr = new Instruction(prime_target->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
                        if (!instr)
                            return;

                        instr->SetSpeed(750);
                        instr->GetRLoc() = rloc2;

                        pge->AddNavPoint(instr);
                    }

                    double extra = 10e3;

                    if (prime_target && prime_target->GetShipDesign())
                    {
                        switch (prime_target->GetShipDesign()->ShipType)
                        {
                        default:
                            extra = 20e3;
                            break;

                        case (int)CLASSIFICATION::FRIGATE:
                            extra = 25e3;
                            break;

                        case (int)CLASSIFICATION::DESTROYER:
                            extra = 30e3;
                            break;

                        case (int)CLASSIFICATION::CRUISER:
                            extra = 50e3;
                            break;

                        case (int)CLASSIFICATION::BATTLESHIP:
                            extra = 70e3;
                            break;

                        case (int)CLASSIFICATION::DREADNAUGHT:
                            extra = 80e3;
                            break;

                        case (int)CLASSIFICATION::SWACS:
                            extra = 30e3;
                            break;

                        case (int)CLASSIFICATION::CARRIER:
                            extra = 90e3;
                            break;
                        }
                    }

                    rloc.SetReferenceLoc(0);
                    rloc.SetBaseLocation(tgt);
                    rloc.SetDistance(100e3 + extra);
                    rloc.SetDistanceVar(15e3);
                    rloc.SetAzimuth(90 * DEGREES);
                    rloc.SetAzimuthVar(45 * DEGREES);

                    instr = new Instruction(prime_target->GetRegion(), dummy, INSTRUCTION_ACTION::ASSAULT);
                    if (!instr)
                        return;

                    instr->SetSpeed(500);
                    instr->GetRLoc() = rloc;
                    instr->SetTarget(FString(prime_target->GetName().data()));

                    ref = &instr->GetRLoc();

                    player_lead->AddNavPoint(instr);

                    for (int i = 1; i < player_group_elements.size(); i++) {
                        MissionElement* pge = player_group_elements[i];
                        RLoc            rloc2;

                        rloc2.SetReferenceLoc(ref);
                        rloc2.SetDistance(50e3);
                        rloc2.SetDistanceVar(5e3);

                        instr = new Instruction(prime_target->GetRegion(), dummy, INSTRUCTION_ACTION::ASSAULT);
                        if (!instr)
                            return;

                        instr->SetSpeed(500);
                        instr->GetRLoc() = rloc2;;
                        instr->SetTarget(FString(prime_target->GetName().data()));

                        pge->AddNavPoint(instr);
                    }
                }
            }
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateTargetsCarrier()
{
    if (!player_group || !player)
        return;

    Text    region = player_group->GetRegion();
    FVector base_loc = player->GetLocation();

    // 3D volumes:
    const FVector PatrolLoc = base_loc + (FMath::VRand() * FMath::FRandRange(75000.0f, 150000.0f));
    const FVector Loc2 = PatrolLoc + (FMath::VRand() * FMath::FRandRange(50000.0f, 100000.0f));

    int ntargets = 2 + (FMath::RandBool() ? 1 : 0);
    int ntries = 8;

    while (ntargets > 0 && ntries > 0) {
        const FVector TargetLoc = FMath::RandBool() ? PatrolLoc : Loc2;

        const int t = CreateRandomTarget(region, TargetLoc);
        ntargets -= t;
        if (t < 1)
            --ntries;
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateTargetsPatrol()
{
    if (!player_group || !player)
        return;

    Text    region = player_group->GetRegion();
    FVector base_loc = player->GetLocation();

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    const FVector PatrolLoc = base_loc + (FMath::VRand() * FMath::FRandRange(170000.0f, 250000.0f));

    Instruction* n0 = new Instruction(region, PatrolLoc, INSTRUCTION_ACTION::PATROL);
    if (n0)
        player->AddNavPoint(n0);

    for (int i = 1; i < player_group_elements.size(); i++) {
        MissionElement* elem = player_group_elements[i];

        Instruction* n1 = new Instruction(region,
            PatrolLoc + ScatterInSphere(FMath::FRandRange(20000.0f, 40000.0f)),
            INSTRUCTION_ACTION::PATROL);
        if (n1 && elem)
            elem->AddNavPoint(n1);
    }

    const FVector Loc2 = PatrolLoc + (FMath::VRand() * FMath::FRandRange(150000.0f, 200000.0f));

    Instruction* n2 = new Instruction(region, Loc2, INSTRUCTION_ACTION::PATROL);
    if (n2)
        player->AddNavPoint(n2);

    for (int i = 1; i < player_group_elements.size(); i++) {
        MissionElement* elem = player_group_elements[i];

        Instruction* n3 = new Instruction(region,
            Loc2 + ScatterInSphere(FMath::FRandRange(20000.0f, 40000.0f)),
            INSTRUCTION_ACTION::PATROL);
        if (n3 && elem)
            elem->AddNavPoint(n3);
    }

    int ntargets = 2 + (FMath::RandBool() ? 1 : 0);
    int ntries = 8;

    while (ntargets > 0 && ntries > 0) {
        const FVector TargetLoc = FMath::RandBool() ? PatrolLoc : Loc2;

        const int t = CreateRandomTarget(region, TargetLoc);
        ntargets -= t;
        if (t < 1)
            --ntries;
    }

    // Objective uses the final patrol leg (Loc2) as reference:
    if (n2) {
        Instruction* obj = new Instruction(*n2);
        if (obj) {
            obj->SetTargetDesc("inbound enemy units");
            player->AddObjective(obj);
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::CreateTargetsFreightEscort()
{
    if (!ward) {
        CreateTargetsPatrol();
        return;
    }

    CombatGroup* s = FindSquadron(enemy, (int) ECOMBATGROUP_TYPE::ATTACK_SQUADRON);
    CombatGroup* s2 = FindSquadron(enemy, (int) ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

    if (!s || !s2)
        return;

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::ASSAULT);
    if (elem) {
        elem->SetIntelLevel(Intel::KNOWN);

        elem->SetLocation(ward->GetLocation() + ScatterInSphere(5.0f));

        Instruction* obj = new Instruction(INSTRUCTION_ACTION::ASSAULT, ward->GetName());
        if (obj)
            elem->AddObjective(obj);

        mission->AddElement(elem);

        MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
        if (e2) {
            e2->SetIntelLevel(Intel::KNOWN);

            e2->SetLocation(elem->GetLocation() + ScatterInSphere(0.25f));

            Instruction* obj2 = new Instruction(INSTRUCTION_ACTION::ESCORT, elem->GetName());
            if (obj2)
                e2->AddObjective(obj2);

            mission->AddElement(e2);
        }
    }

    Instruction* obj3 = new Instruction(mission->GetRegion(),
        FVector(0.0f, 0.0f, 0.0f),
        INSTRUCTION_ACTION::PATROL);
    if (player && obj3) {
        obj3->SetTargetDesc("enemy patrols");
        player->AddObjective(obj3);
    }
}

// +--------------------------------------------------------------------+

int
CampaignMissionStarship::CreateRandomTarget(const char* rgn, FVector base_loc)
{
    int ntargets = 0;
    int ttype = FMath::RandRange(0, 15);

    if (player_group && player_group->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP) {
        switch (ttype) {
        case  0: 
        case  1: 
        case  2: 
        case  3: 
            ttype = 0; 
            break;
        case  4: 
        case  5:    
            ttype = 1; 
            break;
        case  6: 
        case  7:  
            ttype = 2; 
            break;
        case  8: 
        case  9:
            ttype = 3;
            break;
        case 10:
        case 11:
            ttype = 4;
            break;
        case 12:
        case 13:
        case 14:
        case 15:
            ttype = 5;
            break;
        }
    }
    else {
        switch (ttype) {
        case  0: 
        case  1:
        case  2:
        case  3:
        case  4:
        case  5:
            ttype = 0;
            break;
        case  6:
        case  7:
        case  8:                             
            ttype = 1;
            break;
        case  9:
        case 10:     
            ttype = 4;
            break;
        case 11:
        case 12:
        case 13:
        case 14: 
        case 15:     
            ttype = 5;
            break;
        }
    }

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    switch (ttype) {
    case 0: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON);

        if (s) {
            for (int i = 0; i < 2; i++) {
                CombatUnit* u = s->GetRandomUnit();
                MissionElement* elem = CreateSingleElement(s, u);
                if (elem) {
                    elem->SetIntelLevel(Intel::KNOWN);
                    elem->SetRegion(rgn);
                    elem->SetLocation(base_loc + ScatterInSphere(1.5f));
                    elem->SetMissionRole((int)EMISSIONTYPE::FLEET);
                    mission->AddElement(elem);
                    ntargets++;
                }
            }
        }
    } break;

    case 1: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::LCA_SQUADRON);

        if (s) {
            MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::CARGO);
            if (elem) {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->SetRegion(rgn);
                elem->SetLocation(base_loc + ScatterInSphere(2.0f));
                mission->AddElement(elem);
                ntargets++;

                CombatGroup* s2 = FindSquadron(enemy, (int) ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

                if (s2) {
                    MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
                    if (e2) {
                        e2->SetIntelLevel(Intel::KNOWN);
                        e2->SetRegion(rgn);
                        e2->SetLocation(elem->GetLocation() + ScatterInSphere(0.5f));

                        Instruction* obj = new Instruction(INSTRUCTION_ACTION::ESCORT, elem->GetName());
                        if (obj)
                            e2->AddObjective(obj);

                        mission->AddElement(e2);
                    }
                }
            }
        }
    } break;

    case 2: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);

        if (s) {
            MissionElement* elem = CreateFighterPackage(s, 4, (int)EMISSIONTYPE::PATROL);
            if (elem) {
                elem->SetIntelLevel(Intel::SECRET);
                elem->SetRegion(rgn);
                elem->SetLocation(base_loc);
                mission->AddElement(elem);
                ntargets++;
            }
        }
    } break;

    case 3: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

        if (s) {
            MissionElement* elem = CreateFighterPackage(s, 3, (int)EMISSIONTYPE::ASSAULT);
            if (elem) {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->GetLoadouts().destroy();
                elem->GetLoadouts().append(new MissionLoad(-1, "Ship Strike"));
                elem->SetRegion(rgn);
                elem->SetLocation(base_loc + ScatterInSphere(1.0f));
                mission->AddElement(elem);

                if (player) {
                    Instruction* n = new Instruction(player->GetRegion(),
                        player->GetLocation() + ScatterInSphere(1.0f),
                        INSTRUCTION_ACTION::ASSAULT);
                    n->SetTarget(FString(player->GetName().data()));
                    elem->AddNavPoint(n);
                }

                ntargets++;
            }
        }
    } break;

    case 4: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON);

        if (s) {
            MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::ASSAULT);
            if (elem) {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->Loadouts().destroy();
                elem->Loadouts().append(new MissionLoad(-1, "Hvy Ship Strike"));
                elem->SetRegion(rgn);
                elem->SetLocation(base_loc + ScatterInSphere(1.3f));
                mission->AddElement(elem);

                if (player) {
                    Instruction* n = new Instruction(player->GetRegion(),
                        player->GetLocation() + ScatterInSphere(1.0f),
                        INSTRUCTION_ACTION::ASSAULT);
                    n->SetTarget(FString(player->GetName().data()));
                    elem->AddNavPoint(n);
                }

                ntargets++;
            }
        }
    } break;

    default: {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FREIGHT);

        if (s) {
            CombatUnit* u = s->GetRandomUnit();
            MissionElement* elem = CreateSingleElement(s, u);
            if (elem) {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->SetRegion(rgn);
                elem->SetLocation(base_loc + ScatterInSphere(2.0f));
                elem->SetMissionRole((int)EMISSIONTYPE::CARGO);
                mission->AddElement(elem);
                ntargets++;

                CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);

                if (s2) {
                    MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
                    if (e2) {
                        e2->SetIntelLevel(Intel::KNOWN);
                        e2->SetRegion(rgn);
                        e2->SetLocation(elem->GetLocation() + ScatterInSphere(0.5f));

                        Instruction* obj = new Instruction(INSTRUCTION_ACTION::ESCORT, elem->GetName());
                        if (obj)
                            e2->AddObjective(obj);

                        mission->AddElement(e2);
                        ntargets++;
                    }
                }
            }
        }
    } break;
    }

    return ntargets;
}

// +--------------------------------------------------------------------+

MissionElement*
CampaignMissionStarship::CreateFighterPackage(CombatGroup* squadron, int count, int role)
{
    if (!squadron || squadron->IsReserve())
        return nullptr;

    CombatUnit* fighter = squadron->GetUnits().at(0);
    CombatUnit* carrier = FindCarrier(squadron);

    if (!fighter)
        return nullptr;

    const FShipDesign* ShipRow = nullptr;

    if (fighter->GetDesignName().length() > 0)
    {
        ShipRow = ShipDesignRegistry::Find(fighter->GetDesignName().data());
    }

    if (!ShipRow && fighter->GetDesign())
    {
        ShipRow = ShipDesignRegistry::Find(fighter->GetDesign()->name);
    }

    if (!ShipRow)
    {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS CreateFighterPackage: missing ship design row for squadron '%s', unit '%s', design '%s'"),
            ANSI_TO_TCHAR(squadron->GetName().data()),
            ANSI_TO_TCHAR(fighter->GetName().data()),
            ANSI_TO_TCHAR(fighter->GetDesignName().data()));
        return nullptr;
    }

    int avail = fighter->LiveCount();
    int actual = count;

    if (avail < actual)
        actual = avail;

    if (avail < 1) {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMS - Insufficient fighters in squadron '%s' - %d required, %d available"),
            ANSI_TO_TCHAR(squadron->GetName().data()),
            count,
            avail);
        return nullptr;
    }

    MissionElement* elem = new MissionElement;
    if (!elem) {
        Exit();
        return nullptr;
    }

    elem->SetName(Callsign::GetCallsign(fighter->GetIFF()));
    elem->SetElementID(pkg_id++);

    if (carrier) {
        elem->SetCommander(carrier->GetName());
        elem->SetHeading(carrier->GetHeading());
    }
    else {
        elem->SetHeading(fighter->GetHeading());
    }

    elem->SetShipDesign(ShipRow);
    elem->SetCount(actual);
    elem->SetIFF(fighter->GetIFF());
    elem->SetIntelLevel(squadron->GetIntelLevel());
    elem->SetRegion(fighter->GetRegion());
    elem->SetSquadron(fighter->GetName());
    elem->SetMissionRole(role);

    elem->Loadouts().append(new MissionLoad(-1, "ACM Medium Range"));

    auto ScatterInSphere = [](float Radius) -> FVector
        {
            const float r = Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
            return FMath::VRand() * r;
        };

    if (carrier) {
        elem->SetLocation(carrier->Location() + ScatterInSphere(0.3f));
    }
    else {
        elem->SetLocation(fighter->Location() + ScatterInSphere(1.0f));
    }

    elem->SetCombatGroup(squadron);
    elem->SetCombatUnit(fighter);

    return elem;
}

// +--------------------------------------------------------------------+

CombatGroup*
CampaignMissionStarship::FindSquadron(int iff, int type)
{
    if (!player_group)
        return nullptr;

    CombatGroup* result = 0;

    CombatZone* zone = player_group->GetAssignedZone();
    if (!zone)
        zone = player_group->GetCurrentZone();

    if (!zone) {
        UE_LOG(LogStarshatterWars, Warning, TEXT("CMS Warning: no zone for %s"),
            ANSI_TO_TCHAR(player_group->GetName().data()));
        return result;
    }

    ZoneForce* force = zone->FindForce(iff);

    if (force) {
        List<CombatGroup> groups;
        ListIter<CombatGroup> group = force->GetGroups();
        while (++group) {
            CombatGroup* g = group.value();

            if ((int)g->GetType() == type && g->CountUnits() > 0) {
                result = g;
                groups.append(g);
            }
        }

        if (groups.size() > 1) {
            const int index = FMath::RandRange(0, groups.size() - 1);
            result = groups[index];
        }
    }

    return result;
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::DefineMissionObjectives()
{
    if (!mission || !player)
        return;

    if (prime_target) mission->SetTarget(prime_target);
    if (ward)         mission->SetWard(ward);

    Text objectives;

    if (player->Objectives().size() > 0) {
        for (int i = 0; i < player->Objectives().size(); i++) {
            Instruction* obj = player->Objectives().at(i);
            objectives += "* ";
            objectives += obj->GetDescription();
            objectives += ".\n";
        }
    }
    else {
        objectives += "* Perform standard fleet operations in the ";
        objectives += mission->GetRegion();
        objectives += " sector.\n";
    }

    mission->SetObjective(objectives);
}

// +--------------------------------------------------------------------+

MissionInfo*
CampaignMissionStarship::DescribeMission()
{
    if (!mission || !player)
        return nullptr;

    char name[256] = { 0 };

    if (mission_info && mission_info->name.length() > 0) {
        sprintf_s(name, sizeof(name), "MSN-%03d %s",
            mission->GetIdentity(),
            mission_info->name.data());
    }
    else if (ward) {
        sprintf_s(name, sizeof(name), "MSN-%03d %s %s",
            mission->GetIdentity(),
            Game::GetText(mission->GetTypeName()).data(),
            ward->GetName().data());
    }
    else if (prime_target) {
        const char* ClassName = "";
        const FShipDesign* PrimeDesign = prime_target->GetShipDesign();

        if (PrimeDesign)
            ClassName = Ship::GetShipClassName(PrimeDesign->ShipType);

        sprintf_s(name, sizeof(name), "MSN-%03d %s %s %s",
            mission->GetIdentity(),
            Game::GetText(mission->GetTypeName()).data(),
            ClassName,
            prime_target->GetName().data());
    }
    else {
        sprintf_s(name, sizeof(name), "MSN-%03d %s",
            mission->GetIdentity(),
            Game::GetText(mission->GetTypeName()).data());
    }

    char player_info[256] = { 0 };
    if (player && player->GetShipDesign()) {
        const FShipDesign* PlayerDesign = player->GetShipDesign();

        const FString Abbrev =
            !PlayerDesign->Abrv.IsEmpty() ? PlayerDesign->Abrv : TEXT("UNK");

        const FString DesignName =
            !PlayerDesign->DisplayName.IsEmpty() ? PlayerDesign->DisplayName :
            (!PlayerDesign->ShipName.IsEmpty() ? PlayerDesign->ShipName : TEXT("UnknownDesign"));

        const FString ElemName = ANSI_TO_TCHAR(player->GetName().data());

        const FString PlayerInfoStr = FString::Printf(
            TEXT("%d x %s %s '%s'"),
            player->Count(),
            *Abbrev,
            *DesignName,
            *ElemName);

        strcpy_s(player_info, sizeof(player_info), TCHAR_TO_ANSI(*PlayerInfoStr));
    }
    else if (player && player->GetCombatGroup()) {
        const Text desc = player->GetCombatGroup()->GetDescription();
        strcpy_s(player_info, sizeof(player_info), desc.data());
    }
    else {
        strcpy_s(player_info, sizeof(player_info), "(unknown)");
    }

    MissionInfo* info = new MissionInfo;
    if (!info)
        return nullptr;

    info->id = mission->GetIdentity();
    info->mission = mission;

    info->name = name;
    info->type = mission->GetType();
    info->player_info = player_info;

    info->description = mission->GetObjective();
    info->start = mission->GetStart();

    if (mission->GetStarSystem())
        info->system = mission->GetStarSystem()->GetName();

    info->region = mission->GetRegion();

    mission->SetName(name);

    return info;
}

// +--------------------------------------------------------------------+

void
CampaignMissionStarship::Exit()
{
    Starshatter* stars = Starshatter::GetInstance();
    if (stars)
        stars->SetGameMode(EGameMode::MENU);
}
