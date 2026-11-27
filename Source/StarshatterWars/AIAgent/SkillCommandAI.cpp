#include "SkillCommandAI.h"
#include "SkillsSubsystem.h"

FNativeSkillReply SkillCommandAI::Execute(FEconomyState& Economy,const FNativeSkillCommand& C)
{
    FNativeSkillReply R;R.Source=C.Source;R.Skills=C.Source;R.SourceQueue=C.SourceQueue;R.Queue=C.SourceQueue;
    const auto Reject=[&](const FText& Why){R.Reason=Why;return R;};
    if(!C.Receipt.IsValid() || C.NowUtc<=0 || C.NowUtc>1000000000000LL || Economy.CommittedTransactions.Contains(C.Receipt))
        return Reject(INVTEXT("Invalid or already completed skill request."));
    // A failed action never commits its incidental offline advancement or payment.
    auto Candidate=Economy;
    R.bChanged=FSkillTrainingRules::Advance(C.Definitions,R.Skills,R.Queue,C.NowUtc);
    const auto* Def=C.Definitions.Find(C.Skill);
    if(C.Action!=ESkillCommand::Advance && C.Action!=ESkillCommand::Cancel && C.Action!=ESkillCommand::Instant && !Def)
        return Reject(INVTEXT("Skill definition unavailable."));
    if(C.Action==ESkillCommand::Book)
    {
        if(Def->BookPrice>C.MaximumCost || C.Rank<Def->MinimumRank)return Reject(INVTEXT("Skill price or rank requirement changed."));
        if(!FSkillRules::Purchase(Candidate,R.Skills,*Def,C.Rank,C.Empire,C.NowUtc,R.Reason))return R;
    }
    else if(C.Action==ESkillCommand::Advanced)
    {
        auto* Progress=R.Skills.Find(C.Skill);
        if(!Progress || FSkillTrainingRules::Level(Progress)<5 || Progress->bAdvancedBookOwned || !Def->bIsPublished ||
            !Def->bIsAvailableForPurchase || C.Rank<Def->MinimumRank || (!Def->Empire.IsNone() && Def->Empire!=C.Empire) ||
            Def->AdvancedBookPrice<0 || Def->AdvancedBookPrice>C.MaximumCost)
            return Reject(INVTEXT("Advanced skill book is unavailable or already owned."));
        if(!FEconomyLedger::Pay(Candidate,FEconomyLedger::Pilot(),TEXT("Advanced skill publisher"),Def->AdvancedBookPrice,C.Receipt,TEXT("Advanced skill book"),ECommercePurpose::Purchase))
            return Reject(INVTEXT("Insufficient credits for the advanced book."));
        Candidate.Journal.Last().Item=C.Skill;Candidate.Journal.Last().Reference=C.Receipt;
        Progress->bAdvancedBookOwned=true;R.Reason=INVTEXT("Advanced book saved. Levels VI-X are unlocked for training.");
    }
    else if(C.Action==ESkillCommand::Queue)
    {
        if(C.Target<1 || C.Target>10)return Reject(INVTEXT("Invalid training level."));
        int32 Projected=FSkillTrainingRules::Level(R.Skills.Find(C.Skill));for(const auto& E:R.Queue)if(E.Skill==C.Skill)Projected=E.TargetLevel;
        if(Projected>=C.Target)return Reject(INVTEXT("This level is already trained or queued."));
        for(int32 L=Projected+1;L<=C.Target;++L)
            if(!FSkillTrainingRules::Queue(C.Definitions,R.Skills,R.Queue,C.Skill,C.Rank,C.Empire,C.NowUtc,R.Reason))return R;
        R.Reason=INVTEXT("Training queue saved. Each level continues toward its GMT finish time while offline.");
    }
    else if(C.Action==ESkillCommand::Cancel)
    {
        if(!FSkillTrainingRules::Cancel(R.Skills,R.Queue,C.Entry,C.NowUtc))return Reject(INVTEXT("This training entry has finished or was removed."));
        R.Reason=INVTEXT("Cancellation saved. Earned progress was retained.");
    }
    else if(C.Action==ESkillCommand::Instant)
    {
        const auto* Entry=R.Queue.FindByPredicate([&](const auto& E){return E.Id==C.Entry;});
        if(!Entry || FSkillTrainingRules::InstantCost(*Entry,C.NowUtc)>C.MaximumCost)return Reject(INVTEXT("Training entry or price changed. Nothing charged."));
        if(!FSkillTrainingRules::InstantTrain(Candidate,C.Definitions,R.Skills,R.Queue,C.Entry,C.NowUtc,R.Reason))return R;
    }
    else if(C.Action!=ESkillCommand::Advance)return Reject(INVTEXT("Unknown skill request."));
    R.bSucceeded=true;R.bChanged|=C.Action!=ESkillCommand::Advance;
    if(R.bChanged){Candidate.CommittedTransactions.Add(C.Receipt);Economy=MoveTemp(Candidate);}
    if(R.Reason.IsEmpty())R.Reason=INVTEXT("Offline training progress saved.");return R;
}
