#pragma once
#include "AgentAI.h"
#include "GameStructs.h"

// In-memory transaction boundary for agent commands. Persistence owns disk rollback.
// Mutation callbacks must not change runtime campaigns or award/pay anything externally.
namespace AgentJournalTransaction
{
    enum class EResult { Committed, Rejected, SaveFailed };
    inline EResult Commit(AgentAI& Agent, FS_PlayerGameInfo& Player,
        TFunctionRef<bool()> Mutate, TFunctionRef<bool()> Persist)
    {
        const AgentAI BeforeAgent=Agent;
        const FS_PlayerGameInfo BeforePlayer=Player;
        if (!Mutate())
        {
            Agent=BeforeAgent; Player=BeforePlayer;
            return EResult::Rejected;
        }
        if (!Persist())
        {
            // Preserve exact running/queued state; RestoreJournal performs load-time migration.
            Agent=BeforeAgent; Player=BeforePlayer;
            return EResult::SaveFailed;
        }
        return EResult::Committed;
    }
}
