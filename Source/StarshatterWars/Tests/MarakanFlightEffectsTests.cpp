#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs_System.h"
#include "ShipActor.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarakanFlightEffectsTest,"Starshatter.Fighters.Marakan.DefinitionEffects",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMarakanFlightEffectsTest::RunTest(const FString&)
{
 auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
 if(!TestNotNull(TEXT("Ship design table"),Table))return false;
 TMap<FName,FShipDesign> OriginalRows;
 for(const TCHAR* Key:{TEXT("Avenger"),TEXT("Cobra"),TEXT("Intruder"),TEXT("Jackal"),TEXT("Raptor"),TEXT("Razor"),TEXT("Viper")})
  if(const auto* Row=Table->FindRow<FShipDesign>(FName(Key),TEXT("FX test backup")))OriginalRows.Add(FName(Key),*Row);
 ON_SCOPE_EXIT {for(const auto& Entry:OriginalRows)Table->AddRow(Entry.Key,Entry.Value);};
 auto* Importer=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());
 if(!TestTrue(TEXT("Refresh via the same DEF importer used at startup"),Importer->ReimportMarakanFlightEffects()))return false;
 for(const TCHAR* Key:{TEXT("Avenger"),TEXT("Cobra"),TEXT("Intruder"),TEXT("Jackal"),TEXT("Raptor"),TEXT("Razor"),TEXT("Viper")})
 {
  const auto* D=Table->FindRow<FShipDesign>(FName(Key),TEXT("Marakan effect parity"));
  if(!TestNotNull(FString(Key)+TEXT(" imported design"),D))continue;
  const FString Path=FString::Printf(TEXT("/Game/Models/%s/BP_%s.BP_%s_C"),Key,Key,Key);
  const auto* Class=LoadClass<AShipActor>(nullptr,*Path);
  if(!TestNotNull(FString(Key)+TEXT(" Blueprint"),Class))continue;
  const auto* A=CastChecked<AShipActor>(Class->GetDefaultObject());
  int Engine=0,Thruster=0,Light=0;
  for(const auto& Drive:D->Drive) for(const auto& P:Drive.Ports)
  {
   FVector Local=P.Location/D->Scale;
   if(A->bLegacyEmitterCoordinates)Local=FVector(Local.Z,-Local.X,Local.Y);
   if(TestTrue(FString(Key)+TEXT(" engine count"),A->MainEnginePointDefs.IsValidIndex(Engine)))
    TestTrue(FString(Key)+TEXT(" engine offset converts exactly once"),A->MainEnginePointDefs[Engine].LocalOffset.Equals(Local,.001));
   ++Engine;
  }
  for(const auto& RCS:D->Thruster) for(const auto& P:RCS.Ports)
  {
   FVector Local=P.Location/D->Scale;
   if(A->bLegacyEmitterCoordinates)Local=FVector(Local.Z,-Local.X,Local.Y);
   if(TestTrue(FString(Key)+TEXT(" RCS count"),A->ThrusterPointDefs.IsValidIndex(Thruster)))
    TestTrue(FString(Key)+TEXT(" RCS offset converts exactly once"),A->ThrusterPointDefs[Thruster].LocalOffset.Equals(Local,.001));
   ++Thruster;
  }
  for(const auto& Nav:D->Navlight)Light+=Nav.Beacons.Num();
  TestEqual(FString(Key)+TEXT(" runtime engine coverage"),Engine,A->MainEnginePointDefs.Num());
  TestEqual(FString(Key)+TEXT(" runtime RCS coverage"),Thruster,A->ThrusterPointDefs.Num());
  TestEqual(FString(Key)+TEXT(" runtime running light coverage"),Light,A->NavLightDefs.Num());
 }
 return true;
}
#endif


