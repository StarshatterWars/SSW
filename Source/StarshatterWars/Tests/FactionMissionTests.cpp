#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "StandaloneMissionCatalog.h"
#include "AgentCatalogSubsystem.h"
#include "AgentMissionRules.h"
#include "FactionBriefingCatalog.h"
#include "FactionTrainingCatalog.h"
#include "TrainingTestRules.h"
#include "Campaign.h"
#include "Misc/PackageName.h"
#include "MissionArcRules.h"
#include "AgentAI.h"
#include "MissionRewardRules.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignHeaderInformationTest,"Starshatter.Campaign.HeaderInformationJsonParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignHeaderInformationTest::RunTest(const FString&)
{
 const FString Path=FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/campaign_information.json");FString Text;TSharedPtr<FJsonObject> Root;
 if(!TestTrue(TEXT("Header information readable"),FFileHelper::LoadFileToString(Text,*Path)) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)return false;
 TArray<TSharedPtr<FJsonValue>> Headers;
 auto* Data=NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());Data->CampaignDataTable=NewObject<UDataTable>();Data->CampaignDataTable->RowStruct=FS_Campaign::StaticStruct();
 for(int32 I=1;I<=5;++I)
 {
  Data->CampaignDataTable->EmptyTable();const FString File=FPaths::ProjectContentDir()/FString::Printf(TEXT("GameData/Campaigns/%02d/campaign.def"),I);const FTCHARToUTF8 Name(*File);Data->LoadCampaignData(Name.Get(),true,false);
  if(!TestEqual(TEXT("Original campaign parses without changing its asset"),Data->CampaignDataTable->GetRowMap().Num(),1))return false;
  const auto& Row=*reinterpret_cast<const FS_Campaign*>(Data->CampaignDataTable->GetRowMap().CreateConstIterator().Value());auto H=MakeShared<FJsonObject>();
  H->SetNumberField(TEXT("CampaignId"),I);H->SetStringField(TEXT("Empire"),TEXT("Terellian"));H->SetStringField(TEXT("Name"),Row.Name);
  H->SetStringField(TEXT("Description"),Row.Description);H->SetStringField(TEXT("Situation"),Row.Situation);H->SetStringField(TEXT("Start"),Row.Start);
  H->SetStringField(TEXT("System"),Row.System);H->SetStringField(TEXT("Region"),Row.Region);H->SetStringField(TEXT("MainImage"),Row.MainImage);
  H->SetBoolField(TEXT("Scripted"),Row.bScripted);H->SetBoolField(TEXT("Sequential"),Row.bSequential);H->SetBoolField(TEXT("Available"),Row.bAvailable);
  TArray<TSharedPtr<FJsonValue>> Orders;for(const auto& Order:Row.Orders)Orders.Add(MakeShared<FJsonValueString>(Order));H->SetArrayField(TEXT("Orders"),Orders);Headers.Add(MakeShared<FJsonValueObject>(H));
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("SSWExportCampaignInformation")))
 {for(const auto& Existing:Root->GetArrayField(TEXT("campaigns")))if(Existing->AsObject()->GetStringField(TEXT("Empire"))!=TEXT("Terellian"))Headers.Add(Existing);Root->SetArrayField(TEXT("campaigns"),Headers);FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));if(!TestTrue(TEXT("Only header information exported; embedded collections omitted"),FFileHelper::SaveStringToFile(Text,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)))return false;}
 const auto& Actual=Root->GetArrayField(TEXT("campaigns"));if(!TestEqual(TEXT("Five original and seven faction headers present"),Actual.Num(),12))return false;
 for(int32 I=0;I<5;++I)
 {
  FString A,B;FJsonSerializer::Serialize(Actual[I]->AsObject().ToSharedRef(),TJsonWriterFactory<>::Create(&A));FJsonSerializer::Serialize(Headers[I]->AsObject().ToSharedRef(),TJsonWriterFactory<>::Create(&B));
  TestEqual(TEXT("Exact header briefing parity with original DEF"),A,B);
  for(const TCHAR* Embedded:{TEXT("Action"),TEXT("Zone"),TEXT("Missions"),TEXT("Combatant")})TestFalse(TEXT("Information JSON does not embed campaign simulation arrays"),Actual[I]->AsObject()->HasField(Embedded));
 }
 for(int32 I=5;I<Actual.Num();++I)
 {
  const auto H=Actual[I]->AsObject();
  for(const TCHAR* Field:{TEXT("Name"),TEXT("Description"),TEXT("Situation"),TEXT("Start"),TEXT("System"),TEXT("Region"),TEXT("MainImage")})
   TestFalse(*FString::Printf(TEXT("Faction header has readable %s"),Field),H->GetStringField(Field).IsEmpty());
  TestTrue(TEXT("Faction objectives are populated"),H->GetArrayField(TEXT("Orders")).Num()>0);
  TestTrue(TEXT("Faction chapter uses explicit availability"),H->HasField(TEXT("Available")));
 }
 TestEqual(TEXT("Faction introductions are separate header variants"),FactionBriefingCatalog::All().Num(),7);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactionStoryMissionTest,"Starshatter.Mission.FactionStoriesAndIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFactionStoryMissionTest::RunTest(const FString&)
{
 TArray<FS_CampaignMissionList> Lists;TArray<FS_CampaignMission> Rows;FString Error;
 if(!TestTrue(TEXT("All standalone missions load directly from JSON"),StandaloneMissionCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Missions"),Lists,Rows,Error))){AddError(Error);return false;}
 TestEqual(TEXT("Existing forty plus forty-five distinct faction stories"),Rows.Num(),85);
 auto* Agents=NewObject<UAgentCatalogSubsystem>(NewObject<UGameInstance>());Agents->LoadCatalog();
 const bool OwnShips=ShipDesignRegistry::GetAll().IsEmpty(),OwnWeapons=WeaponDesignRegistry::GetAll().IsEmpty();
 ON_SCOPE_EXIT{if(OwnShips)ShipDesignRegistry::Clear();if(OwnWeapons)WeaponDesignRegistry::Clear();};
 if(OwnWeapons)NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>())->LoadWeaponDesigns();
 if(OwnShips)NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>())->LoadAll(false);
 int32 Factions=0,Dantari=0;TSet<int32> MarakanCampaigns;
 for(const auto& Row:Rows)
 {
  if(Row.RequiredEmpire==EEMPIRE_NAME::Neutral)continue;++Factions;
  TestTrue(TEXT("Owning empire may use its mission"),AgentMissionRules::FactionAllows(Row.RequiredEmpire,Row));
  TestFalse(TEXT("Alliance cannot claim other empire story"),AgentMissionRules::FactionAllows(EEMPIRE_NAME::Terellian,Row));
  TestFalse(TEXT("Other faction cannot claim a rival story"),AgentMissionRules::FactionAllows(Row.RequiredEmpire==EEMPIRE_NAME::Marakan?EEMPIRE_NAME::Dantari:EEMPIRE_NAME::Marakan,Row));
  TestTrue(TEXT("Mission keeps counterpart reference"),Row.CounterpartMissionId>0);TestEqual(TEXT("Narrative number does not make campaign own this standalone mission"),Row.StoryCampaignId,0);
  if(Row.RequiredEmpire==EEMPIRE_NAME::Dantari){++Dantari;TestEqual(TEXT("Dantari content remains campaign two"),Row.NarrativeCampaignId,2);}else if(Row.NarrativeCampaignId)MarakanCampaigns.Add(Row.NarrativeCampaignId);
  for(const auto& Element:Row.Element)if(!Element.Design.IsEmpty())TestNotNull(*FString::Printf(TEXT("Faction scenario hull %s exists"),*Element.Design),ShipDesignRegistry::Find(FName(*Element.Design)));
  if(Row.RequiredEmpire==EEMPIRE_NAME::Marakan && Row.NarrativeCampaignId==5)
  {
   int32 Aliens=0;for(const auto& E:Row.Element)if(E.IFFCode!=2){++Aliens;TestEqual(TEXT("Campaign five attackers are Zolons"),E.IFFCode,4);TestTrue(TEXT("Zolon design remains NPC opponent"),E.Design.StartsWith(TEXT("ZZ")) && E.Player==0);}
   TestTrue(TEXT("Campaign five has an alien attack force"),Aliens>=2);
  }
  bool Routed=false;for(const auto& Pair:Agents->GetCatalog())if(Pair.Value.MissionIds.Contains(Row.Id)){Routed=true;TestEqual(TEXT("Story officer belongs to correct empire"),Pair.Value.Empire,Row.RequiredEmpire);TestEqual(TEXT("Officer uses mission system"),Pair.Value.System,Row.MissionSystem);}
  TestTrue(TEXT("New story has an issuing officer"),Routed);
  Mission Model(910000+Row.Id);if(!TestTrue(TEXT("New story provisions through real mission loader"),Model.LoadFromCampaignMissionData(Row)))continue;
  TestNotNull(TEXT("Opposed-side player is provisioned"),Model.GetPlayer());
  const auto* Player=Row.Element.FindByPredicate([](const auto& E){return E.Player>0;});if(!TestNotNull(TEXT("Explicit player element"),Player))continue;
  TestEqual(TEXT("Faction player uses opposing fleet IFF"),Player->IFFCode,2);
  const auto* Hull=ShipDesignRegistry::Find(Player->Design);TestTrue(TEXT("Player craft has correct faction design"),Hull && ((Row.RequiredEmpire==EEMPIRE_NAME::Marakan && Hull->ShipEmpire==EShipEmpire::Marakan) || (Row.RequiredEmpire==EEMPIRE_NAME::Dantari && Hull->ShipEmpire==EShipEmpire::Dantari)));
  const auto Quote=FMissionRewardCatalog::Get().Quote(1000,910000+Row.Id,false,0,0);TestTrue(TEXT("Every faction story pays credits and promotion points"),Quote.Base>0 && Quote.CompletionPoints>0);
  const auto Progress=FMissionArcCatalog::Get().Evaluate(910000+Row.Id,[](int32){return false;});TestNotNull(TEXT("Story belongs to a valid handoff arc"),Progress.Arc);
 }
 for(const auto& Arc:FMissionArcCatalog::Get().GetArcs())
 {
  if(!(Arc.Id.ToString().StartsWith(TEXT("MarakanCampaign")) || Arc.Id==TEXT("MarakanBorderWar") || Arc.Id==TEXT("DantariHighland")))continue;
  AgentAI Journal;const FName Pilot(*Arc.Empire);int32 Finished=0;
  for(const auto& Step:Arc.Steps)
  {
   const int32 Offer[]={Step.MissionId};TestNotNull(TEXT("Next local officer offers unlocked chapter"),Journal.Request(Pilot,Offer,Step.AgentId));
   TestTrue(TEXT("Chapter accepted"),Journal.Accept(Pilot,Step.MissionId));TestTrue(TEXT("Chapter starts"),Journal.Start(Pilot,Step.MissionId));
   TestTrue(TEXT("Chapter success recorded"),Journal.Resolve(Pilot,Step.MissionId,true));
   AgentAI Reopened;TestTrue(TEXT("Chapter handoff survives restart"),Reopened.RestoreJournal(Journal.SaveJournal()));Journal=MoveTemp(Reopened);++Finished;
  }
  const auto Completed=FMissionArcCatalog::Get().Evaluate(Arc.Steps.Last().MissionId,[&](int32 Id){return Journal.IsCompleted(Pilot,Id);});
  TestTrue(TEXT("Entire faction arc completes through agent handoffs"),Completed.bComplete);TestEqual(TEXT("Every chapter completion retained"),Completed.Completed,Finished);
 }
 TestEqual(TEXT("All new stories tested"),Factions,45);TestEqual(TEXT("Ten Dantari opposed-side stories"),Dantari,10);TestEqual(TEXT("Marakan chapters cover campaigns two to five"),MarakanCampaigns.Num(),4);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactionTrainingNativeTest,"Starshatter.Campaign.FactionTrainingNativeScenarios",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFactionTrainingNativeTest::RunTest(const FString&)
{
 auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));if(!TestNotNull(TEXT("Original campaign asset"),Table))return false;
 const FS_Campaign* Original=nullptr;for(const auto& Pair:Table->GetRowMap()){const auto* C=reinterpret_cast<const FS_Campaign*>(Pair.Value);if(C->Name==TEXT("Operation Live Fire")){Original=C;break;}}
 if(!TestNotNull(TEXT("Original training campaign retained"),Original))return false;
 const auto OldShips=ShipDesignRegistry::GetAll();const auto OldWeapons=WeaponDesignRegistry::GetAll();
 ON_SCOPE_EXIT{ShipDesignRegistry::Clear();WeaponDesignRegistry::Clear();for(const auto& P:OldWeapons)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);for(const auto& P:OldShips)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
 NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>())->LoadWeaponDesigns();NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>())->LoadAll(false);
 FString Error;TestTrue(TEXT("Terellian uses original campaign without mutation"),FactionTrainingCatalog::Resolve(*Original,EEMPIRE_NAME::Terellian,Error)==Original);
 for(const auto Empire:{EEMPIRE_NAME::Marakan,EEMPIRE_NAME::Dantari})
 {
  const auto* C=FactionTrainingCatalog::Resolve(*Original,Empire,Error);if(!TestNotNull(TEXT("Native faction catalog resolves"),C)){AddError(Error);continue;}
  TestEqual(TEXT("All seven lessons available in catalog"),C->Missions.Num(),7);TestEqual(TEXT("Save row identity preserved"),C->RowName,Original->RowName);
  const FString System=Empire==EEMPIRE_NAME::Marakan?TEXT("Isham"):TEXT("Solus");
  TestTrue(TEXT("Faction system map exists"),FPackageName::DoesPackageExist(TEXT("/Game/Maps/")+System));
  AgentAI Journal;const FName Pilot(*FString::Printf(TEXT("Faction training %d"),int32(Empire)));
  for(const auto& M:C->Missions)
  {
   TestEqual(TEXT("Correct faction system"),M.MissionSystem,System);TestTrue(TEXT("Own faction gate"),M.RequiredEmpire==Empire);TestTrue(TEXT("Playable training display"),M.DisplayType==EMissionDisplayType::PlayerMission);
   const auto* P=M.Element.FindByPredicate([](const auto& E){return E.Player>0;});if(TestNotNull(TEXT("Lesson provisions player"),P)){TestEqual(TEXT("Training player uses faction IFF"),P->IFFCode,2);TestNotNull(TEXT("Assigned hull resolves"),ShipDesignRegistry::Find(FName(*P->Design)));}
   for(const auto& E:M.Element)if(!E.Design.IsEmpty())TestNotNull(*FString::Printf(TEXT("Native training design %s"),*E.Design),ShipDesignRegistry::Find(FName(*E.Design)));
   Mission Model(M.Id);TestTrue(TEXT("Actual native mission loader provisions lesson"),Model.LoadFromCampaignMissionData(M));TestNotNull(TEXT("Native mission has player craft"),Model.GetPlayer());
   const int32 Ids[]={M.Id};Journal.Request(Pilot,Ids,TEXT("Faction instructor"));TestTrue(TEXT("Lesson accepted"),Journal.Accept(Pilot,M.Id));TestTrue(TEXT("Lesson started"),Journal.Start(Pilot,M.Id));TestTrue(TEXT("Lesson completed"),Journal.Resolve(Pilot,M.Id,true));
   AgentAI Reload;TestTrue(TEXT("Training progression survives restart"),Reload.RestoreJournal(Journal.SaveJournal()));Journal=MoveTemp(Reload);TestTrue(TEXT("Completed lesson retained"),Journal.IsCompleted(Pilot,M.Id));
   if(M.Id==105)
   {
    TSet<FString> Regions;for(const auto& E:M.Element){if(!E.RegionName.IsEmpty())Regions.Add(E.RegionName);for(const auto& N:E.Navpoint)if(!N.OrderRegionName.IsEmpty())Regions.Add(N.OrderRegionName);}
    TestTrue(TEXT("Quantum lesson retains distinct destination sectors"),Regions.Num()>=2);
   }
  }
  TestFalse(TEXT("Higher lessons still require rank"),FTrainingTestRules::Unlocked(105,0,true,false));TestFalse(TEXT("Higher lessons still require hull skill"),FTrainingTestRules::Unlocked(105,3,false,false));
  TestTrue(TEXT("Cached faction snapshot keeps stable campaign pointer"),FactionTrainingCatalog::Resolve(*Original,Empire,Error)==C);
 }
 TestEqual(TEXT("Original campaign name unchanged"),Original->Name,FString(TEXT("Operation Live Fire")));return true;
}
#endif
