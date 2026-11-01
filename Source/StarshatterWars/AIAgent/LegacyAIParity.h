#pragma once
#include "GameStructs.h"
// Explicit bridge: legacy radio recovery constants aliased Instruction values.
// UE's distinct enums no longer share those numeric values.
namespace LegacyAIParity
{
    inline RadioMessageAction RecoveryAction(EInstruction Instruction,RadioMessageAction ExplicitAction)
    {
        if(ExplicitAction!=RadioMessageAction::NONE)return ExplicitAction;
        if(Instruction==EInstruction::RTB)return RadioMessageAction::RTB;
        if(Instruction==EInstruction::Dock)return RadioMessageAction::DOCK_WITH;
        return RadioMessageAction::NONE;
    }
}
