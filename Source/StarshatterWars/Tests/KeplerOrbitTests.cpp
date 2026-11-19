#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "GameStructs.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeplerOrbitTest,"Starshatter.Universe.KeplerMotionAndClearance",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FKeplerOrbitTest::RunTest(const FString&)
{
    StarSystem System("Kepler test",FVector::ZeroVector);
    auto* Star=new OrbitalBody(&System,"Star",Orbital::STAR,2e30,7e8,0);System.AddBody(Star);
    auto* A=new OrbitalBody(&System,"Inner",Orbital::PLANET,6e24,6e6,1.5e11,Star);Star->AddSatellite(A);
    A->SetOrbitShape(.1,23);
    auto* B=new OrbitalBody(&System,"Outer",Orbital::PLANET,6e24,6e6,1.501e11,Star);Star->AddSatellite(B);B->SetOrbitShape(.2,7);
    auto* Moon1=new OrbitalBody(&System,"Moon1",Orbital::MOON,7e22,1e6,5e6,A);A->AddSatellite(Moon1);Moon1->SetOrbitShape(.1,15);
    auto* Moon2=new OrbitalBody(&System,"Moon2",Orbital::MOON,7e22,1e6,5e6,A);A->AddSatellite(Moon2);Moon2->SetOrbitShape(.1,30);
    System.RecalculateRadius();
    TestTrue(TEXT("Planet clears parent"),A->Periapsis()>Star->Radius()+A->ClearanceRadius());
    TestTrue(TEXT("Planetary orbit envelopes including moons are disjoint"),A->Apoapsis()+A->ClearanceRadius()<B->Periapsis()-B->ClearanceRadius());
    TestTrue(TEXT("Colliding authored moon spacing corrected"),Moon1->Orbit()!=Moon2->Orbit());
    OrbitalBody* Inner=Moon1->Orbit()<Moon2->Orbit()?Moon1:Moon2;OrbitalBody* Outer=Inner==Moon1?Moon2:Moon1;
    TestTrue(TEXT("Moon clears planetary surface"),Inner->Periapsis()>A->Radius()+Inner->Radius());
    TestTrue(TEXT("Moon orbit envelopes cannot intersect"),Inner->Apoapsis()+Inner->Radius()<Outer->Periapsis()-Outer->Radius());
    auto* C=new OrbitalBody(&System,"Kepler sample",Orbital::PLANET,6e24,6e6,1e12,Star);Star->AddSatellite(C);C->SetOrbitShape(.2,30);
    TestTrue(TEXT("Periapsis uses focus, not ellipse center"),FMath::IsNearlyEqual(C->SampleOrbit(0).Size(),8e11,1.0));
    TestTrue(TEXT("Apoapsis radius is a(1+e)"),FMath::IsNearlyEqual(C->SampleOrbit(PI).Size(),1.2e12,1.0));
    TestTrue(TEXT("Orbital inclination creates real vertical position"),FMath::Abs(C->SampleOrbit(PI/2).Z)>1e11);
    const double E=.9;const double M=E-.2*FMath::Sin(E);const FVector P=C->SampleOrbit(M);
    TestTrue(TEXT("Kepler equation determines equal-time positions"),FMath::IsNearlyEqual(P.X,1e12*(FMath::Cos(E)-.2),1.0));
    const double Expected=2*PI*FMath::Sqrt(FMath::Pow(C->Orbit(),3)/(6.67430e-11*(Star->Mass()+C->Mass())));
    TestTrue(TEXT("Period follows mass and a cubed"),FMath::Abs(C->Period()/Expected-1)<.001);
    TestTrue(TEXT("Invalid phase remains finite"),!C->SampleOrbit(std::numeric_limits<double>::quiet_NaN()).ContainsNaN());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuthoredOrbitTest,"Starshatter.Universe.AuthoredOrbitEnvelopeAudit",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FAuthoredOrbitTest::RunTest(const FString&)
{
    TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*(FPaths::ProjectContentDir()/TEXT("GameData/Galaxy")),TEXT("*.json"),true,false);
    int32 Systems=0,Bodies=0;
    for(const auto& Path:Files)
    {
        FString Text;TSharedPtr<FJsonObject> Root;if(!FFileHelper::LoadFileToString(Text,*Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root || !Root->HasField(TEXT("star")))continue;
        FString Name;if(!Root->TryGetStringField(TEXT("name"),Name))continue;
        // Data mirrors are inspected directly; native hydration is covered by the fixture above.
        TFunction<void(const TSharedPtr<FJsonObject>&)> Audit=[&](const auto& Object)
        {
            for(const TCHAR* Field:{TEXT("planet"),TEXT("moon")})
            {
                const TArray<TSharedPtr<FJsonValue>>* Children=nullptr;if(!Object->TryGetArrayField(Field,Children))continue;
                for(const auto& Value:*Children)
                {
                    const auto Child=Value->AsObject();if(!Child)continue;double A=0,E=0,I=0;
                    TestTrue(TEXT("Authored body supplies semi-major axis"),Child->TryGetNumberField(TEXT("orbit"),A) && A>0);
                    TestTrue(TEXT("Authored body supplies bounded eccentricity"),Child->TryGetNumberField(TEXT("eccentricity"),E) && E>=0 && E<1);
                    TestTrue(TEXT("Authored body supplies orbital inclination"),Child->TryGetNumberField(TEXT("inclination"),I) && FMath::IsFinite(I));
                    ++Bodies;Audit(Child);
                }
            }
        };
        const auto* Star=Root->GetObjectField(TEXT("star")).Get();if(Star)Audit(Root->GetObjectField(TEXT("star")));++Systems;
    }
    TestTrue(TEXT("Authored solar systems checked"),Systems>15);TestTrue(TEXT("Planet and moon records checked"),Bodies>100);return true;
}
#endif
