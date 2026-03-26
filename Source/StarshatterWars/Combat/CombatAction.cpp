/*  Project Starshatter 4.5
    Fractal Dev Studios
    Copyright © 2025. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CombatAction.cpp
    AUTHOR:       Carlos Bott

    UNREAL PORT:
    - Preserves original logic and flow.
    - Uses FMath::RandRange for probability roll (0..99).
    - Assumes Campaign::GetCampaign(), Player::GetCurrentPlayer(), Combatant/CombatGroup APIs remain as in Starshatter.
*/

#include "CombatAction.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "PlayerCharacter.h"

#include "Math/UnrealMathUtility.h"

static int32 Rand0_99()
{
    return FMath::RandRange(0, 99);
}

CombatAction::CombatAction(int32 InId, int32 InType, int32 InSubtype, int32 InTeam)
    : id(InId)
    , title(nullptr)
    , type((uint8)InType)
    , subtype((uint8)InSubtype)
    , opp_type((uint8)0xFF) // -1 in uint8
    , team((uint8)InTeam)
    , status((uint8)PENDING)
    , min_rank(0)
    , max_rank(100)
    , source(ECombatEventSource::NONE)
    , count(0)
    , start_before((int32)1e9)
    , start_after(0)
    , delay(0)
    , probability(100)
    , rval(-1)
    , time(0)
    , asset_type(0)
    , asset_id(0)
    , target_type(0)
    , target_id(0)
    , target_iff(0)
{
}

CombatAction::~CombatAction()
{
    requirements.destroy();
    asset_kills.destroy();
    target_kills.destroy();
}

bool CombatAction::IsAvailable() const
{
    // Preserve original behavior: may mutate rval/status/delay/start_after
    CombatAction* pThis = const_cast<CombatAction*>(this);

    if (rval < 0)
    {
        pThis->rval = Rand0_99();

        if (rval > probability)
            pThis->status = (uint8)SKIPPED;
    }

    if (status != PENDING)
        return false;

    if (min_rank > 0 || max_rank < 100)
    {
        PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
        if (!player)
            return false;

        if (player->GetRank() < (int)min_rank || player->GetRank() > (int)max_rank)
            return false;
    }

    Campaign* campaign = Campaign::GetCampaign();
    if (campaign)
    {
        if (campaign->GetTime() < start_after)
            return false;

        if (campaign->GetTime() > start_before)
        {
            pThis->status = (uint8)FAILED; // too late
            return false;
        }

        // check requirements against actions in current campaign:
        ListIter<CombatActionReq> iter = pThis->requirements;
        while (++iter)
        {
            CombatActionReq* r = iter.value();
            bool ok = false;

            if (r->action > 0)
            {
                ListIter<CombatAction> action = campaign->GetActions();
                while (++action)
                {
                    CombatAction* a = action.value();

                    if (a->Identity() == r->action)
                    {
                        if (r->notx)
                        {
                            if (a->Status() == r->stat)
                                return false;
                        }
                        else
                        {
                            if (a->Status() != r->stat)
                                return false;
                        }
                    }
                }
            }

            // group-based requirement
            else if (r->group_type != ECOMBATGROUP_TYPE::NONE)
            {
                if (r->c1)
                {
                    CombatGroup* group = r->c1->FindGroup(r->group_type, r->group_id);

                    if (group)
                    {
                        int test = 0;
                        int comp = 0;

                        if (r->intel)
                        {
                            test = group->GetIntelLevel();
                            comp = r->intel;
                        }
                        else
                        {
                            test = group->CalcValue();
                            comp = r->score;
                        }

                        switch (r->comp)
                        {
                        case CombatActionReq::LT: ok = (test < comp); break;
                        case CombatActionReq::LE: ok = (test <= comp); break;
                        case CombatActionReq::GT: ok = (test > comp); break;
                        case CombatActionReq::GE: ok = (test >= comp); break;
                        case CombatActionReq::EQ: ok = (test == comp); break;
                        default:                  ok = false;          break;
                        }
                    }

                    if (!ok)
                        return false;
                }
            }

            // score-based requirement
            else
            {
                if (r->comp <= CombatActionReq::EQ) // absolute
                {
                    if (r->c1)
                    {
                        const int test = r->c1->GetScore();

                        switch (r->comp)
                        {
                        case CombatActionReq::LT: ok = (test < r->score); break;
                        case CombatActionReq::LE: ok = (test <= r->score); break;
                        case CombatActionReq::GT: ok = (test > r->score); break;
                        case CombatActionReq::GE: ok = (test >= r->score); break;
                        case CombatActionReq::EQ: ok = (test == r->score); break;
                        default:                  ok = false;             break;
                        }
                    }
                }
                else // relative
                {
                    if (r->c1 && r->c2)
                    {
                        const int test = r->c1->GetScore() - r->c2->GetScore();

                        switch (r->comp)
                        {
                        case CombatActionReq::RLT: ok = (test < r->score); break;
                        case CombatActionReq::RLE: ok = (test <= r->score); break;
                        case CombatActionReq::RGT: ok = (test > r->score); break;
                        case CombatActionReq::RGE: ok = (test >= r->score); break;
                        case CombatActionReq::REQ: ok = (test == r->score); break;
                        default:                   ok = false;             break;
                        }
                    }
                }

                if (!ok)
                    return false;
            }

            if (delay > 0)
            {
                pThis->start_after = (int)campaign->GetTime() + delay;
                pThis->delay = 0;
                return IsAvailable();
            }
        }
    }

    return true;
}

void CombatAction::FireAction()
{
    Campaign* campaign = Campaign::GetCampaign();
    if (campaign)
        time = (int)campaign->GetTime();

    if (count >= 1)
        count--;

    if (count < 1)
        status = (uint8)COMPLETE;
}

void CombatAction::FailAction()
{
    Campaign* campaign = Campaign::GetCampaign();
    if (campaign)
        time = (int)campaign->GetTime();

    count = 0;
    status = (uint8)FAILED;
}

void CombatAction::AddRequirement(int action, int stat, bool not_req)
{
    requirements.append(new  CombatActionReq(action, stat, not_req));
}

void CombatAction::AddRequirement(Combatant* c1, Combatant* c2, int comp, int score)
{
    requirements.append(new  CombatActionReq(c1, c2, comp, score));
}

void CombatAction::AddRequirement(Combatant* c1, ECOMBATGROUP_TYPE group_type, int group_id, int comp, int score, int intel)
{
    requirements.append(new CombatActionReq(c1, group_type, group_id, comp, score, intel));
}

int CombatAction::TypeFromName(const char* n)
{
    if (!n || !*n)
        return 0;

    if (!_stricmp(n, "NO_ACTION"))             return NO_ACTION;
    if (!_stricmp(n, "MARKER"))                return NO_ACTION;

    if (!_stricmp(n, "STRATEGIC_DIRECTIVE"))   return STRATEGIC_DIRECTIVE;
    if (!_stricmp(n, "STRATEGIC"))             return STRATEGIC_DIRECTIVE;

    if (!_stricmp(n, "ZONE_ASSIGNMENT"))       return ZONE_ASSIGNMENT;
    if (!_stricmp(n, "ZONE"))                  return ZONE_ASSIGNMENT;

    if (!_stricmp(n, "SYSTEM_ASSIGNMENT"))     return SYSTEM_ASSIGNMENT;
    if (!_stricmp(n, "SYSTEM"))                return SYSTEM_ASSIGNMENT;

    if (!_stricmp(n, "MISSION_TEMPLATE"))      return MISSION_TEMPLATE;
    if (!_stricmp(n, "MISSION"))               return MISSION_TEMPLATE;

    if (!_stricmp(n, "COMBAT_EVENT"))          return COMBAT_EVENT;
    if (!_stricmp(n, "EVENT"))                 return COMBAT_EVENT;

    if (!_stricmp(n, "INTEL_EVENT"))           return INTEL_EVENT;
    if (!_stricmp(n, "INTEL"))                 return INTEL_EVENT;

    if (!_stricmp(n, "CAMPAIGN_SITUATION"))    return CAMPAIGN_SITUATION;
    if (!_stricmp(n, "SITREP"))                return CAMPAIGN_SITUATION;

    if (!_stricmp(n, "CAMPAIGN_ORDERS"))       return CAMPAIGN_ORDERS;
    if (!_stricmp(n, "ORDERS"))                return CAMPAIGN_ORDERS;

    return 0;
}

int CombatAction::StatusFromName(const char* n)
{
    if (!n || !*n)
        return 0;

    if (!_stricmp(n, "PENDING"))   return PENDING;
    if (!_stricmp(n, "ACTIVE"))    return ACTIVE;
    if (!_stricmp(n, "SKIPPED"))   return SKIPPED;
    if (!_stricmp(n, "FAILED"))    return FAILED;
    if (!_stricmp(n, "COMPLETE"))  return COMPLETE;

    return 0;
}

int CombatActionReq::CompFromName(const char* n)
{
    if (!n || !*n)
        return 0;

    if (!_stricmp(n, "LT"))   return LT;
    if (!_stricmp(n, "LE"))   return LE;
    if (!_stricmp(n, "GT"))   return GT;
    if (!_stricmp(n, "GE"))   return GE;
    if (!_stricmp(n, "EQ"))   return EQ;

    if (!_stricmp(n, "RLT"))  return RLT;
    if (!_stricmp(n, "RLE"))  return RLE;
    if (!_stricmp(n, "RGT"))  return RGT;
    if (!_stricmp(n, "RGE"))  return RGE;
    if (!_stricmp(n, "REQ"))  return REQ;

    return 0;
}

void CombatAction::SetTitle(const char* t)
{
    if (t && *t)
    {
        title = Text(t);   // correct
    }
    else
    {
        title = Text("");  // clear
    }
}

const char* CombatAction::GetTitle() const
{
    return title.data();   // or title.c_str() depending on implementation
}

void CombatAction::SetSoundFile(const char* s)
{
    if (s && *s)
    {
        sound_file = Text(s);
    }
    else
    {
        sound_file = Text("");
    }
}

const char* CombatAction::GetSoundFile() const
{
    return sound_file.data();
}