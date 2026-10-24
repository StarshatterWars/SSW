#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"
#include "ShipActor.h"
#include "Skin.h"
#include "ShipDesignRegistry.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Parser.h"
#include "Reader.h"
#include "Term.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeHullSkinTest,"Starshatter.Audit.F15.NativeHullSkin",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeHullSkinTest::RunTest(const FString& Parameters)
{
    auto* Data=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());Data->BeginDesignParse("CargoA.def");
    Parser Input(new BlockReader("skin: {name: Blue mtl: {name: Hull Ka:(0.5,0.5,0.7) Kd:(1,1,1) Ks:(1,0.9,0.8) Ke:(1,1,1) power:20 bump:3 tex_d:MAIN_D2 tex_s:MAIN_S tex_b:MAIN_B tex_e:MAIN_G2}}"));
    Term* Parsed=Input.ParseTerm();
    if(!TestNotNull(TEXT("Authored skin syntax parses"),Parsed)) return false;
    auto* Definition=Parsed->isDef();
    if(!Definition || !Definition->term() || !Definition->term()->isStruct()) {delete Parsed;return false;}
    Data->ParseSkin(Definition->term()->isStruct(),"CargoA.def");delete Parsed;
    if(!TestEqual(TEXT("Skin importer preserves skin"),Data->NewShipSkinArray.Num(),1)) return false;
    const FShipSkin& ParsedSkin=Data->NewShipSkinArray[0];
    if(!TestEqual(TEXT("Importer preserves material cell"),ParsedSkin.Cells.Num(),1)) return false;
    const FSkinMtlCell& Cell=ParsedSkin.Cells[0];
    TestEqual(TEXT("Legacy diffuse texture key retained"),Cell.TextureName,FString("MAIN_D2"));
    TestEqual(TEXT("Legacy specular texture key retained"),Cell.SpecularTextureName,FString("MAIN_S"));
    TestEqual(TEXT("Legacy bump texture key retained"),Cell.BumpTextureName,FString("MAIN_B"));
    TestEqual(TEXT("Legacy emissive texture key retained"),Cell.EmissiveTextureName,FString("MAIN_G2"));
    TestTrue(TEXT("Explicit color and scalar flags retained"),Cell.bHasAmbientColor&&Cell.bHasDiffuseColor&&Cell.bHasSpecularPower&&Cell.bHasBumpScale);
    FShipDesign OldRow;OldRow.ShipName=TEXT("CargoA");
    FShipSkin OldSkin;OldSkin.Name=TEXT("Blue");OldSkin.SourceFile=TEXT("CargoA.def");
    FSkinMtlCell OldCell;OldCell.MaterialName=TEXT("Hull");OldSkin.Cells.Add(OldCell);OldRow.Skin.Add(OldSkin);
    TUniquePtr<ShipDesign> Hydrated(ShipDesignRegistry::ConvertToLegacyDesign(TEXT("CargoA"),OldRow));
    const Skin* HydratedBlue=Hydrated->FindSkin("Blue");
    TestTrue(TEXT("Existing tables recover missing legacy skin metadata without reimport"),
        HydratedBlue && HydratedBlue->GetNativeDefinition() && HydratedBlue->GetNativeDefinition()->Cells[0].TextureName==TEXT("MAIN_D2"));
    const FName Key(TEXT("F15_AuditSkin"));
    if(ShipDesignRegistry::Has(Key)) {AddError(TEXT("Audit design key unexpectedly exists"));return false;}
    ON_SCOPE_EXIT {
        delete ShipDesignRegistry::LegacyDesignsByName.FindRef(Key);
        ShipDesignRegistry::LegacyDesignsByName.Remove(Key);ShipDesignRegistry::DesignsByName.Remove(Key);
    };
    FShipDesign Row;Row.ShipName=Key.ToString();Row.Skin=Data->NewShipSkinArray;
    ShipDesignRegistry::RegisterDesign(Key,Row);
    const ShipDesign* Legacy=ShipDesignRegistry::FindLegacy(Key);
    const Skin* Blue=Legacy?Legacy->FindSkin("Blue"):nullptr;
    if(!TestNotNull(TEXT("Skin survives native-to-legacy design bridge"),Blue)) return false;
    TestTrue(TEXT("Skin owns independent native metadata"),Blue->GetNativeDefinition()!=&ParsedSkin&&Blue->NumCells()==1);
    Mission MissionModel;
    const FString MissionText=TEXT("MISSION\nname: Audit\ntype: PATROL\nelement: {name: Test design: F15_AuditSkin skin: Blue count:1}");
    MissionModel.ParseMission(TCHAR_TO_ANSI(*MissionText));
    TestTrue(TEXT("Mission text selects runtime skin"),MissionModel.GetElements().size()==1&&MissionModel.GetElements()[0]->GetSkin()==Blue);
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    AShipActor* A=World->SpawnActor<AShipActor>();AShipActor* B=World->SpawnActor<AShipActor>();
    auto* Mesh=NewObject<UStaticMesh>(World);auto* Original=NewObject<UMaterial>(World);auto* Replacement=NewObject<UMaterial>(World);
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Original,TEXT("Hull"),TEXT("Hull")));
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Original,TEXT("Other"),TEXT("Other")));
    A->HullMesh->SetStaticMesh(Mesh);B->HullMesh->SetStaticMesh(Mesh);
    A->HullSkinMaterialOverrides.Add(TEXT("Blue.Hull"),Replacement);
    A->ApplyHullSkin(Blue);
    TestTrue(TEXT("Selected skin affects matching native hull slot"),A->HullMesh->GetMaterial(0)==Replacement);
    TestTrue(TEXT("Unmatched hull slot is preserved"),A->HullMesh->GetMaterial(1)==Original);
    TestTrue(TEXT("Other ships retain shared base materials"),B->HullMesh->GetMaterial(0)==Original);
    A->ApplyHullSkin(nullptr);TestTrue(TEXT("Removing skin restores original material"),A->HullMesh->GetMaterial(0)==Original);
    A->HullSkinMaterialOverrides.Reset();A->ApplyHullSkin(Blue);
    TestTrue(TEXT("Default bridge creates a per-ship dynamic material"),Cast<UMaterialInstanceDynamic>(A->HullMesh->GetMaterial(0))!=nullptr);
    A->RestoreHullSkin();World->DestroyWorld(false);
    return true;
}
#endif
