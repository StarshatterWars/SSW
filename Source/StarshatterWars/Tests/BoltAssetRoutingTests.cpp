#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "BoltWeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/Blueprint.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltAssetRoutingTest,"Starshatter.Weapons.BoltAssetRouting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBoltAssetRoutingTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    struct FRow{const TCHAR* Name;const TCHAR* BP;const TCHAR* MI;const TCHAR* OldBP;const TCHAR* OldNative;};
    const FRow Rows[]={
        {TEXT("Alpha Cannon"),TEXT("/Game/Model/Bolt/Alpha/BP_AlphaBolt.BP_AlphaBolt_C"),TEXT("/Game/Model/Bolt/Alpha/MI_AlphaBolt.MI_AlphaBolt"),TEXT("/Game/Models/Weapons/AlphaCannon/BP_AlphaCannon.BP_AlphaCannon_C"),TEXT("/Script/StarshatterWars.AlphaCannonBoltActor")},
        {TEXT("Delta Cannon"),TEXT("/Game/Model/Bolt/Delta/BP_DeltaBolt.BP_DeltaBolt_C"),TEXT("/Game/Model/Bolt/Delta/MI_DeltaBolt.MI_DeltaBolt"),TEXT("/Game/Models/Weapons/DeltaCannon/BP_Deltacannon.BP_Deltacannon_C"),TEXT("/Script/StarshatterWars.DeltaCannonBoltActor")},
        {TEXT("Heavy Cannon"),TEXT("/Game/Model/Bolt/Heavy/BP_HeavyBolt.BP_HeavyBolt_C"),TEXT("/Game/Model/Bolt/Heavy/MI_HeavyBolt.MI_HeavyBolt"),TEXT("/Game/Models/Weapons/HeavyCannon/BP_HeavyCannon.BP_HeavyCannon_C"),TEXT("/Script/StarshatterWars.HeavyCannonBoltActor")},
        {TEXT("Fighter Turret"),TEXT("/Game/Model/Bolt/Fighter/BP_FighterBolt.BP_FighterBolt_C"),TEXT("/Game/Model/Bolt/Fighter/MI_FighterBolt.MI_FighterBolt"),TEXT("/Game/Models/Weapons/FighterTurret/BP_FighterTurret.BP_FighterTurret_C"),TEXT("/Script/StarshatterWars.FighterTurretBoltActor")},
        {TEXT("LCA Turret"),TEXT("/Game/Model/Bolt/LCA/BP_LCABolt.BP_LCABolt_C"),TEXT("/Game/Model/Bolt/LCA/MI_LCABolt.MI_LCABolt"),TEXT("/Game/Models/Weapons/LCATurret/BP_LCATurret.BP_LCATurret_C"),TEXT("/Script/StarshatterWars.LCATurretBoltActor")},
        {TEXT("Blockade Cannon"),TEXT("/Game/Model/Bolt/Blockade/BP_BlockadeBolt.BP_BlockadeBolt_C"),TEXT("/Game/Model/Bolt/Blockade/MI_BlockadeBolt.MI_BlockadeBolt"),TEXT("/Game/Models/Weapons/BlockadeCannon/BP_BlockadeCannon.BP_BlockadeCannon_C"),TEXT("/Script/StarshatterWars.BlockadeCannonBoltActor")},
        {TEXT("Vulcan Cannon"),TEXT("/Game/Model/Bolt/Vulcan/BP_VulcanBolt.BP_VulcanBolt_C"),TEXT("/Game/Model/Bolt/Vulcan/MI_VulcanBolt.MI_VulcanBolt"),TEXT("/Game/Models/Weapons/VulcanCannon/BP_VulcanCannon.BP_VulcanCannon_C"),TEXT("/Script/StarshatterWars.VulcanCannonBoltActor")},
        {TEXT("Phalanx Cannon"),TEXT("/Game/Model/Bolt/Phalanx/BP_PhalanxBolt.BP_PhalanxBolt_C"),TEXT("/Game/Model/Bolt/Phalanx/MI_PhalanxBolt.MI_PhalanxBolt"),TEXT("/Game/Models/Weapons/PhalanxCannon/BP_PhalanxCannon.BP_PhalanxCannon_C"),TEXT("/Script/StarshatterWars.PhalanxCannonBoltActor")},
        {TEXT("Viking Cannon"),TEXT("/Game/Model/Bolt/Viking/BP_VikingBolt.BP_VikingBolt_C"),TEXT("/Game/Model/Bolt/Viking/MI_VikingBolt.MI_VikingBolt"),TEXT("/Game/Models/Weapons/VikingCannon/BP_VikingCannon.BP_VikingCannon_C"),TEXT("/Script/StarshatterWars.VikingCannonBoltActor")},
        {TEXT("Ursa Cannon"),TEXT("/Game/Model/Bolt/Ursa/BP_UrsaBolt.BP_UrsaBolt_C"),TEXT("/Game/Model/Bolt/Ursa/MI_UrsaBolt.MI_UrsaBolt"),TEXT("/Game/Models/Weapons/UrsaCannon/BP_UrsaCannon.BP_UrsaCannon_C"),TEXT("/Script/StarshatterWars.UrsaCannonBoltActor")},
        {TEXT("Orca Cannon"),TEXT("/Game/Model/Bolt/Orca/BP_OrcaBolt.BP_OrcaBolt_C"),TEXT("/Game/Model/Bolt/Orca/MI_OrcaBolt.MI_OrcaBolt"),TEXT("/Game/Models/Weapons/OrcaCannon/BP_OrcaCannon.BP_OrcaCannon_C"),TEXT("/Script/StarshatterWars.OrcaCannonBoltActor")},
        {TEXT("Orca Inverted"),TEXT("/Game/Model/Bolt/OrcaInverted/BP_OrcaInvertedBolt.BP_OrcaInvertedBolt_C"),TEXT("/Game/Model/Bolt/OrcaInverted/MI_OrcaInvertedBolt.MI_OrcaInvertedBolt"),TEXT("/Game/Models/Weapons/OrcaInverted/BP_OrcaInverted.BP_OrcaInverted_C"),TEXT("/Script/StarshatterWars.OrcaInvertedBoltActor")},
        {TEXT("Orca 2 Cannon"),TEXT("/Game/Model/Bolt/Orca2/BP_Orca2Bolt.BP_Orca2Bolt_C"),TEXT("/Game/Model/Bolt/Orca2/MI_Orca2Bolt.MI_Orca2Bolt"),TEXT("/Game/Models/Weapons/Orca2Cannon/BP_Orca2Cannon.BP_Orca2Cannon_C"),TEXT("/Script/StarshatterWars.Orca2CannonBoltActor")},
        {TEXT("Orca 2 Inverted"),TEXT("/Game/Model/Bolt/Orca2Inverted/BP_Orca2InvertedBolt.BP_Orca2InvertedBolt_C"),TEXT("/Game/Model/Bolt/Orca2Inverted/MI_Orca2InvertedBolt.MI_Orca2InvertedBolt"),TEXT("/Game/Models/Weapons/Orca2Inverted/BP_Orca2Inverted.BP_Orca2Inverted_C"),TEXT("/Script/StarshatterWars.Orca2InvertedBoltActor")},
        {TEXT("Vanguard"),TEXT("/Game/Model/Bolt/Vanguard/BP_VanguardBolt.BP_VanguardBolt_C"),TEXT("/Game/Model/Bolt/Vanguard/MI_VanguardBolt.MI_VanguardBolt"),TEXT("/Game/Models/Weapons/Vanguard/BP_Vanguard.BP_Vanguard_C"),TEXT("/Script/StarshatterWars.VanguardBoltActor")},
        {TEXT("Helicon"),TEXT("/Game/Model/Bolt/Helicon/BP_HeliconBolt.BP_HeliconBolt_C"),TEXT("/Game/Model/Bolt/Helicon/MI_HeliconBolt.MI_HeliconBolt"),TEXT("/Game/Models/Weapons/Helicon/BP_Helicon.BP_Helicon_C"),TEXT("/Script/StarshatterWars.HeliconBoltActor")},
        {TEXT("Light-PDB"),TEXT("/Game/Model/Bolt/LightPDB/BP_LightPDBBolt.BP_LightPDBBolt_C"),TEXT("/Game/Model/Bolt/LightPDB/MI_LightPDBBolt.MI_LightPDBBolt"),TEXT("/Game/Models/Weapons/LightPDB/BP_LightPDB.BP_LightPDB_C"),TEXT("/Script/StarshatterWars.LightPDBBoltActor")},
        {TEXT("Rail-5"),TEXT("/Game/Model/Bolt/Rail5/BP_Rail5Bolt.BP_Rail5Bolt_C"),TEXT("/Game/Model/Bolt/Rail5/MI_Rail5Bolt.MI_Rail5Bolt"),TEXT("/Game/Models/Weapons/Rail5/BP_Rail5.BP_Rail5_C"),TEXT("/Script/StarshatterWars.Rail5BoltActor")},
        {TEXT("Rail-12"),TEXT("/Game/Model/Bolt/Rail12/BP_Rail12Bolt.BP_Rail12Bolt_C"),TEXT("/Game/Model/Bolt/Rail12/MI_Rail12Bolt.MI_Rail12Bolt"),TEXT("/Game/Models/Weapons/Rail12/BP_Rail12.BP_Rail12_C"),TEXT("/Script/StarshatterWars.Rail12BoltActor")},
        {TEXT("Rail-12 Inverted"),TEXT("/Game/Model/Bolt/Rail12Inverted/BP_Rail12InvertedBolt.BP_Rail12InvertedBolt_C"),TEXT("/Game/Model/Bolt/Rail12Inverted/MI_Rail12InvertedBolt.MI_Rail12InvertedBolt"),TEXT("/Game/Models/Weapons/Rail12Inverted/BP_Rail12Inverted.BP_Rail12Inverted_C"),TEXT("/Script/StarshatterWars.Rail12InvertedBoltActor")},
        {TEXT("Rail-15"),TEXT("/Game/Model/Bolt/Rail15/BP_Rail15Bolt.BP_Rail15Bolt_C"),TEXT("/Game/Model/Bolt/Rail15/MI_Rail15Bolt.MI_Rail15Bolt"),TEXT("/Game/Models/Weapons/Rail15/BP_Rail15.BP_Rail15_C"),TEXT("/Script/StarshatterWars.Rail15BoltActor")},
        {TEXT("Mag-18"),TEXT("/Game/Model/Bolt/Mag18/BP_Mag18Bolt.BP_Mag18Bolt_C"),TEXT("/Game/Model/Bolt/Mag18/MI_Mag18Bolt.MI_Mag18Bolt"),TEXT("/Game/Models/Weapons/Mag18/BP_Mag18.BP_Mag18_C"),TEXT("/Script/StarshatterWars.Mag18BoltActor")},
        {TEXT("AAA Cannon"),TEXT("/Game/Model/Bolt/AAA/BP_AAABolt.BP_AAABolt_C"),TEXT("/Game/Model/Bolt/AAA/MI_AAABolt.MI_AAABolt"),TEXT("/Game/Models/Weapons/AAACannon/BP_AAACannon.BP_AAACannon_C"),TEXT("/Script/StarshatterWars.AAACannonBoltActor")},
        {TEXT("Quad AAA"),TEXT("/Game/Model/Bolt/QuadAAA/BP_QuadAAABolt.BP_QuadAAABolt_C"),TEXT("/Game/Model/Bolt/QuadAAA/MI_QuadAAABolt.MI_QuadAAABolt"),TEXT("/Game/Models/Weapons/QuadAAA/BP_QuadAAA.BP_QuadAAA_C"),TEXT("/Script/StarshatterWars.QuadAAABoltActor")},
    };
    for(const auto& Row:Rows)
    {
        UClass* Expected=LoadClass<ABoltWeaponActor>(nullptr,Row.BP);
        UClass* Resolved=ABoltWeaponActor::BlueprintClassForWeapon(Row.Name).Get();
        if(!TestNotNull(Row.Name,Expected))continue;
        TestEqual(TEXT("Weapon resolves its own BP"),Resolved,Expected);
        FSoftClassPath OldBP(Row.OldBP); OldBP.FixupCoreRedirects();
        TestEqual(TEXT("Serialized Blueprint class reference redirects"),OldBP.TryLoadClass<ABoltWeaponActor>(),Expected);
        FSoftClassPath OldNative(Row.OldNative); OldNative.FixupCoreRedirects();
        TestEqual(TEXT("Serialized native class reference redirects"),OldNative.TryLoadClass<ABoltWeaponActor>(),ABoltWeaponActor::ClassForWeapon(Row.Name).Get());
        const FString OldAsset=FString(Row.OldBP).LeftChop(2);
        FSoftObjectPath OldObject(OldAsset); OldObject.FixupCoreRedirects();
        const auto* RedirectedAsset=Cast<UBlueprint>(OldObject.TryLoad());
        if(TestNotNull(TEXT("Old Blueprint asset redirects"),RedirectedAsset))
            TestEqual(TEXT("Old asset resolves the new class"),RedirectedAsset->GeneratedClass.Get(),Expected);
        auto* Bolt=World->SpawnActor<ABoltWeaponActor>(Resolved,FVector(100,200,300),FRotator(0,90,0));
        if(!TestNotNull(TEXT("World projectile spawns"),Bolt))continue;
        TestEqual(TEXT("Exact legacy definition retained"),Bolt->WeaponDefinitionName,FString(Row.Name));
        const auto* Mesh=Bolt->FindComponentByClass<UStaticMeshComponent>();
        if(TestNotNull(TEXT("Sphere core exists"),Mesh))
            TestEqual(TEXT("Construction applies the per-bolt material"),Mesh->GetMaterial(0),LoadObject<UMaterialInterface>(nullptr,Row.MI));
        Bolt->BeginSimulationFlight();
        TestFalse(TEXT("Simulation-driven bolt has no independent tick"),Bolt->IsActorTickEnabled());
        TestFalse(TEXT("UE collision cannot duplicate simulation damage"),Bolt->GetActorEnableCollision());
        TestNull(TEXT("Fired projectile is not attached to a hardpoint"),Bolt->GetAttachParentActor());
        TestTrue(TEXT("Simulation flight leaves the spawn pose intact"),Bolt->GetActorLocation().Equals(FVector(100,200,300)));
        Bolt->Destroy();
    }
    TestEqual(TEXT("Unknown weapon falls back safely"),ABoltWeaponActor::BlueprintClassForWeapon(TEXT("Unknown")).Get(),ABoltWeaponActor::StaticClass());
    return true;
}
#endif
