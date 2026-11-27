#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "SSWCombatGroupSubsystem.h"
#include "SSWGameInstance.h"
#include "OOBJsonCatalog.h"
#include "DesignTableJson.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "CombatGroupRegistry.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOOBJsonParityTest,"Starshatter.Data.OOB.DefJsonAndSavedTableParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOOBJsonParityTest::RunTest(const FString&)
{
 auto* Context=NewObject<USSWGameInstance>();auto* Parser=NewObject<USSWCombatGroupSubsystem>(Context);
 auto* Expected=NewObject<UDataTable>();Expected->RowStruct=FS_CombatGroup::StaticStruct();
 const FString Base=FPaths::ProjectContentDir()/TEXT("GameData/Campaigns");
 for(const TCHAR* Faction:{TEXT("Alliance"),TEXT("Dantari"),TEXT("Haiche"),TEXT("Hegemony"),TEXT("Pirates"),TEXT("Silessia"),TEXT("Solus"),TEXT("Zolons")})
 {
  auto* Legacy=NewObject<UDataTable>();Legacy->RowStruct=FS_CombatGroup::StaticStruct();Parser->CombatGroupDataTable=Legacy;
  const FTCHARToUTF8 Path(*(Base/(FString(Faction)+TEXT(".def"))));Parser->LoadCombatRoster(Path.Get(),-1);
  auto* Json=NewObject<UDataTable>();
  if(!TestTrue(Faction,DesignTableJson::Import(*Json,*FS_CombatGroup::StaticStruct(),Base/TEXT("OOB")/(FString(Faction)+TEXT(".json")))))return false;
  // Authored correction: Sintar is a Tarsus region, not an unspecified inherited system.
  for(const auto& Pair:Legacy->GetRowMap())
  {
   auto* Row=reinterpret_cast<FS_CombatGroup*>(Pair.Value);
   if(Row->Id==8579)
   {
    TestEqual(TEXT("Archived Sintar system was unspecified"),Row->System,FString());
    const auto* Current=Json->FindRow<FS_CombatGroup>(Pair.Key,TEXT("Sintar correction"),false);
    if(!TestNotNull(TEXT("Sintar station identity retained"),Current))return false;
    TestEqual(TEXT("Authored Sintar endpoint belongs to Tarsus"),Current->System,FString(TEXT("Tarsus")));
    Row->System=TEXT("Tarsus");
   }
  }
  TestTrue(TEXT("All nested fields, exact keys, enums and coordinates round-trip"),DesignTableJson::Compare(*Legacy,*Json));
  for(const auto& Pair:Legacy->GetRowMap())Expected->AddRow(Pair.Key,*reinterpret_cast<const FS_CombatGroup*>(Pair.Value));
 }
 auto* Combined=NewObject<UDataTable>();FString Error;
 if(!TestTrue(TEXT("All factions merge through the standard DataTable bridge"),OOBJsonCatalog::ImportDirectory(*Combined,Base/TEXT("OOB"),Error))){AddError(Error);return false;}
 TestTrue(TEXT("Merged JSON matches all legacy groups"),DesignTableJson::Compare(*Expected,*Combined));
 auto* Saved=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_CombatGroup.DT_CombatGroup"));
 if(TestNotNull(TEXT("Same saved combat-group table"),Saved))
 {
  auto* Reference=NewObject<UDataTable>();Reference->RowStruct=FS_CombatGroup::StaticStruct();
  for(const auto& Pair:Saved->GetRowMap())
  {
   auto Row=*reinterpret_cast<const FS_CombatGroup*>(Pair.Value);
   if(Row.Id==8579){TestTrue(TEXT("Historical Sintar reference is blank or corrected"),Row.System.IsEmpty() || Row.System==TEXT("Tarsus"));Row.System=TEXT("Tarsus");}
   Reference->AddRow(Pair.Key,Row);
  }
  TestTrue(TEXT("Saved table contains the converted OOBs, with the explicit Sintar endpoint correction"),DesignTableJson::Compare(*Expected,*Reference));
 }
 TMap<FName,FS_CombatGroup> Native;
 TestTrue(TEXT("Native reader requires no DataTable"),OOBJsonCatalog::LoadDirectory(Native,Base/TEXT("OOB"),Error));
 TestEqual(TEXT("Native OOB retains every group"),Native.Num(),Expected->GetRowMap().Num());
 int32 Units=0;for(const auto& Pair:Native)
 {
  const auto* Original=Expected->FindRow<FS_CombatGroup>(Pair.Key,TEXT("Native parity"),false);
  TestTrue(TEXT("Every native nested OOB field matches the former table"),Original && FS_CombatGroup::StaticStruct()->CompareScriptStruct(Original,&Pair.Value,0));Units+=Pair.Value.Unit.Num();
 }
 TestEqual(TEXT("All 967 authored unit rows retained"),Units,967);
 const auto PreviousRegistry=CombatGroupRegistry::GetAll();ON_SCOPE_EXIT{CombatGroupRegistry::Clear();for(const auto& Pair:PreviousRegistry)CombatGroupRegistry::RegisterGroup(Pair.Key,Pair.Value);};
 Parser->CombatGroupDataTable=nullptr;Parser->LoadAll(false);
 TestNull(TEXT("Runtime OOB loading never resolves or creates a DataTable"),Parser->GetCombatGroupDataTable());
 TestEqual(TEXT("Runtime queries use the full native roster"),Parser->GetCombatRosterData().Num(),Native.Num());
 TestEqual(TEXT("Native OOB feeds the sim registry"),CombatGroupRegistry::Num(),Native.Num());
 // Invalid replacement must never erase a working table.
 const FString Temp=FPaths::ProjectSavedDir()/TEXT("Automation/OOB-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 IFileManager::Get().MakeDirectory(*Temp,true);FFileHelper::SaveStringToFile(TEXT("[]"),*(Temp/TEXT("Invalid.json")));
 const int32 Count=Combined->GetRowMap().Num();TestFalse(TEXT("Empty faction replacement is rejected"),OOBJsonCatalog::ImportDirectory(*Combined,Temp,Error));TestEqual(TEXT("Failed reload retains the previous catalog"),Combined->GetRowMap().Num(),Count);
 IFileManager::Get().Delete(*(Temp/TEXT("Invalid.json")));IFileManager::Get().DeleteDirectory(*Temp,false,false);
 return true;
}
#endif
