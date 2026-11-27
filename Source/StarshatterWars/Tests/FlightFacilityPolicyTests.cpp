#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FlightFacilityCatalog.h"
#include "GameStructs_System.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Camera.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFacilityPolicyTest,"Starshatter.Facilities.AuthoredFlightPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFacilityPolicyTest::RunTest(const FString&)
{
    FString Error,Text;TestTrue(TEXT("Facility policy reload"),FlightFacilityCatalog::Reload(Error));
    if(!Error.IsEmpty()){AddError(Error);return false;}
    TArray<TSharedPtr<FJsonValue>> Rows;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Rows))return false;
    int32 Facilities=0,DockingOnly=0,Operational=0;
    TSet<FName> Seen;
    for(const auto& Value:Rows)
    {
        const auto Object=Value->AsObject();const FString Kind=Object->GetStringField(TEXT("ShipClass"));
        if(!Kind.Equals(TEXT("Carrier"),ESearchCase::IgnoreCase) && !Kind.Equals(TEXT("Station"),ESearchCase::IgnoreCase) && !Kind.Equals(TEXT("Starbase"),ESearchCase::IgnoreCase))continue;
        ++Facilities;const FName Name(*Object->GetStringField(TEXT("Row")));Seen.Add(Name);
        TestTrue(*FString::Printf(TEXT("Explicit policy for %s"),*Name.ToString()),FlightFacilityCatalog::Contains(Name));
        if(FlightFacilityCatalog::IsDockingOnly(Name))
        {
            ++DockingOnly;FShipFlightDeck Empty;
            TestFalse(TEXT("Docking-only policy cannot create a flight deck"),FlightFacilityCatalog::ResolveDeck(Name,0,Empty));
            TestTrue(TEXT("Only user-approved facilities are docking-only"),Name==TEXT("Colony") || Name==TEXT("Station7") || Name==TEXT("Mining Station"));
            continue;
        }
        ++Operational;bool Launch=false,Recovery=false;int32 Index=0;
        for(const auto& Raw:Object->GetArrayField(TEXT("FlightDeck")))
        {
            FShipFlightDeck Deck;if(!FJsonObjectConverter::JsonObjectToUStruct(Raw->AsObject().ToSharedRef(),&Deck))return false;
            const auto Original=Deck;TestTrue(TEXT("Operational deck retained"),FlightFacilityCatalog::ResolveDeck(Name,Index++,Deck));
            Launch|=Deck.bLaunchDeck;Recovery|=Deck.bRecoveryDeck;
            TestTrue(TEXT("Exactly one flight role"),Deck.bLaunchDeck!=Deck.bRecoveryDeck);
            TestTrue(TEXT("Finite radians"),FMath::IsFinite(Deck.AzimuthRadians) && FMath::Abs(Deck.AzimuthRadians)<=2*PI+1.e-5);
            TestTrue(TEXT("Positive size/cycle and bounded slots"),Deck.Size>0 && Deck.CycleTime>0 && Deck.Slots.Num()>0 && Deck.Slots.Num()<=10);
            for(const auto& Slot:Deck.Slots)TestTrue(TEXT("Finite compatible launch/recovery slot"),!Slot.Location.ContainsNaN() && Slot.FilterMask>0);
            TestTrue(TEXT("Finite deck and camera coordinates"),!Deck.Location.ContainsNaN() && !Deck.CamLocation.ContainsNaN());
            if(Name==TEXT("Orion"))TestTrue(TEXT("Previously corrected Orion coordinates never rescaled"),Deck.Slots[0].Location.Equals(Original.Slots[0].Location,.001));
            if(Name==TEXT("Asteroid Base"))TestTrue(TEXT("Asteroid base radians never converted twice"),FMath::IsNearlyEqual(Deck.AzimuthRadians,Original.AzimuthRadians,.00001f));
            if(Name==TEXT("Station1") && Index==1)TestTrue(TEXT("Transfer bay definition scale restored once"),Deck.Slots[0].Location.Equals(FVector(1850,120,4600),.001));
            if(Deck.bRecoveryDeck)
            {
                // SSW horizontal plane is X/Z, not UE X/Y.
                const FVector Flat(Deck.EndPoint.X-Deck.StartPoint.X,0,Deck.EndPoint.Z-Deck.StartPoint.Z);
                Camera Rail;Rail.Yaw(Deck.AzimuthRadians);
                TestTrue(*FString::Printf(TEXT("Recovery heading follows approach: %s/%d"),*Name.ToString(),Index-1),FVector::DotProduct(Flat.GetSafeNormal(),Rail.vpn())>.90);
            }
        }
        TestTrue(*FString::Printf(TEXT("Both launch and recovery provided: %s"),*Name.ToString()),Launch && Recovery);
    }
    TestEqual(TEXT("All 23 operational/docking facilities covered"),Facilities,23);
    TestEqual(TEXT("20 facilities retain flight operations"),Operational,20);TestEqual(TEXT("Three deliberately docking-only"),DockingOnly,3);
    for(const FName Name:FlightFacilityCatalog::Names())TestTrue(TEXT("No orphan facility policy"),Seen.Contains(Name));
    FShipFlightDeck Mod;Mod.AzimuthRadians=1.25f;Mod.Location=FVector(1,2,3);
    TestTrue(TEXT("Unknown mod retains native contract"),FlightFacilityCatalog::ResolveDeck(TEXT("UnregisteredModFacility"),0,Mod));
    TestTrue(TEXT("No implicit mod units/scale guesses"),Mod.AzimuthRadians==1.25f && Mod.Location==FVector(1,2,3));
    return true;
}
#endif
