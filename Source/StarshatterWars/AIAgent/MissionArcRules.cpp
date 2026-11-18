#include "MissionArcRules.h"
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
            !Object->TryGetArrayField(TEXT("Steps"),Steps) || !Steps || Steps->Num()<2 || Steps->Num()>8)
        { Error=INVTEXT("Each short arc requires a unique ID, title, empire and two to eight steps."); return false; }
        Arc.Id=FName(*Id); ArcIds.Add(Arc.Id);
        for (const auto& Value:*Steps)
        {
            const auto StepObject=Value.IsValid()?Value->AsObject():nullptr; FSSWMissionArcStep Step;
            if (!StepObject || !StepObject->TryGetNumberField(TEXT("MissionId"),Step.MissionId) || Step.MissionId<=910000 || Step.MissionId>=1000000 || Missions.Contains(Step.MissionId) ||
                !StepObject->TryGetStringField(TEXT("Title"),Step.Title) || Step.Title.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Issuer"),Step.Issuer) || Step.Issuer.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("System"),Step.System) || Step.System.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Region"),Step.Region) || Step.Region.IsEmpty() ||
                !StepObject->TryGetStringField(TEXT("Brief"),Step.Brief))
            { Error=INVTEXT("Arc steps require unique regional mission IDs and an agent/location handoff."); return false; }
            Missions.Add(Step.MissionId); Arc.Steps.Add(MoveTemp(Step));
        }
        Imported.Add(MoveTemp(Arc));
    }
    Arcs=MoveTemp(Imported); bLoaded=true; Error=FText::GetEmpty(); return true;
}
FSSWMissionArcProgress FMissionArcCatalog::Evaluate(int32 MissionId,TFunctionRef<bool(int32)> Completed) const
{
    FSSWMissionArcProgress Result;
    for (const auto& Arc:Arcs)
    {
        const int32 Selected=Arc.Steps.IndexOfByPredicate([MissionId](const auto& Step){return Step.MissionId==MissionId;});
        if (Selected==INDEX_NONE) continue;
        Result.Arc=&Arc; Result.Selected=Selected;
        for(int32 Index=0;Index<Arc.Steps.Num();++Index)
            if(Completed(Arc.Steps[Index].MissionId)) ++Result.Completed;
            else if(Result.Current==INDEX_NONE) Result.Current=Index;
        Result.bSelectedComplete=Completed(MissionId);
        Result.bEligible=!Result.bSelectedComplete && Result.Current==Selected;
        break;
    }
    return Result;
}
FText FMissionArcCatalog::Requirement(int32 MissionId,TFunctionRef<bool(int32)> Completed) const
{
    if(!bLoaded && MissionId>910000 && MissionId<1000000) return INVTEXT("The short mission arc catalog is unavailable.");
    const auto Progress=Evaluate(MissionId,Completed);
    if(!Progress.Arc || Progress.bEligible) return FText::GetEmpty();
    if(Progress.bSelectedComplete) return INVTEXT("This arc step is complete. Regional missions and their rewards are available once per pilot.");
    const auto& Required=Progress.Arc->Steps[Progress.Current];
    return FText::Format(INVTEXT("Complete step {0}: {1}. Contact {2} in {3} / {4}."),FText::AsNumber(Progress.Current+1),FText::FromString(Required.Title),FText::FromString(Required.Issuer),FText::FromString(Required.System),FText::FromString(Required.Region));
}
FText FMissionArcCatalog::Status(int32 MissionId,TFunctionRef<bool(int32)> Completed) const
{
    const auto Progress=Evaluate(MissionId,Completed);
    if(!Progress.Arc) return FText::GetEmpty();
    const auto& Arc=*Progress.Arc; const auto& Selected=Arc.Steps[Progress.Selected];
    FString Text=FString::Printf(TEXT("%s | STEP %d OF %d | %d COMPLETE\n%s\n%s"),*Arc.Title,Progress.Selected+1,Arc.Steps.Num(),Progress.Completed,*Arc.Description,*Selected.Brief);
    if(Progress.Current==INDEX_NONE) Text+=TEXT("\nARC COMPLETE. All mission rewards have already been settled through the normal mission result.");
    else
    {
        const auto& Next=Arc.Steps[Progress.Current];
        Text+=FString::Printf(TEXT("\nCURRENT: %s — %s / %s / %s"),*Next.Title,*Next.Issuer,*Next.System,*Next.Region);
        if(Progress.Current+1<Arc.Steps.Num())
        {
            const auto& After=Arc.Steps[Progress.Current+1];
            Text+=FString::Printf(TEXT("\nNEXT: %s — %s / %s / %s"),*After.Title,*After.Issuer,*After.System,*After.Region);
        }
    }
    return FText::FromString(Text);
}
