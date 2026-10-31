#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignSceneDlg.h"
#include "Campaign.h"
#include "Mission.h"
#include "MissionEvent.h"
#include "Engine/DataTable.h"
#include "Components/TextBlock.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignScenePresentationTest,"Starshatter.Campaign.ScenePresentation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignScenePresentationTest::RunTest(const FString& Parameters)
{
    auto* Scene=NewObject<UCampaignSceneDlg>();
    FS_MissionEvent First;First.EventId=0;First.EventType=MISSIONEVENT_TYPE::DISPLAY;First.EventTime=7;
    First.EventMessage=TEXT("Force:Space Fourth Fleet");
    FS_MissionEvent Second=First;Second.EventMessage=TEXT("7th Carrier Group 'Titan'");
    FS_MissionEvent End;End.EventId=0;End.EventType=MISSIONEVENT_TYPE::END_SCENE;End.EventTime=85;
    Scene->ActiveMissionData.Event={First,Second,End};Scene->BuildSortedEventQueue();
    TestEqual(TEXT("Authored 85-second scene is not truncated to 30 seconds"),Scene->ResolveSceneDurationSeconds(),85.f);
    TestEqual(TEXT("Equal-time title/subtitle retains authored order"),Scene->SortedEvents[0].EventMessage,First.EventMessage);
    TestFalse(TEXT("Anonymous events receive distinct presentation identities"),Scene->SortedEvents[0].EventId==Scene->SortedEvents[1].EventId);
    TestTrue(TEXT("Implicit timed event can fire at authored time"),Scene->IsSceneEventReady(Scene->SortedEvents[0],7.f));
    TestFalse(TEXT("Timed event does not fire early"),Scene->IsSceneEventReady(Scene->SortedEvents[0],6.f));
    Scene->MessageTitleText=NewObject<UTextBlock>(Scene);Scene->MessageSubtitleText=NewObject<UTextBlock>(Scene);
    Scene->ExecuteDisplayEvent(First);
    TestEqual(TEXT("Shot title preserved"),Scene->MessageTitleText->GetText().ToString(),First.EventMessage);
    TestEqual(TEXT("Shot subtitle preserved"),Scene->MessageSubtitleText->GetText().ToString(),Second.EventMessage);
    End.TriggerNParams=1;End.TriggerParam[0]=4;End.EventDelay=35;
    Scene->ActiveMissionData.Event={End};Scene->BuildSortedEventQueue();
    TestTrue(TEXT("Dock/event-driven scene cannot expire on a fixed timer"),Scene->ResolveSceneDurationSeconds()>1000000.f);
    TestFalse(TEXT("Triggered shot cannot fire just because time elapsed"),Scene->IsSceneEventReady(Scene->SortedEvents[0],100000.f));

    FS_CampaignMission Model;Model.Scene=TEXT("Cinematic conversion fixture");
    FS_MissionEvent Hold;Hold.EventId=4;Hold.EventType=MISSIONEVENT_TYPE::HOLD;Hold.EventTime=20;
    Hold.EventNParams=1;Hold.EventParam[0]=99;Hold.TriggerNParams=1;Hold.TriggerParam[0]=3;
    Hold.TriggerShip=TEXT("Proteus");Hold.EventDelay=6;Model.Event={Hold};
    Mission Converted;Converted.LoadFromCampaignMissionData(Model);
    if(TestEqual(TEXT("Cinematic runtime event retained"),Converted.GetEvents().size(),1))
    {
        auto* Event=Converted.GetEvents()[0];
        TestEqual(TEXT("Authored hold parameter survives conversion"),Event->EventParam(0),99);
        TestEqual(TEXT("Authored trigger dependency survives conversion"),Event->TriggerParam(0),3);
        TestEqual(TEXT("Authored delay survives conversion"),Event->Delay(),6.);
    }

    auto* Campaigns=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));
    auto* Designs=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Campaign table"),Campaigns)||!TestNotNull(TEXT("Ship design table"),Designs)) return false;
    TArray<FString> Lines;Lines.Add(TEXT("Campaign,Scene,Events,Duration,Panels,Voices,MissingModels"));
    TSet<FString> CheckedPanels,CheckedVoices;int32 Count=0,MissingModels=0,MissingVoices=0;
    for(const auto& Entry:Campaigns->GetRowMap())
    {
        const auto& Row=*reinterpret_cast<const FS_Campaign*>(Entry.Value);
        for(const auto& Mission:Row.Missions)
        {
            if(Mission.Scene.IsEmpty() || Mission.MissionType==EMissionType::TRAINING) continue;
            ++Count;Scene->SetCampaignNumber(Row.Index+1);Scene->ActiveMissionData=Mission;Scene->BuildSortedEventQueue();
            int32 Panels=0,Voices=0,ModelsMissing=0;
            for(const auto& Event:Mission.Event)
            {
                if(!Event.EventImage.IsEmpty())
                {
                    ++Panels;
                    FString Key=FString::FromInt(Row.Index)+TEXT(":")+Event.EventImage;
                    if(!CheckedPanels.Contains(Key))
                    {
                        CheckedPanels.Add(Key);
                        TestFalse(TEXT("Panel definitions use bare names"),Event.EventImage.Contains(TEXT("/"))||Event.EventImage.EndsWith(TEXT(".pcx")));
                        TestFalse(TEXT("Campaign-specific panel resolves"),Scene->ResolveScenePanelPath(Event.EventImage).IsEmpty());
                    }
                }
                if(!Event.EventSound.IsEmpty())
                {
                    ++Voices;
                    FString Key=FString::FromInt(Row.Index)+TEXT(":")+Event.EventSound;
                    if(!CheckedVoices.Contains(Key))
                    {
                        CheckedVoices.Add(Key);
                        if(Scene->ResolveSceneSoundPath(Event.EventSound).IsEmpty())
                        {++MissingVoices;AddWarning(TEXT("Missing narration asset: ")+Key);}
                    }
                }
            }
            TSet<FString> Names;
            for(const auto& Element:Mission.Element)
            {
                if(Element.Design.IsEmpty()||Names.Contains(Element.Design))continue;
                Names.Add(Element.Design);
                const FShipDesign* Design=Designs->FindRow<FShipDesign>(FName(*Element.Design),TEXT("Scene presentation audit"),false);
                FString ModelName=Design&&!Design->Model.IsEmpty()?Design->Model:Element.Design;
                if(!FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/Models/%s/BP_%s"),*ModelName,*ModelName)))
                {++ModelsMissing;AddWarning(TEXT("Missing cinematic model blueprint: ")+ModelName+TEXT(" in ")+Mission.Scene);}
            }
            MissingModels+=ModelsMissing;
            Lines.Add(FString::Printf(TEXT("%d,%s,%d,%.2f,%d,%d,%d"),Row.Index+1,*Mission.Scene,Mission.Event.Num(),Scene->ResolveSceneDurationSeconds(),Panels,Voices,ModelsMissing));
        }
    }
    TestEqual(TEXT("All authored cutscenes are present"),Count,71);
    FString Report;if(FParse::Value(FCommandLine::Get(),TEXT("SceneInventoryReport="),Report))
        TestTrue(TEXT("Scene inventory written"),FFileHelper::SaveStringArrayToFile(Lines,*Report));
    AddInfo(FString::Printf(TEXT("Scenes=%d; missing model references=%d; missing narration references=%d. This is asset validation, not full playback."),Count,MissingModels,MissingVoices));
    return true;
}
#endif
