#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BoltCollisionCandidates.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltCandidatesTest,"Starshatter.Weapons.ParallelBoltCandidates",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FBoltCandidatesTest::RunTest(const FString&)
{
    TArray<SSWBoltCollision::FHull> Hulls;
    for(int32 Index=0;Index<1031;++Index)
        Hulls.Add({FVector(Index*17-5000,(Index%19)*11,Index%7),double(Index%31)});
    Hulls[0]={FVector(50,2,0),1};
    Hulls[1]={FVector(50,2.001,0),1};
    for(const FVector End:{FVector(100,0,0),FVector::ZeroVector,FVector(-2000,140,5)})
    {
        TArray<uint8> Serial,Parallel;
        SSWBoltCollision::FindCandidates(Hulls,FVector::ZeroVector,End,Serial,false);
        SSWBoltCollision::FindCandidates(Hulls,FVector::ZeroVector,End,Parallel,true);
        TestTrue(TEXT("Parallel output preserves every candidate and ordering"),Serial==Parallel);
        for(int32 Index=0;Index<Hulls.Num();++Index)
        {
            const auto& Hull=Hulls[Index];
            const bool Expected=!(FVector::DistSquared(FMath::ClosestPointOnSegment(Hull.Position,FVector::ZeroVector,End),Hull.Position)>FMath::Square(Hull.Radius*2.0));
            if(!TestEqual(TEXT("Matches native rejection"),bool(Parallel[Index]),Expected)) break;
        }
    }
    return true;
}
#endif
