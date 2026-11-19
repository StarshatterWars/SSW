#include "MissionArcRules.h"
#include "AgentMissionRules.h"
namespace { bool IsArcMission(int32 Id){int32 Campaign=0,Mission=0;return (Id>910000 && Id<1000000) || AgentMissionRules::Decode(Id,Campaign,Mission);} }
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FMissionArcCatalog& FMissionArcCatalog::Get()
{
    static const FMissionArcCatalog Catalog = []()
    {
        FMissionArcCatalog Result; FString Json; FText Error;
        if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir()/TEXT("GameData/Agents/mission_arcs.json"))) || !Result.LoadJson(Json, Error))
        {
            UE_LOG(LogTemp, Error, TEXT("[MissionArcs] Could not load short arcs: %s"), *Error.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[MissionArcs] Loaded %d short regional arcs."), Result.GetArcs().Num());
        }
        return Result;
    }();
    return Catalog;
}
bool FMissionArcCatalog::LoadJson(const FString& Json, FText& Error)
{
    TArray<TSharedPtr<FJsonValue>> Rows;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows) || Rows.IsEmpty() || Rows.Num()>128)
    { Error=INVTEXT("Mission arcs must be a nonempty JSON list."); return false; }
    TArray<FSSWMissionArc> Imported; TSet<FName> ArcIds; TSet<int32> Missions;
    for (const auto& Row:Rows)
    {
        const auto Object=Row.IsValid()?Row->AsObject():nullptr;
        FString Id; FSSWMissionArc Arc; const TArray<TSharedPtr<FJsonValue>>* Steps=nullptr;
        if (!Object || !Object->TryGetStringField(TEXT("Id"),Id) || Id.IsEmpty() || ArcIds.Contains(FName(*Id)) ||
            !Object->TryGetStringField(TEXT("Title"),Arc.Title) || Arc.Title.IsEmpty() ||
            !Object->TryGetStringField(TEXT("Empire"),Arc.Empire) || Arc.Empire.IsEmpty() ||
            !Object->TryGetStringField(TEXT("Description"),Arc.Description) ||
            !Object->TryGetArrayField(TEXT("Steps"),Steps) || !Steps || Steps->Num()<2 || Steps->Num()>128)
        { Error=INVTEXT("Each short arc requires a unique ID, title, empire and two to 128 steps."); return false; }
        Arc.Id=FName(*Id); ArcIds.Add(Arc.Id);
        for (const auto& Value:*Steps)
        {
            const auto StepObject=Value.IsValid()?Value->AsObject():nullptr; FSSWMissionArcStep Step;
            if (!StepObject || !StepObject->TryGetNumberField(TEXT("MissionId"),Step.MissionId) || !IsArcMission(Step.MissionId) || Missions.Contains(Step.MissionId) ||
                !StepObject->TryGetStringField(TEXT("Title"),Step.Title) || Step.Title.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Issuer"),Step.Issuer) || Step.Issuer.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("System"),Step.System) || Step.System.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Region"),Step.Region) || Step.Region.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Brief"),Step.Brief))
            { Error=INVTEXT("Arc steps require unique regional mission IDs and an agent/location handoff."); return false; }
            FString Field;
            if(StepObject->TryGetStringField(TEXT("AgentId"),Field))Step.AgentId=FName(*Field);
            if(StepObject->TryGetStringField(TEXT("Chapter"),Field))Step.Chapter=FName(*Field);
            if(StepObject->TryGetStringField(TEXT("ChoiceGroup"),Field))Step.ChoiceGroup=FName(*Field);
            const TArray<TSharedPtr<FJsonValue>>* Links=nullptr;
            if(StepObject->HasField(TEXT("NextMissionIds")))
            {
                if(!StepObject->TryGetArrayField(TEXT("NextMissionIds"),Links)){Error=INVTEXT("NextMissionIds must be a list.");return false;}
                for(const auto& Link:*Links){double IdValue=0;if(!Link.IsValid() || !Link->TryGetNumber(IdValue) || IdValue!=FMath::FloorToDouble(IdValue) || IdValue>MAX_int32 || !IsArcMission(static_cast<int32>(IdValue)) || Step.NextMissionIds.Contains(static_cast<int32>(IdValue))){Error=INVTEXT("Invalid or duplicate arc link.");return false;}Step.NextMissionIds.Add(static_cast<int32>(IdValue));}
            }
            else if(Arc.Steps.Num()+1<Steps->Num())
            {
                int32 NextId=0;const auto NextObject=(*Steps)[Arc.Steps.Num()+1]->AsObject();
                if(!NextObject || !NextObject->TryGetNumberField(TEXT("MissionId"),NextId)){Error=INVTEXT("Invalid next mission.");return false;}Step.NextMissionIds.Add(NextId);
            }
            Missions.Add(Step.MissionId); Arc.Steps.Add(MoveTemp(Step));
        }
        // Reject cycles, broken links and disconnected chapters before replacing live data.
        TMap<int32,int32> Marks;TSet<int32> Children;
        TFunction<bool(int32)> Visit=[&](int32 Id)
        {
            if(Marks.FindRef(Id)==1)return false;if(Marks.FindRef(Id)==2)return true;
            const auto* Step=Arc.Steps.FindByPredicate([Id](const auto& S){return S.MissionId==Id;});if(!Step)return false;
            Marks.Add(Id,1);for(int32 Next:Step->NextMissionIds){Children.Add(Next);if(!Visit(Next))return false;}Marks.Add(Id,2);return true;
        };
        if(!Visit(Arc.Steps[0].MissionId) || Marks.Num()!=Arc.Steps.Num() || Children.Contains(Arc.Steps[0].MissionId))
        {Error=INVTEXT("Arc chapters must form a connected acyclic graph from the first mission.");return false;}
        Imported.Add(MoveTemp(Arc));
    }
    Arcs=MoveTemp(Imported); bLoaded=true; Error=FText::GetEmpty(); return true;
}
FSSWMissionArcProgress FMissionArcCatalog::Evaluate(int32 MissionId,TFunctionRef<bool(int32)> Completed,TFunction<bool(int32)> Chosen) const
{
    FSSWMissionArcProgress Result;
    for(const auto& Arc:Arcs)
    {
        Result.Selected=Arc.Steps.IndexOfByPredicate([MissionId](const auto& Step){return Step.MissionId==MissionId;});
        if(Result.Selected==INDEX_NONE)continue;
        Result.Arc=&Arc;Result.bSelectedComplete=Completed(MissionId);
        for(int32 Index=0;Index<Arc.Steps.Num();++Index)
        {
            const auto& Step=Arc.Steps[Index];
            if(Completed(Step.MissionId)){++Result.Completed;continue;}
            bool bBlocked=false;
            if(!Step.ChoiceGroup.IsNone())for(const auto& Sibling:Arc.Steps)
                if(Sibling.MissionId!=Step.MissionId && Sibling.ChoiceGroup==Step.ChoiceGroup && (Completed(Sibling.MissionId) || (Chosen && Chosen(Sibling.MissionId))))bBlocked=true;
            bool bIncoming=false,bUnlocked=Index==0;
            for(const auto& Previous:Arc.Steps)if(Previous.NextMissionIds.Contains(Step.MissionId))
            {bIncoming=true;bUnlocked|=Completed(Previous.MissionId);}
            if(!bBlocked && bUnlocked && (Index==0 || bIncoming))Result.Available.Add(Index);
        }
        Result.Current=Result.Available.IsEmpty()?INDEX_NONE:Result.Available[0];
        Result.bEligible=Result.Available.Contains(Result.Selected);
        Result.bComplete=false;
        for(const auto& Step:Arc.Steps)if(Step.NextMissionIds.IsEmpty() && Completed(Step.MissionId))Result.bComplete=true;
        if(Result.bComplete){Result.Available.Reset();Result.Current=INDEX_NONE;Result.bEligible=false;}
        break;
    }
    return Result;
}
FText FMissionArcCatalog::Requirement(int32 MissionId,TFunctionRef<bool(int32)> Completed,TFunction<bool(int32)> Chosen) const
{
    if(!bLoaded && MissionId>910000 && MissionId<1000000)return INVTEXT("The mission arc catalog is unavailable.");
    const auto P=Evaluate(MissionId,Completed,Chosen);
    if(!P.Arc || P.bEligible)return FText::GetEmpty();
    if(P.bSelectedComplete)return INVTEXT("This arc mission is complete. Rewards are available once per pilot.");
    if(P.bComplete)return INVTEXT("This mission arc is complete.");
    if(P.Current==INDEX_NONE)return INVTEXT("This chapter is unavailable on your selected campaign branch.");
    const auto& Required=P.Arc->Steps[P.Current];
    return FText::Format(INVTEXT("Complete the current chapter: {0}. Contact {1} in {2} / {3}."),FText::FromString(Required.Title),FText::FromString(Required.Issuer),FText::FromString(Required.System),FText::FromString(Required.Region));
}
FText FMissionArcCatalog::Status(int32 MissionId,TFunctionRef<bool(int32)> Completed,TFunction<bool(int32)> Chosen) const
{
    const auto P=Evaluate(MissionId,Completed,Chosen);if(!P.Arc)return FText::GetEmpty();
    const auto& Selected=P.Arc->Steps[P.Selected];
    FString Text=FString::Printf(TEXT("%s | %d COMPLETE\n%s"),*P.Arc->Title,P.Completed,*Selected.Brief);
    if(!Selected.Chapter.IsNone())Text+=TEXT("\nCHAPTER: ")+Selected.Chapter.ToString();
    if(P.bComplete)Text+=TEXT("\nARC COMPLETE");
    else for(int32 Index:P.Available)
    {
        const auto& Next=P.Arc->Steps[Index];
        Text+=FString::Printf(TEXT("\nCURRENT: %s — %s / %s / %s"),*Next.Title,*Next.Issuer,*Next.System,*Next.Region);
    }
    return FText::FromString(Text);
}
