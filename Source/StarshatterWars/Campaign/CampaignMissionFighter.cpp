/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignMissionFighter.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignMissionFighter generates missions and mission
    info for the player's FIGHTER SQUADRON as part of a
    dynamic campaign.

    UE PORT NOTES
    =============
    - Legacy logging replaced with UE_LOG.
    - MemDebug / placement-new removed.
    - Legacy Text retained for compatibility.
    - Current implementation remains a compatibility shell;
      full original generation logic can be merged back in later.
*/

#include "CampaignMissionFighter.h"

#include "CampaignMissionRequest.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "Combatant.h"
#include "Mission.h"
#include "Instruction.h"
#include "MissionInfo.h"
#include "MissionElement.h"
#include "MissionTemplate.h"
#include "OrbitalRegion.h"
#include "Starsystem.h"
#include "CombatZone.h"
#include "Galaxy.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Callsign.h"
#include "PlayerCharacter.h"

#include "StarshatterWarsLog.h"
#include "GameStructs.h"
#include "Random.h"

#include "Logging/LogMacros.h"

static int pkg_id = 1000;
static int dump_missions = 0;

// +--------------------------------------------------------------------+

static CombatGroup* FindCombatGroup(CombatGroup* G, ECOMBATGROUP_TYPE Type)
{
    if (!G)
    {
        return nullptr;
    }

    if (G->GetIntelLevel() <= Intel::RESERVE)
    {
        return nullptr;
    }

    if (G->GetUnits().size() > 0)
    {
        for (int32 i = 0; i < G->GetUnits().size(); i++)
        {
            CombatUnit* U = G->GetUnits().at(i);

            if (U && U->LiveCount() > 0 && G->GetType() == Type)
            {
                return G;
            }
        }
    }

    CombatGroup* Result = nullptr;

    ListIter<CombatGroup> Subgroup = G->GetComponents();
    while (++Subgroup && !Result)
    {
        Result = FindCombatGroup(Subgroup.value(), Type);
    }

    return Result;
}

CampaignMissionFighter::CampaignMissionFighter(Campaign* c)
    : campaign(c)
    , request(nullptr)
    , mission_info(nullptr)
    , squadron(nullptr)
    , strike_group(nullptr)
    , strike_target(nullptr)
    , mission(nullptr)
    , player_elem(nullptr)
    , carrier_elem(nullptr)
    , ward(nullptr)
    , prime_target(nullptr)
    , escort(nullptr)
    , air_region()
    , orb_region()
    , airborne(false)
    , airbase(false)
    , ownside(0)
    , enemy(-1)
    , mission_type(0)
{
    if (!campaign || !campaign->GetPlayerGroup())
    {
        UE_LOG(LogStarshatterWars, Error,
            TEXT("ERROR - CMF campaign=%p player_group=%p"),
            campaign,
            campaign ? campaign->GetPlayerGroup() : nullptr);
        return;
    }

    CombatGroup* player_group = campaign->GetPlayerGroup();

    switch ((int)player_group->GetType())
    {
    case (int)ECOMBATGROUP_TYPE::WING:
    {
        CombatGroup* wing = player_group;
        ListIter<CombatGroup> iter = wing->GetComponents();

        while (++iter)
        {
            CombatGroup* g = iter.value();
            if (g && g->GetType() == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON)
            {
                squadron = g;
            }
        }
    }
    break;

    case (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
    case (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
    case (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
        squadron = player_group;
        break;

    default:
        UE_LOG(LogStarshatterWars, Error,
            TEXT("ERROR - CMF invalid player group: %s IFF %d"),
            ANSI_TO_TCHAR(player_group->GetDescription()),
            player_group->GetIFF());
        break;
    }

    if (squadron)
    {
        CombatGroup* carrier = squadron->FindCarrier();
        if (carrier && carrier->GetType() == ECOMBATGROUP_TYPE::STARBASE)
        {
            airbase = true;
        }
    }
}

CampaignMissionFighter::~CampaignMissionFighter()
{
}

// +--------------------------------------------------------------------+

void CampaignMissionFighter::CreateMission(CampaignMissionRequest* req)
{
    if (!campaign || !squadron || !req)
        return;

    UE_LOG(LogStarshatterWars, Log, TEXT("-----------------------------------------------"));

    if (req->Script().Len())
    {
        UE_LOG(LogStarshatterWars, Log,
            TEXT("CMF CreateMission() request: %s '%s'"),
            ANSI_TO_TCHAR(Mission::GetRoleName(req->Type())),
            *req->Script());
    }
    else
    {
        const char* ObjName = req->GetObjective() ? req->GetObjective()->GetName().data() : "(no target)";

        UE_LOG(LogStarshatterWars, Log,
            TEXT("CMF CreateMission() request: %s %s"),
            ANSI_TO_TCHAR(Mission::GetRoleName(req->Type())),
            ANSI_TO_TCHAR(ObjName));
    }

    request = req;
    mission_info = nullptr;

    if (request->GetPrimaryGroup())
    {
        switch ((int)request->GetPrimaryGroup()->GetType())
        {
        case (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
        case (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
        case (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
            squadron = request->GetPrimaryGroup();
            break;
        }
    }

    ownside = squadron->GetIFF();

    for (int i = 0; i < campaign->GetCombatants().size(); i++)
    {
        Combatant* c = campaign->GetCombatants().at(i);
        if (!c)
            continue;

        const int iff = c->GetIFF();
        if (iff > 0 && iff != ownside)
        {
            enemy = iff;
            break;
        }
    }

    static int id_key = 1;
    GenerateMission(id_key++);

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] After GenerateMission: mission=%s IsOK=%s"),
        mission ? TEXT("VALID") : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"));

    DefineMissionObjectives();

    MissionInfo* info = DescribeMission();

    if (info)
    {
        campaign->GetMissionList().append(info);

        UE_LOG(LogStarshatterWars, Log,
            TEXT("CMF Created %03d '%s' %s"),
            info->id,
            ANSI_TO_TCHAR(info->name.data()),
            ANSI_TO_TCHAR(Mission::GetRoleName(mission ? mission->GetType() : 0)));

        if (dump_missions && mission)
        {
            Text script = mission->Serialize();
            char fname[32] = { 0 };

            sprintf_s(fname, sizeof(fname), "msn%03d.def", info->id);

            FILE* f = nullptr;
            fopen_s(&f, fname, "w");
            if (f)
            {
                fprintf(f, "%s\n", script.data());
                fclose(f);
            }
        }
    }
    else
    {
        UE_LOG(LogStarshatterWars, Warning, TEXT("CMF failed to create mission."));
    }
}

// +--------------------------------------------------------------------+

Mission* CampaignMissionFighter::GenerateMission(int id)
{
    bool found = false;

    SelectType();

    if (request && request->Script().Len())
    {
        // Still legacy/script-backed unless you convert this branch too
        const FString Script = request->Script();
        const FString Path = campaign->Path();

        MissionTemplate* mt = new MissionTemplate(
            id,
            TCHAR_TO_ANSI(*Script),
            TCHAR_TO_ANSI(*Path)
        );

        if (mt)
        {
            mt->SetPlayerSquadron(squadron);
        }

        mission = mt;
        found = (mission != nullptr);
    }
    else
    {
        const FS_CampaignMission* mission_data =
            campaign->FindCampaignMissionData(mission_type, squadron);

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

    if (!mission)
    {
        Exit();
        return nullptr;
    }

    char name[64] = { 0 };
    sprintf_s(name, sizeof(name), "Fighter Mission %d", id);

    mission->SetName(name);
    mission->SetTeam(squadron->GetIFF());
    mission->SetStart(request->StartTime());

    SelectRegion();
    GenerateStandardElements();
    CreatePatrols();

    if (!found)
    {
        GenerateMissionElements();
        mission->SetOK(true);
        mission->Validate();
    }
    else
    {
        if (mission->IsOK())
        {
            player_elem = mission->GetPlayer();
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
            mission->SetTeam(squadron->GetIFF());
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

// +--------------------------------------------------------------------+

bool CampaignMissionFighter::IsGroundObjective(CombatGroup* obj)
{
    if (!obj || !campaign)
        return false;

    CombatGroup* pgroup = campaign->GetPlayerGroup();
    if (!pgroup)
        return false;

    CombatZone* zone = pgroup->GetAssignedZone();
    if (!zone)
        return false;

    StarSystem* system = campaign->GetSystem(zone->GetSystem());
    if (!system)
        return false;

    OrbitalRegion* region = system->FindRegion(obj->GetRegion());
    return region && region->Type() == Orbital::TERRAIN;
}

// +--------------------------------------------------------------------+

void CampaignMissionFighter::SelectType()
{
    int type = (int) EMISSIONTYPE::PATROL;

    if (request)
    {
        type = request->Type();

        if (type == (int) EMISSIONTYPE::STRIKE)
        {
            strike_group = request->GetPrimaryGroup();

            if (!IsGroundObjective(request->GetObjective()))
            {
                type = (int)EMISSIONTYPE::ASSAULT;
            }
        }
        else if (type == (int)EMISSIONTYPE::ESCORT_STRIKE)
        {
            strike_group = request->GetSecondaryGroup();
            if (!strike_group || strike_group->CalcValue() < 1)
            {
                type = (int)EMISSIONTYPE::SWEEP;
                strike_group = nullptr;
            }
        }
    }

    mission_type = type;
}

void CampaignMissionFighter::SelectRegion()
{
    if (!squadron || !mission || !campaign)
        return;

    CombatZone* zone = squadron->GetAssignedZone();
    if (!zone)
        zone = squadron->GetCurrentZone();

    if (zone)
    {
        mission->SetStarSystem(campaign->GetSystem(zone->GetSystem()));
        mission->SetRegion(*zone->GetRegions().at(0));

        orb_region = mission->GetRegion();

        if (zone->GetRegions().size() > 1)
        {
            air_region = *zone->GetRegions().at(1);

            StarSystem* system = mission->GetStarSystem();
            OrbitalRegion* rgn = nullptr;

            if (system)
                rgn = system->FindRegion(air_region);

            if (!rgn || rgn->Type() != Orbital::TERRAIN)
                air_region = "";
        }

        if (air_region.length() > 0)
        {
            if (request && IsGroundObjective(request->GetObjective()))
            {
                airborne = true;
            }
            else if (mission->GetType() >= (int)EMISSIONTYPE::AIR_PATROL &&
                mission->GetType() <= (int)EMISSIONTYPE::AIR_INTERCEPT)
            {
                airborne = true;
            }
            else if (mission->GetType() == (int)EMISSIONTYPE::STRIKE ||
                mission->GetType() == (int)EMISSIONTYPE::ESCORT_STRIKE)
            {
                if (strike_group)
                {
                    strike_target = campaign->FindStrikeTarget(ownside, strike_group);

                    if (strike_target && strike_target->GetRegion() == air_region)
                        airborne = true;
                }
            }

            if (airbase)
            {
                mission->SetRegion(air_region);
            }
        }
    }
    else
    {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("WARNING: CMF - No zone for '%s'"),
            ANSI_TO_TCHAR(squadron->GetName().data()));

        StarSystem* s = campaign->GetSystemList()[0];
        mission->SetStarSystem(s);
        mission->SetRegion(s->Regions()[0]->Name());
    }

    if (!airborne)
    {
        switch (mission->GetType())
        {
        case (int)EMISSIONTYPE::AIR_PATROL: 
            mission->SetType((int) EMISSIONTYPE::PATROL); 
            break;
        case (int)EMISSIONTYPE::AIR_SWEEP: 
            mission->SetType((int)EMISSIONTYPE::SWEEP);  
            break;
        case (int)EMISSIONTYPE::AIR_INTERCEPT:
            mission->SetType((int)EMISSIONTYPE::INTERCEPT); 
            break;
        default: break;
        }
    }
}

// +--------------------------------------------------------------------+

void
CampaignMissionFighter::GenerateStandardElements()
{
    ListIter<CombatZone> z_iter = campaign->GetZones();
    while (++z_iter)
    {
        CombatZone* z = z_iter.value();
        if (!z)
        {
            continue;
        }

        ListIter<ZoneForce> iter = z->GetForces();
        while (++iter)
        {
            ZoneForce* force = iter.value();
            if (!force)
            {
                continue;
            }

            ListIter<CombatGroup> group = force->GetGroups();
            while (++group)
            {
                CombatGroup* g = group.value();
                if (!g)
                {
                    continue;
                }

                switch (g->GetType())
                {
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

                default:
                    break;
                }
            }
        }
    }
}

void CampaignMissionFighter::GenerateMissionElements()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] GenerateMissionElements: mission=%s squadron=%s"),
        mission ? TEXT("VALID") : TEXT("NULL"),
        squadron ? TEXT("VALID") : TEXT("NULL"));

    CreateWards();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] After CreateWards: Player=%s IsOK=%s"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"));

    CreatePlayer(squadron);

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] After CreatePlayer: Player=%s IsOK=%s"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"));

    CreateTargets();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] After CreateTargets: Player=%s IsOK=%s"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"));

    CreateEscorts();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionGen:Fighter] After CreateEscorts: Player=%s IsOK=%s"),
        (mission && mission->GetPlayer()) ? TEXT("VALID") : TEXT("NULL"),
        (mission && mission->IsOK()) ? TEXT("true") : TEXT("false"));
}

void CampaignMissionFighter::CreateElements(CombatGroup* g)
{
    if (!g || !mission)
    {
        return;
    }

    // Iterate all units in this group
    ListIter<CombatUnit> iter = g->GetUnits();
    while (++iter)
    {
        CombatUnit* unit = iter.value();
        if (!unit)
        {
            continue;
        }

        MissionElement* elem = CreateSingleElement(g, unit);
        if (!elem)
        {
            continue;
        }

        // Assign team
        elem->SetIFF(g->GetIFF());

        // Assign region
        if (airborne && air_region.length() > 0)
        {
            elem->SetRegion(air_region);
        }
        else
        {
            elem->SetRegion(orb_region);
        }

        // Add to mission
        mission->AddElement(elem);
    }
}

void CampaignMissionFighter::CreateSquadron(CombatGroup* g)
{
    if (!g || !mission)
    {
        return;
    }

    const int GroupType = (int)g->GetType();

    if (GroupType != (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON &&
        GroupType != (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON &&
        GroupType != (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON &&
        GroupType != (int)ECOMBATGROUP_TYPE::LCA_SQUADRON)
    {
        return;
    }

    // Player squadron is handled specially:
    if (g == squadron)
    {
        CreatePlayer(g);
        return;
    }

    int role = (int) EMISSIONFIGHTER::FIGHTER;

    switch ((ECOMBATGROUP_TYPE)g->GetType())
    {
    case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
        role = (int) EMISSIONFIGHTER::INTERCEPT;
        break;

    case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
        role = (int) EMISSIONFIGHTER::ATTACK;
        break;

    case ECOMBATGROUP_TYPE::LCA_SQUADRON:
        role = (int) EMISSIONFIGHTER::LANDING;
        break;

    case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
    default:
        role = (int)EMISSIONFIGHTER::FIGHTER;
        break;
    }

    // Use the squadron strength if available, otherwise fall back to a basic package size.
    int count = 4;

    if (g->GetUnits().size() > 0)
    {
        count = g->GetUnits().size();
    }

    MissionElement* elem = CreateFighterPackage(g, count, role);
    if (!elem)
    {
        return;
    }

    elem->SetIFF(g->GetIFF());

    if (airborne && air_region.length() > 0)
    {
        elem->SetRegion(air_region);
        PlanetaryInsertion(elem);
    }
    else
    {
        elem->SetRegion(orb_region);
        OrbitalInsertion(elem);
    }

    mission->AddElement(elem);
}

void CampaignMissionFighter::CreatePlayer(CombatGroup* g)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[CMF] CreatePlayer: BEGIN mission=%s group=%s"),
        mission ? TEXT("VALID") : TEXT("NULL"),
        g ? TEXT("VALID") : TEXT("NULL"));

    if (!g || !mission)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CMF] CreatePlayer: early return (g or mission null)"));
        return;
    }

    int role = (int)EMISSIONFIGHTER::FIGHTER;

    switch ((ECOMBATGROUP_TYPE)g->GetType())
    {
    case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
        role = (int)EMISSIONFIGHTER::INTERCEPT;
        break;

    case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
        role = (int)EMISSIONFIGHTER::ATTACK;
        break;

    case ECOMBATGROUP_TYPE::LCA_SQUADRON:
        role = (int)EMISSIONFIGHTER::LANDING;
        break;

    case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
    default:
        role = (int)EMISSIONFIGHTER::FIGHTER;
        break;
    }

    int count = 4;

    if (g->GetUnits().size() > 0)
    {
        count = g->GetUnits().size();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CMF] CreatePlayer: role=%d unit_count=%d"),
        role,
        g->GetUnits().size());

    player_elem = CreateFighterPackage(g, count, role);

    UE_LOG(LogTemp, Warning,
        TEXT("[CMF] CreatePlayer: CreateFighterPackage returned %s"),
        player_elem ? TEXT("VALID") : TEXT("NULL"));

    if (!player_elem)
    {
        UE_LOG(LogStarshatterWars, Warning,
            TEXT("CMF CreatePlayer: failed to create player package for '%s'"),
            ANSI_TO_TCHAR(g->GetName().data()));
        return;
    }

    player_elem->SetIFF(g->GetIFF());

    if (airborne && air_region.length() > 0)
    {
        player_elem->SetRegion(air_region);
        PlanetaryInsertion(player_elem);
    }
    else
    {
        player_elem->SetRegion(orb_region);
        OrbitalInsertion(player_elem);
    }

    mission->AddElement(player_elem);
    mission->SetPlayer(player_elem);

    UE_LOG(LogTemp, Warning,
        TEXT("[CMF] CreatePlayer: after AddElement+SetPlayer mission->GetPlayer()=%s"),
        mission->GetPlayer() ? TEXT("VALID") : TEXT("NULL"));

    CombatUnit* carrier = FindCarrier(g);
    if (carrier)
    {
        carrier_elem = CreateSingleElement(g->FindCarrier(), carrier);

        UE_LOG(LogTemp, Warning,
            TEXT("[CMF] CreatePlayer: carrier=%s carrier_elem=%s"),
            TEXT("VALID"),
            carrier_elem ? TEXT("VALID") : TEXT("NULL"));

        if (carrier_elem)
        {
            carrier_elem->SetIFF(g->GetIFF());

            if (airbase && air_region.length() > 0)
            {
                carrier_elem->SetRegion(air_region);
                PlanetaryInsertion(carrier_elem);
            }
            else
            {
                carrier_elem->SetRegion(orb_region);
                OrbitalInsertion(carrier_elem);
            }

            mission->AddElement(carrier_elem);
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CMF] CreatePlayer: END mission->GetPlayer()=%s IsOK=%s"),
        mission->GetPlayer() ? TEXT("VALID") : TEXT("NULL"),
        mission->IsOK() ? TEXT("true") : TEXT("false"));
}

void CampaignMissionFighter::CreatePatrols()
{
    List<MissionElement> Patrols;

    ListIter<MissionElement> Iter = mission->GetElements();
    while (++Iter)
    {
        MissionElement* SquadElem = Iter.value();
        CombatGroup* Squadron = SquadElem ? SquadElem->GetCombatGroup() : nullptr;
        CombatUnit* Unit = SquadElem ? SquadElem->GetCombatUnit() : nullptr;

        if (!SquadElem || !SquadElem->IsSquadron() || !Squadron || !Unit || Unit->LiveCount() < 4)
        {
            continue;
        }

        if (Squadron->GetType() == ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON ||
            Squadron->GetType() == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON)
        {
            StarSystem* System = mission->GetStarSystem();
            CombatGroup* Base = Squadron->FindCarrier();

            if (!Base || !System)
            {
                continue;
            }

            OrbitalRegion* Region = System->FindRegion(Base->GetRegion());
            if (!Region)
            {
                continue;
            }

            int PatrolType = (int)EMISSIONTYPE::PATROL;
            FVector BaseLoc;

            if (Region->Type() == Orbital::TERRAIN)
            {
                PatrolType = (int)EMISSIONTYPE::AIR_PATROL;

                if (FMath::RandRange(1, 3) <= 2)
                {
                    continue;
                }
            }

            BaseLoc =
                FVector(Base->GetLocation().X, Base->GetLocation().Y, Base->GetLocation().Z) +
                FVector(GetRandomPoint().X, GetRandomPoint().Y, GetRandomPoint().Z) * 1.5f;

            if (Region->Type() == Orbital::TERRAIN)
            {
                BaseLoc += FVector(0.0f, 0.0f, 14000.0f);
            }

            MissionElement* Elem = CreateFighterPackage(Squadron, 2, PatrolType);
            if (Elem)
            {
                Elem->SetIntelLevel(Intel::KNOWN);
                Elem->SetRegion(Base->GetRegion());
                Elem->SetLocation(BaseLoc);
                Patrols.append(Elem);
            }
        }
    }

    Iter.attach(Patrols);
    while (++Iter)
    {
        mission->AddElement(Iter.value());
    }
}

void CampaignMissionFighter::CreateWards()
{
    switch (mission ? mission->GetType() : mission_type)
    {
    case (int) EMISSIONTYPE::ESCORT_FREIGHT: CreateWardFreight(); break;
    case (int) EMISSIONTYPE::ESCORT_SHUTTLE: CreateWardShuttle(); break;
    case (int) EMISSIONTYPE::ESCORT_STRIKE:  CreateWardStrike();  break;
    default: break;
    }
}

void CampaignMissionFighter::CreateWardFreight()
{
    if (!mission || !mission->GetStarSystem())
    {
        return;
    }

    CombatUnit* carrier = FindCarrier(squadron);
    CombatGroup* freight = nullptr;

    if (request)
    {
        freight = request->GetObjective();
    }

    if (!freight)
    {
        freight = campaign->FindGroup(ownside, (int)ECOMBATGROUP_TYPE::FREIGHT);
    }

    if (!freight || freight->CalcValue() < 1)
    {
        return;
    }

    CombatUnit* unit = freight->GetNextUnit();
    if (!unit)
    {
        return;
    }

    MissionElement* elem = CreateSingleElement(freight, unit);
    if (!elem)
    {
        return;
    }

    elem->SetMissionRole((int)EMISSIONTYPE::CARGO);
    elem->SetIntelLevel(Intel::KNOWN);
    elem->SetRegion(squadron->GetRegion());

    if (carrier)
    {
        elem->SetLocation(
            FVector(carrier->Location().X, carrier->Location().Y, carrier->Location().Z) +
            FVector(GetRandomPoint().X, GetRandomPoint().Y, GetRandomPoint().Z) * 2.0f);
    }

    ward = elem;
    mission->AddElement(elem);

    StarSystem* system = mission->GetStarSystem();
    OrbitalRegion* rgn1 = system ? system->FindRegion(elem->GetRegion()) : nullptr;
    if (!rgn1 || !rgn1->Primary())
    {
        return;
    }

    FVector delta(
        rgn1->Location().X - rgn1->Primary()->Location().X,
        rgn1->Location().Y - rgn1->Primary()->Location().Y,
        rgn1->Location().Z - rgn1->Primary()->Location().Z);

    FVector npt_loc(
        elem->GetLocation().X,
        elem->GetLocation().Y,
        elem->GetLocation().Z);

    Instruction* n = nullptr;

    delta.Normalize();
    delta *= 200000.0f;
    npt_loc += delta;

    n = new Instruction(elem->GetRegion(), npt_loc, INSTRUCTION_ACTION::VECTOR);
    if (n)
    {
        n->SetSpeed(500);
        elem->AddNavPoint(n);
    }

    Text rgn2 = elem->GetRegion();
    List<CombatZone>& zones = campaign->GetZones();

    if (zones.size() > 0)
    {
        if (zones[zones.size() - 1]->HasRegion(rgn2))
        {
            rgn2 = *zones[0]->GetRegions()[0];
        }
        else
        {
            rgn2 = *zones[zones.size() - 1]->GetRegions()[0];
        }

        n = new Instruction(rgn2, FVector(0.0f, 0.0f, 0.0f), INSTRUCTION_ACTION::VECTOR);
        if (n)
        {
            n->SetSpeed(750);
            elem->AddNavPoint(n);
        }
    }
}

void CampaignMissionFighter::CreateWardShuttle()
{
    if (!mission || !mission->GetStarSystem())
    {
        return;
    }

    CombatUnit* Carrier = FindCarrier(squadron);
    CombatGroup* Shuttle = campaign->FindGroup(ownside, (int)ECOMBATGROUP_TYPE::LCA_SQUADRON);

    if (!Shuttle || Shuttle->CalcValue() < 1)
    {
        return;
    }

    MissionElement* Elem = CreateFighterPackage(Shuttle, 1, (int)EMISSIONTYPE::CARGO);
    if (!Elem)
    {
        return;
    }

    Elem->SetIntelLevel(Intel::KNOWN);
    Elem->SetRegion(orb_region);
    Elem->Loadouts().destroy();

    if (Carrier)
    {
        const FVector CarrierLoc = Carrier->Location();
        const FVector Offset = GetRandomPoint() * 2.0f;

        Elem->SetLocation(CarrierLoc + Offset);
    }

    ward = Elem;
    mission->AddElement(Elem);

    // If there is terrain nearby, have the shuttle descend toward it:
    if (air_region.length() > 0)
    {
        StarSystem* System = mission->GetStarSystem();
        OrbitalRegion* Rgn1 = System ? System->FindRegion(Elem->GetRegion()) : nullptr;
        if (!Rgn1 || !Rgn1->Primary())
        {
            return;
        }

        FVector Delta = Rgn1->Location() - Rgn1->Primary()->Location();

        FVector NptLoc = Elem->GetLocation();
        Instruction* N = nullptr;

        Delta = Delta.GetSafeNormal() * -200000.0f;
        NptLoc += Delta;

        N = new Instruction(Elem->GetRegion(), NptLoc, INSTRUCTION_ACTION::VECTOR);
        if (N)
        {
            N->SetSpeed(500);
            Elem->AddNavPoint(N);
        }

        N = new Instruction(air_region, FVector(0.0f, 0.0f, 10000.0f), INSTRUCTION_ACTION::VECTOR);
        if (N)
        {
            N->SetSpeed(500);
            Elem->AddNavPoint(N);
        }
    }

    // Otherwise escort the shuttle toward a carrier landing:
    else if (Carrier)
    {
        const FVector CarrierLoc = Carrier->Location();
        const FVector Src = CarrierLoc + GetRandomDirection() * 150000.0f;
        const FVector Dst = CarrierLoc + GetRandomDirection() * 25000.0f;

        Instruction* N = nullptr;

        Elem->SetLocation(Src);

        N = new Instruction(Elem->GetRegion(), Dst, INSTRUCTION_ACTION::DOCK);
        if (N)
        {
            N->SetTarget(FString(ANSI_TO_TCHAR(Carrier->GetName().data())));
            N->SetSpeed(500);
            Elem->AddNavPoint(N);
        }
    }
}

void CampaignMissionFighter::CreateWardStrike()
{
    if (!mission || !mission->GetStarSystem())
    {
        return;
    }

    CombatUnit* carrier = FindCarrier(squadron);
    CombatGroup* strike = strike_group;

    if (!strike || strike->CalcValue() < 1)
    {
        return;
    }

    int type = (int)EMISSIONTYPE::ASSAULT;
    if (airborne)
    {
        type = (int)EMISSIONTYPE::STRIKE;
    }

    MissionElement* elem = CreateFighterPackage(strike, 2, type);
    if (!elem)
    {
        return;
    }

    // Original logic toggled alert state if the strike package shares the same parent.
    // Keep the structure, but leave this line out unless your current player object
    // still exposes FlyingStart().
    // if (strike->GetParent() == squadron->GetParent()) { ... }

    elem->SetIntelLevel(Intel::KNOWN);
    elem->SetRegion(squadron->GetRegion());

    if (strike_target)
    {
        const FString Target = FString(ANSI_TO_TCHAR(strike_target->GetName().data()));

        Instruction* obj = new Instruction(
            INSTRUCTION_ACTION::ASSAULT,
            TCHAR_TO_ANSI(*Target)
        );

        if (obj)
        {
            if (airborne)
            {
                obj->SetAction(INSTRUCTION_ACTION::STRIKE);
            }

            elem->AddObjective(obj);
        }
    }

    ward = elem;
    mission->AddElement(elem);

    StarSystem* system = mission->GetStarSystem();
    OrbitalRegion* rgn1 = system ? system->FindRegion(elem->GetRegion()) : nullptr;
    if (!rgn1 || !rgn1->Primary())
    {
        return;
    }

    FVector delta(
        rgn1->Location().X - rgn1->Primary()->Location().X,
        rgn1->Location().Y - rgn1->Primary()->Location().Y,
        rgn1->Location().Z - rgn1->Primary()->Location().Z
    );

    FVector npt_loc(
        elem->GetLocation().X,
        elem->GetLocation().Y,
        elem->GetLocation().Z
    );

    Instruction* n = nullptr;

    if (airborne)
    {
        delta.Normalize();
        delta *= -30000.0f;
        npt_loc += delta;

        n = new Instruction(elem->GetRegion(), npt_loc, INSTRUCTION_ACTION::VECTOR);
        if (n)
        {
            n->SetSpeed(500);
            elem->AddNavPoint(n);
        }

        npt_loc = FVector(0.0f, 0.0f, 10000.0f);

        n = new Instruction(air_region, npt_loc, INSTRUCTION_ACTION::VECTOR);
        if (n)
        {
            n->SetSpeed(500);
            elem->AddNavPoint(n);
        }
    }

    // IP:
    if (strike_target)
    {
        delta = FVector(
            strike_target->GetLocation().X - npt_loc.X,
            strike_target->GetLocation().Y - npt_loc.Y,
            strike_target->GetLocation().Z - npt_loc.Z
        );

        delta.Normalize();
        delta *= 15000.0f;

        npt_loc = FVector(
            strike_target->GetLocation().X,
            strike_target->GetLocation().Y,
            strike_target->GetLocation().Z
        ) + delta + FVector(0.0f, 0.0f, 8000.0f);

        n = new Instruction(strike_target->GetRegion(), npt_loc, INSTRUCTION_ACTION::VECTOR);
        if (n)
        {
            n->SetSpeed(500);
            elem->AddNavPoint(n);
        }
    }

    if (carrier)
    {
        FVector src(
            carrier->Location().X,
            carrier->Location().Y,
            carrier->Location().Z
        );

        src += GetRandomDirection() * 100000.0f;
        elem->SetLocation(src);
    }
}

void CampaignMissionFighter::CreateEscorts()
{
    bool escort_needed = false;

    if (mission->GetType() == (int) EMISSIONTYPE::STRIKE || mission->GetType() == (int) EMISSIONTYPE::ASSAULT)
    {
        if (request && request->GetObjective())
        {
            int tgt_type = (int) request->GetObjective()->GetType();

            if (tgt_type == (int) ECOMBATGROUP_TYPE::CARRIER_GROUP ||
                tgt_type == (int) ECOMBATGROUP_TYPE::STATION ||
                tgt_type == (int) ECOMBATGROUP_TYPE::STARBASE)
            {
                escort_needed = true;
            }
        }
    }

    if (player_elem && escort_needed)
    {
        CombatGroup* s = FindSquadron(ownside, (int) ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);

        if (s && s->IsAssignable())
        {
            MissionElement* elem = CreateFighterPackage(s, 2, (int) EMISSIONTYPE::ESCORT_STRIKE);

            if (elem)
            {
                FVector offset(2000.0f, 2000.0f, 1000.0f);

                ListIter<Instruction> npt_iter = player_elem->NavList();
                while (++npt_iter)
                {
                    Instruction* npt = npt_iter.value();

                    FVector loc(
                        npt->Location().X + offset.X,
                        npt->Location().Y + offset.Y,
                        npt->Location().Z + offset.Z
                    );

                    Instruction* n = new Instruction(
                        npt->RegionName(),
                        loc,
                        INSTRUCTION_ACTION::ESCORT
                    );

                    if (n)
                    {
                        n->SetSpeed(npt->Speed());
                        elem->AddNavPoint(n);
                    }
                }

                mission->AddElement(elem);
            }
        }
    }
}

void CampaignMissionFighter::CreateTargets()
{
    switch (mission ? mission->GetType() : mission_type)
    {
    case (int) EMISSIONTYPE::PATROL:
    case (int) EMISSIONTYPE::AIR_PATROL:
        CreateTargetsPatrol();
        break;

    case (int) EMISSIONTYPE::SWEEP:
    case (int) EMISSIONTYPE::AIR_SWEEP:
        CreateTargetsSweep();
        break;

    case (int) EMISSIONTYPE::INTERCEPT:
    case (int) EMISSIONTYPE::AIR_INTERCEPT:
        CreateTargetsIntercept();
        break;

    case (int) EMISSIONTYPE::ESCORT_FREIGHT:
        CreateTargetsFreightEscort();
        break;

    case (int) EMISSIONTYPE::ESCORT_SHUTTLE:
        CreateTargetsShuttleEscort();
        break;

    case (int) EMISSIONTYPE::ESCORT_STRIKE:
        CreateTargetsStrikeEscort();
        break;

    case (int) EMISSIONTYPE::STRIKE:
        CreateTargetsStrike();
        break;

    case (int) EMISSIONTYPE::ASSAULT:
        CreateTargetsAssault();
        break;

    default:
        CreateTargetsPatrol();
        break;
    }
}

void CampaignMissionFighter::CreateTargetsPatrol()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    Text region = squadron->GetRegion();

    FVector base_loc(
        player_elem->GetLocation().X,
        player_elem->GetLocation().Y,
        player_elem->GetLocation().Z
    );

    FVector patrol_loc(0.0f, 0.0f, 0.0f);

    if (airborne)
    {
        base_loc =
            FVector(GetRandomPoint().X, GetRandomPoint().Y, GetRandomPoint().Z) * 2.0f +
            FVector(0.0f, 0.0f, 12000.0f);
    }
    else if (carrier_elem)
    {
        base_loc = FVector(
            carrier_elem->GetLocation().X,
            carrier_elem->GetLocation().Y,
            carrier_elem->GetLocation().Z
        );
    }

    if (airborne)
    {
        if (!airbase)
        {
            PlanetaryInsertion(player_elem);
        }

        region = air_region;

        const FVector Dir = GetRandomDirection();
        const float Dist = FMath::FRandRange(60000.0f, 100000.0f);

        patrol_loc = base_loc + Dir * Dist;
    }
    else
    {
        const FVector Dir = GetRandomDirection();
        const float Dist = FMath::FRandRange(110000.0f, 160000.0f);

        patrol_loc = base_loc + Dir * Dist;
    }

    Instruction* n = new Instruction(
        region,
        patrol_loc,
        INSTRUCTION_ACTION::PATROL
    );

    if (n)
    {
        player_elem->AddNavPoint(n);
    }

    int32 ntargets = FMath::RandRange(2, 5);
    while (ntargets > 0)
    {
        int t = CreateRandomTarget(region, patrol_loc);
        ntargets -= t;

        if (t < 1)
        {
            break;
        }
    }

    if (airborne && !airbase)
    {
        OrbitalInsertion(player_elem);
    }

    if (n)
    {
        Instruction* obj = new Instruction(*n);
        obj->SetTargetDesc(TCHAR_TO_ANSI(TEXT("inbound enemy units")));
        player_elem->AddObjective(obj);
    }

    if (carrier_elem && !airborne)
    {
        Instruction* obj = new Instruction(
            INSTRUCTION_ACTION::DEFEND,
            carrier_elem->GetName().data()
        );

        if (obj)
        {
            const FString Desc =
                FString(TEXT("the ")) +
                FString(ANSI_TO_TCHAR(carrier_elem->GetName().data())) +
                FString(TEXT(" battle group"));

            obj->SetTargetDesc(TCHAR_TO_ANSI(*Desc));
            player_elem->AddObjective(obj);
        }
    }
}

void CampaignMissionFighter::CreateTargetsSweep()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    double traverse = PI;

    double a = FMath::FRandRange(-PI / 2.0, PI / 2.0);

    FVector base_loc(
        player_elem->GetLocation().X,
        player_elem->GetLocation().Y,
        player_elem->GetLocation().Z
    );

    FVector sweep_loc = base_loc;
    Text region = player_elem->GetRegion();
    Instruction* n = nullptr;

    if (carrier_elem)
    {
        base_loc = FVector(
            carrier_elem->GetLocation().X,
            carrier_elem->GetLocation().Y,
            carrier_elem->GetLocation().Z
        );
    }

    if (airborne)
    {
        PlanetaryInsertion(player_elem);
        region = air_region;

        sweep_loc =
            FVector(GetRandomPoint().X, GetRandomPoint().Y, GetRandomPoint().Z) +
            FVector(0.0f, 0.0f, 10000.0f); // keep it airborne!
    }

    sweep_loc += FVector(
        FMath::Sin(a),
        -FMath::Cos(a),
        0.0f
    ) * 100000.0f;

    n = new Instruction(
        region,
        sweep_loc,
        INSTRUCTION_ACTION::VECTOR
    );

    if (n)
    {
        n->SetSpeed(750);
        player_elem->AddNavPoint(n);
    }

    int index = 0;
    int ntargets = 6;

    while (traverse > 0)
    {
        double a1 = FMath::FRandRange(PI / 4.0, PI / 2.0);
        traverse -= a1;
        a += a1;

        sweep_loc += FVector(
            FMath::Sin(a),
            -FMath::Cos(a),
            0.0f
        ) * 80000.0f;

        n = new Instruction(
            region,
            sweep_loc,
            INSTRUCTION_ACTION::SWEEP
        );

        if (n)
        {
            n->SetSpeed(750);
            n->SetFormation(INSTRUCTION_FORMATION::SPREAD);
            player_elem->AddNavPoint(n);
        }

        if (ntargets && FMath::FRand() < 0.5f)
        {
            ntargets -= CreateRandomTarget(region, sweep_loc);
        }

        index++;
    }

    if (ntargets > 0)
    {
        CreateRandomTarget(region, sweep_loc);
    }

    if (airborne && !airbase)
    {
        OrbitalInsertion(player_elem);
        region = player_elem->GetRegion();
    }

    sweep_loc = base_loc;
    sweep_loc.Y += 30000.0f;

    n = new Instruction(
        region,
        sweep_loc,
        INSTRUCTION_ACTION::VECTOR
    );

    if (n)
    {
        n->SetSpeed(750);
        player_elem->AddNavPoint(n);
    }

    Instruction* obj = new Instruction(
        region,
        sweep_loc,
        INSTRUCTION_ACTION::SWEEP
    );

    if (obj)
    {
        obj->SetTargetDesc(TCHAR_TO_ANSI(TEXT("enemy patrols")));
        player_elem->AddObjective(obj);
    }

    if (carrier_elem && !airborne)
    {
        obj = new Instruction(
            INSTRUCTION_ACTION::DEFEND,
            carrier_elem->GetName().data()
        );

        if (obj)
        {
            const FString Desc =
                FString(TEXT("the ")) +
                FString(ANSI_TO_TCHAR(carrier_elem->GetName().data())) +
                FString(TEXT(" battle group"));

            obj->SetTargetDesc(TCHAR_TO_ANSI(*Desc));
            player_elem->AddObjective(obj);
        }
    }
}

void CampaignMissionFighter::CreateTargetsIntercept()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    CombatUnit* carrier = FindCarrier(squadron);
    CombatGroup* s = FindSquadron(enemy, (int) ECOMBATGROUP_TYPE::ATTACK_SQUADRON);
    CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

    if (!s || !s2)
    {
        return;
    }

    int ninbound = 2 + (int)(GetRandomIndex() < 5);
    bool second = ninbound > 2;
    Text attacker;

    while (ninbound--)
    {
        MissionElement* elem = CreateFighterPackage(s, 4, (int)EMISSIONTYPE::ASSAULT);
        if (elem)
        {
            elem->SetIntelLevel(Intel::KNOWN);
            elem->Loadouts().destroy();
            elem->Loadouts().append(new MissionLoad(-1, "Hvy Ship Strike"));

            if (carrier)
            {
                Instruction* obj = new Instruction(
                    INSTRUCTION_ACTION::ASSAULT,
                    carrier->GetName().data()
                );

                if (obj)
                {
                    elem->AddObjective(obj);

                    FVector randPt(
                        GetRandomPoint().X,
                        GetRandomPoint().Y,
                        GetRandomPoint().Z
                    );

                    FVector carrierLoc(
                        carrier->Location().X,
                        carrier->Location().Y,
                        carrier->Location().Z
                    );

                    elem->SetLocation(carrierLoc + randPt * 6.0f);
                }
            }
            else
            {
                FVector randPt(
                    GetRandomPoint().X,
                    GetRandomPoint().Y,
                    GetRandomPoint().Z
                );

                FVector squadLoc(
                    squadron->GetLocation().X,
                    squadron->GetLocation().Y,
                    squadron->GetLocation().Z
                );

                elem->SetLocation(squadLoc + randPt * 5.0f);
            }

            mission->AddElement(elem);

            attacker = elem->GetName();

            if (!prime_target)
            {
                prime_target = elem;

                Instruction* obj = new Instruction(
                    INSTRUCTION_ACTION::INTERCEPT,
                    attacker.data()
                );

                if (obj)
                {
                    const FString TargetDesc =
                        FString(TEXT("inbound strike package '")) +
                        FString(ANSI_TO_TCHAR(elem->GetName().data())) +
                        FString(TEXT("'"));

                    obj->SetTargetDesc(TCHAR_TO_ANSI(*TargetDesc));
                    player_elem->AddObjective(obj);
                }
            }

            MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
            if (e2)
            {
                e2->SetIntelLevel(Intel::KNOWN);

                FVector randPt(
                    GetRandomPoint().X,
                    GetRandomPoint().Y,
                    GetRandomPoint().Z
                );

                FVector elemLoc(
                    elem->GetLocation().X,
                    elem->GetLocation().Y,
                    elem->GetLocation().Z
                );

                e2->SetLocation(elemLoc + randPt * 0.25f);

                Instruction* obj = new Instruction(
                    INSTRUCTION_ACTION::ESCORT,
                    elem->GetName().data()
                );

                if (obj)
                {
                    e2->AddObjective(obj);
                }

                mission->AddElement(e2);
            }
        }
    }

    if (second)
    {
        CombatGroup* friendlySquadron = FindSquadron(ownside, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

        if (friendlySquadron)
        {
            MissionElement* elem = CreateFighterPackage(friendlySquadron, 2, (int)EMISSIONTYPE::INTERCEPT);
            if (elem)
            {
                PlayerCharacter* p = PlayerCharacter::GetCurrentPlayer();
                elem->SetAlert(p ? !p->FlyingStart() : true);

                Instruction* obj = new Instruction(
                    INSTRUCTION_ACTION::INTERCEPT,
                    attacker.data()
                );

                if (obj)
                {
                    elem->AddObjective(obj);
                }

                mission->AddElement(elem);
            }
        }
    }

    if (carrier && !airborne)
    {
        Instruction* obj = new Instruction(
            INSTRUCTION_ACTION::DEFEND,
            carrier->GetName().data()
        );

        if (obj)
        {
            const FString TargetDesc =
                FString(TEXT("the ")) +
                FString(ANSI_TO_TCHAR(carrier->GetName().data())) +
                FString(TEXT(" battle group"));

            obj->SetTargetDesc(TCHAR_TO_ANSI(*TargetDesc));
            player_elem->AddObjective(obj);
        }
    }
}

void CampaignMissionFighter::CreateTargetsFreightEscort()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    if (!ward)
    {
        CreateTargetsPatrol();
        return;
    }

    CombatUnit* carrier = FindCarrier(squadron);
    CombatGroup* s = FindSquadron(enemy, (int) ECOMBATGROUP_TYPE::ATTACK_SQUADRON);
    CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

    if (!s)
    {
        s = s2;
    }

    if (!s || !s2)
    {
        return;
    }

    MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::ASSAULT);
    if (elem)
    {
        elem->SetIntelLevel(Intel::KNOWN);

        const FVector RandPt = GetRandomPoint();
        elem->SetLocation(ward->GetLocation() + RandPt * 5.0f);

        Instruction* obj = new Instruction(
            INSTRUCTION_ACTION::ASSAULT,
            ward->GetName().data()
        );

        if (obj)
        {
            elem->AddObjective(obj);
        }

        mission->AddElement(elem);

        MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
        if (e2)
        {
            e2->SetIntelLevel(Intel::KNOWN);

            const FVector EscortOffset = GetRandomPoint();
            e2->SetLocation(elem->GetLocation() + EscortOffset * 0.25f);

            Instruction* obj2 = new Instruction(
                INSTRUCTION_ACTION::ESCORT,
                elem->GetName().data()
            );

            if (obj2)
            {
                e2->AddObjective(obj2);
            }

            mission->AddElement(e2);
        }
    }

    Instruction* obj3 = new Instruction(
        mission->GetRegion(),
        FVector(0.0f, 0.0f, 0.0f),
        INSTRUCTION_ACTION::PATROL
    );

    if (obj3)
    {
        obj3->SetTargetDesc("enemy patrols");
        player_elem->AddObjective(obj3);
    }
}

void
CampaignMissionFighter::CreateTargetsShuttleEscort()
{
    CreateTargetsFreightEscort();
}

void CampaignMissionFighter::CreateTargetsStrikeEscort()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    if (ward)
    {
        FVector Offset(2000.0f, 2000.0f, 1000.0f);

        ListIter<Instruction> NptIter = ward->NavList();
        while (++NptIter)
        {
            Instruction* Npt = NptIter.value();

            Instruction* N = new Instruction(
                Npt->RegionName(),
                Npt->Location() + Offset,
                INSTRUCTION_ACTION::ESCORT
            );

            if (N)
            {
                N->SetSpeed(Npt->Speed());
                player_elem->AddNavPoint(N);
            }
        }
    }
}

void CampaignMissionFighter::CreateTargetsStrike()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    if (request && request->GetObjective())
    {
        strike_target = request->GetObjective();
    }

    if (strike_target && strike_group)
    {
        CreateElements(strike_target);

        ListIter<MissionElement> EIter = mission->GetElements();
        while (++EIter)
        {
            MissionElement* Elem = EIter.value();

            if (Elem->GetCombatGroup() == strike_target)
            {
                prime_target = Elem;

                Instruction* Obj = new Instruction(
                    INSTRUCTION_ACTION::STRIKE,
                    Elem->GetName().data()
                );

                if (Obj)
                {
                    const FString TargetDesc =
                        FString(TEXT("preplanned target '")) +
                        FString(ANSI_TO_TCHAR(Elem->GetName().data())) +
                        FString(TEXT("'"));

                    Obj->SetTargetDesc(TCHAR_TO_ANSI(*TargetDesc));
                    player_elem->AddObjective(Obj);
                }

                // create flight plan:
                RLoc Rloc;
                FVector Loc(0.0f, 0.0f, 15000.0f);
                Instruction* N = nullptr;

                PlanetaryInsertion(player_elem);

                // target approach and strike:
                FVector Delta = prime_target->GetLocation() - Loc;

                if (Delta.Size() >= 100000.0f)
                {
                    FVector Mid = Loc + Delta * 0.5f;
                    Mid.Z = 10000.0f;

                    Rloc.SetReferenceLoc(0);
                    Rloc.SetBaseLocation(Mid);
                    Rloc.SetDistance(20000.0f);
                    Rloc.SetDistanceVar(5000.0f);
                    Rloc.SetAzimuth(90 * DEGREES);
                    Rloc.SetAzimuthVar(25 * DEGREES);

                    N = new Instruction(
                        prime_target->GetRegion(),
                        FVector::ZeroVector,
                        INSTRUCTION_ACTION::VECTOR
                    );

                    if (N)
                    {
                        N->SetSpeed(750);
                        N->GetRLoc() = Rloc;
                        player_elem->AddNavPoint(N);
                    }

                    Loc = Mid;
                }

                Delta = Loc - prime_target->GetLocation();
                Delta.Normalize();
                Delta *= 25000.0f;

                Loc = prime_target->GetLocation() + Delta;
                Loc.Z = 8000.0f;

                N = new Instruction(
                    prime_target->GetRegion(),
                    Loc,
                    INSTRUCTION_ACTION::STRIKE
                );

                if (N)
                {
                    N->SetSpeed(500);
                    player_elem->AddNavPoint(N);
                }

                // exeunt:
                Rloc.SetReferenceLoc(0);
                Rloc.SetBaseLocation(FVector(0.0f, 0.0f, 30000.0f));
                Rloc.SetDistance(50000.0f);
                Rloc.SetDistanceVar(5000.0f);
                Rloc.SetAzimuth(-90 * DEGREES);
                Rloc.SetAzimuthVar(25 * DEGREES);

                N = new Instruction(
                    prime_target->GetRegion(),
                    FVector::ZeroVector,
                    INSTRUCTION_ACTION::VECTOR
                );

                if (N)
                {
                    N->SetSpeed(750);
                    N->GetRLoc() = Rloc;
                    player_elem->AddNavPoint(N);
                }

                if (carrier_elem)
                {
                    Rloc.SetReferenceLoc(0);
                    Rloc.SetBaseLocation(carrier_elem->GetLocation());
                    Rloc.SetDistance(60000.0f);
                    Rloc.SetDistanceVar(10000.0f);
                    Rloc.SetAzimuth(180 * DEGREES);
                    Rloc.SetAzimuthVar(30 * DEGREES);

                    N = new Instruction(
                        carrier_elem->GetRegion(),
                        FVector::ZeroVector,
                        INSTRUCTION_ACTION::RTB
                    );

                    if (N)
                    {
                        N->SetSpeed(750);
                        N->GetRLoc() = Rloc;
                        player_elem->AddNavPoint(N);
                    }
                }

                break;
            }
        }
    }
}

void CampaignMissionFighter::CreateTargetsAssault()
{
    if (!squadron || !player_elem)
    {
        return;
    }

    CombatGroup* Assigned = nullptr;

    if (request)
    {
        Assigned = request->GetObjective();
    }

    if (Assigned)
    {
        if (Assigned->GetType() > ECOMBATGROUP_TYPE::WING && Assigned->GetType() < ECOMBATGROUP_TYPE::FLEET)
        {
            mission->AddElement(CreateFighterPackage(Assigned, 2, (int)EMISSIONTYPE::CARGO));
        }
        else
        {
            CreateElements(Assigned);
        }

        // select the prime target element - choose the lowest ranking
        // unit of a DESRON, CBG, or CVBG:

        ListIter<MissionElement> EIter = mission->GetElements();
        while (++EIter)
        {
            MissionElement* Elem = EIter.value();

            if (Elem->GetCombatGroup() == Assigned)
            {
                if (!prime_target || Assigned->GetType() <= ECOMBATGROUP_TYPE::CARRIER_GROUP)
                {
                    prime_target = Elem;
                }
            }
        }

        if (prime_target)
        {
            MissionElement* Elem = prime_target;

            Instruction* Obj = new Instruction(
                INSTRUCTION_ACTION::ASSAULT,
                Elem->GetName().data()
            );

            if (Obj)
            {
                const FString TargetDesc =
                    FString(TEXT("preplanned target '")) +
                    FString(ANSI_TO_TCHAR(Elem->GetName().data())) +
                    FString(TEXT("'"));

                Obj->SetTargetDesc(TCHAR_TO_ANSI(*TargetDesc));
                player_elem->AddObjective(Obj);
            }

            // create flight plan:
            RLoc Rloc;
            FVector Dummy(0.0f, 0.0f, 0.0f);
            Instruction* Instr = nullptr;

            FVector Loc(
                player_elem->GetLocation().X,
                player_elem->GetLocation().Y,
                player_elem->GetLocation().Z
            );

            FVector Tgt(
                Elem->GetLocation().X,
                Elem->GetLocation().Y,
                Elem->GetLocation().Z
            );

            FVector Mid(0.0f, 0.0f, 0.0f);

            CombatGroup* TgtGroup = Elem->GetCombatGroup();
            if (TgtGroup && TgtGroup->GetFirstUnit() && TgtGroup->IsMovable())
            {
                Tgt = FVector(
                    TgtGroup->GetFirstUnit()->Location().X,
                    TgtGroup->GetFirstUnit()->Location().Y,
                    TgtGroup->GetFirstUnit()->Location().Z
                );
            }

            if (carrier_elem)
            {
                Loc = FVector(
                    carrier_elem->GetLocation().X,
                    carrier_elem->GetLocation().Y,
                    carrier_elem->GetLocation().Z
                );
            }

            Mid = Loc + (FVector(
                Elem->GetLocation().X,
                Elem->GetLocation().Y,
                Elem->GetLocation().Z
            ) - Loc) * 0.5f;

            Rloc.SetReferenceLoc(0);
            Rloc.SetBaseLocation(Mid);
            Rloc.SetDistance(40000.0f);
            Rloc.SetDistanceVar(5000.0f);
            Rloc.SetAzimuth(90 * DEGREES);
            Rloc.SetAzimuthVar(45 * DEGREES);

            Instr = new Instruction(
                Elem->GetRegion(),
                Dummy,
                INSTRUCTION_ACTION::VECTOR
            );

            if (Instr)
            {
                Instr->SetSpeed(750);
                Instr->GetRLoc() = Rloc;

                player_elem->AddNavPoint(Instr);

                if (FMath::FRand() < 0.5f)
                {
                    CreateRandomTarget(Elem->GetRegion(), Rloc.Location());
                }
            }

            Rloc.SetReferenceLoc(0);
            Rloc.SetBaseLocation(Tgt);
            Rloc.SetDistance(60000.0f);
            Rloc.SetDistanceVar(5000.0f);
            Rloc.SetAzimuth(120 * DEGREES);
            Rloc.SetAzimuthVar(15 * DEGREES);

            Instr = new Instruction(
                Elem->GetRegion(),
                Dummy,
                INSTRUCTION_ACTION::ASSAULT
            );

            if (Instr)
            {
                Instr->SetSpeed(750);
                Instr->GetRLoc() = Rloc;
                Instr->SetTarget(FString(ANSI_TO_TCHAR(Elem->GetName().data())));

                player_elem->AddNavPoint(Instr);
            }

            if (carrier_elem)
            {
                Rloc.SetReferenceLoc(0);
                Rloc.SetBaseLocation(Loc);
                Rloc.SetDistance(30000.0f);
                Rloc.SetDistanceVar(0.0f);
                Rloc.SetAzimuth(180 * DEGREES);
                Rloc.SetAzimuthVar(60 * DEGREES);

                Instr = new Instruction(
                    carrier_elem->GetRegion(),
                    Dummy,
                    INSTRUCTION_ACTION::RTB
                );

                if (Instr)
                {
                    Instr->SetSpeed(500);
                    Instr->GetRLoc() = Rloc;

                    player_elem->AddNavPoint(Instr);
                }
            }
        }
    }
}

int32 CampaignMissionFighter::CreateRandomTarget(const char* rgn, FVector base_loc)
{
    if (!mission)
    {
        return 0;
    }

    int32 ntargets = 0;
    int32 ttype = GetRandomIndex();
    bool oca = (mission->GetType() == (int)EMISSIONTYPE::SWEEP);

    if (ttype < 8)
    {
        CombatGroup* s = nullptr;

        if (ttype < 4)
        {
            s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);
        }
        else
        {
            s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);
        }

        if (s)
        {
            MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::SWEEP);
            if (elem)
            {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->SetRegion(rgn);

                const FVector RandPt = GetRandomPoint();
                elem->SetLocation(base_loc + RandPt * 1.5f);

                mission->AddElement(elem);
                ntargets++;
            }
        }
    }
    else if (ttype < 12)
    {
        if (oca)
        {
            CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::LCA_SQUADRON);

            if (s)
            {
                MissionElement* elem = CreateFighterPackage(s, 1, (int)EMISSIONTYPE::CARGO);
                if (elem)
                {
                    elem->SetIntelLevel(Intel::KNOWN);
                    elem->SetRegion(rgn);

                    const FVector RandPt = GetRandomPoint();
                    elem->SetLocation(base_loc + RandPt * 2.0f);

                    mission->AddElement(elem);
                    ntargets++;

                    CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

                    if (s2)
                    {
                        MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
                        if (e2)
                        {
                            e2->SetIntelLevel(Intel::KNOWN);
                            e2->SetRegion(rgn);

                            const FVector EscortOffset = GetRandomPoint();
                            e2->SetLocation(elem->GetLocation() + EscortOffset * 0.5f);

                            Instruction* obj = new Instruction(
                                INSTRUCTION_ACTION::ESCORT,
                                elem->GetName().data()
                            );
                            if (obj)
                            {
                                e2->AddObjective(obj);
                            }

                            mission->AddElement(e2);
                            ntargets++;
                        }
                    }
                }
            }
        }
        else
        {
            CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON);

            if (s)
            {
                MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::ASSAULT);
                if (elem)
                {
                    elem->SetIntelLevel(Intel::KNOWN);
                    elem->SetRegion(rgn);

                    const FVector RandPt = GetRandomPoint();
                    elem->SetLocation(base_loc + RandPt * 1.3f);

                    mission->AddElement(elem);
                    ntargets++;
                }
            }
        }
    }
    else if (ttype < 15)
    {
        if (oca)
        {
            CombatGroup* s = nullptr;

            if (airborne)
            {
                s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::LCA_SQUADRON);
            }
            else
            {
                s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FREIGHT);
            }

            if (s)
            {
                MissionElement* elem = CreateFighterPackage(s, 1, (int)EMISSIONTYPE::CARGO);
                if (elem)
                {
                    elem->SetIntelLevel(Intel::KNOWN);
                    elem->SetRegion(rgn);

                    const FVector RandPt = GetRandomPoint();
                    elem->SetLocation(base_loc + RandPt * 2.0f);

                    mission->AddElement(elem);
                    ntargets++;

                    CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);

                    if (s2)
                    {
                        MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
                        if (e2)
                        {
                            e2->SetIntelLevel(Intel::KNOWN);
                            e2->SetRegion(rgn);

                            const FVector EscortOffset = GetRandomPoint();
                            e2->SetLocation(elem->GetLocation() + EscortOffset * 0.5f);

                            Instruction* obj = new Instruction(
                                INSTRUCTION_ACTION::ESCORT,
                                elem->GetName().data()
                            );
                            if (obj)
                            {
                                e2->AddObjective(obj);
                            }

                            mission->AddElement(e2);
                            ntargets++;
                        }
                    }
                }
            }
        }
        else
        {
            CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON);

            if (s)
            {
                MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::ASSAULT);
                if (elem)
                {
                    elem->SetIntelLevel(Intel::KNOWN);
                    elem->SetRegion(rgn);

                    const FVector RandPt = GetRandomPoint();
                    elem->SetLocation(base_loc + RandPt * 1.1f);

                    mission->AddElement(elem);
                    ntargets++;

                    CombatGroup* s2 = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);

                    if (s2)
                    {
                        MissionElement* e2 = CreateFighterPackage(s2, 2, (int)EMISSIONTYPE::ESCORT);
                        if (e2)
                        {
                            e2->SetIntelLevel(Intel::KNOWN);
                            e2->SetRegion(rgn);

                            const FVector EscortOffset = GetRandomPoint();
                            e2->SetLocation(elem->GetLocation() + EscortOffset * 0.5f);

                            Instruction* obj = new Instruction(
                                INSTRUCTION_ACTION::ESCORT,
                                elem->GetName().data()
                            );
                            if (obj)
                            {
                                e2->AddObjective(obj);
                            }

                            mission->AddElement(e2);
                            ntargets++;
                        }
                    }
                }
            }
        }
    }
    else
    {
        CombatGroup* s = FindSquadron(enemy, (int)ECOMBATGROUP_TYPE::LCA_SQUADRON);

        if (s)
        {
            MissionElement* elem = CreateFighterPackage(s, 2, (int)EMISSIONTYPE::CARGO);
            if (elem)
            {
                elem->SetIntelLevel(Intel::KNOWN);
                elem->SetRegion(rgn);

                const FVector RandPt = GetRandomPoint();
                elem->SetLocation(base_loc + RandPt * 2.0f);

                mission->AddElement(elem);
                ntargets++;
            }
        }
    }

    return ntargets;
}

void CampaignMissionFighter::PlanetaryInsertion(MissionElement* elem)
{
    if (!mission || !elem)
    {
        return;
    }

    if (!mission->GetStarSystem())
    {
        return;
    }

    MissionElement* carrier = mission->FindElement(elem->GetCommander());
    StarSystem* system = mission->GetStarSystem();
    OrbitalRegion* rgn1 = system->FindRegion(elem->GetRegion());
    OrbitalRegion* rgn2 = system->FindRegion(air_region);

    FVector npt_loc(
        elem->GetLocation().X,
        elem->GetLocation().Y,
        elem->GetLocation().Z
    );

    Instruction* n = nullptr;
    PlayerCharacter* p = PlayerCharacter::GetCurrentPlayer();

    int32 flying_start = p ? p->FlyingStart() : 0;

    if (carrier && !flying_start)
    {
        FVector carrierLoc(
            carrier->GetLocation().X,
            carrier->GetLocation().Y,
            carrier->GetLocation().Z
        );

        npt_loc = carrierLoc + FVector(1000.0f, -5000.0f, 0.0f);
    }

    if (rgn1 && rgn2)
    {
        const double delta_t = mission->GetStart() - campaign->GetTime();

        const FVector r1 = rgn1->PredictLocation(delta_t);
        const FVector r2 = rgn2->PredictLocation(delta_t);

        FVector Delta = r2 - r1;

        Delta.Y *= -1.0f;
        Delta.Normalize();
        Delta *= 10000.0f;

        npt_loc += Delta;

        n = new Instruction(
            elem->GetRegion(),
            npt_loc,
            INSTRUCTION_ACTION::VECTOR
        );

        if (n)
        {
            n->SetSpeed(750);
            elem->AddNavPoint(n);
        }
    }

    n = new Instruction(
        air_region,
        FVector(0.0f, 0.0f, 15000.0f),
        INSTRUCTION_ACTION::VECTOR
    );

    if (n)
    {
        n->SetSpeed(750);
        elem->AddNavPoint(n);
    }
}

void CampaignMissionFighter::OrbitalInsertion(MissionElement* elem)
{
    Instruction* n = new Instruction(
        air_region,
        FVector(0.0f, 0.0f, 30000.0f),
        INSTRUCTION_ACTION::VECTOR
    );

    if (n)
    {
        n->SetSpeed(750);
        elem->AddNavPoint(n);
    }
}

MissionElement* CampaignMissionFighter::CreateSingleElement(CombatGroup* G, CombatUnit* U)
{
    if (!G || G->IsReserve())
    {
        return nullptr;
    }

    if (!U || U->LiveCount() < 1)
    {
        return nullptr;
    }

    // make sure this unit is actually in the right star system:
    Galaxy* GalaxyInst = Galaxy::GetInstance();
    if (GalaxyInst)
    {
        if (GalaxyInst->FindSystemByRegion(U->GetRegion()) !=
            GalaxyInst->FindSystemByRegion(squadron->GetRegion()))
        {
            return nullptr;
        }
    }

    // make sure this unit isn't already in the mission:
    ListIter<MissionElement> EIter = mission->GetElements();
    while (++EIter)
    {
        MissionElement* Elem = EIter.value();

        if (Elem && Elem->GetCombatUnit() == U)
        {
            return nullptr;
        }
    }

    MissionElement* Elem = new MissionElement;
    if (!Elem)
    {
        Exit();
        return nullptr;
    }

    if (U->GetName().length() > 0)
    {
        Elem->SetName(U->GetName());
    }
    else
    {
        Elem->SetName(U->GetDesignName());
    }

    Elem->SetElementID(pkg_id++);

    Elem->SetShipDesign(U->GetDesign());
    Elem->SetCount(U->LiveCount());
    Elem->SetIFF(U->GetIFF());
    Elem->SetIntelLevel(G->GetIntelLevel());
    Elem->SetRegion(U->GetRegion());
    Elem->SetHeading(U->GetHeading());

    const int32 UnitIndex = G->GetUnits().index(U);
    FVector BaseLoc = U->Location();
    bool bExact = U->IsStatic(); // exact unit-level placement

    if (BaseLoc.Size() < 1.0f)
    {
        BaseLoc = G->GetLocation();
        bExact = false;
    }

    if (UnitIndex < 0 || (UnitIndex > 0 && !bExact))
    {
        FVector Loc = GetRandomDirection();

        if (!U->IsStatic())
        {
            while (FMath::Abs(Loc.Y) > FMath::Abs(Loc.X))
            {
                Loc = GetRandomDirection();
            }

            Loc *= 10000.0f + 9000.0f * UnitIndex;
        }
        else
        {
            Loc *= 2000.0f + 2000.0f * UnitIndex;
        }

        Elem->SetLocation(BaseLoc + Loc);
    }
    else
    {
        Elem->SetLocation(BaseLoc);
    }

    if (G->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP)
    {
        if (U->Type() == (int) CLASSIFICATION::CARRIER)
        {
            Elem->SetMissionRole((int)EMISSIONTYPE::FLIGHT_OPS);

            if (squadron && Elem->GetCombatGroup() == squadron->FindCarrier())
            {
                carrier_elem = Elem;
            }
            else if (!carrier_elem && U->GetIFF() == squadron->GetIFF())
            {
                carrier_elem = Elem;
            }
        }
        else
        {
            Elem->SetMissionRole((int)EMISSIONTYPE::ESCORT);
        }
    }
    else if (U->Type() == (int) CLASSIFICATION::STATION ||
        U->Type() == (int) CLASSIFICATION::STARBASE)
    {
        Elem->SetMissionRole((int)EMISSIONTYPE::FLIGHT_OPS);

        if (squadron && Elem->GetCombatGroup() == squadron->FindCarrier())
        {
            carrier_elem = Elem;

            if (U->Type() == (int) CLASSIFICATION::STARBASE)
            {
                airbase = true;
            }
        }
    }
    else if (U->Type() == (int) CLASSIFICATION::FARCASTER)
    {
        Elem->SetMissionRole((int)EMISSIONTYPE::OTHER);

        // link farcaster to other terminus:
        const FString Name = FString(ANSI_TO_TCHAR(U->GetName().data()));
        int32 Dash = INDEX_NONE;

        for (int32 i = 0; i < Name.Len(); i++)
        {
            if (Name[i] == TCHAR('-'))
            {
                Dash = i;
            }
        }

        const FString Src = (Dash != INDEX_NONE) ? Name.Left(Dash) : Name;
        const FString Dst = (Dash != INDEX_NONE) ? Name.Mid(Dash + 1) : FString();

        const FString Link = Dst + TEXT("-") + Src;

        Instruction* Obj = new Instruction(
            INSTRUCTION_ACTION::VECTOR,
            TCHAR_TO_ANSI(*Link)
        );

        Elem->AddObjective(Obj);
    }
    else if ((U->Type() & (int)CLASSIFICATION::STARSHIPS) != 0)
    {
        Elem->SetMissionRole((int) EMISSIONTYPE::FLEET);
    }

    Elem->SetCombatGroup(G);
    Elem->SetCombatUnit(U);

    return Elem;
}

CombatUnit* CampaignMissionFighter::FindCarrier(CombatGroup* G)
{
    CombatGroup* Carrier = G ? G->FindCarrier() : nullptr;

    if (Carrier && Carrier->GetUnits().size())
    {
        MissionElement* CarrierElem = mission->FindElement(Carrier->GetName());

        if (CarrierElem)
        {
            return Carrier->GetUnits().at(0);
        }
    }

    return nullptr;
}

MissionElement* CampaignMissionFighter::CreateFighterPackage(CombatGroup* InSquadron, int32 count, int32 role)
{
    if (!InSquadron)
    {
        return nullptr;
    }

    CombatUnit* fighter = InSquadron->GetUnits().at(0);
    CombatUnit* carrier = FindCarrier(InSquadron);

    if (!fighter)
    {
        return nullptr;
    }

    int32 avail = fighter->LiveCount();
    int32 actual = count;

    if (avail < actual)
    {
        actual = avail;
    }

    if (avail < 1)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("CMF - Insufficient fighters in squadron '%s' - %d required, %d available"),
            *FString(ANSI_TO_TCHAR(InSquadron->GetName().data())),
            count,
            avail
        );

        return nullptr;
    }

    MissionElement* elem = new MissionElement;
    if (!elem)
    {
        Exit();
        return nullptr;
    }

    elem->SetName(Callsign::GetCallsign(fighter->GetIFF()));
    elem->SetElementID(pkg_id++);

    if (carrier)
    {
        elem->SetCommander(carrier->GetName());
        elem->SetHeading(carrier->GetHeading());
    }
    else
    {
        elem->SetHeading(fighter->GetHeading());
    }

    elem->SetShipDesign(fighter->GetDesign());
    elem->SetCount(actual);
    elem->SetIFF(fighter->GetIFF());
    elem->SetIntelLevel(InSquadron->GetIntelLevel());
    elem->SetRegion(fighter->GetRegion());
    elem->SetSquadron(InSquadron->GetName());
    elem->SetMissionRole(role);

    switch ((EMISSIONTYPE)role)
    {
    case EMISSIONTYPE::ASSAULT:
        if (request && request->GetObjective() &&
            request->GetObjective()->GetType() == ECOMBATGROUP_TYPE::MINEFIELD)
        {
            elem->Loadouts().append(new MissionLoad(-1, "Rockets"));
        }
        else
        {
            elem->Loadouts().append(new MissionLoad(-1, "Ship Strike"));
        }
        break;

    case EMISSIONTYPE::STRIKE:
        elem->Loadouts().append(new MissionLoad(-1, "Ground Strike"));
        break;

    default:
        elem->Loadouts().append(new MissionLoad(-1, "ACM Medium Range"));
        break;
    }

    if (carrier)
    {
        FVector Offset = GetRandomPoint() * 0.3f;
        Offset.Y = FMath::Abs(Offset.Y);
        Offset.Z += 2000.0f;

        elem->SetLocation(carrier->Location() + Offset);
    }
    else
    {
        const FVector RandPt = GetRandomPoint();
        elem->SetLocation(fighter->Location() + RandPt);
    }

    elem->SetCombatGroup(InSquadron);
    elem->SetCombatUnit(fighter);

    return elem;
}

CombatGroup* CampaignMissionFighter::FindSquadron(int32 Iff, int32 Type)
{
    if (!squadron)
    {
        return nullptr;
    }

    CombatGroup* Result = nullptr;
    Campaign* CampaignPtr = Campaign::GetCampaign();

    if (CampaignPtr)
    {
        ListIter<Combatant> CombatantIter = CampaignPtr->GetCombatants();
        while (++CombatantIter && !Result)
        {
            if (CombatantIter->GetIFF() == Iff)
            {
                Result = ::FindCombatGroup(
                    CombatantIter->GetForce(),
                    static_cast<ECOMBATGROUP_TYPE>(Type)
                );

                if (Result && Result->CountUnits() < 1)
                {
                    Result = nullptr;
                }
            }
        }
    }

    return Result;
}

void CampaignMissionFighter::DefineMissionObjectives()
{
    if (!mission || !player_elem)
    {
        return;
    }

    if (prime_target)
    {
        mission->SetTarget(prime_target);
    }

    if (ward)
    {
        mission->SetWard(ward);
    }

    FString Objectives;

    for (int32 i = 0; i < player_elem->Objectives().size(); i++)
    {
        Instruction* Obj = player_elem->Objectives().at(i);
        if (!Obj)
        {
            continue;
        }

        Objectives += TEXT("* ");
        Objectives += FString(ANSI_TO_TCHAR(Obj->GetDescription()));
        Objectives += TEXT(".\n");
    }

    mission->SetObjective(TCHAR_TO_ANSI(*Objectives));
}

MissionInfo* CampaignMissionFighter::DescribeMission()
{
    if (!mission || !player_elem)
    {
        return nullptr;
    }

    FString Name;
    FString PlayerInfo;

    const char* RawTypeName = mission->GetTypeName();
    const FString TypeName = RawTypeName ? FString(ANSI_TO_TCHAR(RawTypeName)) : TEXT("Unknown");

    if (mission_info && mission_info->name.length() && mission_info->name.data())
    {
        Name = FString::Printf(
            TEXT("MSN-%03d %s"),
            mission->GetIdentity(),
            *FString(ANSI_TO_TCHAR(mission_info->name.data()))
        );
    }
    else if (ward)
    {
        const char* RawWardName = ward->GetName().data();

        Name = FString::Printf(
            TEXT("MSN-%03d %s %s"),
            mission->GetIdentity(),
            *TypeName,
            *(RawWardName ? FString(ANSI_TO_TCHAR(RawWardName)) : FString(TEXT("Unknown")))
        );
    }
    else if (prime_target)
    {
        const char* RawClassName = nullptr;
        const char* RawPrimeName = prime_target->GetName().data();

        if (prime_target->GetShipDesign())
        {
            RawClassName = Ship::GetShipClassName(prime_target->GetShipDesign()->type);
        }

        /*Name = FString::Printf(
            TEXT("MSN-%03d %s %s %s"),
            mission->GetIdentity(),
            *TypeName,
            *(RawClassName ? FString(ANSI_TO_TCHAR(RawClassName)) : FString(TEXT("UnknownClass"))),
            *(RawPrimeName ? FString(ANSI_TO_TCHAR(RawPrimeName)) : FString(TEXT("UnknownTarget")))
        );*/

        Name = FString::Printf(
            TEXT("MSN-%03d %s %s %s"),
            mission->GetIdentity(),
            *TypeName,
            *(RawClassName ? FString(ANSI_TO_TCHAR(RawClassName)) : FString(TEXT(""))),
            *(RawPrimeName ? FString(ANSI_TO_TCHAR(RawPrimeName)) : FString(TEXT("")))
        );
    }
    else
    {
        Name = FString::Printf(
            TEXT("MSN-%03d %s"),
            mission->GetIdentity(),
            *TypeName
        );
    }

    if (player_elem && player_elem->GetShipDesign())
    {
        const char* RawAbrv = (const char*)player_elem->GetShipDesign()->abrv;
        const char* RawDesignName = (const char*)player_elem->GetShipDesign()->name;
        const char* RawElemName = player_elem->GetName().data();

        PlayerInfo = FString::Printf(
            TEXT("%d x %s %s '%s'"),
            player_elem->Count(),
            *(RawAbrv ? FString(ANSI_TO_TCHAR(RawAbrv)) : FString(TEXT("UNK"))),
            *(RawDesignName ? FString(ANSI_TO_TCHAR(RawDesignName)) : FString(TEXT("UnknownDesign"))),
            *(RawElemName ? FString(ANSI_TO_TCHAR(RawElemName)) : FString(TEXT("UnknownElement")))
        );
    }

    MissionInfo* Info = new MissionInfo;
    if (!Info)
    {
        return nullptr;
    }

    Info->id = mission->GetIdentity();
    Info->mission = mission;
    Info->name = TCHAR_TO_ANSI(*Name);
    Info->type = mission->GetType();
    Info->player_info = TCHAR_TO_ANSI(*PlayerInfo);
    Info->description = mission->GetObjective();
    Info->start = mission->GetStart();

    if (mission->GetStarSystem())
    {
        Info->system = mission->GetStarSystem()->GetName();
    }

    Info->region = mission->GetRegion();

    mission->SetName(TCHAR_TO_ANSI(*Name));

    return Info;
}

// +--------------------------------------------------------------------+

void CampaignMissionFighter::Exit()
{
    request = nullptr;
    mission_info = nullptr;

    squadron = nullptr;
    strike_group = nullptr;
    strike_target = nullptr;
    mission = nullptr;

    player_elem = nullptr;
    carrier_elem = nullptr;
    ward = nullptr;
    prime_target = nullptr;
    escort = nullptr;

    air_region = "";
    orb_region = "";

    airborne = false;
    airbase = false;

    ownside = 0;
    enemy = -1;
    mission_type = 0;
}