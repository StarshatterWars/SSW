#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "BoltWeaponActor.h"
#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltPhotonVisibilityTest,"Starshatter.Weapons.BoltPhotonVisibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBoltPhotonVisibilityTest::RunTest(const FString&)
{
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);if(!TestNotNull(TEXT("World"),World))return false;
 const TCHAR* Names[]={TEXT("Alpha Cannon"),TEXT("Delta Cannon"),TEXT("Heavy Cannon"),TEXT("Fighter Turret"),TEXT("LCA Turret"),TEXT("Blockade Cannon"),TEXT("Vulcan Cannon"),TEXT("Phalanx Cannon"),TEXT("Viking Cannon"),TEXT("Ursa Cannon"),TEXT("Orca Cannon"),TEXT("Orca Inverted"),TEXT("Orca 2 Cannon"),TEXT("Orca 2 Inverted"),TEXT("Vanguard"),TEXT("Helicon"),TEXT("Light-PDB"),TEXT("Rail-5"),TEXT("Rail-12"),TEXT("Rail-12 Inverted"),TEXT("Rail-15"),TEXT("Mag-18"),TEXT("AAA Cannon"),TEXT("Quad AAA"),TEXT("Zolon Cannon"),TEXT("Plasma Cannon")};
 for(auto* Name:Names){auto* B=World->SpawnActor<ABoltWeaponActor>(ABoltWeaponActor::BlueprintClassForWeapon(Name));if(!TestNotNull(Name,B))continue;B->BeginSimulationFlight();
  auto* Glow=B->FindComponentByClass<UMaterialBillboardComponent>();if(!TestNotNull(TEXT("Photon halo inherited by saved Blueprint"),Glow))continue;
  if(auto* Core=B->FindComponentByClass<UStaticMeshComponent>())TestTrue(TEXT("Saved bolt uses compact core scale"),Core->GetRelativeScale3D().Equals(FVector(.04f),.00001));
  TestTrue(TEXT("Saved bolt has no enlarged actor scale"),B->GetActorScale3D().Equals(FVector::OneVector,.00001));
  TestEqual(TEXT("World and screen halos"),Glow->Elements.Num(),2);TestEqual(TEXT("Glow cannot apply damage"),Glow->GetCollisionEnabled(),ECollisionEnabled::NoCollision);TestFalse(TEXT("Simulation controls visual lifetime"),B->IsActorTickEnabled());
  if(Glow->Elements.Num()==2){TestFalse(TEXT("Near halo uses world dimensions"),bool(Glow->Elements[0].bSizeIsInScreenSpace));TestTrue(TEXT("Near halo is scaled back"),Glow->Elements[0].BaseSizeX<=9.01f);TestTrue(TEXT("Distant halo stays readable"),bool(Glow->Elements[1].bSizeIsInScreenSpace));TestTrue(TEXT("Useful screen radius"),Glow->Elements[1].BaseSizeX>=.006f);
   auto* MID=Cast<UMaterialInstanceDynamic>(Glow->GetMaterial(0));if(TestNotNull(TEXT("Per-projectile color instance"),MID)){TestEqual(TEXT("Unlit additive glow"),MID->GetBlendMode(),BLEND_Additive);TestTrue(TEXT("HDR brightness"),MID->K2_GetScalarParameterValue(TEXT("GlowIntensity"))>=8);}}
  B->Destroy();
 }
 World->DestroyWorld(false);return true;
}
#endif
