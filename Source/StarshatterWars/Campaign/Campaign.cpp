/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Campaign.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Campaign defines a strategic military scenario.
*/

#include "Campaign.h"

#include "CampaignPlanStrategic.h"
#include "CampaignPlanAssignment.h"
#include "CampaignPlanEvent.h"
#include "CampaignPlanMission.h"
#include "CampaignPlanMovement.h"
#include "CampaignSituationReport.h"
#include "CampaignSaveGame.h"
#include "Combatant.h"
#include "CombatAction.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "CombatRoster.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Galaxy.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "StarSystem.h"
#include "Starshatter.h"
#include "PlayerCharacter.h"
#include "MissionInfo.h"

#include "Game.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "FormatUtil.h"
#include "GameScreen.h"
#include "GameStructs.h"
#include "FormattingUtils.h"
#include "StarshatterGameDataSubsystem.h"

// Unreal minimal support:
#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogCampaign, Log, All);

const int TIME_NEVER = (int)1e9;
const int ONE_DAY = (int)24 * 3600;

TemplateList::TemplateList()
    : mission_type(0)
    , group_type(0)
    , index(0)
{
}

TemplateList::~TemplateList()
{
    missions.destroy();
}

// +====================================================================+

static List<Campaign> campaigns;
static Campaign* current_campaign = 0;

// +--------------------------------------------------------------------+
static void DumpCombatGroupTree(CombatGroup* Group, int Depth = 0)
{
    if (!Group)
        return;

    FString Indent;
    for (int i = 0; i < Depth; i++)
    {
        Indent += TEXT("  ");
    }

    UE_LOG(LogCampaign, Warning,
        TEXT("%s[Group] name=%s type=%d iff=%d reserve=%d value=%d children=%d"),
        *Indent,
        Group->GetName().data() ? ANSI_TO_TCHAR(Group->GetName().data()) : TEXT("NULL"),
        (int)Group->GetType(),
        Group->GetIFF(),
        Group->IsReserve() ? 1 : 0,
        Group->CalcValue(),
        Group->GetComponents().size());

    List<CombatGroup>& Children = Group->GetComponents();
    for (int i = 0; i < Children.size(); i++)
    {
        DumpCombatGroupTree(Children[i], Depth + 1);
    }
}

Campaign::Campaign(int id, const char* n)
    : campaign_id(id)
    , name(n)
    , mission_id(-1)
    , mission(0)
    , net_mission(0)
    , scripted(IsScripted())
    , sequential(false)
    , time(0)
    , startTime(0)
    , loadTime(0)
    , player_group(0)
    , player_unit(0)
    , campaign_status(ECampaignStatus::INIT)
    , lockout(0)
    , loaded_from_savegame(false)
{
    FMemory::Memzero(path, sizeof(path));
    Load();
}

Campaign::Campaign(int id, const char* n, const char* p)
    : campaign_id(id)
    , name(n)
    , mission_id(-1)
    , mission(0)
    , net_mission(0)
    , scripted(IsScripted())
    , sequential(false)
    , time(0)
    , startTime(0)
    , loadTime(0)
    , player_group(0)
    , player_unit(0)
    , campaign_status(ECampaignStatus::INIT)
    , lockout(0)
    , loaded_from_savegame(false)
{
    FMemory::Memzero(path, sizeof(path));
    FCStringAnsi::Strncpy(path, p ? p : "", (int)sizeof(path));
    Load();
}

Campaign::Campaign(int id, const char* n, bool bSkipLoad)
    : campaign_id(id)
    , campaign_status(ECampaignStatus::INIT)
    , name(n)
    , mission_id(-1)
    , mission(0)
    , net_mission(0)
    , scripted(IsScripted())
    , sequential(false)
    , loaded_from_savegame(false)
    , player_group(0)
    , player_unit(0)
    , time(0)
    , loadTime(0)
    , startTime(0)
    , updateTime(0)
    , lockout(0)
{
    FMemory::Memzero(filename, sizeof(filename));
    FMemory::Memzero(path, sizeof(path));

    if (!bSkipLoad)
    {
        Load();
    }
}
// +--------------------------------------------------------------------+

Campaign::~Campaign()
{
    for (int i = 0; i < NUM_IMAGES; i++)
        image[i].ClearImage();

    delete net_mission;

    actions.destroy();
    events.destroy();
    missions.destroy();
    templates.destroy();
    planners.destroy();
    zones.destroy();
    combatants.destroy();
}

// +--------------------------------------------------------------------+

void Campaign::SetCampaignData(const FS_Campaign* InData)
{
    CampaignData = InData;
}

void
Campaign::Initialize()
{
    Campaign* c = 0;
    DataLoader* loader = DataLoader::GetLoader();

    for (int i = 1; i < 100; i++) {
        char TempPath[256];
        FCStringAnsi::Snprintf(TempPath, sizeof(TempPath), "Campaigns/%02d/", i);

        loader->UseFileSystem(true);
        loader->SetDataPath(TempPath);

        if (loader->FindFile("campaign.def")) {
            char txt[256];
            FCStringAnsi::Snprintf(txt, sizeof(txt), "Dynamic Campaign %02d", i);

            c = new Campaign(i, txt);

            if (c)
                campaigns.insertSort(c);
        }
    }

    c = new Campaign(SINGLE_MISSIONS, "Single Missions");
    if (c) {
        campaigns.insertSort(c);
        current_campaign = c;
    }

    c = new Campaign(MULTIPLAYER_MISSIONS, "Multiplayer Missions");
    if (c) {
        campaigns.insertSort(c);
    }

    c = new Campaign(CUSTOM_MISSIONS, "Custom Missions");
    if (c) {
        campaigns.insertSort(c);
    }
}

void
Campaign::Close()
{
    UE_LOG(LogCampaign, Log, TEXT("Campaign::Close() - destroying all campaigns"));
    current_campaign = 0;
    campaigns.destroy();
}

Campaign*
Campaign::GetCampaign()
{
    return current_campaign;
}

Campaign*
Campaign::SelectCampaign(const char* InName)
{
    Campaign* c = 0;
    ListIter<Campaign> iter = campaigns;

    while (++iter && !c) {
        if (InName && FCStringAnsi::Stricmp(iter->Name(), InName) == 0)
            c = iter.value();
    }

    if (c) {
        UE_LOG(LogCampaign, Log, TEXT("Campaign: Selected '%s'"), ANSI_TO_TCHAR(c->Name()));
        current_campaign = c;
    }
    else {
        UE_LOG(LogCampaign, Warning, TEXT("Campaign: could not find '%s'"), InName ? ANSI_TO_TCHAR(InName) : TEXT("(null)"));
    }

    return c;
}

Campaign*
Campaign::CreateCustomCampaign(const char* InName, const char* InPath)
{
    int id = 0;

    if (InName && *InName && InPath && *InPath) {
        ListIter<Campaign> iter = campaigns;

        while (++iter) {
            Campaign* c = iter.value();

            if (c->GetCampaignId() >= id)
                id = c->GetCampaignId() + 1;

            if (FCStringAnsi::Strcmp(c->Name(), InName) == 0) {
                UE_LOG(LogCampaign, Warning, TEXT("Campaign: custom campaign '%s' already exists."), ANSI_TO_TCHAR(InName));
                return 0;
            }
        }
    }

    if (id == 0)
        id = CUSTOM_MISSIONS + 1;

    Campaign* c = new Campaign(id, InName, InPath);
    UE_LOG(LogCampaign, Log, TEXT("Campaign: created custom campaign %d '%s'"), id, InName ? ANSI_TO_TCHAR(InName) : TEXT("(null)"));
    campaigns.append(c);

    return c;
}

List<Campaign>&
Campaign::GetAllCampaigns()
{
    return campaigns;
}

int
Campaign::GetLastCampaignId()
{
    int result = 0;

    for (int i = 0; i < campaigns.size(); i++) {
        Campaign* c = campaigns.at(i);

        if (c->IsDynamic() && c->GetCampaignId() > result) {
            result = c->GetCampaignId();
        }
    }

    return result;
}

// +--------------------------------------------------------------------+

CombatEvent*
Campaign::GetLastEvent()
{
    CombatEvent* result = 0;

    if (!events.isEmpty())
        result = events.last();

    return result;
}

// +--------------------------------------------------------------------+

int
Campaign::CountNewEvents() const
{
    int result = 0;

    for (int i = 0; i < events.size(); i++) {
        if (!events[i]->Visited())
            result++;
    }

    return result;
}

// +--------------------------------------------------------------------+

void
Campaign::Clear()
{
    missions.destroy();
    planners.destroy();
    combatants.destroy();
    events.destroy();
    actions.destroy();

    player_group = 0;
    player_unit = 0;

    updateTime = time;
}

// +--------------------------------------------------------------------+

void
Campaign::Load()
{
    // first, unload any existing data:
    Unload();

    if (!path[0]) {
        // then load the campaign from files:
        switch (campaign_id) {
        case SINGLE_MISSIONS:      FCStringAnsi::Strcpy(path, "Missions/");       break;
        case CUSTOM_MISSIONS:      FCStringAnsi::Strcpy(path, "Mods/Missions/");  break;
        case MULTIPLAYER_MISSIONS: FCStringAnsi::Strcpy(path, "Multiplayer/");    break;
        default:                   FCStringAnsi::Snprintf(path, sizeof(path), "Campaigns/%02d/", campaign_id); break;
        }
    }

    DataLoader* loader = DataLoader::GetLoader();
    loader->UseFileSystem(true);
    loader->SetDataPath(path);
    systems.clear();

    if (loader->FindFile("zones.def"))
        zones.append(CombatZone::Load("zones.def"));

    for (int i = 0; i < zones.size(); i++) {
        Text s = zones[i]->GetSystem();
        bool found = false;

        for (int n = 0; !found && n < systems.size(); n++) {
            if (s == systems[n]->GetName())
                found = true;
        }

        if (!found)
            systems.append(Galaxy::GetInstance()->GetSystem(s));
    }

    loader->UseFileSystem(Starshatter::UseFileSystem());

    if (loader->FindFile("campaign.def"))
        LoadCampaign(loader);

    if (campaign_id == CUSTOM_MISSIONS) {
        loader->SetDataPath(path);
        LoadCustomMissions(loader);
    }
    else {
        bool found = false;

        if (loader->FindFile("missions.def")) {
            loader->SetDataPath(path);
            LoadMissionList(loader);
            found = true;
        }

        if (loader->FindFile("templates.def")) {
            loader->SetDataPath(path);
            LoadTemplateList(loader);
            found = true;
        }

        if (!found) {
            loader->SetDataPath(path);
            LoadCustomMissions(loader);
        }
    }

    loader->UseFileSystem(true);
    loader->SetDataPath(path);

    if (loader->FindFile("image.pcx")) {
        loader->LoadGameBitmap("image.pcx", image[0]);
        loader->LoadGameBitmap("selected.pcx", image[1]);
        loader->LoadGameBitmap("unavail.pcx", image[2]);
        loader->LoadGameBitmap("banner.pcx", image[3]);
    }

    loader->SetDataPath(0);
    loader->UseFileSystem(Starshatter::UseFileSystem());
}

void
Campaign::Unload()
{
    SetCampaignStatus(ECampaignStatus::INIT);

    Game::ResetGameTime();
    StarSystem::SetBaseTime(0);

    startTime = GetStardate();
    loadTime = startTime;
    lockout = 0;

    for (int i = 0; i < NUM_IMAGES; i++)
        image[i].ClearImage();

    Clear();

    zones.destroy();
}

void
Campaign::LoadCampaign(DataLoader* loader, bool full)
{
    BYTE* block = 0;
    const char* SourceFilename = "campaign.def";

    loader->UseFileSystem(true);
    loader->LoadBuffer(SourceFilename, block, true);
    loader->UseFileSystem(Starshatter::UseFileSystem());

    Parser parser(new BlockReader((const char*)block));
    Term* term = parser.ParseTerm();

    if (!term) {
        return;
    }
    else {
        TermText* file_type = term->isText();
        if (!file_type || file_type->value() != "CAMPAIGN") {
            return;
        }
    }

    do {
        delete term; term = 0;
        term = parser.ParseTerm();

        if (term) {
            TermDef* def = term->isDef();
            if (def) {
                if (def->name()->value() == "name") {
                    if (!def->term() || !def->term()->isText()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: name missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        name = def->term()->isText()->value();
                        name = Game::GetText(name);
                    }
                }
                else if (def->name()->value() == "desc") {
                    if (!def->term() || !def->term()->isText()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: description missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        description = def->term()->isText()->value();
                        description = Game::GetText(description);
                    }
                }
                else if (def->name()->value() == "situation") {
                    if (!def->term() || !def->term()->isText()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: situation missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        situation = def->term()->isText()->value();
                        situation = Game::GetText(situation);
                    }
                }
                else if (def->name()->value() == "orders") {
                    if (!def->term() || !def->term()->isText()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: orders missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        orders = def->term()->isText()->value();
                        orders = Game::GetText(orders);
                    }
                }
                else if (def->name()->value() == "scripted") {
                    if (def->term() && def->term()->isBool()) {
                        scripted = def->term()->isBool()->value();
                    }
                }
                else if (def->name()->value() == "sequential") {
                    if (def->term() && def->term()->isBool()) {
                        sequential = def->term()->isBool()->value();
                    }
                }
                else if (full && def->name()->value() == "combatant") {
                    if (!def->term() || !def->term()->isStruct()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: combatant struct missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        TermStruct* val = def->term()->isStruct();

                        char        cname[64];
                        CombatGroup* force = 0;
                        CombatGroup* clone = 0;

                        FMemory::Memzero(cname, sizeof(cname));

                        for (int i = 0; i < val->elements()->size(); i++) {
                            TermDef* pdef = val->elements()->at(i)->isDef();
                            if (pdef) {
                                if (pdef->name()->value() == "name") {
                                    GetDefText(cname, pdef, SourceFilename);

                                    force = CombatRoster::GetInstance()->GetForce(cname);

                                    if (force)
                                        clone = force->Clone(false); // shallow copy
                                }
                                else if (pdef->name()->value() == "group") {
                                    ParseGroup(pdef->term()->isStruct(), force, clone, SourceFilename);
                                }
                            }
                        }

                        loader->SetDataPath(path);
                        Combatant* c = new Combatant(cname, clone);
                        if (c) {
                            combatants.append(c);
                        }
                        else {
                            Unload();
                            return;
                        }
                    }
                }
                else if (full && def->name()->value() == "action") {
                    if (!def->term() || !def->term()->isStruct()) {
                        UE_LOG(LogCampaign, Warning, TEXT("WARNING: action struct missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath() ? loader->GetDataPath() : ""),
                            ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        TermStruct* val = def->term()->isStruct();
                        ParseAction(val, SourceFilename);
                    }
                }
            }
        }
    } while (term);

    loader->ReleaseBuffer(block);
}

// +--------------------------------------------------------------------+

void
Campaign::ParseGroup(TermStruct* val, CombatGroup* force, CombatGroup* clone, const char* SourceFilename)
{
    if (!val) {
        UE_LOG(LogCampaign, Warning, TEXT("invalid combat group in campaign %s"), ANSI_TO_TCHAR(name.data()));
        return;
    }

    ECOMBATGROUP_TYPE type = ECOMBATGROUP_TYPE::NONE;
    int id = 0;

    for (int i = 0; i < val->elements()->size(); i++) {
        TermDef* pdef = val->elements()->at(i)->isDef();
        if (pdef) {
            if (pdef->name()->value() == "type") {
                char type_name[64];
                GetDefText(type_name, pdef, SourceFilename);
                type = CombatGroup::TypeFromName(type_name);
            }
            else if (pdef->name()->value() == "id") {
                GetDefNumber(id, pdef, SourceFilename);
            }
        }
    }

    if ((int) type && id && force && clone) {
        CombatGroup* g = force->FindGroup(type, id);

        // found original group, now clone it over
        if (g && g->GetParent()) {
            CombatGroup* parent = CloneOver(force, clone, g->GetParent());
            if (parent)
                parent->AddComponent(g->Clone());
        }
    }
}

// +--------------------------------------------------------------------+

void
Campaign::ParseAction(TermStruct* val, const char* SourceFilename)
{
    if (!val) {
        UE_LOG(LogCampaign, Warning, TEXT("invalid action in campaign %s"), ANSI_TO_TCHAR(name.data()));
        return;
    }

    int     id = 0;
    int     type = 0;
    int     subtype = 0;
    int     opp_type = -1;
    int     team = 0;
    ECombatEventSource     source = ECombatEventSource::NONE;
    FVector loc(0.0f, 0.0f, 0.0f);

    Text    system;
    Text    region;
    Text    file;
    Text    image_file;
    Text    scene_file;
    Text    text;

    int     count = 1;
    int     start_before = TIME_NEVER;
    int     start_after = 0;
    int     min_rank = 0;
    int     max_rank = 100;
    int     delay = 0;
    int     probability = 100;

    int     asset_type = 0;
    int     asset_id = 0;
    int     target_type = 0;
    int     target_id = 0;
    int     target_iff = 0;

    CombatAction* action = 0;

    for (int i = 0; i < val->elements()->size(); i++) {
        TermDef* pdef = val->elements()->at(i)->isDef();
        if (pdef) {
            if (pdef->name()->value() == "id") {
                GetDefNumber(id, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "type") {
                char txt[64];
                GetDefText(txt, pdef, SourceFilename);
                type = CombatAction::TypeFromName(txt);
            }
            else if (pdef->name()->value() == "subtype") {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(subtype, pdef, SourceFilename);
                }
                else if (pdef->term()->isText()) {
                    char txt[64];
                    GetDefText(txt, pdef, SourceFilename);

                    if (type == ECombatActionType::MISSION_TEMPLATE)
                        subtype = Mission::TypeFromName(txt);
                    else if (type == ECombatActionType::COMBAT_EVENT)
                        subtype = (int) CombatEvent::GetTypeFromName(txt);
                    else if (type == ECombatActionType::INTEL_EVENT)
                        subtype = Intel::IntelFromName(txt);
                }
            }
            else if (pdef->name()->value() == "opp_type") {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(opp_type, pdef, SourceFilename);
                }
                else if (pdef->term()->isText()) {
                    char txt[64];
                    GetDefText(txt, pdef, SourceFilename);

                    if (type == ECombatActionType::MISSION_TEMPLATE)
                        opp_type = Mission::TypeFromName(txt);
                }
            }
            else if (pdef->name()->value() == "source") {
                char txt[64];
                GetDefText(txt, pdef, SourceFilename);
                source = CombatEvent::GetSourceFromName(txt);
            }
            else if (pdef->name()->value() == "team" || pdef->name()->value() == "iff") {
                GetDefNumber(team, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "count") {
                GetDefNumber(count, pdef, SourceFilename);
            }
            else if (pdef->name()->value().contains("before")) {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(start_before, pdef, SourceFilename);
                }
                else {
                    GetDefTime(start_before, pdef, SourceFilename);
                    start_before -= ONE_DAY;
                }
            }
            else if (pdef->name()->value().contains("after")) {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(start_after, pdef, SourceFilename);
                }
                else {
                    GetDefTime(start_after, pdef, SourceFilename);
                    start_after -= ONE_DAY;
                }
            }
            else if (pdef->name()->value() == "min_rank") {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(min_rank, pdef, SourceFilename);
                }
                else {
                    char rank_name[64];
                    GetDefText(rank_name, pdef, SourceFilename);
                    min_rank = PlayerCharacter::RankFromName(rank_name);
                }
            }
            else if (pdef->name()->value() == "max_rank") {
                if (pdef->term()->isNumber()) {
                    GetDefNumber(max_rank, pdef, SourceFilename);
                }
                else {
                    char rank_name[64];
                    GetDefText(rank_name, pdef, SourceFilename);
                    max_rank = PlayerCharacter::RankFromName(rank_name);
                }
            }
            else if (pdef->name()->value() == "delay") {
                GetDefNumber(delay, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "probability") {
                GetDefNumber(probability, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "asset_type") {
                char type_name[64];
                GetDefText(type_name, pdef, SourceFilename);
                asset_type = (int) CombatGroup::TypeFromName(type_name);
            }
            else if (pdef->name()->value() == "target_type") {
                char type_name[64];
                GetDefText(type_name, pdef, SourceFilename);
                target_type = (int) CombatGroup::TypeFromName(type_name);
            }
            else if (pdef->name()->value() == "location" || pdef->name()->value() == "loc") {
                GetDefVec(loc, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "system" || pdef->name()->value() == "sys") {
                GetDefText(system, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "region" || pdef->name()->value() == "rgn" || pdef->name()->value() == "zone") {
                GetDefText(region, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "file") {
                GetDefText(file, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "image") {
                GetDefText(image_file, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "scene") {
                GetDefText(scene_file, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "text") {
                GetDefText(text, pdef, SourceFilename);
                text = Game::GetText(text);
            }
            else if (pdef->name()->value() == "asset_id") {
                GetDefNumber(asset_id, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "target_id") {
                GetDefNumber(target_id, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "target_iff") {
                GetDefNumber(target_iff, pdef, SourceFilename);
            }
            else if (pdef->name()->value() == "asset_kill") {
                if (!action)
                    action = new CombatAction(id, type, subtype, team);

                if (action) {
                    char txt[64];
                    GetDefText(txt, pdef, SourceFilename);
                    action->AssetKills().append(new Text(txt));
                }
            }
            else if (pdef->name()->value() == "target_kill") {
                if (!action)
                    action = new CombatAction(id, type, subtype, team);

                if (action) {
                    char txt[64];
                    GetDefText(txt, pdef, SourceFilename);
                    action->TargetKills().append(new Text(txt));
                }
            }
            else if (pdef->name()->value() == "req") {
                if (!action)
                    action = new CombatAction(id, type, subtype, team);

                if (!pdef->term() || !pdef->term()->isStruct()) {
                    UE_LOG(LogCampaign, Warning, TEXT("WARNING: action req struct missing in '%s'"),
                        SourceFilename ? ANSI_TO_TCHAR(SourceFilename) : TEXT("(null)"));
                }
                else if (action) {
                    TermStruct* val2 = pdef->term()->isStruct();

                    int  act = 0;
                    int  stat = ECombatActionStatus::COMPLETE;
                    bool not_flag = false;

                    Combatant* c1 = 0;
                    Combatant* c2 = 0;
                    int        comp = 0;
                    int        score = 0;
                    int        intel = 0;
                    ECOMBATGROUP_TYPE gtype = ECOMBATGROUP_TYPE::NONE;
                    int        gid = 0;

                    for (int j = 0; j < val2->elements()->size(); j++) {
                        TermDef* pdef2 = val2->elements()->at(j)->isDef();
                        if (pdef2) {
                            if (pdef2->name()->value() == "action") {
                                GetDefNumber(act, pdef2, SourceFilename);
                            }
                            else if (pdef2->name()->value() == "status") {
                                char txt[64];
                                GetDefText(txt, pdef2, SourceFilename);
                                stat = CombatAction::StatusFromName(txt);
                            }
                            else if (pdef2->name()->value() == "not") {
                                GetDefBool(not_flag, pdef2, SourceFilename);
                            }
                            else if (pdef2->name()->value() == "c1") {
                                char txt[64];
                                GetDefText(txt, pdef2, SourceFilename);
                                c1 = GetCombatant(txt);
                            }
                            else if (pdef2->name()->value() == "c2") {
                                char txt[64];
                                GetDefText(txt, pdef2, SourceFilename);
                                c2 = GetCombatant(txt);
                            }
                            else if (pdef2->name()->value() == "comp") {
                                char txt[64];
                                GetDefText(txt, pdef2, SourceFilename);
                                comp = CombatActionReq::CompFromName(txt);
                            }
                            else if (pdef2->name()->value() == "score") {
                                GetDefNumber(score, pdef2, SourceFilename);
                            }
                            else if (pdef2->name()->value() == "intel") {
                                if (pdef2->term()->isNumber()) {
                                    GetDefNumber(intel, pdef2, SourceFilename);
                                }
                                else if (pdef2->term()->isText()) {
                                    char txt[64];
                                    GetDefText(txt, pdef2, SourceFilename);
                                    intel = Intel::IntelFromName(txt);
                                }
                            }
                            else if (pdef2->name()->value() == "group_type") {
                                char type_name[64];
                                GetDefText(type_name, pdef2, SourceFilename);
                                gtype = CombatGroup::TypeFromName(type_name);
                            }
                            else if (pdef2->name()->value() == "group_id") {
                                GetDefNumber(gid, pdef2, SourceFilename);
                            }
                        }
                    }

                    if (act)
                        action->AddRequirement(act, stat, not_flag);
                    else if ((int) gtype)
                        action->AddRequirement(c1, gtype, gid, comp, score, intel);
                    else
                        action->AddRequirement(c1, c2, comp, score);
                }
            }
        }
    }

    if (!action)
        action = new CombatAction(id, type, subtype, team);

    if (action) {
        action->SetSource(source);
        action->SetOpposingType(opp_type);
        action->SetLocation(loc);
        action->SetSystem(system);
        action->SetRegion(region);
        action->SetFilename(file);
        action->SetImageFile(image_file);
        action->SetSceneFile(scene_file);

        action->SetCount(count);
        action->SetStartAfter(start_after);
        action->SetStartBefore(start_before);
        action->SetMinRank(min_rank);
        action->SetMaxRank(max_rank);
        action->SetDelay(delay);
        action->SetProbability(probability);

        action->SetAssetId(asset_id);
        action->SetAssetType(asset_type);
        action->SetTargetId(target_id);
        action->SetTargetIFF(target_iff);
        action->SetTargetType(target_type);
        action->SetText(text);

        actions.append(action);
    }
}

// +--------------------------------------------------------------------+

CombatGroup*
Campaign::CloneOver(CombatGroup* force, CombatGroup* clone, CombatGroup* group)
{
    CombatGroup* orig_parent = group ? group->GetParent() : 0;

    if (orig_parent) {
        CombatGroup* clone_parent = clone ? clone->FindGroup(orig_parent->GetType(), orig_parent->GetID()) : 0;

        if (!clone_parent)
            clone_parent = CloneOver(force, clone, orig_parent);

        CombatGroup* new_clone = clone ? clone->FindGroup(group->GetType(), group->GetID()) : 0;

        if (!new_clone) {
            new_clone = group->Clone(false);
            if (clone_parent)
                clone_parent->AddComponent(new_clone);
        }

        return new_clone;
    }

    return clone;
}

// +--------------------------------------------------------------------+

void
Campaign::LoadMissionList(DataLoader* loader)
{
    bool        ok = true;
    BYTE* block = 0;
    const char* SourceFilename = "Missions.def";

    loader->UseFileSystem(true);
    loader->LoadBuffer(SourceFilename, block, true);
    loader->UseFileSystem(Starshatter::UseFileSystem());

    Parser parser(new BlockReader((const char*)block));
    Term* term = parser.ParseTerm();

    if (!term) {
        return;
    }
    else {
        TermText* file_type = term->isText();
        if (!file_type || file_type->value() != "MISSIONLIST") {
            UE_LOG(LogCampaign, Warning, TEXT("WARNING: invalid mission list file '%s'"),
                ANSI_TO_TCHAR(SourceFilename));
            return;
        }
    }

    do {
        delete term; term = 0;
        term = parser.ParseTerm();

        if (term) {
            TermDef* def = term->isDef();
            if (def && def->name()->value() == "mission") {
                if (!def->term() || !def->term()->isStruct()) {
                    UE_LOG(LogCampaign, Warning, TEXT("WARNING: mission struct missing in '%s'"),
                        ANSI_TO_TCHAR(SourceFilename));
                }
                else {
                    TermStruct* val = def->term()->isStruct();

                    int   id = 0;
                    Text  mname;
                    Text  desc;
                    char  script[256];
                    char  system[256];
                    char  region[256];
                    int   start = 0;
                    int   type = 0;

                    FMemory::Memzero(script, sizeof(script));
                    FCStringAnsi::Strcpy(system, "Unknown");
                    FCStringAnsi::Strcpy(region, "Unknown");

                    for (int i = 0; i < val->elements()->size(); i++) {
                        TermDef* pdef = val->elements()->at(i)->isDef();
                        if (!pdef) continue;

                        if (pdef->name()->value() == "id") {
                            GetDefNumber(id, pdef, SourceFilename);
                        }
                        else if (pdef->name()->value() == "name") {
                            GetDefText(mname, pdef, SourceFilename);
                            mname = Game::GetText(mname);
                        }
                        else if (pdef->name()->value() == "desc") {
                            GetDefText(desc, pdef, SourceFilename);
                            if (desc.length() > 0 && desc.length() < 32)
                                desc = Game::GetText(desc);
                        }
                        else if (pdef->name()->value() == "start") {
                            GetDefTime(start, pdef, SourceFilename);
                        }
                        else if (pdef->name()->value() == "system") {
                            GetDefText(system, pdef, SourceFilename);
                        }
                        else if (pdef->name()->value() == "region") {
                            GetDefText(region, pdef, SourceFilename);
                        }
                        else if (pdef->name()->value() == "script") {
                            GetDefText(script, pdef, SourceFilename);
                        }
                        else if (pdef->name()->value() == "type") {
                            char typestr[64];
                            GetDefText(typestr, pdef, SourceFilename);
                            type = Mission::TypeFromName(typestr);
                        }
                    }

                    MissionInfo* info = new MissionInfo;
                    if (info) {
                        info->id = id;
                        info->name = mname;
                        info->description = desc;
                        info->system = system;
                        info->region = region;
                        info->script = script;
                        info->start = start;
                        info->type = type;
                        info->mission = 0;

                        info->script.setSensitive(false);

                        missions.append(info);
                    }
                    else {
                        ok = false;
                    }
                }
            }
        }
    } while (term);

    loader->ReleaseBuffer(block);

    if (!ok)
        Unload();
}

void
Campaign::LoadCustomMissions(DataLoader* loader)
{
    bool       ok = true;
    List<Text> files;

    loader->UseFileSystem(true);
    loader->ListFiles("*.*", files);

    for (int i = 0; i < files.size(); i++) {
        Text file = *files[i];
        file.setSensitive(false);

        if (file.contains(".def")) {
            BYTE* block = 0;
            const char* SourceFilename = file.data();

            loader->UseFileSystem(true);
            loader->LoadBuffer(SourceFilename, block, true);
            loader->UseFileSystem(Starshatter::UseFileSystem());

            if (strstr((const char*)block, "MISSION") == (const char*)block) {
                Text  mname;
                Text  desc;
                Text  system = "Unknown";
                Text  region = "Unknown";
                int   start = 0;
                int   type = 0;
                int   msn_id = 0;

                Parser parser(new BlockReader((const char*)block));
                Term* term = parser.ParseTerm();

                if (!term) {
                    UE_LOG(LogCampaign, Warning, TEXT("ERROR: could not parse '%s'"), ANSI_TO_TCHAR(SourceFilename));
                    loader->ReleaseBuffer(block);
                    continue;
                }
                else {
                    TermText* file_type = term->isText();
                    if (!file_type || file_type->value() != "MISSION") {
                        UE_LOG(LogCampaign, Warning, TEXT("ERROR: invalid mission file '%s'"), ANSI_TO_TCHAR(SourceFilename));
                        delete term;
                        loader->ReleaseBuffer(block);
                        continue;
                    }
                }

                do {
                    delete term; term = 0;
                    term = parser.ParseTerm();

                    if (term) {
                        TermDef* def = term->isDef();
                        if (!def) continue;

                        if (def->name()->value() == "name") {
                            GetDefText(mname, def, SourceFilename);
                            mname = Game::GetText(mname);
                        }
                        else if (def->name()->value() == "type") {
                            char typestr[64];
                            GetDefText(typestr, def, SourceFilename);
                            type = Mission::TypeFromName(typestr);
                        }
                        else if (def->name()->value() == "id") {
                            GetDefNumber(msn_id, def, SourceFilename);
                        }
                        else if (def->name()->value() == "desc") {
                            GetDefText(desc, def, SourceFilename);
                            if (desc.length() > 0 && desc.length() < 32)
                                desc = Game::GetText(desc);
                        }
                        else if (def->name()->value() == "system") {
                            GetDefText(system, def, SourceFilename);
                        }
                        else if (def->name()->value() == "region") {
                            GetDefText(region, def, SourceFilename);
                        }
                        else if (def->name()->value() == "start") {
                            GetDefTime(start, def, SourceFilename);
                        }
                    }
                } while (term);

                loader->ReleaseBuffer(block);

                // Legacy ID inference:
                if (strstr(SourceFilename, "custom") == SourceFilename) {
                    sscanf_s(SourceFilename + 6, "%d", &msn_id);
                    if (msn_id <= i) msn_id = i + 1;
                }
                else if (msn_id < 1) {
                    msn_id = i + 1;
                }

                MissionInfo* info = new MissionInfo;
                if (info) {
                    info->id = msn_id;
                    info->name = mname;
                    info->type = type;
                    info->description = desc;
                    info->system = system;
                    info->region = region;
                    info->script = SourceFilename;
                    info->start = start;
                    info->mission = 0;

                    info->script.setSensitive(false);

                    missions.append(info);
                }
                else {
                    ok = false;
                }
            }

            loader->ReleaseBuffer(block);
        }
    }

    files.destroy();

    if (!ok)
        Unload();
    else
        missions.sort();
}

void
Campaign::LoadTemplateList(DataLoader* loader)
{
    BYTE* block = 0;
    const char* SourceFilename = "Templates.def";

    loader->UseFileSystem(true);
    loader->LoadBuffer(SourceFilename, block, true);
    loader->UseFileSystem(Starshatter::UseFileSystem());

    Parser parser(new BlockReader((const char*)block));
    Term* term = parser.ParseTerm();

    if (!term) {
        return;
    }
    else {
        TermText* file_type = term->isText();
        if (!file_type || file_type->value() != "TEMPLATELIST") {
            UE_LOG(LogCampaign, Warning, TEXT("WARNING: invalid template list file '%s'"),
                ANSI_TO_TCHAR(SourceFilename));
            return;
        }
    }

    do {
        delete term; term = 0;
        term = parser.ParseTerm();

        if (term) {
            TermDef* def = term->isDef();
            if (def && def->name()->value() == "mission") {
                if (!def->term() || !def->term()->isStruct()) {
                    UE_LOG(LogCampaign, Warning, TEXT("WARNING: mission struct missing in '%s'"),
                        ANSI_TO_TCHAR(SourceFilename));
                }
                else {
                    TermStruct* val = def->term()->isStruct();

                    char name_buf[256];
                    char script[256];
                    char region_buf[256];

                    int  id = 0;
                    int  msn_type = 0;
                    int  grp_type = 0;

                    int  min_rank = 0;
                    int  max_rank = 0;
                    int  action_id = 0;
                    int  action_status = 0;
                    int  exec_once = 0;
                    int  start_before = TIME_NEVER;
                    int  start_after = 0;

                    name_buf[0] = 0;
                    script[0] = 0;
                    region_buf[0] = 0;

                    for (int i = 0; i < val->elements()->size(); i++) {
                        TermDef* pdef = val->elements()->at(i)->isDef();
                        if (!pdef) continue;

                        if (pdef->name()->value() == "id")
                            GetDefNumber(id, pdef, SourceFilename);

                        else if (pdef->name()->value() == "name")
                            GetDefText(name_buf, pdef, SourceFilename);

                        else if (pdef->name()->value() == "script")
                            GetDefText(script, pdef, SourceFilename);

                        else if (pdef->name()->value() == "rgn" || pdef->name()->value() == "region")
                            GetDefText(region_buf, pdef, SourceFilename);

                        else if (pdef->name()->value() == "type") {
                            char typestr[64];
                            GetDefText(typestr, pdef, SourceFilename);
                            msn_type = Mission::TypeFromName(typestr);
                        }

                        else if (pdef->name()->value() == "group") {
                            char typestr[64];
                            GetDefText(typestr, pdef, SourceFilename);
                            grp_type = (int) CombatGroup::TypeFromName(typestr);
                        }

                        else if (pdef->name()->value() == "min_rank")
                            GetDefNumber(min_rank, pdef, SourceFilename);

                        else if (pdef->name()->value() == "max_rank")
                            GetDefNumber(max_rank, pdef, SourceFilename);

                        else if (pdef->name()->value() == "action_id")
                            GetDefNumber(action_id, pdef, SourceFilename);

                        else if (pdef->name()->value() == "action_status")
                            GetDefNumber(action_status, pdef, SourceFilename);

                        else if (pdef->name()->value() == "exec_once")
                            GetDefNumber(exec_once, pdef, SourceFilename);

                        else if (pdef->name()->value().contains("before")) {
                            if (pdef->term()->isNumber()) {
                                GetDefNumber(start_before, pdef, SourceFilename);
                            }
                            else {
                                GetDefTime(start_before, pdef, SourceFilename);
                                start_before -= ONE_DAY;
                            }
                        }

                        else if (pdef->name()->value().contains("after")) {
                            if (pdef->term()->isNumber()) {
                                GetDefNumber(start_after, pdef, SourceFilename);
                            }
                            else {
                                GetDefTime(start_after, pdef, SourceFilename);
                                start_after -= ONE_DAY;
                            }
                        }
                    }

                    MissionInfo* info = new MissionInfo;
                    if (info) {
                        info->id = id;
                        info->name = name_buf;
                        info->script = script;
                        info->region = region_buf;
                        info->type = msn_type;
                        info->min_rank = min_rank;
                        info->max_rank = max_rank;
                        info->action_id = action_id;
                        info->action_status = action_status;
                        info->exec_once = exec_once;
                        info->start_before = start_before;
                        info->start_after = start_after;

                        info->script.setSensitive(false);

                        TemplateList* templist = GetTemplateList(msn_type, grp_type);

                        if (!templist) {
                            templist = new TemplateList;
                            templist->mission_type = msn_type;
                            templist->group_type = grp_type;
                            templates.append(templist);
                        }

                        templist->missions.append(info);
                    }
                }
            }
        }
    } while (term);

    loader->ReleaseBuffer(block);
}

// +--------------------------------------------------------------------+

void
Campaign::CreatePlanners()
{
    UE_LOG(LogCampaign, Log, TEXT("[Campaign] CreatePlanners"));
    if (planners.size() > 0)
        planners.destroy();

    CampaignPlan* p = 0;

    // PLAN EVENT MUST BE FIRST PLANNER:
    p = new CampaignPlanEvent(this);
    if (p) planners.append(p);

    p = new CampaignPlanStrategic(this);
    if (p) planners.append(p);

    p = new CampaignPlanAssignment(this);
    if (p) planners.append(p);

    p = new CampaignPlanMovement(this);
    if (p) planners.append(p);

    p = new CampaignPlanMission(this);
    if (p) planners.append(p);

    if (lockout > 0 && planners.size()) {
        ListIter<CampaignPlan> plan = planners;
        while (++plan)
            plan->SetLockout(lockout);
    }
}

// +--------------------------------------------------------------------+

int
Campaign::GetPlayerIFF()
{
    int iff = 1;

    if (player_group)
        iff = player_group->GetIFF();

    return iff;
}

void
Campaign::SetPlayerGroup(CombatGroup* pg)
{
    if (player_group != pg) {
        UE_LOG(LogCampaign, Log, TEXT("[Campaign] SetPlayerGroup(%s)"),
            pg ? ANSI_TO_TCHAR(pg->GetName().data()) : TEXT("0"));

        player_group = pg;
        player_unit = 0;

        // need to regenerate missions when changing player combat group:
        if (IsDynamic()) {
            UE_LOG(LogCampaign, Log, TEXT("  destroying mission list..."));
            missions.destroy();
        }
    }
}

void
Campaign::SetPlayerUnit(CombatUnit* unit)
{
    if (player_unit != unit) {
        UE_LOG(LogCampaign, Log, TEXT("Campaign::SetPlayerUnit(%s)"),
            unit ? ANSI_TO_TCHAR(unit->GetName().data()) : TEXT("0"));

        player_unit = unit;

        if (unit)
            player_group = unit->GetCombatGroup();

        // need to regenerate missions when changing player combat unit:
        if (IsDynamic()) {
            UE_LOG(LogCampaign, Log, TEXT("  destroying mission list..."));
            missions.destroy();
        }
    }
}

// +--------------------------------------------------------------------+

CombatZone*
Campaign::GetZone(const char* rgn)
{
    UE_LOG(LogCampaign, Warning,
        TEXT("[Campaign] GetZone lookup for '%s' (zones=%d)"),
        ANSI_TO_TCHAR(rgn),
        zones.size());

    ListIter<CombatZone> z = zones;
    while (++z) {
        CombatZone* Zone = z.value();
        if (!Zone)
            continue;

        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] Testing zone system=%s"),
            ANSI_TO_TCHAR(Zone->GetSystem()));

        if (z->HasRegion(rgn))
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] GetZone matched '%s'"),
                ANSI_TO_TCHAR(rgn));
            return z.value();
        }
    }

    UE_LOG(LogCampaign, Warning,
        TEXT("[Campaign] GetZone FAILED for '%s'"),
        ANSI_TO_TCHAR(rgn));

    return 0;
}

StarSystem*
Campaign::GetSystem(const char* sys)
{
    return Galaxy::GetInstance()->GetSystem(sys);
}

Combatant*
Campaign::GetCombatant(const char* cname)
{
    ListIter<Combatant> iter = combatants;
    while (++iter) {
        Combatant* c = iter.value();
        if (FCStringAnsi::Strcmp(c->GetName(), cname) == 0)
            return c;
    }

    return 0;
}

// +--------------------------------------------------------------------+

Mission*
Campaign::GetMission()
{
    return GetMission(mission_id);
}

Mission* Campaign::GetMission(int32 Id)
{
    if (Id < 0)
    {
        UE_LOG(LogCampaign, Error, TEXT("Campaign::GetMission(%d) invalid mission id"), Id);
        return nullptr;
    }

    if (mission && mission->GetIdentity() == Id)
    {
        return mission;
    }

    MissionInfo* Info = nullptr;

    for (int32 i = 0; i < missions.size(); i++)
    {
        if (missions[i] && missions[i]->id == Id)
        {
            Info = missions[i];
            break;
        }
    }

    if (!Info)
    {
        UE_LOG(LogCampaign, Warning, TEXT("Campaign::GetMission(%d) could not find mission info"), Id);
        return nullptr;
    }

    if (!Info->mission)
    {
        UE_LOG(LogCampaign, Log, TEXT("Campaign::GetMission(%d) creating mission from campaign data..."), Id);

        Info->mission = new Mission();
        if (!Info->mission)
        {
            UE_LOG(LogCampaign, Error, TEXT("Campaign::GetMission(%d) failed to allocate Mission"), Id);
            return nullptr;
        }

        // Fill from MissionInfo / DT_Campaign-backed data:
        Info->mission->SetIdentity(Info->id);
        Info->mission->SetName(Info->name);
        Info->mission->SetDescription(Info->description);
        Info->mission->SetType(Info->type);
        Info->mission->SetSystem(Info->system);
        Info->mission->SetRegion(Info->region);
        Info->mission->SetStart(Info->start);
        Info->mission->SetEnd(Info->end);
        Info->mission->SetScriptName(Info->script);
        Info->mission->SetDisplayTime(Info->DisplayType);

        // If you still need post-build initialization:
        Info->mission->InitializeFromInfo(*Info);
    }

    if (IsDynamic())
    {
        if (Info->mission)
        {
            if (FCStringAnsi::Stricmp(Info->mission->GetSituation(), "Unknown") == 0)
            {
                UE_LOG(LogCampaign, Log, TEXT("Campaign::GetMission(%d) generating sitrep..."), Id);
                CampaignSituationReport Sitrep(this, Info->mission);
                Sitrep.GenerateSituationReport();
            }
        }
        else
        {
            UE_LOG(LogCampaign, Warning, TEXT("Campaign::GetMission(%d) could not create mission"), Id);
        }
    }

    return Info->mission;
}

Mission* Campaign::GetMissionByFile(const char* InFilename)
{
    UE_LOG(LogCampaign, Warning,
        TEXT("Campaign::GetMissionByFile is legacy/unused and should not be called"));
    return nullptr;
}

MissionInfo*
Campaign::CreateNewMission()
{
    int          maxid = 0;
    MissionInfo* info = 0;

    if (campaign_id == MULTIPLAYER_MISSIONS)
        maxid = 10;

    for (int i = 0; i < missions.size(); i++) {
        MissionInfo* m = missions[i];
        if (m->id > maxid)
            maxid = m->id;
    }

    char NewScript[64];
    FCStringAnsi::Snprintf(NewScript, sizeof(NewScript), "custom%03d.def", maxid + 1);

    info = new MissionInfo;
    if (info) {
        info->id = maxid + 1;
        info->name = "New Custom Mission";
        info->script = NewScript;
        info->mission = new Mission(info->id, NewScript, path);
        info->mission->SetName(info->name);

        info->script.setSensitive(false);

        missions.append(info);
    }

    return info;
}

void
Campaign::DeleteMission(int id)
{
    if (id < 0) {
        UE_LOG(LogCampaign, Error, TEXT("ERROR - Campaign::DeleteMission(%d) invalid mission id"), id);
        return;
    }

    MissionInfo* m = 0;
    int          index = -1;

    for (int i = 0; !m && i < missions.size(); i++) {
        if (missions[i]->id == id) {
            m = missions[i];
            index = i;
        }
    }

    if (m) {
        char full_path[256];

        if (path[FCStringAnsi::Strlen(path) - 1] == '/')
            FCStringAnsi::Snprintf(full_path, sizeof(full_path), "%s%s", path, m->script.data());
        else
            FCStringAnsi::Snprintf(full_path, sizeof(full_path), "%s/%s", path, m->script.data());

        // Unreal-friendly delete:
        IFileManager::Get().Delete(ANSI_TO_TCHAR(full_path), /*RequireExists*/false, /*EvenReadOnly*/true, /*Quiet*/false);

        Load();
    }
    else {
        UE_LOG(LogCampaign, Error, TEXT("ERROR - Campaign::DeleteMission(%d) could not find mission"), id);
    }
}

MissionInfo* Campaign::GetMissionInfo(int id)
{
    if (id < 0)
    {
        UE_LOG(LogCampaign, Error,
            TEXT("ERROR - Campaign::GetMissionInfo(%d) invalid mission id"), id);
        return 0;
    }

    MissionInfo* m = 0;

    for (int i = 0; !m && i < missions.size(); i++)
    {
        if (missions[i]->id == id)
        {
            m = missions[i];
        }
    }

    if (m)
    {
        if (!m->mission)
        {
            m->mission = new Mission(id);

            if (m->mission)
            {
                const FS_CampaignMission* MissionData = FindCampaignMissionById(id);

                if (MissionData)
                {
                    if (!m->mission->LoadFromCampaignMissionData(*MissionData))
                    {
                        delete m->mission;
                        m->mission = 0;

                        UE_LOG(LogCampaign, Error,
                            TEXT("ERROR - Campaign::GetMissionInfo(%d) failed to load from campaign data"),
                            id);
                    }
                }
                else
                {
                    delete m->mission;
                    m->mission = 0;

                    UE_LOG(LogCampaign, Error,
                        TEXT("ERROR - Campaign::GetMissionInfo(%d) no matching FS_CampaignMission row"),
                        id);
                }
            }
        }

        return m;
    }

    UE_LOG(LogCampaign, Error,
        TEXT("ERROR - Campaign::GetMissionInfo(%d) could not find mission"), id);

    return 0;
}

const FS_CampaignMission* Campaign::FindCampaignMissionById(int32 id) const
{
    if (!CampaignData)
        return nullptr;

    for (const FS_CampaignMission& Row : CampaignData->Missions)
    {
        if (Row.MissionId == id)
            return &Row;
    }

    return nullptr;
}
void
Campaign::ReloadMission(int id)
{
    if (mission && mission == net_mission) {
        delete net_mission;
        net_mission = 0;
    }

    mission = 0;

    if (id >= 0 && id < missions.size()) {
        MissionInfo* m = missions[id];
        delete m->mission;
        m->mission = 0;
    }
}

void
Campaign::LoadNetMission(int id, const char* net_mission_script)
{
    if (mission && mission == net_mission) {
        delete net_mission;
        net_mission = 0;
    }

    mission_id = id;
    mission = new Mission(id);

    if (mission && mission->ParseMission(net_mission_script))
        mission->Validate();

    net_mission = mission;
}

// +--------------------------------------------------------------------+

CombatAction*
Campaign::FindAction(int action_id)
{
    ListIter<CombatAction> iter = actions;
    while (++iter) {
        CombatAction* a = iter.value();

        if (a->Identity() == action_id)
            return a;
    }

    return 0;
}

// +--------------------------------------------------------------------+

MissionInfo*
Campaign::FindMissionTemplate(int mission_type, CombatGroup* in_player_group)
{
    MissionInfo* info = 0;

    if (!in_player_group)
        return info;

    TemplateList* templ = GetTemplateList(
        mission_type,
        (int)in_player_group->GetType()
    );

    if (!templ || !templ->missions.size())
        return info;

    int tries = 0;
    int msize = templ->missions.size();

    while (!info && tries < msize) {
        int index = templ->index;
        if (index >= msize)
            index = 0;

        info = templ->missions[index];
        templ->index = index + 1;
        tries++;

        if (info) {
            if (info->action_id) {
                CombatAction* a = FindAction(info->action_id);
                if (a && a->Status() != info->action_status)
                    info = 0;
            }

            if (info && !info->IsAvailable())
                info = 0;
        }
    }

    return info;
}

// +--------------------------------------------------------------------+

TemplateList*
Campaign::GetTemplateList(int msn_type, int grp_type)
{
    for (int i = 0; i < templates.size(); i++) {
        if (templates[i]->mission_type == msn_type &&
            templates[i]->group_type == grp_type)
            return templates[i];
    }

    return 0;
}

// +--------------------------------------------------------------------+

void
Campaign::SetMissionId(int id)
{
    UE_LOG(LogCampaign, Log, TEXT("Campaign::SetMissionId(%d)"), id);

    if (id > 0)
        mission_id = id;
    else
        UE_LOG(LogCampaign, Log, TEXT("   retaining mission id = %d"), mission_id);
}

// +--------------------------------------------------------------------+

double
Campaign::GetStardate()
{
    return StarSystem::GetStardate();
}

// +--------------------------------------------------------------------+

void
Campaign::SelectDefaultPlayerGroup(CombatGroup* g, int type)
{
    if (player_group || !g) return;

    if ((int) g->GetType() == type && !g->IsReserve() && g->GetValue() > 0) {
        player_group = g;
        player_unit = 0;
        return;
    }

    for (int i = 0; i < g->GetComponents().size(); i++)
        SelectDefaultPlayerGroup(g->GetComponents()[i], type);
}

// +--------------------------------------------------------------------+

void Campaign::Prep()
{
    UE_LOG(LogCampaign, Log, TEXT("[Campaign] Prep"));

    UE_LOG(LogCampaign, Log,
        TEXT("[Campaign] Prep: dynamic=%d scripted=%d combatants=%d actions=%d missions=%d templates=%d"),
        IsDynamic() ? 1 : 0,
        IsScripted() ? 1 : 0,
        combatants.size(),
        actions.size(),
        missions.size(),
        templates.size());

    CheckPlayerGroup();

    if (player_group)
    {
        UE_LOG(LogCampaign, Log,
            TEXT("[Campaign] Prep: PlayerGroup type=%d id=%d"),
            (int32)player_group->GetType(),
            player_group->GetID());
    }
    else
    {
        UE_LOG(LogCampaign, Warning, TEXT("[Campaign] Prep: PlayerGroup is NULL"));
    }
}

void
Campaign::Start()
{
    UE_LOG(LogCampaign, Log, TEXT("[Campaign] Start"));

    Prep();

    CreatePlanners();
    SetCampaignStatus(ECampaignStatus::ACTIVE);
}

void
Campaign::ExecFrame()
{
    if (InCutscene())
        return;

    time = GetStardate() - GetStartTime();
    //UE_LOG(LogCampaign, Warning,
    //   TEXT("[Campaign] StartTime=%f Stardate=%f time=%f"),
    //   GetStartTime(),
    //   GetStardate(),
    //   time);

    if (campaign_status < ECampaignStatus::ACTIVE)
        return;

    if (IsDynamic()) {
        bool completed = false;

        ListIter<MissionInfo> m = missions;
        while (++m) {
            if (m->mission && m->mission->IsComplete()) {
                UE_LOG(LogCampaign, Log, TEXT("[Campaign] ExecFrame() completed mission %d '%s'"),
                    m->id, ANSI_TO_TCHAR(m->name.data()));
                completed = true;
            }
        }

        if (completed) {
            UE_LOG(LogCampaign, Log, TEXT("[Campaign] ExecFrame() destroying mission list after completion..."));
            missions.destroy();

            if (!player_group || player_group->IsFighterGroup())
                time += 10 * 3600;
            else
                time += 20 * 3600;

            const double DesiredStardate = GetStartTime() + time;
            const double NewBaseTime = DesiredStardate - StarSystem::GetSimulationTime() - 0.5e9;

            StarSystem::SetBaseTime(NewBaseTime, true);
            StarSystem::CalcStardate();
        }
        else {
            m.reset();

            while (++m) {
                if (m->start < time && (!m->mission || !m->mission->IsActive())) {
                    MissionInfo* info = m.removeItem();

                    if (info) {
                        UE_LOG(LogCampaign, Log, TEXT("[Campaign] ExecFrame() deleting expired mission %d start: %d current: %d"),
                            info->id, info->start, (int)time);
                        delete info;
                    }
                }
            }
        }

        if (loaded_from_savegame && planners.size() > 0) {
            CampaignPlanEvent* plan_event = (CampaignPlanEvent*)planners.first();
            plan_event->ExecScriptedEvents();
            loaded_from_savegame = false;
        }

        ListIter<CampaignPlan> plan = planners;
        while (++plan) {
            CheckPlayerGroup();
            plan->ExecFrame();
        }

        CheckPlayerGroup();

        if (completed) {
            CampaignSaveGame save(this);
            save.SaveAuto();
        }
    }
    else {
        if (planners.size() > 0) {
            CampaignPlanEvent* plan_event = (CampaignPlanEvent*)planners.first();
            plan_event->ExecScriptedEvents();
        }
    }
}

// +--------------------------------------------------------------------+

void
Campaign::LockoutEvents(int seconds)
{
    lockout = seconds;
}

void Campaign::CheckPlayerGroup()
{
    if (!player_group || player_group->IsReserve() || player_group->CalcValue() < 1)
    {
        const int PlayerIFF = GetPlayerIFF();

        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] CheckPlayerGroup: PlayerIFF=%d combatants=%d"),
            PlayerIFF, combatants.size());

        player_group = 0;

        CombatGroup* Force = 0;

        for (int i = 0; i < combatants.size() && !Force; i++)
        {
            Combatant* C = combatants[i];
            if (!C)
                continue;

            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] Combatant[%d]: iff=%d"),
                i,
                C->GetIFF());

            if (C->GetIFF() == PlayerIFF)
            {
                Force = C->GetForce();

                DumpCombatGroupTree(Force);

                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] Matching combatant[%d], GetForce()=%p"),
                    i,
                    Force);

                if (!Force)
                {
                    UE_LOG(LogCampaign, Warning,
                        TEXT("[Campaign] Matching combatant[%d] returned null force"),
                        i);
                }
            }
        }

        if (Force)
        {
            Force->CalcValue();

            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] Selecting default player group from force=%s"),
                Force->GetName().data() ? ANSI_TO_TCHAR(Force->GetName().data()) : TEXT("NULL"));

            SelectDefaultPlayerGroup(Force, (int)ECOMBATGROUP_TYPE::WING);

            if (!player_group)
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] No WING found, trying DESTROYER_SQUADRON"));

                SelectDefaultPlayerGroup(Force, (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON);
            }

            if (!player_group)
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] WARNING: No player group selected for IFF=%d"),
                    PlayerIFF);
            }
            else
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] PlayerGroup selected: name=%s type=%d"),
                    player_group->GetName().data()
                    ? ANSI_TO_TCHAR(player_group->GetName().data())
                    : TEXT("NULL"),
                    (int)player_group->GetType());
            }
        }
        else
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] ERROR: No force found for PlayerIFF=%d"),
                PlayerIFF);
        }
    }

    if (player_unit && player_unit->GetValue() < 1)
    {
        UE_LOG(LogCampaign, Warning, TEXT("[Campaign] PlayerUnit invalid, clearing"));
        SetPlayerUnit(0);
    }
}

CombatGroup* Campaign::FindFirstPlayableGroup(CombatGroup* Group)
{
    if (!Group)
        return 0;

    if (!Group->IsReserve() && Group->CalcValue() > 0)
    {
        const int Type = (int) Group->GetType();

        if (Type != (int)ECOMBATGROUP_TYPE::FORCE)
        {
            return Group;
        }
    }

    List<CombatGroup>& Children = Group->GetComponents();
    for (int i = 0; i < Children.size(); i++)
    {
        CombatGroup* Found = FindFirstPlayableGroup(Children[i]);
        if (Found)
            return Found;
    }

    return 0;
}

// +--------------------------------------------------------------------+

void
Campaign::StartMission()
{
    Mission* m = GetMission();

    if (m) {
        UE_LOG(LogCampaign, Log, TEXT("Campaign Start Mission - %d. '%s'"),
            m->GetIdentity(), ANSI_TO_TCHAR(m->GetName()));

        if (!IsScripted()) {

            double gtime = (double)Game::GameTime() / 1000.0;
            double base = GetStartTime() + m->GetStart() - 15 - gtime;

            StarSystem::SetBaseTime(base);

            double current_time = GetStardate() -GetStartTime();

            char buffer[32];
            FormatDayTime(buffer, current_time);
            UE_LOG(LogCampaign, Log, TEXT("  current time:  %s"), ANSI_TO_TCHAR(buffer));

            FormatDayTime(buffer, m->GetStart());
            UE_LOG(LogCampaign, Log, TEXT("  mission start: %s"), ANSI_TO_TCHAR(buffer));
        }
    }
}

void
Campaign::RollbackMission()
{
    UE_LOG(LogCampaign, Log, TEXT("Campaign::RollbackMission()"));

    Mission* m = GetMission();

    if (m) {
        if (!IsScripted()) {

            double gtime = (double)Game::GameTime() / 1000.0;
            double base = GetStartTime() + m->GetStart() - 60 - gtime;

            StarSystem::SetBaseTime(base);

            double current_time = GetStardate() - GetStartTime();
            UE_LOG(LogCampaign, Log, TEXT("  mission start: %d"), m->GetStart());
            UE_LOG(LogCampaign, Log, TEXT("  current time:  %d"), (int)current_time);
        }

        m->SetActive(false);
        m->SetComplete(false);
    }
}

// +--------------------------------------------------------------------+

bool
Campaign::InCutscene() const
{
    Starshatter* stars = Starshatter::GetInstance();
    return stars ? stars->InCutscene() : false;
}

bool
Campaign::IsDynamic() const
{
    return campaign_id >= DYNAMIC_CAMPAIGN && campaign_id < SINGLE_MISSIONS;
}

bool
Campaign::IsTraining() const
{
    return campaign_id == TRAINING_CAMPAIGN;
}

bool
Campaign::IsScripted() const
{
    return scripted;
}

bool
Campaign::IsSequential() const
{
    return sequential;
}

// +--------------------------------------------------------------------+

static CombatGroup* FindGroup_r(CombatGroup* g, int type, int id)
{
    if ((int) g->GetType() == type && g->GetID() == id)
        return g;

    CombatGroup* result = 0;

    ListIter<CombatGroup> subgroup = g->GetComponents();
    while (++subgroup && !result)
        result = FindGroup_r(subgroup.value(), type, id);

    return result;
}

CombatGroup*
Campaign::FindGroup(int iff, int type, int id)
{
    CombatGroup* result = 0;

    ListIter<Combatant> combatant = combatants;
    while (++combatant && !result) {
        if (combatant->GetIFF() == iff) {
            result = FindGroup_r(combatant->GetForce(), type, id);
        }
    }

    return result;
}

// +--------------------------------------------------------------------+

static void FindGroups(CombatGroup* g, int type, CombatGroup* near_group, List<CombatGroup>& groups)
{
    if ((int) g->GetType() == type && g->GetIntelLevel() > Intel::RESERVE) {
        if (!near_group || g->GetAssignedZone() == near_group->GetAssignedZone())
            groups.append(g);
    }

    ListIter<CombatGroup> subgroup = g->GetComponents();
    while (++subgroup)
        FindGroups(subgroup.value(), type, near_group, groups);
}

CombatGroup*
Campaign::FindGroup(int iff, int type, CombatGroup* near_group)
{
    CombatGroup* result = 0;
    List<CombatGroup> groups;

    ListIter<Combatant> combatant = combatants;
    while (++combatant) {
        if (combatant->GetIFF() == iff) {
            FindGroups(combatant->GetForce(), type, near_group, groups);
        }
    }

    if (groups.size() > 0) {
        const int MaxIndex = groups.size() - 1;
        const int index = MaxIndex > 0 ? FMath::RandRange(0, MaxIndex) : 0;
        result = groups[index];
    }

    return result;
}

// +--------------------------------------------------------------------+

static void FindStrikeTargets(CombatGroup* g, CombatGroup* strike_group, List<CombatGroup>& groups)
{
    if (!strike_group || !strike_group->GetAssignedZone()) return;

    if (g->IsStrikeTarget() && g->GetIntelLevel() > Intel::RESERVE) {
        if (strike_group->GetAssignedZone() == g->GetAssignedZone() ||
            strike_group->GetAssignedZone()->HasRegion(g->GetRegion()))
            groups.append(g);
    }

    ListIter<CombatGroup> subgroup = g->GetComponents();
    while (++subgroup)
        FindStrikeTargets(subgroup.value(), strike_group, groups);
}

CombatGroup*
Campaign::FindStrikeTarget(int iff, CombatGroup* strike_group)
{
    CombatGroup* result = 0;
    List<CombatGroup> groups;

    ListIter<Combatant> combatant = GetCombatants();
    while (++combatant) {
        if (combatant->GetIFF() != 0 && combatant->GetIFF() != iff) {
            FindStrikeTargets(combatant->GetForce(), strike_group, groups);
        }
    }

    if (groups.size() > 0) {
        const int MaxIndex = groups.size() - 1;
        const int index = MaxIndex > 0 ? FMath::RandRange(0, MaxIndex) : 0;
        result = groups[index];
    }

    return result;
}

// +--------------------------------------------------------------------+

void
Campaign::CommitExpiredActions()
{
    ListIter<CombatAction> iter = actions;
    while (++iter) {
        CombatAction* a = iter.value();

        if (a->IsAvailable())
            a->SetStatus(ECombatActionStatus::COMPLETE);
    }

    updateTime = time;
}

// +--------------------------------------------------------------------+

int
Campaign::GetPlayerTeamScore()
{
    int score_us = 0;
    int score_them = 0;

    if (player_group) {
        int iff = player_group->GetIFF();

        ListIter<Combatant> iter = combatants;
        while (++iter) {
            Combatant* c = iter.value();

            if (iff <= 1) {
                if (c->GetIFF() <= 1) score_us += c->GetScore();
                else                  score_them += c->GetScore();
            }
            else {
                if (c->GetIFF() <= 1) score_them += c->GetScore();
                else                  score_us += c->GetScore();
            }
        }
    }

    return score_us - score_them;
}

// +--------------------------------------------------------------------+

void
Campaign::SetCampaignStatus(ECampaignStatus s)
{
    campaign_status = s;

    // record the win in player profile:
    if (campaign_status == ECampaignStatus::SUCCESS) {
        PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
        if (player)
            player->SetCampaignComplete(campaign_id);
    }

    if (campaign_status > ECampaignStatus::ACTIVE) {
        UE_LOG(LogCampaign, Log, TEXT("Campaign::SetStatus() destroying mission list at campaign end"));
        missions.destroy();
    }
}

// +--------------------------------------------------------------------+

static void GetCombatUnits(CombatGroup* g, List<CombatUnit>& units)
{
    if (g) {
        ListIter<CombatUnit> unit = g->GetUnits();
        while (++unit) {
            CombatUnit* u = unit.value();

            if (u->Count() - u->DeadCount() > 0)
                units.append(u);
        }

        ListIter<CombatGroup> comp = g->GetComponents();
        while (++comp) {
            CombatGroup* g2 = comp.value();

            if (!g2->IsReserve())
                GetCombatUnits(g2, units);
        }
    }
}

int
Campaign::GetAllCombatUnits(int iff, List<CombatUnit>& units)
{
    units.clear();

    ListIter<Combatant> iter = combatants;
    while (++iter) {
        Combatant* c = iter.value();

        if (iff < 0 || c->GetIFF() == iff) {
            GetCombatUnits(c->GetForce(), units);
        }
    }

    return units.size();
}

void Campaign::LoadFromData(const FS_Campaign& Data)
{
    // Reset runtime-owned state:
    Clear();
    systems.clear();
    templates.destroy();
    zones.destroy();

    // Clear() already destroys missions/planners/combatants/events/actions,
    // but keeping runtime state explicit here is fine:
    mission = nullptr;
    net_mission = nullptr;
    mission_id = -1;

    campaign_id = Data.Index + 1;
    SourceRowName = Data.RowName;

    name = TCHAR_TO_ANSI(*Data.Name);
    description = TCHAR_TO_ANSI(*Data.Description);
    situation = TCHAR_TO_ANSI(*Data.Situation);

    FString CombinedOrders;
    for (const FString& Line : Data.Orders)
    {
        if (!Line.IsEmpty())
        {
            if (!CombinedOrders.IsEmpty())
            {
                CombinedOrders += TEXT("\n");
            }

            CombinedOrders += Line;
        }
    }

    orders = TCHAR_TO_ANSI(*CombinedOrders);

    SetScripted(Data.bScripted);
    sequential = Data.bSequential;
    double RelativeStart = UFormattingUtils::ParseStarshatterTime(*Data.Start);
    startTime = StarSystem::GetStardate() + RelativeStart;
    time = startTime;
    loadTime = startTime;
    updateTime = startTime;
    lockout = 0;

    UE_LOG(LogCampaign, Log,
        TEXT("[Campaign] LoadFromData: Campaign='%s' MissionList=%d Missions=%d TemplateList=%d TemplateMissions=%d Actions=%d Combatants=%d"),
        *Data.Name,
        Data.MissionList.Num(),
        Data.Missions.Num(),
        Data.TemplateList.Num(),
        Data.TemplateMissions.Num(),
        Data.Action.Num(),
        Data.Combatant.Num());

    // ------------------------------------------------------------
    // Zones + system references
    // ------------------------------------------------------------
    Galaxy* GalaxyInstance = Galaxy::GetInstance();

    for (const FS_CampaignZone& ZoneRow : Data.Zone)
    {
        CombatZone* NewZone = new CombatZone;
        if (NewZone)
        {
            NewZone->SetSystem(TCHAR_TO_ANSI(*ZoneRow.System));
            NewZone->AddRegion(TCHAR_TO_ANSI(*ZoneRow.Region));
            zones.append(NewZone);
        }

        if (!GalaxyInstance)
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] LoadFromData: Galaxy not initialized yet"));
        }
        else if (!ZoneRow.System.IsEmpty())
        {
            StarSystem* Sys = GalaxyInstance->GetSystem(TCHAR_TO_ANSI(*ZoneRow.System));
            if (Sys)
            {
                bool bExists = false;

                for (int i = 0; i < systems.size(); i++)
                {
                    if (systems[i] == Sys)
                    {
                        bExists = true;
                        break;
                    }
                }

                if (!bExists)
                {
                    systems.append(Sys);
                }
            }
            else
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] LoadFromData: System '%s' not found in Galaxy"),
                    *ZoneRow.System);
            }
        }
    }

    // ------------------------------------------------------------
    // Combatants
    // FS_Combatant.Name is EEMPIRE_NAME, so convert to a readable name
    // and use that to look up the source force in CombatRoster.
    // ------------------------------------------------------------
    CombatRoster* Roster = CombatRoster::GetInstance();

    auto GetCombatantNameString = [](EEMPIRE_NAME Empire) -> FString
        {
            const UEnum* EnumObj = StaticEnum<EEMPIRE_NAME>();
            if (!EnumObj)
            {
                return TEXT("Unknown");
            }

            // Prefer display name so it matches authored names more closely:
            return EnumObj->GetDisplayNameTextByValue((int64)Empire).ToString();
        };

    if (!Roster)
    {
        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] LoadFromData: CombatRoster is null, skipping combatants"));
    }
    else
    {
        for (const FS_Combatant& CombatantRow : Data.Combatant)
        {
            const FString CombatantName = GetCombatantNameString(CombatantRow.Name);
            if (CombatantName.IsEmpty())
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] LoadFromData: combatant has empty name"));
                continue;
            }

            CombatGroup* SourceForce = Roster->GetForce(TCHAR_TO_ANSI(*CombatantName));
            if (!SourceForce)
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] LoadFromData: source force '%s' not found in CombatRoster"),
                    *CombatantName);
                continue;
            }

            CombatGroup* CloneForce = SourceForce->Clone(false);
            if (!CloneForce)
            {
                UE_LOG(LogCampaign, Warning,
                    TEXT("[Campaign] LoadFromData: failed to clone source force '%s'"),
                    *CombatantName);
                continue;
            }

            // Rebuild selected groups into the clone, matching legacy ParseGroup behavior.
            for (const FS_CombatantGroup& GroupRow : CombatantRow.Group)
            {
                const ECOMBATGROUP_TYPE GroupType = GroupRow.Type;
                const int32 GroupId = GroupRow.Id;

                if (GroupType == ECOMBATGROUP_TYPE::NONE || GroupId <= 0)
                {
                    continue;
                }

                CombatGroup* SourceGroup = SourceForce->FindGroup(GroupType, GroupId);
                if (!SourceGroup || !SourceGroup->GetParent())
                {
                    UE_LOG(LogCampaign, Warning,
                        TEXT("[Campaign] LoadFromData: combatant '%s' missing group type=%d id=%d"),
                        *CombatantName,
                        (int32)GroupType,
                        GroupId);
                    continue;
                }

                CombatGroup* ParentClone = CloneOver(SourceForce, CloneForce, SourceGroup->GetParent());
                if (ParentClone)
                {
                    ParentClone->AddComponent(SourceGroup->Clone());
                }
            }

            Combatant* NewCombatant = new Combatant(TCHAR_TO_ANSI(*CombatantName), CloneForce);
            if (NewCombatant)
            {
                combatants.append(NewCombatant);
            }
            else
            {
                delete CloneForce;
            }
        }
    }

    // ------------------------------------------------------------
    // Actions
    // Uses FS_CampaignAction + FS_CampaignReq
    // ------------------------------------------------------------
    for (const FS_CampaignAction& ActionRow : Data.Action)
    {
        const int ActionType = CombatAction::TypeFromName(TCHAR_TO_ANSI(*ActionRow.Type));
        const int Subtype = ActionRow.Subtype;
        const int OppType = ActionRow.OppType;

        const ECombatEventSource Source =
            CombatEvent::GetSourceFromName(TCHAR_TO_ANSI(*ActionRow.Source));

        CombatAction* NewAction = new CombatAction(
            ActionRow.Id,
            ActionType,
            Subtype,
            ActionRow.Team);

        if (!NewAction)
        {
            continue;
        }

        NewAction->SetSource(Source);
        NewAction->SetOpposingType(OppType);

        NewAction->SetSystem(TCHAR_TO_ANSI(*ActionRow.System));
        NewAction->SetRegion(TCHAR_TO_ANSI(*ActionRow.Region));

        // Title/Message map into campaign text fields:
        if (!ActionRow.Title.IsEmpty())
        {
            NewAction->SetTitle(TCHAR_TO_ANSI(*ActionRow.Title));
        }

        if (!ActionRow.Message.IsEmpty())
        {
            NewAction->SetText(TCHAR_TO_ANSI(*ActionRow.Message));
        }

        NewAction->SetImageFile(TCHAR_TO_ANSI(*ActionRow.Image));
        NewAction->SetSoundFile(TCHAR_TO_ANSI(*ActionRow.Audio));
        NewAction->SetSceneFile(TCHAR_TO_ANSI(*ActionRow.Scene));

        NewAction->SetCount(ActionRow.Count);
        NewAction->SetStartBefore(ActionRow.StartBefore > 0 ? ActionRow.StartBefore : TIME_NEVER);
        NewAction->SetStartAfter(ActionRow.StartAfter);
        NewAction->SetMinRank(ActionRow.MinRank);
        NewAction->SetMaxRank(ActionRow.MaxRank > 0 ? ActionRow.MaxRank : 100);
        NewAction->SetDelay(ActionRow.Delay);
        NewAction->SetProbability(ActionRow.Probability > 0 ? ActionRow.Probability : 100);

        NewAction->SetAssetType((int)CombatGroup::TypeFromName(TCHAR_TO_ANSI(*ActionRow.AssetType)));
        NewAction->SetAssetId(ActionRow.AssetId);

        NewAction->SetTargetType((int)CombatGroup::TypeFromName(TCHAR_TO_ANSI(*ActionRow.TargetType)));
        NewAction->SetTargetId(ActionRow.TargetId);
        NewAction->SetTargetIFF(ActionRow.TargetIff);

        NewAction->SetLocation(ActionRow.Location);

        if (!ActionRow.AssetKill.IsEmpty())
        {
            NewAction->AssetKills().append(new Text(TCHAR_TO_ANSI(*ActionRow.AssetKill)));
        }

        if (!ActionRow.TargetKill.IsEmpty())
        {
            NewAction->TargetKills().append(new Text(TCHAR_TO_ANSI(*ActionRow.TargetKill)));
        }

        // Requirements
        for (const FS_CampaignReq& ReqRow : ActionRow.Requirement)
        {
            Combatant* C1 = nullptr;
            Combatant* C2 = nullptr;

            if (!ReqRow.Combatant1.IsEmpty())
            {
                C1 = GetCombatant(TCHAR_TO_ANSI(*ReqRow.Combatant1));
            }

            if (!ReqRow.Combatant2.IsEmpty())
            {
                C2 = GetCombatant(TCHAR_TO_ANSI(*ReqRow.Combatant2));
            }

            if (ReqRow.Action > 0)
            {
                NewAction->AddRequirement(
                    ReqRow.Action,
                    ReqRow.Status,
                    ReqRow.NotAction);
            }
            else if (ReqRow.GroupType > 0)
            {
                NewAction->AddRequirement(
                    C1,
                    (ECOMBATGROUP_TYPE)ReqRow.GroupType,
                    ReqRow.GroupId,
                    ReqRow.Comp,
                    ReqRow.Score,
                    (int)ReqRow.Intel);
            }
            else
            {
                NewAction->AddRequirement(
                    C1,
                    C2,
                    ReqRow.Comp,
                    ReqRow.Score);
            }
        }

        actions.append(NewAction);
    }

    // ------------------------------------------------------------
    // Mission list
    // FS_CampaignMissionList -> MissionInfo
    // ------------------------------------------------------------
    for (const FS_CampaignMissionList& MissionRow : Data.MissionList)
    {
        MissionInfo* Info = new MissionInfo;
        if (!Info)
        {
            continue;
        }

        Info->id = MissionRow.Id;
        Info->name = TCHAR_TO_ANSI(*MissionRow.Name);
        Info->description = TCHAR_TO_ANSI(*MissionRow.Description);
        Info->player_info = TCHAR_TO_ANSI(*MissionRow.Objective);
        Info->system = TCHAR_TO_ANSI(*MissionRow.System);
        Info->region = TCHAR_TO_ANSI(*MissionRow.Region);
        Info->script = TCHAR_TO_ANSI(*MissionRow.Script);
       
       double RelativeMsnStart = UFormattingUtils::ParseStarshatterTime(*MissionRow.Start);
        Info->start = StarSystem::GetStardate() + RelativeMsnStart;

        Info->type = MissionRow.Type;
        Info->DisplayType = MissionRow.DisplayType;

        Info->min_rank = 0;
        Info->max_rank = 100;
        Info->action_id = 0;
        Info->action_status = 0;
        Info->exec_once = 0;
        Info->start_before = 0;
        Info->start_after = 0;
        Info->mission = 0;

        missions.append(Info);
    }

    // ------------------------------------------------------------
    // Template list
    // FS_CampaignTemplateList -> TemplateList/MissionInfo
    // ------------------------------------------------------------
    int32 NextTemplateMissionId = 1000;

    for (const FS_CampaignTemplateList& TemplateRow : Data.TemplateList)
    {
        MissionInfo* Info = new MissionInfo;
        if (!Info)
        {
            continue;
        }

        Info->id = (TemplateRow.Id > 0) ? TemplateRow.Id : NextTemplateMissionId++;
        Info->name = TCHAR_TO_ANSI(*TemplateRow.Name);
        Info->script = TCHAR_TO_ANSI(*TemplateRow.Script);
        Info->region = TCHAR_TO_ANSI(*TemplateRow.Region);
        Info->type = (int) TemplateRow.MissionType;
        Info->min_rank = TemplateRow.MinRank;
        Info->max_rank = TemplateRow.MaxRank;
        Info->action_id = TemplateRow.ActionId;
        Info->action_status = TemplateRow.ActionStatus;
        Info->exec_once = TemplateRow.ExecOnce;
        Info->start_before = TemplateRow.StartBefore;
        Info->start_after = TemplateRow.StartAfter;
        Info->mission = 0;
        Info->DisplayType = TemplateRow.DisplayType;

        TemplateList* Templ = GetTemplateList((int)TemplateRow.MissionType, (int)TemplateRow.GroupType);
        if (!Templ)
        {
            Templ = new TemplateList;
            if (Templ)
            {
                Templ->mission_type = (int)TemplateRow.MissionType;
                Templ->group_type = (int)TemplateRow.GroupType;
                Templ->index = 0;
                templates.append(Templ);
            }
        }

        if (Templ)
        {
            Templ->missions.append(Info);
        }
        else
        {
            delete Info;
        }
    }

    UE_LOG(LogCampaign, Log,
        TEXT("[Campaign] LoadFromData complete: Campaign='%s' zones=%d systems=%d combatants=%d actions=%d missions=%d templates=%d"),
        *Data.Name,
        zones.size(),
        systems.size(),
        combatants.size(),
        actions.size(),
        missions.size(),
        templates.size());

    // Add this line right after
    DumpAllMissionState("After LoadFromData");
}

Campaign* Campaign::CreateFromData(const FS_Campaign& Data)
{
    Campaign* NewCampaign = new Campaign(Data.Index + 1, TCHAR_TO_ANSI(*Data.Name), true);
    if (NewCampaign)
    {
        NewCampaign->SetCampaignData(&Data);
        NewCampaign->LoadFromData(Data);
    }

    return NewCampaign;
}

Campaign* Campaign::SelectFromData(const FS_Campaign& Data)
{
    if (current_campaign)
    {
        delete current_campaign;
        current_campaign = 0;
    }

    current_campaign = CreateFromData(Data);

    if (current_campaign)
    {
        UE_LOG(LogCampaign, Log,
            TEXT("Campaign::SelectFromData: Selected '%s' (Row=%s Index=%d)"),
            *Data.Name,
            *Data.RowName.ToString(),
            Data.Index + 1);
    }
    else
    {
        UE_LOG(LogCampaign, Error,
            TEXT("Campaign::SelectFromData: Failed to create runtime campaign from '%s'"),
            *Data.Name);
    }

    return current_campaign;
}

FString Campaign::GetCombatantNameString(EEMPIRE_NAME Empire) const
{
    const UEnum* EnumObj = StaticEnum<EEMPIRE_NAME>();
    if (!EnumObj)
    {
        return TEXT("Unknown");
    }

    return EnumObj->GetDisplayNameTextByValue((int64)Empire).ToString();
}

void Campaign::LoadCombatantsFromData(const FS_Campaign& Data)
{
    CombatRoster* Roster = CombatRoster::GetInstance();
    if (!Roster)
    {
        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] LoadCombatantsFromData: CombatRoster is null"));
        return;
    }

    for (const FS_Combatant& CombatantRow : Data.Combatant)
    {
        const FString CombatantName = GetCombatantNameString(CombatantRow.Name);
        if (CombatantName.IsEmpty())
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] LoadCombatantsFromData: combatant name is empty"));
            continue;
        }

        CombatGroup* SourceForce = Roster->GetForce(TCHAR_TO_ANSI(*CombatantName));
        if (!SourceForce)
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] LoadCombatantsFromData: source force '%s' not found"),
                *CombatantName);
            continue;
        }

        CombatGroup* CloneForce = SourceForce->Clone(false);
        if (!CloneForce)
        {
            UE_LOG(LogCampaign, Warning,
                TEXT("[Campaign] LoadCombatantsFromData: failed to clone '%s'"),
                *CombatantName);
            continue;
        }

        ApplyCombatantGroupsFromData(CombatantRow.Group, SourceForce, CloneForce);

        Combatant* NewCombatant = new Combatant(TCHAR_TO_ANSI(*CombatantName), CloneForce);
        if (NewCombatant)
        {
            combatants.append(NewCombatant);
        }
        else
        {
            delete CloneForce;
        }
    }

    UE_LOG(LogCampaign, Log,
        TEXT("[Campaign] LoadCombatantsFromData: loaded %d combatants"),
        combatants.size());
}

void Campaign::ApplyCombatantGroupsFromData(
    const TArray<FS_CombatantGroup>& GroupRows,
    CombatGroup* SourceForce,
    CombatGroup* CloneForce)
{
    if (!SourceForce || !CloneForce)
    {
        return;
    }

    for (const FS_CombatantGroup& GroupRow : GroupRows)
    {
        const ECOMBATGROUP_TYPE GroupType = GroupRow.Type;
        const int32 GroupId = GroupRow.Id;

        if (GroupType == ECOMBATGROUP_TYPE::NONE || GroupId <= 0)
        {
            continue;
        }

        CombatGroup* SourceGroup = SourceForce->FindGroup(GroupType, GroupId);
        if (!SourceGroup || !SourceGroup->GetParent())
        {
            continue;
        }

        CombatGroup* ParentClone = CloneOver(SourceForce, CloneForce, SourceGroup->GetParent());
        if (ParentClone)
        {
            ParentClone->AddComponent(SourceGroup->Clone());
        }
    }
}

void Campaign::LoadActionsFromData(const FS_Campaign& Data)
{
    for (const FS_CampaignAction& ActionRow : Data.Action)
    {
        const int ActionType = CombatAction::TypeFromName(TCHAR_TO_ANSI(*ActionRow.Type));
        const int Subtype = ActionRow.Subtype;
        const int OppType = ActionRow.OppType;

        const ECombatEventSource Source =
            CombatEvent::GetSourceFromName(TCHAR_TO_ANSI(*ActionRow.Source));

        CombatAction* NewAction = new CombatAction(
            ActionRow.Id,
            ActionType,
            Subtype,
            ActionRow.Team);

        if (!NewAction)
        {
            continue;
        }

        NewAction->SetSource(Source);
        NewAction->SetOpposingType(OppType);

        NewAction->SetSystem(TCHAR_TO_ANSI(*ActionRow.System));
        NewAction->SetRegion(TCHAR_TO_ANSI(*ActionRow.Region));

        if (!ActionRow.Title.IsEmpty())
        {
            NewAction->SetTitle(TCHAR_TO_ANSI(*ActionRow.Title));
        }

        if (!ActionRow.Message.IsEmpty())
        {
            NewAction->SetText(TCHAR_TO_ANSI(*ActionRow.Message));
        }

        NewAction->SetImageFile(TCHAR_TO_ANSI(*ActionRow.Image));
        NewAction->SetSoundFile(TCHAR_TO_ANSI(*ActionRow.Audio));
        NewAction->SetSceneFile(TCHAR_TO_ANSI(*ActionRow.Scene));

        NewAction->SetCount(ActionRow.Count);
        NewAction->SetStartBefore(ActionRow.StartBefore > 0 ? ActionRow.StartBefore : TIME_NEVER);
        NewAction->SetStartAfter(ActionRow.StartAfter);
        NewAction->SetMinRank(ActionRow.MinRank);
        NewAction->SetMaxRank(ActionRow.MaxRank > 0 ? ActionRow.MaxRank : 100);
        NewAction->SetDelay(ActionRow.Delay);
        NewAction->SetProbability(ActionRow.Probability > 0 ? ActionRow.Probability : 100);

        NewAction->SetAssetType((int32)CombatGroup::TypeFromName(TCHAR_TO_ANSI(*ActionRow.AssetType)));
        NewAction->SetAssetId(ActionRow.AssetId);

        NewAction->SetTargetType((int32)CombatGroup::TypeFromName(TCHAR_TO_ANSI(*ActionRow.TargetType)));
        NewAction->SetTargetId(ActionRow.TargetId);
        NewAction->SetTargetIFF(ActionRow.TargetIff);

        NewAction->SetLocation(ActionRow.Location);

        if (!ActionRow.AssetKill.IsEmpty())
        {
            NewAction->AssetKills().append(new Text(TCHAR_TO_ANSI(*ActionRow.AssetKill)));
        }

        if (!ActionRow.TargetKill.IsEmpty())
        {
            NewAction->TargetKills().append(new Text(TCHAR_TO_ANSI(*ActionRow.TargetKill)));
        }

        for (const FS_CampaignReq& ReqRow : ActionRow.Requirement)
        {
            AddActionRequirementFromData(NewAction, ReqRow);
        }

        actions.append(NewAction);
    }

    UE_LOG(LogCampaign, Log,
        TEXT("[Campaign] LoadActionsFromData: loaded %d actions"),
        actions.size());
}

void Campaign::AddActionRequirementFromData(
    CombatAction* Action,
    const FS_CampaignReq& ReqRow)
{
    if (!Action)
    {
        return;
    }

    Combatant* C1 = nullptr;
    Combatant* C2 = nullptr;

    if (!ReqRow.Combatant1.IsEmpty())
    {
        C1 = GetCombatant(TCHAR_TO_ANSI(*ReqRow.Combatant1));
    }

    if (!ReqRow.Combatant2.IsEmpty())
    {
        C2 = GetCombatant(TCHAR_TO_ANSI(*ReqRow.Combatant2));
    }

    if (ReqRow.Action > 0)
    {
        Action->AddRequirement(
            ReqRow.Action,
            ReqRow.Status,
            ReqRow.NotAction);
    }
    else if (ReqRow.GroupType > 0)
    {
        Action->AddRequirement(
            C1,
            (ECOMBATGROUP_TYPE)ReqRow.GroupType,
            ReqRow.GroupId,
            ReqRow.Comp,
            ReqRow.Score,
            (int) ReqRow.Intel);
    }
    else
    {
        Action->AddRequirement(
            C1,
            C2,
            ReqRow.Comp,
            ReqRow.Score);
    }
}

void Campaign::DumpMissionList(const FString& Label, const List<MissionInfo>& SourceList) const
{
    const FString SafeLabel = Label.IsEmpty() ? TEXT("MissionList") : Label;

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s BEGIN ====="), *SafeLabel);

    for (int i = 0; i < SourceList.size(); i++)
    {
        const MissionInfo& Info = *SourceList[i];

        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] %s[%d]: id=%d name=%s script=%s mission=%p"),
            *SafeLabel,
            i,
            Info.id,
            Info.name.data() ? ANSI_TO_TCHAR(Info.name.data()) : TEXT("NULL"),
            Info.script.data() ? ANSI_TO_TCHAR(Info.script.data()) : TEXT("NULL"),
            Info.mission);
    }

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s END ====="), *SafeLabel);
}

void Campaign::DumpTemplateBuckets(const FString& Label, const List<TemplateList>& SourceList) const
{
    const FString SafeLabel = Label.IsEmpty() ? TEXT("Templates") : Label;

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s BEGIN ====="), *SafeLabel);

    for (int i = 0; i < SourceList.size(); i++)
    {
        const TemplateList& Bucket = *SourceList[i];

        UE_LOG(LogCampaign, Warning,
            TEXT("[Campaign] %s[%d]: mission_type=%d group_type=%d index=%d missions=%d"),
            *SafeLabel,
            i,
            Bucket.mission_type,
            Bucket.group_type,
            Bucket.index,
            Bucket.missions.size());

        FString NestedLabel = FString::Printf(TEXT("%s[%d].missions"), *SafeLabel, i);

        DumpMissionList(NestedLabel, Bucket.missions);
    }

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s END ====="), *SafeLabel);
}

void Campaign::DumpAllMissionState(const FString& Label) const
{
    const FString SafeLabel = Label.IsEmpty() ? TEXT("CampaignState") : Label;

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s SUMMARY BEGIN ====="), *SafeLabel);

    UE_LOG(LogCampaign, Warning,
        TEXT("[Campaign] %s: Missions=%d Templates=%d"),
        *SafeLabel,
        missions.size(),
        templates.size());

    UE_LOG(LogCampaign, Warning, TEXT("[Campaign] ===== %s SUMMARY END ====="), *SafeLabel);

    DumpMissionList(TEXT("Missions"), missions);
    DumpTemplateBuckets(TEXT("Templates"), templates);
}

const FS_CampaignMission* Campaign::FindCampaignMissionData(int32 MissionType, CombatGroup* Squadron) const
{
    if (!CampaignData)
    {
        return nullptr;
    }

    const FString SquadronName =
        Squadron ? UTF8_TO_TCHAR(Squadron->GetName()) : FString();

    for (const FS_CampaignMission& MissionRow : CampaignData->Missions)
    {
        if (static_cast<int32>(MissionRow.MissionType) != MissionType)
        {
            continue;
        }

        if (Squadron)
        {
            for (const FS_MissionElement& Elem : MissionRow.Element)
            {
                if (Elem.Player && Elem.Squadron.Equals(SquadronName, ESearchCase::IgnoreCase))
                {
                    return &MissionRow;
                }
            }
        }

        return &MissionRow;
    }

    return nullptr;
}

const FS_CampaignMission* Campaign::FindCampaignMissionByScript(const char* ScriptName) const
{
    if (!CampaignData || !ScriptName || !*ScriptName)
    {
        return nullptr;
    }

    const FString Script = UTF8_TO_TCHAR(ScriptName);

    for (const FS_CampaignMission& Row : CampaignData->Missions)
    {
        if (Row.Scene.Equals(Script, ESearchCase::IgnoreCase))
        {
            return &Row;
        }
    }

    return nullptr;
}
