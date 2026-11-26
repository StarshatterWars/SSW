#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "HullRegistry.h"
#include "GeneratedShipNames.h"
#include "LocalGameStateSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPermanentHullRegistryTest,"Starshatter.Identity.PermanentRegistry",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPermanentHullRegistryTest::RunTest(const FString&)
{
 FHullRegistry Registry;TSet<FGuid> Seen;TSet<FString> Numbers;
 for(int32 I=0;I<10000;++I){const FGuid Id=FHullRegistry::Allocate();Seen.Add(Id);Numbers.Add(FHullRegistry::Number(Id));}
 TestEqual(TEXT("Ten thousand allocations are unique"),Seen.Num(),10000);
 TestEqual(TEXT("Hull numbers are unique"),Numbers.Num(),10000);
 const FGuid Id=FHullRegistry::Allocate();
 TestTrue(TEXT("New hull registered"),Registry.Register(Id,TEXT("Falcon"),TEXT("Alpha 1"),TEXT("Pilot"),1));
 TestTrue(TEXT("Rename is permitted"),Registry.Rename(Id,TEXT("Falcon")));
 TestEqual(TEXT("Rename preserves VIN"),Registry.Find(Id)->HullId,Id);
 TestTrue(TEXT("Authorized rental changes custody only"),Registry.Transfer(Id,TEXT("Pilot"),TEXT("Renter"),50,TEXT("Rental")));
 TestFalse(TEXT("Authorized renter is not marked stolen"),Registry.Find(Id)->bStolen);
 TestEqual(TEXT("Rental does not transfer legal title"),Registry.Find(Id)->LegalOwner,FName(TEXT("Pilot")));
 TestTrue(TEXT("Possession can differ from title"),Registry.SetPossessor(Id,TEXT("Thief")));
 TestTrue(TEXT("Theft detected"),Registry.Find(Id)->bStolen);
 TestEqual(TEXT("Theft preserves legal owner"),Registry.Find(Id)->LegalOwner,FName(TEXT("Pilot")));
 TestEqual(TEXT("Theft preserves VIN"),Registry.Find(Id)->HullId,Id);
 TestTrue(TEXT("Legitimate sale recorded"),Registry.Transfer(Id,TEXT("Buyer"),TEXT("Buyer"),100,TEXT("Ship store sale")));
 TestFalse(TEXT("Sale clears stolen state"),Registry.Find(Id)->bStolen);
 TestEqual(TEXT("Ownership history persisted"),Registry.Find(Id)->OwnershipHistory.Num(),1);
 auto* Save=NewObject<ULocalUniverseState>();Save->HullRegistry=Registry.Snapshot();TArray<uint8> Bytes;
 TestTrue(TEXT("Registry serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 auto* Reload=Cast<ULocalUniverseState>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(!TestNotNull(TEXT("Registry reloads"),Reload))return false;
 FHullRegistry Imported;FString Error;
 TestTrue(TEXT("Registry import succeeds"),Imported.Import(Reload->HullRegistry,Error));
 TestEqual(TEXT("Reload retains ownership history"),Imported.Find(Id)->OwnershipHistory.Num(),1);
 auto Collision=Reload->HullRegistry;Collision.Hulls.FindChecked(Id).Design=TEXT("Stormhawk");
 TestFalse(TEXT("Conflicting imported VIN rejected"),Imported.Import(Collision,Error));
 TestEqual(TEXT("Rejected import changes nothing"),Imported.Find(Id)->Design,FName(TEXT("Falcon")));
 TestTrue(TEXT("Destruction creates permanent tombstone"),Imported.SetStatus(Id,EHullOperationalState::Destroyed));
 TestFalse(TEXT("Destroyed hull cannot be repaired alive"),Imported.SetStatus(Id,EHullOperationalState::Operational));
 TestFalse(TEXT("Old copied save cannot resurrect destroyed identity"),Imported.Import(Reload->HullRegistry,Error));
 TestFalse(TEXT("Destroyed identity cannot be reused"),Imported.Register(Id,TEXT("Falcon"),TEXT("Replacement"),TEXT("Pilot"),1));
 FGeneratedShipNames Names;
 TestTrue(TEXT("JSON vocabulary loads"),Names.Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/generated_ship_names.json"),Error));
 TestFalse(TEXT("Generated vessel has a name"),Names.ForHull(Id).IsEmpty());
 TestEqual(TEXT("Name is deterministic for permanent identity"),Names.ForHull(Id),Names.ForHull(Id));
 return true;
}
#endif
