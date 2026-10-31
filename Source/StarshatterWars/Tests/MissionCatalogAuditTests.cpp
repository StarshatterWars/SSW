#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionCatalogAuditTest,"Starshatter.Mission.CatalogInventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionCatalogAuditTest::RunTest(const FString& Parameters)
{
    TArray<FString> Lines;Lines.Add(TEXT("Source,Id,Name,System,Region,Scene,MapAvailable,Elements,PlayerElements,Waypoints,Objectives,Events"));
    int32 Missions=0,MissingMaps=0;
    auto Quote=[](FString Text){Text.ReplaceInline(TEXT("\""),TEXT("\"\""));return TEXT("\"")+Text+TEXT("\"");};
    auto Record=[&](const FString& Source,const FS_CampaignMission& Mission,const FString& ParentSystem,const FString& ParentRegion)
    {
        const FString System=Mission.MissionSystem.TrimStartAndEnd().IsEmpty()?ParentSystem:Mission.MissionSystem.TrimStartAndEnd();
        const FString Region=Mission.MissionRegion.TrimStartAndEnd().IsEmpty()?ParentRegion:Mission.MissionRegion.TrimStartAndEnd();
        const bool MapAvailable=!System.IsEmpty() && FPackageName::DoesPackageExist(TEXT("/Game/Maps/")+System);
        int32 Players=0,Waypoints=0,Objectives=0;
        for(const auto& Element:Mission.Element){if(Element.Player>0)++Players;Waypoints+=Element.Navpoint.Num();Objectives+=Element.Instruction.Num();}
        if(Players>0 && Mission.Scene.IsEmpty() && !MapAvailable)++MissingMaps;
        Lines.Add(Quote(Source)+TEXT(",")+FString::FromInt(Mission.Id)+TEXT(",")+Quote(Mission.MissionName)+TEXT(",")+Quote(System)+TEXT(",")+Quote(Region)+TEXT(",")+Quote(Mission.Scene)+TEXT(",")+(MapAvailable?TEXT("true"):TEXT("false"))+FString::Printf(TEXT(",%d,%d,%d,%d,%d"),Mission.Element.Num(),Players,Waypoints,Objectives,Mission.Event.Num()));
        ++Missions;
    };
    auto* Campaigns=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));
    if(TestNotNull(TEXT("Campaign mission data table loads"),Campaigns) && Campaigns->GetRowStruct()==FS_Campaign::StaticStruct())
        for(const auto& Entry:Campaigns->GetRowMap())
        {
            const auto& Campaign=*reinterpret_cast<const FS_Campaign*>(Entry.Value);
            for(const auto& Mission:Campaign.Missions) Record(Campaign.Name,Mission,Campaign.System,Campaign.Region);
        }
    const TCHAR* Tables[]={TEXT("/Game/Game/DT_SingleMissions.DT_SingleMissions"),TEXT("/Game/Game/DT_MultiplayerMissions.DT_MultiplayerMissions"),TEXT("/Game/Game/DT_CustomMissions.DT_CustomMissions")};
    for(const TCHAR* Path:Tables)
        if(auto* Table=LoadObject<UDataTable>(nullptr,Path))
            if(Table->GetRowStruct()==FS_CampaignMission::StaticStruct())
                for(const auto& Entry:Table->GetRowMap())Record(Table->GetName(),*reinterpret_cast<const FS_CampaignMission*>(Entry.Value),TEXT(""),TEXT(""));
    TestTrue(TEXT("Mission inventory is populated"),Missions>0);
    if(MissingMaps)AddWarning(FString::Printf(TEXT("%d mission rows with a player element and no explicit scene lack an available system map; review inventory."),MissingMaps));
    FString Report;if(FParse::Value(FCommandLine::Get(),TEXT("MissionInventoryReport="),Report))TestTrue(TEXT("Mission inventory exported"),FFileHelper::SaveStringArrayToFile(Lines,*Report,FFileHelper::EEncodingOptions::ForceUTF8));
    AddInfo(FString::Printf(TEXT("Mission rows inventoried=%d; map-review rows=%d. This is data inspection, not a gameplay pass."),Missions,MissingMaps));
    return true;
}
#endif
