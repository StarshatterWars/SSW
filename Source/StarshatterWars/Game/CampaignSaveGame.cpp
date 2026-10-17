/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright � 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSaveGame.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignSaveGame contains the logic needed to save and load
    campaign games in progress.
*/

#include "CampaignSaveGame.h"
#include "CampaignSlotSaveGame.h"
#include "Kismet/GameplayStatics.h"

#include "Campaign.h"
#include "Combatant.h"
#include "CombatAction.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Galaxy.h"
#include "Mission.h"
#include "StarSystem.h"
#include "PlayerCharacter.h"

#include "Game.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "FormatUtil.h"

// Minimal Unreal includes (for logging / platform shim):
#include "HAL/PlatformProcess.h"   // FPlatformProcess::Sleep
#include "Misc/Paths.h"            // (optional, but commonly needed in file work)
#include "Misc/DateTime.h"         // (optional, matches existing time handling patterns)
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>              // CreateFileA, CreateDirectoryA, CloseHandle, DeleteFileA, RemoveDirectoryA
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCampaignSaveGame, Log, All);

static const char* SAVE_DIR = "SaveGame";

// +--------------------------------------------------------------------+

CampaignSaveGame::CampaignSaveGame(Campaign* c)
    : campaign(c)
{
}

// +--------------------------------------------------------------------+

CampaignSaveGame::~CampaignSaveGame()
{
}

// +--------------------------------------------------------------------+

Text
CampaignSaveGame::GetSaveDirectory()
{
    return GetSaveDirectory(PlayerCharacter::GetCurrentPlayer());
}

Text
CampaignSaveGame::GetSaveDirectory(PlayerCharacter* player)
{
    if (player) {
        char save_dir[32];
        sprintf_s(save_dir, "%s/%02d", SAVE_DIR, player->Identity());

        return save_dir;
    }

    return SAVE_DIR;
}

void
CampaignSaveGame::CreateSaveDirectory()
{
#if PLATFORM_WINDOWS
    HANDLE hDir = CreateFileA(SAVE_DIR, 0, 0, 0, OPEN_EXISTING, 0, 0);

    if (hDir == INVALID_HANDLE_VALUE)
        CreateDirectoryA(SAVE_DIR, NULL);
    else
        CloseHandle(hDir);

    Text dir = GetSaveDirectory();
    HANDLE hDir2 = CreateFileA(dir.data(), 0, 0, 0, OPEN_EXISTING, 0, 0);

    if (hDir2 == INVALID_HANDLE_VALUE)
        CreateDirectoryA(dir.data(), NULL);
    else
        CloseHandle(hDir2);
#else
    // Non-Windows: implement via your platform layer / DataLoader abstraction if needed.
#endif
}

// +--------------------------------------------------------------------+

static char multiline[4096];

static char* FormatMultiLine(const char* s)
{
    int   i = 4095;
    char* p = multiline;

    while (*s && i > 0) {
        if (*s == '\n') {
            *p++ = '\\';
            *p++ = 'n';
            s++;
            i -= 2;
        }
        else if (*s == '"') {
            *p++ = '\\';
            *p++ = '"';
            s++;
            i -= 2;
        }
        else {
            *p++ = *s++;
            i--;
        }
    }

    *p = 0;
    return multiline;
}

static char* ParseMultiLine(const char* s)
{
    int   i = 4095;
    char* p = multiline;

    while (*s && i > 0) {
        if (*s == '\\') {
            s++;
            if (*s == 'n') {
                *p++ = '\n';
                s++;
                i--;
            }
            else if (*s == '"') {
                *p++ = '"';
                s++;
                i--;
            }
            else {
                *p++ = *s++;
                i--;
            }
        }
        else {
            *p++ = *s++;
            i--;
        }
    }

    *p = 0;
    return multiline;
}

static const int32 SAVE_USER = 0;

static int32 CurrentPlayerId()
{
    if (PlayerCharacter* P = PlayerCharacter::GetCurrentPlayer())
        return P->Identity();
    return 0;
}

static FString SlotNameFor(const char* Name, int32 PlayerId)
{
    return FString::Printf(TEXT("Campaign_%02d_%s"), PlayerId, ANSI_TO_TCHAR(Name ? Name : "AutoSave"));
}

static FString IndexSlotFor(int32 PlayerId)
{
    return FString::Printf(TEXT("Campaign_%02d_Index"), PlayerId);
}

static void RememberSlot(const FString& Slot, int32 PlayerId)
{
    UCampaignSaveIndex* Index = Cast<UCampaignSaveIndex>(
        UGameplayStatics::LoadGameFromSlot(IndexSlotFor(PlayerId), SAVE_USER));
    if (!Index)
        Index = Cast<UCampaignSaveIndex>(UGameplayStatics::CreateSaveGameObject(UCampaignSaveIndex::StaticClass()));
    if (!Index) return;
    Index->SlotNames.AddUnique(Slot);
    UGameplayStatics::SaveGameToSlot(Index, IndexSlotFor(PlayerId), SAVE_USER);
}

static void ForgetSlot(const FString& Slot, int32 PlayerId)
{
    UCampaignSaveIndex* Index = Cast<UCampaignSaveIndex>(
        UGameplayStatics::LoadGameFromSlot(IndexSlotFor(PlayerId), SAVE_USER));
    if (!Index) return;
    Index->SlotNames.Remove(Slot);
    UGameplayStatics::SaveGameToSlot(Index, IndexSlotFor(PlayerId), SAVE_USER);
}

void
CampaignSaveGame::Load(const char* SourceFilename)
{
    UE_LOG(LogCampaignSaveGame, Log, TEXT("-------------------------"));
    UE_LOG(LogCampaignSaveGame, Log, TEXT("LOADING SAVEGAME (%s)."), ANSI_TO_TCHAR(SourceFilename ? SourceFilename : ""));
    campaign = 0;

    if (!SourceFilename || !SourceFilename[0])
        return;

    const FString Slot = SlotNameFor(SourceFilename, CurrentPlayerId());
    UCampaignSlotSaveGame* Saved = Cast<UCampaignSlotSaveGame>(
        UGameplayStatics::LoadGameFromSlot(Slot, SAVE_USER));
    if (!Saved || Saved->SaveText.IsEmpty())
    {
        UE_LOG(LogCampaignSaveGame, Error, TEXT("ERROR: no Unreal save slot '%s'"), *Slot);
        return;
    }

    const FTCHARToUTF8 Utf8(*Saved->SaveText);
    DataLoader* loader = DataLoader::GetLoader();
    Parser parser(new BlockReader(Utf8.Get()));
    Term* term = parser.ParseTerm();

    if (!term) {
        UE_LOG(LogCampaignSaveGame, Error, TEXT("ERROR: could not parse save game '%s'"), ANSI_TO_TCHAR(SourceFilename));
        loader->SetDataPath(0);
        return;
    }
    else {
        TermText* file_type = term->isText();
        if (!file_type || file_type->value() != "SAVEGAME") {
            UE_LOG(LogCampaignSaveGame, Error, TEXT("ERROR: invalid save game file '%s'"), ANSI_TO_TCHAR(SourceFilename));
            term->print(10);
            loader->SetDataPath(0);
            delete term;
            return;
        }
    }

    int                grp_iff = 0;
    int                grp_type = 0;
    int                grp_id = 0;
    ECampaignStatus    status = ECampaignStatus::INIT;
    double             baseTime = 0;
    double             time = 0;
    Text               unit;
    Text               sitrep;
    Text               orders;

    do {
        FPlatformProcess::Sleep(0.005f);

        delete term;
        term = 0;

        term = parser.ParseTerm();

        if (term) {
            TermDef* def = term->isDef();
            if (def) {

                if (def->name()->value() == "campaign") {
                    Text cname;
                    int  cid = 0;

                    if (def->term()) {
                        if (def->term()->isText())
                            cname = def->term()->isText()->value();
                        else if (def->term()->isNumber())
                            cid = (int)def->term()->isNumber()->value();
                    }

                    if (!campaign) {
                        List<Campaign>& list = Campaign::GetAllCampaigns();

                        for (int i = 0; i < list.size() && !campaign; i++) {
                            Campaign* c = list.at(i);

                            if (cname == c->GetName() || cid == c->GetCampaignId()) {
                                campaign = c;
                                campaign->Load();
                                campaign->Prep(); // restore campaign to pristine state

                                loader->SetDataPath(GetSaveDirectory() + "/");
                            }
                        }
                    }
                }

                else if (def->name()->value() == "grp_iff") {
                    GetDefNumber(grp_iff, def, SourceFilename);
                }

                else if (def->name()->value() == "grp_type") {
                    GetDefNumber(grp_type, def, SourceFilename);
                }

                else if (def->name()->value() == "grp_id") {
                    GetDefNumber(grp_id, def, SourceFilename);
                }

                else if (def->name()->value() == "unit") {
                    GetDefText(unit, def, SourceFilename);
                }

                else if (def->name()->value() == "status") {
                    int StatusValue = 0;
                    GetDefNumber(StatusValue, def, SourceFilename);

                    status = static_cast<ECampaignStatus>(StatusValue);
                }

                else if (def->name()->value() == "basetime") {
                    GetDefNumber(baseTime, def, SourceFilename);
                }

                else if (def->name()->value() == "time") {
                    GetDefNumber(time, def, SourceFilename);
                }

                else if (def->name()->value() == "sitrep") {
                    GetDefText(sitrep, def, SourceFilename);
                }

                else if (def->name()->value() == "orders") {
                    GetDefText(orders, def, SourceFilename);
                }

                else if (def->name()->value() == "combatant") {
                    if (!def->term() || !def->term()->isStruct()) {
                        UE_LOG(LogCampaignSaveGame, Warning, TEXT("WARNING: combatant struct missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath()), ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        TermStruct* val = def->term()->isStruct();

                        char  cname[64];
                        int   iff = 0;
                        int   score = 0;

                        ZeroMemory(cname, sizeof(cname));

                        for (int i = 0; i < val->elements()->size(); i++) {
                            TermDef* pdef = val->elements()->at(i)->isDef();
                            if (pdef) {
                                if (pdef->name()->value() == "name")
                                    GetDefText(cname, pdef, SourceFilename);

                                else if (pdef->name()->value() == "iff") {
                                    GetDefNumber(iff, pdef, SourceFilename);
                                }

                                else if (pdef->name()->value() == "score") {
                                    GetDefNumber(score, pdef, SourceFilename);
                                }
                            }
                        }

                        if (campaign && cname[0]) {
                            Combatant* combatant = campaign->GetCombatant(cname);

                            if (combatant) {
                                CombatGroup::MergeOrderOfBattle((BYTE*)Utf8.Get(), SourceFilename, iff, combatant, campaign);
                                combatant->SetScore(score);
                            }
                            else {
                                UE_LOG(LogCampaignSaveGame, Warning, TEXT("WARNING: could not find combatant '%s' in campaign."),
                                    ANSI_TO_TCHAR(cname));
                            }
                        }
                    }
                }

                else if (def->name()->value() == "event") {
                    if (!def->term() || !def->term()->isStruct()) {
                        UE_LOG(LogCampaignSaveGame, Warning, TEXT("WARNING: event struct missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath()), ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        TermStruct* val = def->term()->isStruct();

                        char  type[64];
                        char  source[64];
                        char  region[64];
                        char  title[256];
                        char  file[256];
                        char  image[256];
                        char  scene[256];
                        char  info[4096];
                        int   ev_time = 0;
                        int   team = 0;
                        int   points = 0;

                        type[0] = 0;
                        info[0] = 0;
                        file[0] = 0;
                        image[0] = 0;
                        scene[0] = 0;
                        title[0] = 0;
                        region[0] = 0;
                        source[0] = 0;

                        for (int i = 0; i < val->elements()->size(); i++) {
                            TermDef* pdef = val->elements()->at(i)->isDef();
                            if (pdef) {
                                if (pdef->name()->value() == "type")
                                    GetDefText(type, pdef, SourceFilename);

                                else if (pdef->name()->value() == "source")
                                    GetDefText(source, pdef, SourceFilename);

                                else if (pdef->name()->value() == "region")
                                    GetDefText(region, pdef, SourceFilename);

                                else if (pdef->name()->value() == "title")
                                    GetDefText(title, pdef, SourceFilename);

                                else if (pdef->name()->value() == "file")
                                    GetDefText(file, pdef, SourceFilename);

                                else if (pdef->name()->value() == "image")
                                    GetDefText(image, pdef, SourceFilename);

                                else if (pdef->name()->value() == "scene")
                                    GetDefText(scene, pdef, SourceFilename);

                                else if (pdef->name()->value() == "info")
                                    GetDefText(info, pdef, SourceFilename);

                                else if (pdef->name()->value() == "time")
                                    GetDefNumber(ev_time, pdef, SourceFilename);

                                else if (pdef->name()->value() == "team")
                                    GetDefNumber(team, pdef, SourceFilename);

                                else if (pdef->name()->value() == "points")
                                    GetDefNumber(points, pdef, SourceFilename);
                            }
                        }

                        if (campaign && type[0]) {
                            loader->SetDataPath(campaign->Path());

                            CombatEvent* event = new CombatEvent(
                                campaign,
                                CombatEvent::GetTypeFromName(type),
                                ev_time,
                                team,
                                CombatEvent::GetSourceFromName(source),
                                region);

                            if (event) {
                                event->SetTitle(title);
                                event->SetFilename(file);
                                event->SetImageFile(image);
                                event->SetSceneFile(scene);
                                event->Load();

                                if (info[0])
                                    event->SetInformation(ParseMultiLine(info));

                                event->SetVisited(true);
                                campaign->GetEvents().append(event);
                            }
                        }
                    }
                }

                else if (def->name()->value() == "action") {
                    if (!def->term() || !def->term()->isStruct()) {
                        UE_LOG(LogCampaignSaveGame, Warning, TEXT("WARNING: action struct missing in '%s/%s'"),
                            ANSI_TO_TCHAR(loader->GetDataPath()), ANSI_TO_TCHAR(SourceFilename));
                    }
                    else {
                        TermStruct* val = def->term()->isStruct();

                        int   id = -1;
                        int   stat = 0;
                        int   count = 0;
                        int   after = 0;

                        for (int i = 0; i < val->elements()->size(); i++) {
                            TermDef* pdef = val->elements()->at(i)->isDef();
                            if (pdef) {
                                if (pdef->name()->value() == "id")
                                    GetDefNumber(id, pdef, SourceFilename);

                                else if (pdef->name()->value() == "stat")
                                    GetDefNumber(stat, pdef, SourceFilename);

                                else if (pdef->name()->value() == "count")
                                    GetDefNumber(count, pdef, SourceFilename);

                                else if (pdef->name()->value().contains("after"))
                                    GetDefNumber(after, pdef, SourceFilename);
                            }
                        }

                        if (campaign && id >= 0) {
                            ListIter<CombatAction> a_iter = campaign->GetActions();
                            while (++a_iter) {
                                CombatAction* a = a_iter.value();
                                if (a->Identity() == id) {
                                    a->SetStatus(stat);

                                    if (count)
                                        a->SetCount(count);

                                    if (after)
                                        a->SetStartAfter(after);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (term);

    if (term) {
        delete term;
        term = 0;
    }

    if (campaign) {
        campaign->SetSaveGame(true);

        List<Campaign>& list = Campaign::GetAllCampaigns();

        if (status < ECampaignStatus::SUCCESS) {
            campaign->SetCampaignStatus(status);
            if (sitrep.length()) campaign->SetSituation(sitrep);
            if (orders.length()) campaign->SetOrders(orders);
            campaign->SetStartTime(baseTime);
            campaign->SetLoadTime(baseTime + time);
            campaign->LockoutEvents(3600);
            campaign->Start();

            if (grp_type >= (int) ECOMBATGROUP_TYPE::FLEET && grp_type <= (int) ECOMBATGROUP_TYPE::PRIVATE) {
                CombatGroup* player_group = campaign->FindGroup(grp_iff, grp_type, grp_id);
                if (player_group) {
                    CombatUnit* player_unit = 0;

                    if (unit.length())
                        player_unit = player_group->FindUnit(unit);

                    if (player_unit)
                        campaign->SetPlayerUnit(player_unit);
                    else
                        campaign->SetPlayerGroup(player_group);
                }
            }
        }

        // failed - restart current campaign:
        else if (status == ECampaignStatus::FAILED) {
            UE_LOG(LogCampaignSaveGame, Log, TEXT("CampaignSaveGame: Loading FAILED campaign, restarting '%s'"),
                ANSI_TO_TCHAR(campaign->GetName()));

            campaign->Load();
            campaign->Prep(); // restore campaign to pristine state
            campaign->SetSaveGame(false);

            loader->SetDataPath(GetSaveDirectory() + "/");
        }

        // start next campaign:
        else if (status == ECampaignStatus::SUCCESS) {
            UE_LOG(LogCampaignSaveGame, Log, TEXT("CampaignSaveGame: Loading COMPLETED campaign '%s', searching for next campaign..."),
                ANSI_TO_TCHAR(campaign->GetName()));

            bool found = false;

            for (int i = 0; i < list.size() && !found; i++) {
                Campaign* c = list.at(i);

                if (c->GetCampaignId() == campaign->GetCampaignId() + 1) {
                    campaign = c;
                    campaign->Load();
                    campaign->Prep(); // restore campaign to pristine state

                    UE_LOG(LogCampaignSaveGame, Log, TEXT("Advanced to campaign %d '%s'"),
                        campaign->GetCampaignId(), ANSI_TO_TCHAR(campaign->GetName()));

                    loader->SetDataPath(GetSaveDirectory() + "/");
                    found = true;
                }
            }

            // if no next campaign found, start over from the beginning:
            for (int i = 0; i < list.size() && !found; i++) {
                Campaign* c = list.at(i);

                if (c->IsDynamic()) {
                    campaign = c;
                    campaign->Load();
                    campaign->Prep(); // restore campaign to pristine state

                    UE_LOG(LogCampaignSaveGame, Log, TEXT("Completed full series, restarting at %d '%s'"),
                        campaign->GetCampaignId(), ANSI_TO_TCHAR(campaign->GetName()));

                    loader->SetDataPath(GetSaveDirectory() + "/");
                    found = true;
                }
            }
        }
    }

    loader->SetDataPath(0);

    UE_LOG(LogCampaignSaveGame, Log, TEXT("SAVEGAME LOADED (%s)."), ANSI_TO_TCHAR(SourceFilename));
}

// +--------------------------------------------------------------------+

void
CampaignSaveGame::Save(const char* name)
{
    if (!campaign || !name || !*name)
        return;

    FString Text;
    char timestr[32] = { 0 };
    FormatDayTime(timestr, campaign->GetTime());

    CombatGroup* player_group = campaign->GetPlayerGroup();
    CombatUnit* player_unit = campaign->GetPlayerUnit();

    Text += TEXT("SAVEGAME\n\n");
    Text += FString::Printf(TEXT("campaign: \"%s\"\n\n"), ANSI_TO_TCHAR(campaign->GetName()));

    if (player_group) {
        Text += FString::Printf(TEXT("grp_iff:  %d\n"), (int)player_group->GetIFF());
        Text += FString::Printf(TEXT("grp_type: %d\n"), (int)player_group->GetType());
        Text += FString::Printf(TEXT("grp_id:   %d\n"), (int)player_group->GetID());
    }
    else {
        Text += TEXT("grp_iff:  0\ngrp_type: 0\ngrp_id:   0\n");
    }

    if (player_unit)
        Text += FString::Printf(TEXT("unit:     \"%s\"\n"), ANSI_TO_TCHAR(player_unit->GetName().data()));

    Text += FString::Printf(TEXT("status:   %d\n"), (int)campaign->GetCampaignStatus());
    Text += FString::Printf(TEXT("basetime: %f\n"), campaign->GetStartTime());
    Text += FString::Printf(TEXT("time:     %f // %s\n\n"), campaign->GetTime(), ANSI_TO_TCHAR(timestr));
    Text += FString::Printf(TEXT("sitrep:   \"%s\"\n"), ANSI_TO_TCHAR(campaign->Situation()));
    Text += FString::Printf(TEXT("orders:   \"%s\"\n\n"), ANSI_TO_TCHAR(campaign->Orders()));

    ListIter<Combatant> c_iter = campaign->GetCombatants();
    while (++c_iter) {
        Combatant* c = c_iter.value();
        if (!c) continue;
        Text += FString::Printf(TEXT("combatant: { name:\"%s\", iff:%d, score:%d, }\n"),
            ANSI_TO_TCHAR(c->GetName()), c->GetIFF(), c->GetScore());
    }
    Text += TEXT("\n");

    ListIter<CombatAction> a_iter = campaign->GetActions();
    while (++a_iter) {
        CombatAction* a = a_iter.value();
        if (!a) continue;
        Text += FString::Printf(TEXT("action: { id:%4d, stat:%d"), a->Identity(), (int)a->Status());
        if (a->Status() == ECombatActionStatus::PENDING) {
            if (a->Count()) Text += FString::Printf(TEXT(", count:%d"), a->Count());
            if (a->StartAfter()) Text += FString::Printf(TEXT(", after:%d"), a->StartAfter());
        }
        Text += TEXT(" }\n");
    }
    Text += TEXT("\n");

    ListIter<CombatEvent> e_iter = campaign->GetEvents();
    while (++e_iter) {
        CombatEvent* e = e_iter.value();
        if (!e) continue;
        const FVector& P = e->GetPoints();
        Text += FString::Printf(TEXT("event: { type:%s, time:0x%08x, team:%d, points:{ x:%f, y:%f, z:%f }, source:\"%s\", region:\"%s\", title:\"%s\","),
            *e->GetEventTypeName(), e->GetTime(), e->GetIFF(), P.X, P.Y, P.Z,
            *e->GetEventSourceName(), ANSI_TO_TCHAR(e->GetRegion()), ANSI_TO_TCHAR(e->GetTitle()));
        if (e->Filename()) Text += FString::Printf(TEXT(" file:\"%s\","), ANSI_TO_TCHAR(e->Filename()));
        if (e->ImageFile()) Text += FString::Printf(TEXT(" image:\"%s\","), ANSI_TO_TCHAR(e->ImageFile()));
        if (e->SceneFile()) Text += FString::Printf(TEXT(" scene:\"%s\","), ANSI_TO_TCHAR(e->SceneFile()));
        if (!e->Filename() || *e->Filename() == 0)
            Text += FString::Printf(TEXT(" info:\"%s\""), ANSI_TO_TCHAR(FormatMultiLine(e->GetInformation())));
        Text += TEXT(" }\n");
    }

    Text += TEXT("\n// ORDER OF BATTLE:\n\n");
    c_iter.reset();
    while (++c_iter) {
        Combatant* c = c_iter.value();
        if (!c) continue;
        if (CombatGroup* g = c->GetForce())
            CombatGroup::AppendOrderOfBattle(Text, g);
    }

    UCampaignSlotSaveGame* SlotSave = Cast<UCampaignSlotSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCampaignSlotSaveGame::StaticClass()));
    if (!SlotSave) return;

    const int32 PlayerId = CurrentPlayerId();
    const FString Slot = SlotNameFor(name, PlayerId);
    SlotSave->SlotName = Slot;
    SlotSave->CampaignName = ANSI_TO_TCHAR(campaign->GetName());
    SlotSave->SaveText = Text;
    UGameplayStatics::SaveGameToSlot(SlotSave, Slot, SAVE_USER);
    RememberSlot(Slot, PlayerId);
}

void
CampaignSaveGame::Delete(const char* name)
{
    if (!name || !*name) return;
    const int32 PlayerId = CurrentPlayerId();
    const FString Slot = SlotNameFor(name, PlayerId);
    UGameplayStatics::DeleteGameInSlot(Slot, SAVE_USER);
    ForgetSlot(Slot, PlayerId);
}

void
CampaignSaveGame::RemovePlayer(PlayerCharacter* p)
{
    if (!p) return;
    const int32 PlayerId = p->Identity();
    UCampaignSaveIndex* Index = Cast<UCampaignSaveIndex>(
        UGameplayStatics::LoadGameFromSlot(IndexSlotFor(PlayerId), SAVE_USER));
    if (Index)
    {
        for (const FString& Slot : Index->SlotNames)
            UGameplayStatics::DeleteGameInSlot(Slot, SAVE_USER);
    }
    UGameplayStatics::DeleteGameInSlot(IndexSlotFor(PlayerId), SAVE_USER);
}

// +--------------------------------------------------------------------+

void
CampaignSaveGame::SaveAuto()
{
    Save("AutoSave");
}

void
CampaignSaveGame::LoadAuto()
{
    Load("AutoSave");
}

// +--------------------------------------------------------------------+

Text
CampaignSaveGame::GetResumeFile()
{
    const FString Slot = SlotNameFor("AutoSave", CurrentPlayerId());
    if (UGameplayStatics::DoesSaveGameExist(Slot, SAVE_USER))
        return "AutoSave";
    return Text();
}

int
CampaignSaveGame::GetSaveGameList(List<Text>& save_list)
{
    save_list.destroy();
    UCampaignSaveIndex* Index = Cast<UCampaignSaveIndex>(
        UGameplayStatics::LoadGameFromSlot(IndexSlotFor(CurrentPlayerId()), SAVE_USER));
    if (!Index) return 0;

    for (const FString& Slot : Index->SlotNames)
    {
        FString Name = Slot;
        const FString Prefix = FString::Printf(TEXT("Campaign_%02d_"), CurrentPlayerId());
        if (Name.StartsWith(Prefix))
            Name.RightChopInline(Prefix.Len());
        save_list.append(new Text(TCHAR_TO_ANSI(*Name)));
    }
    return save_list.size();
}
