#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeSimulationWorker.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
namespace
{
FNativeCollisionInput CollisionFixture(int32 Count,int32 Queries=0)
{
    FNativeCollisionInput In;In.Epoch=17;In.Sequence=1;In.bShipPairs=true;FRandomStream R(9137);
    for(int32 I=0;I<Count;++I)In.Hulls.Add({FVector(R.FRandRange(-10000.f,10000.f),R.FRandRange(-10000.f,10000.f),R.FRandRange(-10000.f,10000.f)),R.FRandRange(1.f,300.f)});
    for(int32 I=0;I<Queries;++I)In.Queries.Add({uint32(I+1),FVector(-12000,I*10,0),FVector(12000,I*10,0)});
    return In;
}
double Median(TArray<double> V){V.Sort();return V[V.Num()/2];}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipPairsThreadTest,"Starshatter.Domains.ShipPairsParityAndStaleGeometry",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FShipPairsThreadTest::RunTest(const FString&)
{
    auto In=CollisionFixture(256);In.Hulls[0]={FVector::ZeroVector,1};In.Hulls[1]={FVector(2,0,0),1};In.Hulls[2]={FVector(2.001,0,0),1};
    const auto Hulls=In.Hulls;FNativeSimulationWorker Worker;
    if(!TestTrue(TEXT("Ship-only frame dispatches without a projectile"),Worker.Submit(MoveTemp(In))))return false;
    auto Out=Worker.Finish();if(!TestNotNull(TEXT("Owned result exists"),Out.Get()))return false;
    TestTrue(TEXT("Ship sphere arithmetic runs off game thread"),Out->bOffGameThread);
    TestEqual(TEXT("Bounded mask size"),Out->ShipPairs.Num(),256*256);
    for(int32 A=0;A<Hulls.Num();++A)for(int32 B=A+1;B<Hulls.Num();++B)
    {
        const bool Expected=!(FVector::Dist(Hulls[A].Position,Hulls[B].Position)>Hulls[A].Radius+Hulls[B].Radius);
        if(!TestEqual(TEXT("Matches Ship::CollidesWith sphere rejection including tangent"),FNativeSimulationWorker::MayCollide(*Out,Hulls,A,B,Hulls[A],Hulls[B]),Expected))return false;
    }
    TestTrue(TEXT("Once-per-pass validation accepts unchanged layout"),FNativeSimulationWorker::MatchesGeometry(*Out,Hulls,Hulls));
    auto Live=Hulls;Live[2].Position=FVector::ZeroVector;
    TestFalse(TEXT("Any moved hull disables prepared mask"),FNativeSimulationWorker::MatchesGeometry(*Out,Hulls,Live));
    Live=Hulls;Live[2].Radius=100;
    TestFalse(TEXT("Any resized hull disables prepared mask"),FNativeSimulationWorker::MatchesGeometry(*Out,Hulls,Live));
    TestTrue(TEXT("Prepared tangent is retained"),FNativeSimulationWorker::PreparedPair(*Out,Hulls.Num(),0,1));
    TestFalse(TEXT("Prepared noncontact is rejected"),FNativeSimulationWorker::PreparedPair(*Out,Hulls.Num(),0,2));
    auto Moved=Hulls[2];Moved.Position=FVector::ZeroVector;
    TestTrue(TEXT("Movement after dispatch restores narrow phase"),FNativeSimulationWorker::MayCollide(*Out,Hulls,0,2,Hulls[0],Moved));
    auto Resized=Hulls[2];Resized.Radius=100;
    TestTrue(TEXT("Radius changes restore narrow phase"),FNativeSimulationWorker::MayCollide(*Out,Hulls,0,2,Hulls[0],Resized));
    TestTrue(TEXT("Unknown layout falls back"),FNativeSimulationWorker::MayCollide(*Out,Hulls,0,999,Hulls[0],Hulls[2]));
    TestTrue(TEXT("Reversed indices preserve tangent"),FNativeSimulationWorker::MayCollide(*Out,Hulls,1,0,Hulls[1],Hulls[0]));
    TestFalse(TEXT("Oversized square matrix is not allocated"),FNativeSimulationWorker::WorthShipPairs(2049));
    TestFalse(TEXT("Small region avoids overhead"),FNativeSimulationWorker::WorthShipPairs(255));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimulationTimingTest,"Starshatter.Domains.SimulationThreadTiming",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSimulationTimingTest::RunTest(const FString&)
{
    // Deterministic CPU microbenchmark, not rendered FPS. Interleave serial/worker
    // trials; report handoff + immediate barrier (no invented overlap or FPS gain).
    TArray<TSharedPtr<FJsonValue>> Rows;FNativeSimulationWorker Worker;
    for(int32 N:{256,512,1024})
    {
        const auto Base=CollisionFixture(N,32);TArray<double> Serial,Threaded,Copy,Submit,Wait,Compute,Consume;
        int64 SerialHits=0,ThreadHits=0;
        for(int32 Trial=-3;Trial<21;++Trial)
        {
            double Begin=FPlatformTime::Seconds();int64 Hits=0;
            for(int32 A=0;A<N;++A)for(int32 B=A+1;B<N;++B)
                Hits+=!(FVector::Dist(Base.Hulls[A].Position,Base.Hulls[B].Position)>Base.Hulls[A].Radius+Base.Hulls[B].Radius);
            for(const auto& Q:Base.Queries){TArray<uint8> Mask;SSWBoltCollision::FindCandidates(Base.Hulls,Q.Start,Q.End,Mask,false);for(uint8 V:Mask)Hits+=V;}
            const double SerialMs=(FPlatformTime::Seconds()-Begin)*1000;SerialHits=Hits;
            Begin=FPlatformTime::Seconds();auto Input=Base;const double Copied=FPlatformTime::Seconds();
            if(!Worker.Submit(MoveTemp(Input))){AddError(TEXT("Native simulation worker refused timing batch"));return false;}
            const double Submitted=FPlatformTime::Seconds();auto Out=Worker.Finish();const double Finished=FPlatformTime::Seconds();
            if(!Out){AddError(TEXT("Missing timing result"));return false;}
            if(!FNativeSimulationWorker::MatchesGeometry(*Out,Base.Hulls,Base.Hulls)){AddError(TEXT("Timing layout mismatch"));return false;}
            Hits=0;for(int32 A=0;A<N;++A)for(int32 B=A+1;B<N;++B)Hits+=FNativeSimulationWorker::PreparedPair(*Out,N,A,B);
            for(const auto& Mask:Out->Candidates)for(uint8 V:Mask)Hits+=V;
            const double End=FPlatformTime::Seconds();ThreadHits=Hits;
            if(Trial>=0){Serial.Add(SerialMs);Threaded.Add((End-Begin)*1000);Copy.Add((Copied-Begin)*1000);Submit.Add((Submitted-Copied)*1000);Wait.Add((Finished-Submitted)*1000);Compute.Add(Out->CalculationSeconds*1000);Consume.Add((End-Finished)*1000);}
        }
        TestEqual(TEXT("Measured paths preserve every candidate"),ThreadHits,SerialHits);
        auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("hulls"),N);Row->SetNumberField(TEXT("projectiles"),32);Row->SetNumberField(TEXT("samples"),21);
        Row->SetNumberField(TEXT("serialMedianMs"),Median(Serial));Row->SetNumberField(TEXT("threadedImmediateBarrierMedianMs"),Median(Threaded));Row->SetNumberField(TEXT("snapshotMedianMs"),Median(Copy));Row->SetNumberField(TEXT("submitMedianMs"),Median(Submit));Row->SetNumberField(TEXT("barrierMedianMs"),Median(Wait));Row->SetNumberField(TEXT("workerCalculationMedianMs"),Median(Compute));Row->SetNumberField(TEXT("consumeMedianMs"),Median(Consume));
        Rows.Add(MakeShared<FJsonValueObject>(Row));AddInfo(FString::Printf(TEXT("CPU microbenchmark %d hulls/32 shots: serial %.3f ms; worker + immediate barrier + consume %.3f ms; worker calculation %.3f ms. Not FPS."),N,Median(Serial),Median(Threaded),Median(Compute)));
    }
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("scope"),TEXT("Deterministic collision CPU microbenchmark; includes snapshot/submit/immediate barrier/consumption, excludes actors/rendering and real overlap. Not a live FPS comparison."));Report->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());Report->SetArrayField(TEXT("workloads"),Rows);
    FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
    TestTrue(TEXT("Timing evidence saved"),FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("Analysis/SimulationThreadTiming.json"))));return true;
}
#endif
