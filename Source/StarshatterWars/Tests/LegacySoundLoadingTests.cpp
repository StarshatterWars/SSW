#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "DataLoader.h"
#include "Sound.h"
#include "UObject/GarbageCollection.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegacySoundLoadingTest,"Starshatter.Audit.F07.LegacySoundLoading",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLegacySoundLoadingTest::RunTest(const FString& Parameters)
{
    DataLoader Loader;Loader.SetDataPath("Sounds/");USound* Sound=nullptr;
    TestTrue(TEXT("Legacy WAV name resolves actual imported UE sound"),Loader.LoadSound("Tires.wav",Sound,USound::LOCALIZED|USound::LOC_3D)!=0);
    if(!TestNotNull(TEXT("Sound wrapper created"),Sound))return false;
    ON_SCOPE_EXIT {if(IsValid(Sound))Sound->Release();};
    TestTrue(TEXT("Loaded sound is ready and owns a GC root"),Sound->IsReady()&&Sound->IsRooted()&&Sound->SoundAsset!=nullptr);
    TestEqual(TEXT("Spatial flags preserved"),Sound->GetFlags(),DWORD(USound::LOCALIZED|USound::LOC_3D));
    TestEqual(TEXT("Resolver uses Audio/Sounds content"),Sound->AssetReference,FString(TEXT("/Game/Audio/Sounds/Tires.Tires")));
    USound* Copy=Sound->Duplicate();TestTrue(TEXT("Duplicate owns independent legacy lifetime"),Copy && Copy!=Sound && Copy->IsRooted());
    if(Copy)
    {
        TestTrue(TEXT("Duplicate shares asset, not playback component"),Copy->SoundAsset==Sound->SoundAsset && Copy->AudioComp==nullptr);
        TWeakObjectPtr<USound> Weak=Copy;CollectGarbage(RF_NoFlags);
        TestTrue(TEXT("Raw legacy pointer survives collection until released"),Weak.IsValid());
        Copy->bReleaseWhenFinished=true;Copy->HandleFinished();
        TestFalse(TEXT("One-shot completion releases GC root"),Copy->IsRooted());
        TestTrue(TEXT("One-shot completion clears asset and state"),Copy->IsDone()&&Copy->SoundAsset==nullptr);
    }
    Sound->HandleFinished();TestTrue(TEXT("Reusable prototype remains rooted after completion"),Sound->IsRooted());
    USound* Explicit=nullptr;
    TestTrue(TEXT("Explicit object paths supported"),Loader.LoadSound("/Game/Audio/Sounds/Tires.Tires",Explicit,0,true)!=0);
    if(Explicit)Explicit->Release();
    USound* Gear=nullptr;TestTrue(TEXT("Second actual sound resolves independently"),Loader.LoadSound("GearTransit.wav",Gear,0,true)!=0);
    if(Gear)Gear->Release();
    USound* Missing=Sound;TestEqual(TEXT("Missing optional sound returns failure"),Loader.LoadSound("Audit_Missing_12345.wav",Missing,0,true),0);
    TestNull(TEXT("Failure clears output pointer"),Missing);
    Loader.EnableMedia(false);TestEqual(TEXT("Legacy media disable honored"),Loader.LoadSound("Tires.wav",Missing,0,true),0);
    const FString LongRef=TEXT("/Game/Audio/SomeVeryLongFolderName/AnotherLongFolderName/AnotherFolder/Tires.Tires");
    Sound->SetFilename(TCHAR_TO_UTF8(*LongRef));TestEqual(TEXT("Asset reference is not truncated to legacy display buffer"),Sound->AssetReference,LongRef);
    Sound->Release();TestFalse(TEXT("Explicit Release relinquishes root"),Sound->IsRooted());
    return true;
}
#endif
