#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignJsonCatalog.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignJsonParityTest,"Starshatter.Campaign.JsonActionsAndZonesParity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignJsonParityTest::RunTest(const FString&)
{
    auto* Context=NewObject<UGameInstance>();auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
    Data->CampaignDataTable=NewObject<UDataTable>();Data->CampaignDataTable->RowStruct=FS_Campaign::StaticStruct();
    int32 Actions=0,Zones=0;
    for(int32 I=1;I<=5;++I)
    {
        const FString Directory=FPaths::ProjectContentDir()/FString::Printf(TEXT("GameData/Campaigns/%02d"),I);
        if(!TestTrue(TEXT("Actions JSON exists"),FPaths::FileExists(Directory/TEXT("actions.json"))) ||
           !TestTrue(TEXT("Zones JSON exists"),FPaths::FileExists(Directory/TEXT("zones.json"))))return false;
        Data->CampaignDataTable->EmptyTable();const FTCHARToUTF8 Path(*(Directory/TEXT("campaign.def")));
        Data->LoadCampaignData(Path.Get(),true,false);
        if(!TestEqual(TEXT("Legacy campaign fixture loaded"),Data->CampaignDataTable->GetRowMap().Num(),1))return false;
        const auto& Row=*reinterpret_cast<const FS_Campaign*>(Data->CampaignDataTable->GetRowMap().CreateConstIterator().Value());
        FS_Campaign Imported=Row;FString Error;
        if(!TestTrue(TEXT("Campaign JSON loads"),CampaignJsonCatalog::Apply(Directory,Imported,nullptr,nullptr,Error)))return false;
        TestEqual(TEXT("Action count preserved"),Imported.Action.Num(),Row.Action.Num());
        TestEqual(TEXT("Zone count preserved"),Imported.Zone.Num(),Row.Zone.Num());
        if(Imported.Action.Num()!=Row.Action.Num() || Imported.Zone.Num()!=Row.Zone.Num())return false;
        for(int32 N=0;N<Row.Action.Num();++N)
            TestTrue(TEXT("Every action field, requirement, date and target is preserved"),FS_CampaignAction::StaticStruct()->CompareScriptStruct(&Row.Action[N],&Imported.Action[N],0));
        for(int32 N=0;N<Row.Zone.Num();++N)
            TestTrue(TEXT("Zone ordering, systems and regions preserved"),FS_CampaignZone::StaticStruct()->CompareScriptStruct(&Row.Zone[N],&Imported.Zone[N],0));
        Actions+=Row.Action.Num();Zones+=Row.Zone.Num();
    }
    AddInfo(FString::Printf(TEXT("Campaign JSON parity: five campaigns, %d actions, %d zones"),Actions,Zones));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignJsonCommitTest,"Starshatter.Campaign.JsonAtomicReloadAndOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignJsonCommitTest::RunTest(const FString&)
{
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Automation/CampaignJson")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
    IFileManager::Get().MakeDirectory(*Folder,true);
    FS_Campaign Row;Row.Name=TEXT("Synthetic");FS_CampaignAction Action;Action.Campaign=Row.Name;Action.Id=7;Action.Type=TEXT("MISSION");
    FS_CampaignReq Requirement;Requirement.Action=2;Requirement.NotAction=true;Action.Requirement.Add(Requirement);Row.Action.Add(Action);
    FS_CampaignZone Zone;Zone.System=TEXT("Janus");Zone.Region=TEXT("Janek");Row.Zone.Add(Zone);
    auto* Actions=NewObject<UDataTable>();Actions->RowStruct=FS_CampaignAction::StaticStruct();
    auto* Zones=NewObject<UDataTable>();Zones->RowStruct=FS_CampaignZone::StaticStruct();
    Actions->AddRow(TEXT("Synthetic.Action999"),Action);Actions->AddRow(TEXT("Other.Action001"),Action);
    FString Error;
    if(!TestTrue(TEXT("Fixture export"),CampaignJsonCatalog::Export(Folder,Row,Error)))return false;
    TestTrue(TEXT("JSON commits into existing tables"),CampaignJsonCatalog::Apply(Folder,Row,Actions,Zones,Error));
    TestTrue(TEXT("Stable authored action ID is table key"),Actions->GetRowMap().Contains(TEXT("Synthetic.Action007")));
    TestFalse(TEXT("Reload removes stale campaign action"),Actions->GetRowMap().Contains(TEXT("Synthetic.Action999")));
    TestTrue(TEXT("Other campaigns are untouched"),Actions->GetRowMap().Contains(TEXT("Other.Action001")));
    TestTrue(TEXT("Stable zone key"),Zones->GetRowMap().Contains(TEXT("Synthetic.Zone000")));
    const FS_Campaign Before=Row;
    FFileHelper::SaveStringToFile(TEXT("{\"schemaVersion\":1,\"campaign\":\"Wrong owner\",\"zones\":[]}"),*(Folder/TEXT("zones.json")));
    TestFalse(TEXT("Wrong owner rejects complete import"),CampaignJsonCatalog::Apply(Folder,Row,Actions,Zones,Error));
    TestTrue(TEXT("Failed import retains live campaign"),FS_Campaign::StaticStruct()->CompareScriptStruct(&Before,&Row,0));
    TestEqual(TEXT("Failed import retains action table"),Actions->GetRowMap().Num(),2);
    TestEqual(TEXT("Failed import retains zone table"),Zones->GetRowMap().Num(),1);
    TestTrue(TEXT("Restore valid files"),CampaignJsonCatalog::Export(Folder,Before,Error));
    Row=Before;Row.Action.Add(Action);CampaignJsonCatalog::Export(Folder,Row,Error);Row=Before;
    TestFalse(TEXT("Duplicate IDs cannot silently overwrite rows"),CampaignJsonCatalog::Apply(Folder,Row,Actions,Zones,Error));
    TestTrue(TEXT("Duplicate rejection keeps original campaign"),FS_Campaign::StaticStruct()->CompareScriptStruct(&Before,&Row,0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignHeaderJsonTest, "Starshatter.Campaign.JsonHeaderAndMergedCatalogParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignHeaderJsonTest::RunTest(const FString&)
{
    auto* Data = NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());
    Data->CampaignDataTable = NewObject<UDataTable>();
    Data->CampaignDataTable->RowStruct = FS_Campaign::StaticStruct();
    for (int32 Index = 1; Index <= 5; ++Index)
    {
        const FString Directory = FPaths::ProjectContentDir() / FString::Printf(TEXT("GameData/Campaigns/%02d"), Index);
        const FTCHARToUTF8 Path(*(Directory / TEXT("campaign.def")));
        Data->CampaignDataTable->EmptyTable(); Data->LoadCampaignData(Path.Get(), true, false);
        if (!TestEqual(TEXT("Legacy reference campaign"), Data->CampaignDataTable->GetRowMap().Num(), 1)) return false;
        const FS_Campaign Legacy = *reinterpret_cast<const FS_Campaign*>(Data->CampaignDataTable->GetRowMap().CreateConstIterator().Value());
        Data->CampaignDataTable->EmptyTable(); Data->LoadCampaignData(Path.Get(), true, true);
        if (!TestEqual(TEXT("JSON header and separate catalogs load"), Data->CampaignDataTable->GetRowMap().Num(), 1)) return false;
        const FS_Campaign Native = *reinterpret_cast<const FS_Campaign*>(Data->CampaignDataTable->GetRowMap().CreateConstIterator().Value());
        TestEqual(TEXT("Campaign name"), Native.Name, Legacy.Name);
        TestEqual(TEXT("Description"), Native.Description, Legacy.Description);
        TestEqual(TEXT("Situation"), Native.Situation, Legacy.Situation);
        TestEqual(TEXT("Start date"), Native.Start, Legacy.Start);
        TestTrue(TEXT("Orders"), Native.Orders == Legacy.Orders);
        TestEqual(TEXT("System"), Native.System, Legacy.System);
        TestEqual(TEXT("Region"), Native.Region, Legacy.Region);
        TestEqual(TEXT("Identity"), Native.Index, Legacy.Index);
        TestEqual(TEXT("Scripted"), Native.bScripted, Legacy.bScripted);
        TestEqual(TEXT("Sequential"), Native.bSequential, Legacy.bSequential);
        TestEqual(TEXT("Mission catalog count"), Native.MissionList.Num(), Legacy.MissionList.Num());
        TestEqual(TEXT("Scene count"), Native.Missions.Num(), Legacy.Missions.Num());
        TestEqual(TEXT("Template count"), Native.TemplateMissions.Num(), Legacy.TemplateMissions.Num());
        TestEqual(TEXT("Script count"), Native.ScriptedMissions.Num(), Legacy.ScriptedMissions.Num());
        TestEqual(TEXT("Action count"), Native.Action.Num(), Legacy.Action.Num());
        TestEqual(TEXT("Zone count"), Native.Zone.Num(), Legacy.Zone.Num());
    }
    return true;
}
#endif
