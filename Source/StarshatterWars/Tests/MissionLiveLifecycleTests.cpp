#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionBriefingDlg.h"
#include "Campaign.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    class FCheckLiveUpdates : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
    public:
        explicit FCheckLiveUpdates(TFunction<bool()> InCheck):Check(MoveTemp(InCheck)){}
        virtual bool Update() override {return Check();}
    };
    struct FHiddenTickFixture
    {
        UWorld* World=nullptr;
        TStrongObjectPtr<UMissionBriefingDlg> Briefing;
        TUniquePtr<Campaign> Story;
        int32 Ticks=0;
        ~FHiddenTickFixture(){if(World)World->DestroyWorld(false);}
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionHiddenLiveTickTest,"Starshatter.Mission.LiveUpdatesWhenBriefingCollapsed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionHiddenLiveTickTest::RunTest(const FString& Parameters)
{
    auto Fixture=MakeShared<FHiddenTickFixture>();
    Fixture->World=UWorld::CreateWorld(EWorldType::Game,false);
    Fixture->World->SetGameInstance(NewObject<UGameInstance>());
    Fixture->Briefing.Reset(NewObject<UMissionBriefingDlg>(Fixture->World));
    Fixture->Story=MakeUnique<Campaign>(1,"Live update fixture",true);Fixture->Story->SetTime(86400);
    auto* Briefing=Fixture->Briefing.Get();
    Briefing->CampaignPtr=Fixture->Story.Get();
    Briefing->MissionTPlusText=NewObject<UTextBlock>(Briefing);
    Briefing->MissionTPlusText->SetText(FText::FromString(TEXT("Awaiting live update")));
    Briefing->bLiveMissionStarted=true;
    Briefing->SetVisibility(ESlateVisibility::Collapsed);
    Briefing->StartLiveSessionUpdates();
    TestTrue(TEXT("Live timer active with collapsed briefing"),Fixture->World->GetTimerManager().IsTimerActive(Briefing->TravelPresentationTimer));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckLiveUpdates([this,Fixture]()
    {
        // TimerManager activates pending timers on its first tick; execute on a later frame.
        Fixture->World->GetTimerManager().Tick(0.1f);
        if(++Fixture->Ticks<2)return false;
        auto* Screen=Fixture->Briefing.Get();
        TestFalse(TEXT("Live session updates without calling widget NativeTick"),Screen->MissionTPlusText->GetText().ToString()==TEXT("Awaiting live update"));
        Screen->ClearTravelPresentation();
        TestFalse(TEXT("Leaving mission removes live timer"),Fixture->World->GetTimerManager().TimerExists(Screen->TravelPresentationTimer));
        Screen->bLiveMissionStarted=false;Screen->CampaignPtr=nullptr;
        return true;
    }));
    return true;
}
#endif
