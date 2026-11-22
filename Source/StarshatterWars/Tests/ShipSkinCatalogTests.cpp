#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "ShipSkinCatalog.h"
#include "ShipSkinDlg.h"
#include "StarshatterShipDesignSubsystem.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "PlayerSaveGame.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipSkinCollectionTest,"Starshatter.Skins.CollectionOwnershipAndPersistence",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipSkinCollectionTest::RunTest(const FString&)
{
    const auto BeforeWeapons=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& Pair:BeforeWeapons)WeaponDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    auto* Weapons=NewObject<UDataTable>();
    if(!TestTrue(TEXT("Weapon dependencies import"),UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Weapons,FPaths::ProjectContentDir()/TEXT("GameData/Weapons/wep.json"))))return false;
    for(const auto& Pair:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FWeaponDesign*>(Pair.Value));
    const auto Before=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Before)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value,false);};
    auto* Ships=NewObject<UDataTable>();
    if(!TestTrue(TEXT("Native ship JSON imports"),UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Ships,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))))return false;
    for(const auto& Pair:Ships->GetRowMap())ShipDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FShipDesign*>(Pair.Value),false);
    TArray<FShipSkinCatalogEntry> Entries;FString Error;
    if(!TestTrue(TEXT("Skin catalog imports"),ShipSkinCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/skins.json"),Entries,Error)))return false;
    TestTrue(TEXT("Existing cosmetic variants retained"),Entries.Num()>=24);
    for(const auto& Entry:Entries)
        TestNotNull(*FString::Printf(TEXT("%s resolves to a material-based native skin"),*Entry.Id.ToString()),ShipSkinCatalog::Resolve(Entry));
    if(Entries.IsEmpty())return false;
    FS_PlayerGameInfo Pilot;
    auto& Hull=Pilot.HangarShips.AddDefaulted_GetRef();Hull.HullId=FGuid::NewGuid();Hull.Design=Entries[0].Hull;
    Hull.Location=TEXT("Carrier / Orion");Hull.HullCondition=.65f;Hull.HullValue=750000;
    const FGuid Id=Hull.HullId;
    auto Licensed=Entries[0];Licensed.bIncluded=false;FText Reason;
    auto Included=Licensed;Included.bIncluded=true;
    TestTrue(TEXT("Original materials remain usable without a purchase"),ShipSkinCatalog::IsOwned(Pilot,Included));
    TestFalse(TEXT("Included default paints are not listed as pilot purchases"),ShipSkinCatalog::IsPurchased(Pilot,Included));
    TestFalse(TEXT("The store catalog is not the pilot's collection"),ShipSkinCatalog::IsPurchased(Pilot,Licensed));
    TestFalse(TEXT("Unowned skins cannot be applied"),ShipSkinCatalog::Apply(Pilot,Id,&Licensed,Hull.Location,Reason));
    Pilot.OwnedShipSkins.Add(Licensed.Id);
    TestTrue(TEXT("Purchased license is visible in the pilot collection"),ShipSkinCatalog::IsPurchased(Pilot,Licensed));
    TestFalse(TEXT("Flight has no docked appearance service"),ShipSkinCatalog::Apply(Pilot,Id,&Licensed,FString(),Reason));
    TestFalse(TEXT("Remote stored hull cannot be repainted here"),ShipSkinCatalog::Apply(Pilot,Id,&Licensed,TEXT("Station / Janus / Remote"),Reason));
    auto Incompatible=Licensed;Incompatible.Hull=Licensed.Hull==FName(TEXT("Falcon"))?FName(TEXT("Stormhawk")):FName(TEXT("Falcon"));
    TestFalse(TEXT("Hull compatibility is enforced"),ShipSkinCatalog::Apply(Pilot,Id,&Incompatible,Hull.Location,Reason));
    TestTrue(TEXT("Owned compatible skin applies at its dock"),ShipSkinCatalog::Apply(Pilot,Id,&Licensed,Hull.Location,Reason));
    TestEqual(TEXT("Selection is per owned hull"),Hull.SelectedSkin,Licensed.Id);
    TestEqual(TEXT("Appearance never repairs damage"),Hull.HullCondition,.65f);
    TestEqual(TEXT("Appearance never changes hull value"),Hull.HullValue,int64(750000));
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo=Pilot;TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Pilot save serializes cosmetics"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Pilot save reloads"),Loaded))return false;
    TestTrue(TEXT("Skin ownership survives reload"),Loaded->PlayerInfo.OwnedShipSkins.Contains(Licensed.Id));
    TestTrue(TEXT("Purchased collection filtering survives reload"),ShipSkinCatalog::IsPurchased(Loaded->PlayerInfo,Licensed));
    TestEqual(TEXT("Hull appearance survives reload"),Loaded->PlayerInfo.HangarShips[0].SelectedSkin,Licensed.Id);
    TestTrue(TEXT("Reset is supported"),ShipSkinCatalog::Apply(Pilot,Id,nullptr,Hull.Location,Reason));
    TestTrue(TEXT("Reset restores original appearance ID"),Hull.SelectedSkin.IsNone());
    Hull.bLost=true;
    TestFalse(TEXT("Lost hull cannot be modified"),ShipSkinCatalog::Apply(Pilot,Id,&Licensed,Hull.Location,Reason));
    TestTrue(TEXT("Registry driver is a standard base screen"),UShipSkinDlg::StaticClass()->IsChildOf(UBaseScreen::StaticClass()));
    return true;
}
#endif
