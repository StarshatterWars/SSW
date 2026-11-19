#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "RecordedCutsceneDlg.h"
#include "MissionsDlg.h"
#include "ShipBriefingPreview.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCutsceneWidgetLifetimeTest,"Starshatter.Intel.RecordedReplayGarbageCollection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCutsceneWidgetLifetimeTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options));
    if(!TestNotNull(TEXT("Lifetime fixture world"),World.Get())) return false;
    World->SetGameInstance(NewObject<UGameInstance>());
    {
        TStrongObjectPtr<URecordedCutsceneDlg> Panel(NewObject<URecordedCutsceneDlg>(World.Get()));
        Panel->UUserWidget::Initialize();
        TSharedPtr<SWidget> Slate=Panel->TakeWidget();
        TArray<UObject*> Owned;
        GetObjectsWithOuter(Panel->WidgetTree,Owned,false);
        TArray<TWeakObjectPtr<UImage>> Images;
        for(UObject* Object:Owned) if(auto* Image=Cast<UImage>(Object)) Images.Add(Image);
        TestEqual(TEXT("Movie picture and all four transport icons exist"),Images.Num(),5);
        CollectGarbage(RF_NoFlags);
        for(const auto& Image:Images) TestTrue(TEXT("Every rendered image survives collection while the panel is visible"),Image.IsValid());
        Slate->SlatePrepass();
        Slate.Reset();static_cast<UWidget*>(Panel.Get())->ReleaseSlateResources(true);
        Slate=Panel->TakeWidget();
        CollectGarbage(RF_NoFlags);
        Slate->SlatePrepass();
        Slate.Reset();static_cast<UWidget*>(Panel.Get())->ReleaseSlateResources(true);
    }
    {
        TStrongObjectPtr<UMissionsDlg> Parent(NewObject<UMissionsDlg>(World.Get()));
        TStrongObjectPtr<UShipBriefingPreview> Preview(NewObject<UShipBriefingPreview>(World.Get()));
        Parent->UUserWidget::Initialize();Preview->UUserWidget::Initialize();
        TWeakPtr<SWidget> PreviewSlate=Preview->TakeWidget();
        Parent->SetSupplement(Preview->TakeWidget());
        { auto Slate=Parent->TakeWidget(); Slate->SlatePrepass(); }
        static_cast<UWidget*>(Parent.Get())->ReleaseSlateResources(true);
        TestFalse(TEXT("Released mission panel does not retain a nested SObjectWidget until GC"),PreviewSlate.IsValid());
        Preview->ReleaseSlateResources(true);
    }
    CollectGarbage(RF_NoFlags);
    World->DestroyWorld(false);
    return true;
}
#endif
