#include "FractalVerseShipProvisioning.h"
#include "GameStructs.h"
#include "JsonObjectConverter.h"

bool FFractalVerseShipProvisioning::Validate(const FFractalVersePlayerShip& Ship)
{
    return Ship.Hull.HullId.IsValid() && !Ship.Hull.Design.IsNone() && !Ship.System.IsNone() && !Ship.Region.IsNone() &&
        !Ship.Position.ContainsNaN() && !Ship.Velocity.ContainsNaN() && !Ship.Heading.ContainsNaN() && !Ship.Up.ContainsNaN() &&
        Ship.Heading.IsNormalized() && Ship.Up.IsNormalized() && FMath::Abs(FVector::DotProduct(Ship.Heading,Ship.Up))<.01 &&
        FMath::IsFinite(Ship.Hull.HullCondition) && Ship.Hull.HullCondition>=0 && Ship.Hull.HullCondition<=1;
}
bool FFractalVerseShipProvisioning::Plan(const FFractalVersePilot& Pilot,int32 SpawnSlot,FFractalVersePlayerShip& Out,FString& Reason)
{
    FS_PlayerGameInfo Career;
    if(Pilot.CareerJson.IsEmpty() || !FJsonObjectConverter::JsonObjectStringToUStruct(Pilot.CareerJson,&Career))
    {Reason=TEXT("Server career has no readable hangar.");return false;}
    const FGuid Selected=Career.AssignedHangarHull.IsValid()?Career.AssignedHangarHull:Career.SelectedHangarHull;
    const auto* Hull=Career.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Selected;});
    if(!Hull || Hull->bLost || Hull->bSold || !Hull->HullId.IsValid() || Hull->Design.IsNone())
    {Reason=TEXT("No available assigned or selected hull in this server career.");return false;}
    FFractalVersePlayerShip Candidate; Candidate.Hull=*Hull; Candidate.System=Pilot.HomeSystem; Candidate.Region=Pilot.HomeRegion;
    Candidate.IFF=Career.Empire+1;
    Candidate.Position=FVector(3000.*FMath::Clamp(SpawnSlot,0,7),250.*FMath::Clamp(SpawnSlot,0,7),30000.);
    Candidate.Heading=FVector(0,0,1); Candidate.Up=FVector(0,1,0);
    if(!Validate(Candidate)) {Reason=TEXT("Server hull or starting location is invalid.");return false;}
    Out=MoveTemp(Candidate);Reason.Empty();return true;
}
