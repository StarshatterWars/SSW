#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "Misc/ScopeExit.h"
#include "ShipDesignRegistry.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "ShipVisualCatalog.h"
#include "ShipActor.h"
#include "NavLightComponent.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "DataLoader.h"
#include "Game.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "TerrainRegion.h"
#include "OrbitalRegion.h"
#include "QuantumDrive.h"
#include "Ship.h"
#include "Mission.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "Skin.h"
#include "WorldScapeRoot.h"
#include "HeightMapVolume.h"
#include "HeightMapVolumeData.h"
#include "JanekSurfaceGrade.h"
#include "SurfaceFlightRoute.h"
#include "StarshatterGameDataSubsystem.h"
#include "Terrain.h"
#include "Materials/MaterialInterface.h"
namespace {class FTransitFixture : public Sim {public:FTransitFixture():Sim(nullptr){}using Sim::mission;using Sim::active_region;using Sim::star_system;using Sim::regions;};class FPointDrive : public QuantumDrive {public:FPointDrive():QuantumDrive(QUANTUM,100,10){}using QuantumDrive::Jump;};}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceTrainingTransitTest,"Starshatter.Travel.LiveCarrierSurfaceRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSurfaceTrainingTransitTest::RunTest(const FString&)
{
 const auto OldDesigns=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& P:OldDesigns)ShipDesignRegistry::RegisterDesign(P.Key,P.Value,false);};
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
 if(Sim::GetSim()){AddError(TEXT("Travel integration test requires an idle simulation"));return false;}
 auto Owned=MakeUnique<FTransitFixture>();
 auto& S=*Owned;auto* OldMission=S.mission;auto* OldRegion=S.active_region;auto* OldSystem=S.star_system;
 FShipDesign D;D.ShipName=TEXT("Surface Test");D.ShipType=2;D.Category=EShipCategory::Fighter;D.Integrity=100;D.Mass=20;D.Scale=1;
 ShipDesignRegistry::RegisterDesign(FName(TEXT("Surface Test")),D,false);auto* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("Surface Test")));
 StarSystem System("Transit Fixture",FVector::ZeroVector);auto* Planet=new OrbitalBody(&System,"Planet",Orbital::PLANET,6e24,6e6,0);System.AddBody(Planet);
 auto* Orbital=new OrbitalRegion(&System,"Space",0,1000000,6300000,Planet);System.GetRegions().append(Orbital);
 auto* Site=new TerrainRegion(&System,"Surface",100000,Planet);Site->SetSurfaceSiteOffset(FVector(6000000,100000,200000));System.GetRegions().append(Site);
 auto* Space=new SimRegion(&S,Orbital);auto* Ground=new SimRegion(&S,Site);S.regions.append(Space);S.regions.append(Ground);S.star_system=&System;S.active_region=Space;Mission Free(Mission::FreeFlightId);S.mission=&Free;
 ON_SCOPE_EXIT{S.regions.remove(Space);S.regions.remove(Ground);S.active_region=OldRegion;S.star_system=OldSystem;S.mission=OldMission;delete Space;delete Ground;};
 auto* Carrier=S.CreateShip("Carrier","",Design,"Space",FVector(100,200,300),1);auto* Pilot=S.CreateShip("Pilot","",Design,"Space",FVector(110,250,330),1);
 if(!Carrier||!Pilot)return false;Space->SetPlayerShip(Pilot);Pilot->SetFlightPhase(EOPSMode::ACTIVE);
 const FVector Offset=Pilot->GetLocation()-Carrier->GetLocation();const FVector Stop=Site->Location()+Site->SurfaceVectorToOrbital(FVector(0,45000,0))-Space->GetLocation();
 Carrier->SetVelocity(FVector(10,20,30));FPointDrive Drive;Drive.SetShip(Carrier);Drive.SetPointDestination(Space,Stop);Drive.Jump();S.ResolveHyperList();
 TestTrue(TEXT("Precise carrier arrival honors the supplied map point"),Carrier->GetLocation().Equals(OtherHand(Stop),.01));
 TestTrue(TEXT("Carrier stops without a forced 500 m/s departure"),Carrier->GetVelocity().IsNearlyZero());
 TestTrue(TEXT("Nearby fighter stays with its carrier at the original offset"),(Pilot->GetLocation()-Carrier->GetLocation()).Equals(Offset,.01));
 const FVector Local(230,34000,780),LocalVelocity(17,-140,28);
 const FVector EntryMap=Site->Location()+Site->SurfaceVectorToOrbital(Local)-Space->GetLocation();Pilot->MoveTo(OtherHand(EntryMap));Pilot->SetVelocity(OtherHand(Site->SurfaceVectorToOrbital(LocalVelocity)));
 Pilot->DropOrbit();TestTrue(TEXT("Georeferenced descent does not impose the old 10 second blackout"),Pilot->GetTransitionTimeRemaining()<.01);
 Pilot->CompleteTransition();S.ResolveHyperList();
 TestTrue(TEXT("Descent transfers the actual pilot to the surface"),Pilot->GetRegion()==Ground && Ground->GetPlayerShip()==Pilot);
 TestTrue(TEXT("Surface position is continuous at the station tangent origin"),Pilot->GetLocation().Equals(TerrainRegion::JanekStationLocalPosition()+Local,.01));
 TestTrue(TEXT("Descent preserves transformed velocity"),Pilot->GetVelocity().Equals(LocalVelocity,.01));
 Pilot->MoveTo(TerrainRegion::JanekStationLocalPosition()+FVector(230,36000,780));Pilot->SetVelocity(FVector(17,140,28));
 const FVector ExitMap=Site->Location()+Site->SurfaceVectorToOrbital(Pilot->GetLocation()-TerrainRegion::JanekStationLocalPosition())-Space->GetLocation();
 const FVector ExitVelocity=OtherHand(Site->SurfaceVectorToOrbital(Pilot->GetVelocity()));Pilot->MakeOrbit();TestTrue(TEXT("Ascent has no old 5 second blackout"),Pilot->GetTransitionTimeRemaining()<.01);
 Pilot->CompleteTransition();S.ResolveHyperList();
 TestTrue(TEXT("Climb-out restores the same pilot to orbit"),Pilot->GetRegion()==Space && Space->GetPlayerShip()==Pilot);
 TestTrue(TEXT("Climb-out keeps the exact globe-relative position"),Pilot->GetLocation().Equals(OtherHand(ExitMap),.01));
 TestTrue(TEXT("Climb-out preserves transformed velocity"),Pilot->GetVelocity().Equals(ExitVelocity,.01));
 FText Why;TestFalse(TEXT("Unaccepted ground travel cannot bypass clearance"),S.BeginSurfaceTrainingTransit(103,Why));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrionNavLightsTest,"Starshatter.Ships.OrionExteriorNavLights",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOrionNavLightsTest::RunTest(const FString&)
{
 auto* Table=NewObject<UDataTable>();if(!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Table,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json")))return false;
 auto* Row=Table->FindRow<FShipDesign>(TEXT("Orion"),TEXT("Nav beacon audit"),false);if(!Row)return false;
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& P:Old)ShipDesignRegistry::RegisterDesign(P.Key,P.Value,false);};FShipDesign NavOnly=*Row;NavOnly.Weapon.Reset();NavOnly.Hardpoint.Reset();ShipDesignRegistry::RegisterDesign(FName(TEXT("Orion")),NavOnly,false);
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext TestWorldContext1(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
 auto* Class=ShipVisualCatalog::Resolve(TEXT("Orion"),TEXT("Orion"));if(!TestNotNull(TEXT("Orion Blueprint"),Class))return false;
 auto* Actor=World->SpawnActor<AShipActor>(Class);Actor->InitializeSnapshotPropulsion(ShipDesignRegistry::FindLegacy(FName(TEXT("Orion"))));
 TArray<UNavLightComponent*> Lights;Actor->GetComponents(Lights);int32 Red=0,Green=0;
 for(const auto* Lamp:Lights){const auto& Def=Lamp->GetDefinition();const bool R=Def.Color.R>Def.Color.G*2,G=Def.Color.G>Def.Color.R*2;if(!R&&!G)continue;
  Red+=R;Green+=G;TestTrue(TEXT("Exterior colored beacons cast hull-occluded light"),Def.bHullOccluded&&Lamp->CastShadows);TestTrue(TEXT("Beacon illumination stays local to the exterior fixture"),Lamp->AttenuationRadius<=60);
  AddInfo(FString::Printf(TEXT("Orion %s beacon local=%s radius=%.1f"),R?TEXT("RED"):TEXT("GREEN"),*Def.LocalOffset.ToString(),Lamp->AttenuationRadius));
 }
 TestEqual(TEXT("One red port lamp"),Red,1);TestEqual(TEXT("One green starboard lamp"),Green,1);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanekFullMaterialTest,"Starshatter.Planets.JanekFullWorldScapeMaterial",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJanekFullMaterialTest::RunTest(const FString&)
{
 auto* Class=LoadClass<AWorldScapeRoot>(nullptr,TEXT("/Game/Models/Universe/BP_Janek.BP_Janek_C"));if(!TestNotNull(TEXT("Live Janek globe Blueprint"),Class))return false;
 const auto* Default=Class->GetDefaultObject<AWorldScapeRoot>();
 const auto* SurfaceMaterial=Default->TerrainMaterial.DefaultMaterial;
 if(TestNotNull(TEXT("Full terrain material available"),SurfaceMaterial))TestTrue(TEXT("Janek uses the original WorldScape surface material"),SurfaceMaterial->GetPathName().StartsWith(TEXT("/WorldScape/")));
 TestTrue(TEXT("Original ocean remains available with the full material"),Default->bOcean);
 TestNotNull(TEXT("Runway grade heightmap is retained"),LoadObject<UObject>(nullptr,TEXT("/Game/Models/Universe/Planets/Janek/HM_Janek.HM_Janek")));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanekWaterClearanceTest,"Starshatter.Terrain.JanekGradeWaterClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJanekWaterClearanceTest::RunTest(const FString&)
{
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 FScopedSSWTestWorldContext Context(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
 auto* Class=LoadClass<AWorldScapeRoot>(nullptr,TEXT("/Game/Models/Universe/BP_Janek.BP_Janek_C"));
 auto* Globe=Class?World->SpawnActor<AWorldScapeRoot>(Class):nullptr;
 if(!TestNotNull(TEXT("Janek WorldScape instance"),Globe))return false;
 Globe->bGenerateWorldScape=false;Globe->bEnableVolumes=true;Globe->PlanetScale=5400000;Globe->PlanetScaleCode=5400000;
 const FVector Site(0,0,5400542);
 auto* Data=NewObject<UHeightMapVolumeData>(Globe);Data->BaseWidth=Data->BaseHeight=2;
 Data->StoredHeightMapData.Init(0,4);Data->StoredWaterHeightMapData.Init(0,4);Data->bIgnoreHeightMap=false;Data->bIgnoreOceanHeightMap=false;
 auto* Grade=World->SpawnActor<AHeightMapVolume>();Grade->SetActorLocation(Site);Grade->SetActorScale3D(FVector(240));
 Grade->HeightMapData=Data;Grade->Height=true;Grade->HeightAlpha=false;Grade->OverrideHeight=true;Grade->Altitude=540;Grade->MapHeight=0;
 Grade->OceanHeight=true;Grade->OceanHeightAlpha=false;Grade->MinOceanHeight=Grade->MaxOceanHeight=520;
 Grade->UseBlending=false;Grade->Priority=JanekSurfaceGrade::RunwayPriority;Grade->WS_ForceRefresh();
 auto* Lake=World->SpawnActor<AHeightMapVolume>();Lake->SetActorLocation(Site+FVector(8000,0,0));Lake->SetActorScale3D(FVector(180));
 Lake->HeightMapData=Data;Lake->Height=false;Lake->OceanHeight=true;Lake->OceanHeightAlpha=false;
 Lake->MinOceanHeight=Lake->MaxOceanHeight=542;Lake->UseBlending=false;Lake->Priority=JanekSurfaceGrade::LakePriority;Lake->WS_ForceRefresh();
 Globe->HeightMapVolumeList={Lake,Grade};
 for(const FVector Offset:{FVector::ZeroVector,FVector(2000,0,0),FVector(-2000,0,0)})
 {
  const FVector Sample=Site+Offset;
  const double Soil=Globe->GetGroundHeight(Sample,false),Water=Globe->GetGroundHeight(Sample,true);
  AddInfo(FString::Printf(TEXT("Janek site %s soil %.2f ocean %.2f"),*Offset.ToString(),Soil,Water));
  TestTrue(TEXT("Live ground respects the runway grade"),FMath::IsNearlyEqual(Soil,540.0,1.0));
  TestTrue(TEXT("Lake cannot override airport water clearance"),Water<=Soil-15.0);
 }
 // Test T4's authored route against the live planet, not only an analytic hill.
 const auto* Defaults=Class->GetDefaultObject<AWorldScapeRoot>();
 Globe->NoiseIntensity=Defaults->NoiseIntensity * Globe->PlanetScale / Defaults->PlanetScale;
 auto* Catalog=NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());
 FS_CampaignMission T4;
 if(!TestTrue(TEXT("T4 JSON loads for live-planet route check"),Catalog->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/T4.json"),T4)))return false;
 const auto* Player=T4.Element.FindByPredicate([](const auto& E){return E.Player==1;});
 if(!TestNotNull(TEXT("T4 route available"),Player))return false;
 Terrain Ground(nullptr);
 Ground.SetLiveSurface(Globe,Site+FVector(0,25000,0));
 SimRegion Region(nullptr,"Janek Station",SimRegion::REAL_SPACE);
 FVector Previous(25000,0,24);
 for(const auto& Source:Player->Navpoint)
 {
  Instruction Point(TCHAR_TO_UTF8(*Source.OrderRegionName),Source.ObjectiveLocation,Source.Action);
  TestTrue(TEXT("Live T4 waypoint prepared before Autonav"),SurfaceFlightRoute::PrepareWaypoint(Point,&Region,Region.GetName(),Previous,
      [&Ground](double X,double Y){return Ground.Height(-X,Y);}));
  const FVector P=Point.GetLocation();
  const double Height=Ground.Height(-P.X,P.Y);
  AddInfo(FString::Printf(TEXT("T4 %hs ground %.1f waypoint %.1f clearance %.1f"),Instruction::ActionName(Source.Action),Height,P.Z,P.Z-Height));
  TestTrue(TEXT("T4 marker and simulation waypoint clear the live planet"),P.Z>=Height+SurfaceFlightRoute::CruiseClearance-1.0);
 }
 return true;
}
#endif