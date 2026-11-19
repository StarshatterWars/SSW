#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MissionListJson.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
namespace
{
template<class Row> FString Document(const TCHAR* Kind,const TArray<Row>& Rows)
{
    auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("schemaVersion"),1);Root->SetStringField(TEXT("kind"),Kind);
    TArray<TSharedPtr<FJsonValue>> Values;
    for(const auto& RowValue:Rows)
    {auto Object=MakeShared<FJsonObject>();FJsonObjectConverter::UStructToJsonObject(Row::StaticStruct(),&RowValue,Object,0,0);Values.Add(MakeShared<FJsonValueObject>(Object));}
    Root->SetArrayField(TEXT("rows"),Values);FString Text;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text));return Text;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionListJsonRoundtrip,"Starshatter.Data.MissionLists.FullFieldRoundtrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionListJsonRoundtrip::RunTest(const FString&)
{
    FS_CampaignMissionList Row;Row.Id=105;Row.Name=TEXT("Training fifth");Row.Description=TEXT("Description");Row.Sitrep=TEXT("line one\\nline two");
    Row.System=TEXT("Janus");Row.Region=TEXT("Janek");Row.Objective=TEXT("Launch and return");Row.Start=TEXT("02/03:04:05");Row.Script=TEXT("T5.def");
    Row.TypeName=TEXT("Training");Row.Type=static_cast<int32>(EMissionType::TRAINING);Row.MissionImage=TEXT("picture");Row.MissionAudio=TEXT("voice");
    Row.Available=false;Row.Complete=true;Row.MinRank=3;Row.DisplayType=EMissionDisplayType::PlayerMission;
    TArray<FS_CampaignMissionList> Expected{Row},Actual;FString Error;
    TestTrue(TEXT("Full mission object parses"),MissionListJson::ParseMissions(Document(TEXT("missions"),Expected),Actual,Error));
    if(TestEqual(TEXT("One mission"),Actual.Num(),1))TestTrue(TEXT("Every reflected field survives"),FS_CampaignMissionList::StaticStruct()->CompareScriptStruct(&Expected[0],&Actual[0],0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTemplateListJsonVariants,"Starshatter.Data.MissionLists.TemplateVariantOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTemplateListJsonVariants::RunTest(const FString&)
{
    FS_CampaignTemplateList A;A.Id=0;A.Name=TEXT("First variant");A.Script=TEXT("shared.def");A.MinRank=1;A.MaxRank=5;
    A.StartAfter=86400;A.StartBefore=172800;A.ActionId=42;A.ActionStatus=2;A.ExecOnce=1;A.Region=TEXT("Borova");
    FS_CampaignTemplateList B=A;B.Name=TEXT("Second variant");B.ActionId=43;
    TArray<FS_CampaignTemplateList> Expected{A,B},Actual;FString Error;
    TestTrue(TEXT("Repeated zero IDs and shared scripts are valid"),MissionListJson::ParseTemplates(Document(TEXT("templates"),Expected),Actual,Error));
    if(TestEqual(TEXT("Both variants retained"),Actual.Num(),2))
        for(int32 I=0;I<2;++I)TestTrue(TEXT("Full variant and order preserved"),FS_CampaignTemplateList::StaticStruct()->CompareScriptStruct(&Expected[I],&Actual[I],0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionListJsonReject,"Starshatter.Data.MissionLists.TransactionalRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionListJsonReject::RunTest(const FString&)
{
    FS_CampaignMissionList Row;Row.Id=101;Row.Script=TEXT("T1.def");TArray<FS_CampaignMissionList> Rows{Row};FString Error;
    TestFalse(TEXT("Wrong kind rejected"),MissionListJson::ParseMissions(Document(TEXT("templates"),Rows),Rows,Error));
    TestEqual(TEXT("Previous mission retained"),Rows[0].Id,101);
    TestFalse(TEXT("Partial row rejected"),MissionListJson::ParseMissions(TEXT("{\"schemaVersion\":1,\"kind\":\"missions\",\"rows\":[{\"id\":7}]}"),Rows,Error));
    FS_CampaignMissionList Invalid=Row;Invalid.Script.Reset();
    TArray<FS_CampaignMissionList> Candidate{Row,Invalid};
    TestFalse(TEXT("Later bad row cannot partially commit"),MissionListJson::ParseMissions(Document(TEXT("missions"),Candidate),Rows,Error));
    TestEqual(TEXT("Original array retained"),Rows.Num(),1);TestEqual(TEXT("Original ID retained"),Rows[0].Id,101);
    TestTrue(TEXT("Explicit empty catalog accepted"),MissionListJson::ParseMissions(TEXT("{\"schemaVersion\":1,\"kind\":\"missions\",\"rows\":[]}"),Rows,Error));
    TestTrue(TEXT("Empty catalog clears old rows"),Rows.IsEmpty());return true;
}
#endif
