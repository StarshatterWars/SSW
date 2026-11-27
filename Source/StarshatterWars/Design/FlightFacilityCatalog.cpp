#include "FlightFacilityCatalog.h"
#include "GameStructs_System.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace
{
struct FFacilityPolicy
{
    bool bDockingOnly=false,bDegrees=false;
    double CoordinateScale=1;
    TMap<int32,FShipFlightDeck> Overrides;
};
TMap<FName,FFacilityPolicy> FacilityPolicies;
bool bFacilityPoliciesLoaded=false;
bool ValidFacilityDeck(const FShipFlightDeck& D)
{
    if(D.bLaunchDeck==D.bRecoveryDeck || !FMath::IsFinite(D.AzimuthRadians) || FMath::Abs(D.AzimuthRadians)>2*PI+1.e-5 || D.Location.ContainsNaN() || D.StartPoint.ContainsNaN() || D.EndPoint.ContainsNaN() ||
       !FMath::IsFinite(D.Size) || D.Size<=0 || !FMath::IsFinite(D.CycleTime) || D.CycleTime<=0 || D.MaxShips<0 || D.Slots.IsEmpty())return false;
    for(const auto& Slot:D.Slots)if(Slot.Location.ContainsNaN() || Slot.FilterMask<=0)return false;
    if(D.bRecoveryDeck && D.StartPoint.Equals(D.EndPoint,1.e-6))return false;
    return true;
}
void EnsureFacilityPolicies()
{
    check(IsInGameThread());
    if(!bFacilityPoliciesLoaded){FString Why;if(!FlightFacilityCatalog::Reload(Why))UE_LOG(LogTemp,Error,TEXT("[FlightFacilities] %s"),*Why);}
}
}
bool FlightFacilityCatalog::Reload(FString& Error)
{
    check(IsInGameThread());FString Text;TSharedPtr<FJsonObject> Root;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    const FString Path=FPaths::ProjectContentDir()/TEXT("GameData/Ships/flight_facilities.json");
    if(!FFileHelper::LoadFileToString(Text,*Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root || !Root->TryGetArrayField(TEXT("facilities"),Rows) || Rows->IsEmpty())
    {Error=TEXT("Missing or invalid flight facility policy: ")+Path;return false;}
    int32 Schema=0;
    if(!Root->TryGetNumberField(TEXT("schemaVersion"),Schema) || Schema!=1){Error=TEXT("Unsupported flight facility schema");return false;}
    TMap<FName,FFacilityPolicy> Candidate;
    for(const auto& V:*Rows)
    {
        const auto O=V->AsObject();FString Name,Mode,Unit;
        if(!O || !O->TryGetStringField(TEXT("Name"),Name) || Name.IsEmpty() || Name!=Name.TrimStartAndEnd() || !O->TryGetStringField(TEXT("Mode"),Mode) ||
           (Mode!=TEXT("DockingOnly") && Mode!=TEXT("FlightOperations")) || !O->TryGetStringField(TEXT("AzimuthUnit"),Unit) || (Unit!=TEXT("Degrees") && Unit!=TEXT("Radians")) || Candidate.Contains(FName(*Name)))
        {Error=TEXT("Invalid/duplicate flight facility policy: ")+Name;return false;}
        FFacilityPolicy Policy;Policy.bDockingOnly=Mode==TEXT("DockingOnly");Policy.bDegrees=Unit==TEXT("Degrees");
        if(!O->TryGetNumberField(TEXT("CoordinateScale"),Policy.CoordinateScale) || !FMath::IsFinite(Policy.CoordinateScale) || Policy.CoordinateScale<=0)
        {Error=TEXT("Invalid coordinate scale: ")+Name;return false;}
        const TArray<TSharedPtr<FJsonValue>>* Overrides=nullptr;
        if(O->TryGetArrayField(TEXT("DeckOverrides"),Overrides))for(const auto& Entry:*Overrides)
        {
            const auto E=Entry->AsObject();int32 Index=-1;const TSharedPtr<FJsonObject>* Replacement=nullptr;FShipFlightDeck D;
            if(!E || !E->TryGetNumberField(TEXT("Index"),Index) || Index<0 || Index>63 || Policy.bDockingOnly || Policy.Overrides.Contains(Index) || !E->TryGetObjectField(TEXT("Replacement"),Replacement) ||
               !FJsonObjectConverter::JsonObjectToUStruct(Replacement->ToSharedRef(),&D) || !ValidFacilityDeck(D))
            {Error=TEXT("Invalid flight deck replacement: ")+Name;return false;}
            Policy.Overrides.Add(Index,MoveTemp(D));
        }
        Candidate.Add(FName(*Name),MoveTemp(Policy));
    }
    FacilityPolicies=MoveTemp(Candidate);bFacilityPoliciesLoaded=true;Error.Reset();
    UE_LOG(LogTemp,Display,TEXT("[FlightFacilities] JSON-only policies=%d"),FacilityPolicies.Num());return true;
}
bool FlightFacilityCatalog::Contains(FName Design){EnsureFacilityPolicies();return FacilityPolicies.Contains(Design);}
bool FlightFacilityCatalog::IsDockingOnly(FName Design){EnsureFacilityPolicies();const auto* P=FacilityPolicies.Find(Design);return P && P->bDockingOnly;}
TArray<FName> FlightFacilityCatalog::Names(){EnsureFacilityPolicies();TArray<FName> Out;FacilityPolicies.GetKeys(Out);return Out;}
bool FlightFacilityCatalog::ResolveDeck(FName Design,int32 AuthoredIndex,FShipFlightDeck& InOut)
{
    EnsureFacilityPolicies();const auto* P=FacilityPolicies.Find(Design);if(!P)return true;
    if(P->bDockingOnly)return false;
    if(const auto* Replacement=P->Overrides.Find(AuthoredIndex))InOut=*Replacement;
    else
    {
        if(P->bDegrees)InOut.AzimuthRadians=FMath::DegreesToRadians(InOut.AzimuthRadians);
        const double S=P->CoordinateScale;
        InOut.Location*=S;InOut.StartPoint*=S;InOut.EndPoint*=S;InOut.CamLocation*=S;InOut.BoundingBox*=S;InOut.Size*=S;
        for(auto& Point:InOut.ApproachPoints)Point*=S;
        for(auto& Point:InOut.RunwayPoints)Point*=S;
        for(auto& Slot:InOut.Slots)Slot.Location*=S;
    }
    return true;
}
