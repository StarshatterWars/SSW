#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "TurretBaseCatalog.h"
#include "TurretActor.h"
#include "TurretBaseComponent.h"
#include "Components/StaticMeshComponent.h"
#include "OrionShipActor.h"
#include "ZodiacShipActor.h"
#include "FalconShipActor.h"
#include "FighterActor.h"
#include "ShipDesignRegistry.h"
#include "Ship.h"
#include "ShipWeaponMountComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurretBaseAttachmentTest,"Starshatter.Weapons.TurretBaseAttachments",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTurretBaseAttachmentTest::RunTest(const FString&)
{
    const auto Before=WeaponDesignRegistry::GetAll();const auto ShipsBefore=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT{ShipDesignRegistry::Clear();WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);for(const auto& P:ShipsBefore)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
    auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());
    const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadWeaponDesign(Ansi.Get());
    // Renamed DEF keys and old saved model references resolve identically.
    TestEqual(TEXT("LCA model alias"),TurretBaseCatalog::NativeClass(TEXT("LCATurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("LCaTurretBase")).Get());
    TestEqual(TEXT("Blockade model alias"),TurretBaseCatalog::NativeClass(TEXT("BlockadeTurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("mine_base")).Get());
    TestEqual(TEXT("Orca2 model alias"),TurretBaseCatalog::NativeClass(TEXT("Orca2TurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("t2_base")).Get());
    TestEqual(TEXT("Orca2Inverted model alias"),TurretBaseCatalog::NativeClass(TEXT("Orca2InvertedTurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("t2_base_inv")).Get());
    TestEqual(TEXT("Vanguard model alias"),TurretBaseCatalog::NativeClass(TEXT("VanguardTurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("Base1opt")).Get());
    TestEqual(TEXT("Graser model alias"),TurretBaseCatalog::NativeClass(TEXT("GraserTurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("graserbase")).Get());
    TestEqual(TEXT("AAA model alias"),TurretBaseCatalog::NativeClass(TEXT("AAATurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("AAA2Turret")).Get());
    TestEqual(TEXT("QuadAAA model alias"),TurretBaseCatalog::NativeClass(TEXT("QuadAAATurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("AAA4Turret")).Get());
    TestEqual(TEXT("SAM1 model alias"),TurretBaseCatalog::NativeClass(TEXT("SAM1TurretBase")).Get(),TurretBaseCatalog::NativeClass(TEXT("SAMTurret")).Get());
    const TCHAR* Types[]={
        TEXT("LCA Turret"),
        TEXT("Blockade Cannon"),
        TEXT("Orca 2 Cannon"),
        TEXT("Orca 2 Inverted"),
        TEXT("Vanguard"),
        TEXT("Graser Turret"),
        TEXT("AAA Cannon"),
        TEXT("Quad AAA"),
        TEXT("SAM-1"),
    };
    for(const TCHAR* Type:Types)
    {
        const auto* Row=WeaponDesignRegistry::Find(FString(Type));
        if(!TestNotNull(TEXT("Wep.def base weapon"),Row))continue;
        FString Model;float Scale;TurretBaseCatalog::Definition(FName(Type),Model,Scale);
        TestEqual(TEXT("Existing turret_base field drives base identity"),Model,Row->TurretBase);
        TestEqual(TEXT("Existing scale drives the base visual"),Scale,Row->Scale);
        TestTrue(TEXT("Dedicated base class resolves"),TurretBaseCatalog::NativeClass(Model).Get()!=ATurretBaseActor::StaticClass());
        const FString BaseName=TurretBaseCatalog::NativeClass(Model)->GetName().LeftChop(5);
        const FString ExpectedPath=FString::Printf(TEXT("/Game/Models/Weapons/TurretBase/%s/BP_%s.BP_%s_C"),*BaseName,*BaseName,*BaseName);
        TestEqual(TEXT("Renamed base Blueprint resolves"),TurretBaseCatalog::VisualClass(Model)->GetPathName(),ExpectedPath);
        const auto* Legacy=WeaponDesignRegistry::FindLegacy(FString(Type));
        if(TestNotNull(TEXT("Runtime design"),Legacy)){
            TestEqual(TEXT("Turret field crosses registry"),FString(ANSI_TO_TCHAR(Legacy->turret.data())),Row->Turret);
            TestEqual(TEXT("Turret base field crosses registry"),FString(ANSI_TO_TCHAR(Legacy->turret_base.data())),Row->TurretBase);
            TestEqual(TEXT("Turret axis crosses registry"),Legacy->turret_axis,Row->TurretAxis);
        }
    }
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    TArray<AShipActor*> Ships={World->SpawnActor<AOrionShipActor>(),World->SpawnActor<AZodiacShipActor>(),World->SpawnActor<AFalconShipActor>()};
    for(const TCHAR* Path:{TEXT("/Game/Models/Orion/BP_Orion.BP_Orion_C"),TEXT("/Game/Models/Zodiac/BP_Zodiac.BP_Zodiac_C"),TEXT("/Script/StarshatterWars.S7ShipActor"),TEXT("/Script/StarshatterWars.Gnd_sam1ShipActor")})
        if(UClass* Class=LoadClass<AShipActor>(nullptr,Path))Ships.Add(World->SpawnActor<AShipActor>(Class));
    int32 SeenCapital=0,SeenFighter=0;
    for(int32 S=0;S<Ships.Num();++S)
    {
        auto* Ship=Ships[S];if(!TestNotNull(TEXT("Ship actor"),Ship))continue;
        const int32 Count=Ship->AuthoredTurretPointComponents.Num();
        TArray<FTransform> BeforePoses;for(const auto& P:Ship->AuthoredTurretPointComponents)BeforePoses.Add(P->GetRelativeTransform());
        Ship->RebuildTurretActors();Ship->RebuildTurretActors();
        if (Ship->IsA<AOrionShipActor>())
        {
            int32 VanguardPairs = 0;
            int32 PhalanxBalls = 0;
            for (const auto& Point : Ship->AuthoredTurretPointComponents)
            {
                const auto* Base = Cast<UTurretBaseComponent>(Point.Get());
                if (!Base || !Ship->NativeWeaponHardpoints.IsValidIndex(Base->SourceWeaponIndex)) continue;
                const auto* Mount = Cast<UShipWeaponMountComponent>(Ship->NativeWeaponHardpoints[Base->SourceWeaponIndex]);
                const auto* Gun = Mount ? Mount->GetTurretVisualActor() : nullptr;
                if (!TestNotNull(TEXT("Orion gun is present at the base's weapon slot"), Gun)) continue;
                TestNotNull(TEXT("Orion gun mesh"), Gun->GunMesh->GetStaticMesh());
                TestTrue(TEXT("Orion gun and base use the same world mount point"),
                    Mount->GetComponentLocation().Equals(Base->GetComponentLocation(), 0.01));
                if (Base->WeaponType == TEXT("Vanguard"))
                {
                    const auto* Visual = Cast<ATurretBaseActor>(Base->GetChildActor());
                    if (TestNotNull(TEXT("Vanguard separate base actor"), Visual))
                        TestNotNull(TEXT("Vanguard base mesh"), Visual->BaseMesh->GetStaticMesh());
                    ++VanguardPairs;
                }
                else if (Base->WeaponType == TEXT("Phalanx Cannon"))
                {
                    TestNull(TEXT("Phalanx uses its integral ball mount"), Base->GetChildActor());
                    ++PhalanxBalls;
                }
            }
            TestEqual(TEXT("Orion Vanguard gun and base pairs"), VanguardPairs, 6);
            TestEqual(TEXT("Orion integral Phalanx mounts"), PhalanxBalls, 4);
        }
        TestEqual(TEXT("Rebuild never duplicates base components"),Ship->AuthoredTurretPointComponents.Num(),Count);
        TestEqual(TEXT("Base definitions survive derived constructor refresh"),Ship->TurretBasePointDefs.Num(),Count);
        for(int32 I=0;I<Count;++I){
            auto* Base=Cast<UTurretBaseComponent>(Ship->AuthoredTurretPointComponents[I].Get());
            if(!TestNotNull(TEXT("Shared typed base component"),Base))continue;
            TestTrue(TEXT("Authored transform preserved"),Base->GetRelativeTransform().Equals(BeforePoses[I]));
            TestTrue(TEXT("Base attaches to shared ship turret root"),Base->GetAttachParent()==Ship->TurretMountRoot);
            if(Base->LegacyBaseModel.IsEmpty()){TestNull(TEXT("No invented base for definitions without turret_base"),Base->GetChildActor());continue;}
            auto* Visual=Cast<ATurretBaseActor>(Base->GetChildActor());
            if(!TestNotNull(TEXT("Base actor attached"),Visual))continue;
            TestTrue(TEXT("Base points to its weapon slot"),Ship->AuthoredWeaponPointComponents.IsValidIndex(Base->SourceWeaponIndex));
            TestEqual(TEXT("Base model identity preserved"),Visual->LegacyBaseModel,Base->LegacyBaseModel);
            TestTrue(TEXT("Scale applied once on model root"),Visual->ModelRoot->GetRelativeScale3D().Equals(FVector(Base->ModelScale)));
            TestFalse(TEXT("Base actor has no gameplay collision"),Visual->GetActorEnableCollision());
            if(S==0 && I==0){
                Base->SetChildActorClass(ATurretBaseActor::StaticClass());Base->Configure(Base->WeaponType);
                TestEqual(TEXT("Manual base class override is preserved"),Base->GetChildActorClass().Get(),ATurretBaseActor::StaticClass());
            }
            if(S==0)++SeenCapital;if(S==1)++SeenFighter;
        }
    }
    TestTrue(TEXT("Capital ship bases configured"),SeenCapital>0);
    TestTrue(TEXT("Fighter LCA base configured"),SeenFighter>0);
    TestEqual(TEXT("Falcon fixed cannon gets no invented turret base"),Ships[2]->AuthoredTurretPointComponents.Num(),0);
    // Older generic fighter BPs receive missing base components when bound to the DEF.
    auto* ShipTable=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* Zodiac=ShipTable?ShipTable->FindRow<FShipDesign>(TEXT("Zodiac"),TEXT("Turret base")):nullptr;
    if(TestNotNull(TEXT("Zodiac data"),Zodiac)){
        ShipDesignRegistry::RegisterDesign(FName(TEXT("Zodiac")),*Zodiac);
        Ship Runtime("Turret base fixture","",ShipDesignRegistry::FindLegacy(FName(TEXT("Zodiac"))),1,0,nullptr,false);
        auto* Generic=World->SpawnActor<AFighterActor>();Generic->BindRuntimeShip(&Runtime);
        int32 RuntimeBases=0;
        for(const auto& P:Generic->AuthoredTurretPointComponents)
            if(const auto* Base=Cast<UTurretBaseComponent>(P.Get())){
                if(Base->LegacyBaseModel.IsEmpty())continue;
                ++RuntimeBases;TestNotNull(TEXT("Generic fighter base actor"),Base->GetChildActor());
                if(Generic->NativeWeaponHardpoints.IsValidIndex(Base->SourceWeaponIndex))
                    TestTrue(TEXT("Dynamic base uses its weapon mount location"),Base->GetComponentLocation().Equals(Generic->NativeWeaponHardpoints[Base->SourceWeaponIndex]->GetComponentLocation(),.001));
            }
        TestTrue(TEXT("Legacy generic fighter receives its LCA base"),RuntimeBases>0);
        const int32 Count=Generic->AuthoredTurretPointComponents.Num();Generic->ConfigureRuntimeTurretBases();Generic->RebuildTurretActors();
        TestEqual(TEXT("Runtime rebuild is idempotent"),Generic->AuthoredTurretPointComponents.Num(),Count);
        Generic->BindRuntimeShip(nullptr);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponRegistryAliasTest,"Starshatter.Weapons.RegistryAliasOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWeaponRegistryAliasTest::RunTest(const FString&)
{
    const auto Before=WeaponDesignRegistry::GetAll();WeaponDesignRegistry::Clear();
    ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
    for(int32 I=0;I<4;++I){
        FWeaponDesign Row;Row.Name=TEXT("Display alias");
        WeaponDesignRegistry::RegisterDesign(FName(TEXT("RowKey")),Row);
        TestEqual(TEXT("Aliases share one legacy design"),WeaponDesignRegistry::FindLegacy(FName(TEXT("RowKey"))),WeaponDesignRegistry::FindLegacy(FName(TEXT("Display alias"))));
        WeaponDesignRegistry::Clear();
        TestNull(TEXT("Row alias cleared"),WeaponDesignRegistry::FindLegacy(FName(TEXT("RowKey"))));
        TestNull(TEXT("Display alias cleared"),WeaponDesignRegistry::FindLegacy(FName(TEXT("Display alias"))));
    }
    return true;
}
#endif
