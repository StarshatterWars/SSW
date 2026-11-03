#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "TurretCatalog.h"
#include "TurretActor.h"
#include "ShipWeaponMountComponent.h"
#include "Components/StaticMeshComponent.h"
#include "OrionShipActor.h"
#include "ZodiacShipActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurretGunAttachmentTest,"Starshatter.Weapons.TurretGunAttachments",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTurretGunAttachmentTest::RunTest(const FString&)
{
 const auto Before=WeaponDesignRegistry::GetAll();
 ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
 auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());
 const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadWeaponDesign(Ansi.Get());
 TSet<FName> Seen;
 for(const auto& Pair:WeaponDesignRegistry::GetAll()){
  const auto& Row=Pair.Value;if(Row.Turret.IsEmpty() || Seen.Contains(FName(Row.Name)))continue;
  const FName Type(Row.Name);Seen.Add(Type);
  UClass* Native=TurretCatalog::FindClass(Type).Get();
  UClass* Visual=TurretCatalog::VisualClass(Type).Get();
  if(!TestNotNull(TEXT("Native subclass for Wep.def turret"),Native) || !TestNotNull(TEXT("Visual class"),Visual))continue;
  TestTrue(TEXT("Canonical Blueprint derives from native turret"),Visual->IsChildOf(Native));
  TestTrue(TEXT("Organized Blueprint path"),Visual->GetPathName().StartsWith(TEXT("/Game/Models/Weapons/Turret/")));
 }
 TestEqual(TEXT("All 26 Wep.def turret entries covered"),Seen.Num(),26);
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 if(!TestNotNull(TEXT("Preview world"),World))return false;
 ON_SCOPE_EXIT{World->DestroyWorld(false);};
 for(auto* Ship:TArray<AShipActor*>{World->SpawnActor<AOrionShipActor>(),World->SpawnActor<AZodiacShipActor>()}){
  const int32 Count=Ship->NativeWeaponHardpoints.Num();Ship->RebuildTurretActors();Ship->RebuildTurretActors();
  TestEqual(TEXT("Rebuild preserves station count"),Ship->NativeWeaponHardpoints.Num(),Count);
  int32 Guns=0;
  for(const auto& Point:Ship->NativeWeaponHardpoints){auto* Mount=Cast<UShipWeaponMountComponent>(Point.Get());if(!Mount || Mount->bLoadoutHardpoint || !Mount->AllowedWeaponTypes.Num())continue;
   const auto* Row=WeaponDesignRegistry::Find(Mount->AllowedWeaponTypes[0]);if(!Row || Row->Turret.IsEmpty())continue;
   auto* Gun=Mount->GetTurretVisualActor();if(!TestNotNull(TEXT("Fixed turret attached"),Gun))continue;++Guns;
   TestEqual(TEXT("Definition drives gun model"),Gun->LegacyGunModel,Row->Turret);
   TestTrue(TEXT("Definition scale applied once"),Gun->GunPivot->GetRelativeScale3D().Equals(FVector(Row->Scale)));
   TestFalse(TEXT("Visual has no gameplay collision"),Gun->GetActorEnableCollision());
   TestTrue(TEXT("Inherited mesh editable"),Gun->GunMesh->bEditableWhenInherited);
  }
  TestTrue(TEXT("Ship receives turrets"),Guns>0);
 }
 auto* Owner=World->SpawnActor<AActor>();
 auto* Mount=NewObject<UShipWeaponMountComponent>(Owner);Owner->AddInstanceComponent(Mount);Mount->RegisterComponent();
 Mount->ConfigureTurretVisual(TEXT("LCA Turret"));
 Mount->TurretBlueprint=ATurretActor::StaticClass();Mount->ConfigureTurretVisual(TEXT("Vanguard"));
 TestEqual(TEXT("Manual class picker preserved"),Mount->TurretBlueprint.Get(),ATurretActor::StaticClass());
 Mount->SetChildActorClass(ATurretActor::StaticClass());Mount->ConfigureTurretVisual(TEXT("LCA Turret"));
 TestEqual(TEXT("Explicit child class preserved"),Mount->GetChildActorClass().Get(),ATurretActor::StaticClass());
 Mount->bLoadoutHardpoint=true;Mount->ConfigureTurretVisual(TEXT("Rockets"));
 auto* Launcher=Mount->GetTurretVisualActor();TestNotNull(TEXT("Independent launcher created"),Launcher);
 TestTrue(TEXT("Launcher never replaces carried child"),Mount->GetChildActor()!=Launcher);
 Mount->ConfigureTurretVisual(TEXT("Rockets"));TestTrue(TEXT("Launcher configuration idempotent"),Mount->GetTurretVisualActor()==Launcher);
 Mount->ClearRuntimeWeapon();TestNull(TEXT("Unbind removes launcher"),Mount->GetTurretVisualActor());
 Mount->DestroyComponent();
 return true;
}
#endif
