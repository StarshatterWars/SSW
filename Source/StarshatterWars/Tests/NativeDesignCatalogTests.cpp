#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"
#include "Engine/DataTable.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeDesignCatalogTest,"Starshatter.Designs.NativeCatalogParity",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNativeDesignCatalogTest::RunTest(const FString&)
{
 const FString Root=FPaths::ProjectContentDir()/TEXT("GameData");
 FString Error;
 TMap<FName,FWeaponDesign> Weapons;
 auto* Table=NewObject<UDataTable>();
 if(!TestTrue(TEXT("Native weapon JSON loads"),UStarshatterWeaponDesignSubsystem::ReadJsonCatalog(Root/TEXT("Weapons/wep.json"),Weapons,Error)) ||
 !TestTrue(TEXT("Independent table importer loads"),UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Table,Root/TEXT("Weapons/wep.json"))))return false;
 TestEqual(TEXT("All authored weapons retained"),Weapons.Num(),Table->GetRowMap().Num());
 for(const auto& Pair:Weapons){const auto* Row=Table->FindRow<FWeaponDesign>(Pair.Key,TEXT("Native parity"),false);
 TestTrue(*FString::Printf(TEXT("Every weapon field retained: %s"),*Pair.Key.ToString()),Row && FWeaponDesign::StaticStruct()->CompareScriptStruct(&Pair.Value,Row,0));}
 TestNotNull(TEXT("Names containing spaces retain lookup identity"),Weapons.Find(TEXT("Alpha Cannon")));
 TMap<FName,FSystemDesign> Systems;
 if(!TestTrue(TEXT("Native system JSON loads"),UStarshatterSystemDesignSubsystem::ReadJsonCatalog(Root/TEXT("Systems/sys.json"),Systems,Error)))return false;
 FString Json;FSystemDesignDocument Document;
 if(!FFileHelper::LoadFileToString(Json,*(Root/TEXT("Systems/sys.json"))) || !FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Document))return false;
 TestEqual(TEXT("Every system retained"),Systems.Num(),Document.Systems.Num());
 for(auto Row:Document.Systems){Row.Name.TrimStartAndEndInline();Row.SourceFile=TEXT("GameData/Systems/sys.json");if(Row.ComponentGrades.IsEmpty())Row.ComponentGrades=Document.ComponentGrades;
 const auto* Native=Systems.Find(FName(*Row.Name));
 TestTrue(*FString::Printf(TEXT("All component fields and grades retained: %s"),*Row.Name),Native && FSystemDesign::StaticStruct()->CompareScriptStruct(&Row,Native,0));
 if(Native)TestEqual(TEXT("Five component quality grades retained"),Native->ComponentGrades.Num(),5);}
 const int32 WeaponCount=Weapons.Num(),SystemCount=Systems.Num();
 TestFalse(TEXT("Missing weapon JSON rejected"),UStarshatterWeaponDesignSubsystem::ReadJsonCatalog(Root/TEXT("Weapons/missing-native-test.json"),Weapons,Error));
 TestEqual(TEXT("Failed weapon read preserves prior records"),Weapons.Num(),WeaponCount);
 TestFalse(TEXT("Missing system JSON rejected"),UStarshatterSystemDesignSubsystem::ReadJsonCatalog(Root/TEXT("Systems/missing-native-test.json"),Systems,Error));
 TestEqual(TEXT("Failed system read preserves prior records"),Systems.Num(),SystemCount);
 return true;
}
#endif
