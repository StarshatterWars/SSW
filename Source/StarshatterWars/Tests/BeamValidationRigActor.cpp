#include "BeamValidationRigActor.h"
#include "BeamWeaponActor.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesign.h"
#include "WeaponDesignRegistry.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "SimShot.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
struct FBeamValidationState
{
 struct FLane{FString Name;FVector Start;TUniquePtr<Ship> Target;TUniquePtr<SimShot> Shot;TWeakObjectPtr<ABeamWeaponActor> Visual;TWeakObjectPtr<UTextRenderComponent> Text;double Lost=0,Error=0;int Hits=0;};
 TArray<FLane> Lanes;float Time=0,ReportAt=0;bool Ready=false;ACameraActor* Camera=nullptr;
};
ABeamValidationRigActor::ABeamValidationRigActor(){PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));}
ABeamValidationRigActor::~ABeamValidationRigActor(){delete State;}
void ABeamValidationRigActor::BeginPlay()
{
 Super::BeginPlay();State=new FBeamValidationState;
 auto* Loader=NewObject<UStarshatterWeaponDesignSubsystem>(GetGameInstance());Loader->LoadWeaponDesigns();
 auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));const auto* Row=Table?Table->FindRow<FShipDesign>(TEXT("TargetDrone"),TEXT("Beam live test"),false):nullptr;
 if(!Row){UE_LOG(LogTemp,Error,TEXT("[BEAM LIVE] TargetDrone definition missing"));return;}
 ShipDesignRegistry::RegisterDesign(FName(TEXT("BeamValidationTarget")),*Row);auto* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("BeamValidationTarget")));Design->radius=160;Design->integrity=1000000;
 State->Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(2000,-7300,5600),FRotator::ZeroRotator);State->Camera->SetActorRotation((FVector(2000,0,100)-State->Camera->GetActorLocation()).Rotation());
 auto* C=State->Camera->GetCameraComponent();C->ProjectionMode=ECameraProjectionMode::Orthographic;C->OrthoWidth=6500;C->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;C->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;C->PostProcessSettings.AutoExposureMinBrightness=1;C->PostProcessSettings.AutoExposureMaxBrightness=1;
 auto* Light=NewObject<UPointLightComponent>(this);AddInstanceComponent(Light);Light->SetupAttachment(GetRootComponent());Light->SetIntensity(4000000);Light->SetAttenuationRadius(20000);Light->RegisterComponent();Light->SetWorldLocation(FVector(2000,-3000,3000));
 UStaticMesh* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 const TCHAR* Names[]={TEXT("XRay Laser"),TEXT("Gamma Laser"),TEXT("Graser Turret"),TEXT("Zolon Laser")};
 for(int I=0;I<4;++I){
  FBeamValidationState::FLane L;L.Name=Names[I];L.Start=FVector((I-1.5)*1250,100,0);L.Target=MakeUnique<Ship>(TCHAR_TO_ANSI(*L.Name),"",Design,2,0,nullptr,false);L.Target->MoveTo(L.Start+FVector(0,0,4000));
  for(int End=0;End<2;++End){auto* Mesh=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Mesh);Mesh->SetupAttachment(GetRootComponent());Mesh->SetStaticMesh(Sphere);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->RegisterComponent();Mesh->SetWorldLocation(FVector(End?4000:0,L.Start.X,100));Mesh->SetWorldScale3D(FVector(End?3.2:.5));}
  auto* Text=NewObject<UTextRenderComponent>(this);AddInstanceComponent(Text);Text->SetupAttachment(GetRootComponent());Text->SetWorldSize(85);Text->SetHorizontalAlignment(EHTA_Center);Text->SetTextRenderColor(FColor::White);Text->RegisterComponent();Text->SetWorldLocation(FVector(2000,L.Start.X,500));Text->SetWorldRotation((State->Camera->GetActorLocation()-Text->GetComponentLocation()).Rotation());L.Text=Text;State->Lanes.Add(MoveTemp(L));
 }
 State->Ready=true;
}
void ABeamValidationRigActor::Tick(float Seconds)
{
 Super::Tick(Seconds);if(!State || !State->Ready)return;
 if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(State->Camera);
 State->Time+=Seconds;const float Phase=FMath::Fmod(State->Time,10.f);const bool Fire=Phase>=2 && Phase<7;const double Step=FMath::Min(double(Seconds),.05);
 for(auto& L:State->Lanes){
  if(Fire){
   auto* D=WeaponDesignRegistry::FindLegacy(L.Name);if(!L.Shot || L.Shot->GetLife()<=0){Camera Aim;L.Shot=MakeUnique<SimShot>(L.Start,Aim,D,nullptr);L.Shot->SetCharge(D->charge);}
   L.Shot->SetBeamPoints(L.Start,L.Start+FVector(0,0,50000));L.Shot->ExecFrame(Step);FVector Impact;
   const double Before=L.Target->GetIntegrity();const int Hit=L.Target->HitBy(L.Shot.Get(),Impact);L.Lost+=Before-L.Target->GetIntegrity();if(Hit)++L.Hits;
   const FVector Start(L.Start.Z,L.Start.X,L.Start.Y),End(L.Shot->GetLocation().Z,L.Shot->GetLocation().X,L.Shot->GetLocation().Y);
   auto* V=L.Visual.Get();if(!V){V=ABeamWeaponActor::SpawnBeam(GetWorld(),ABeamWeaponActor::BlueprintClassForWeapon(L.Name),Start,End);V->BeginSimulationBeam(L.Name);L.Visual=V;}
   V->SetBeamPoints(Start,End);L.Error=FMath::Max(L.Error,FMath::Abs(End.X-3840.0));
  }else{
   if(L.Visual.IsValid())L.Visual->Destroy();L.Visual.Reset();L.Shot.Reset();if(Phase<2)L.Target->SetIntegrity(1000000);
  }
  if(L.Text.IsValid())L.Text->SetText(FText::FromString(FString::Printf(TEXT("%s\nHP %.0f | hits %d\nimpact error %.3f"),*L.Name,L.Target->GetIntegrity(),L.Hits,L.Error)));
 }
 if(State->Time-State->ReportAt>=1){
  State->ReportAt=State->Time;
  FString Text=FString::Printf(TEXT("{\"elapsed\":%.3f,\"phase\":%.3f,\"weapons\":["),State->Time,Phase);
  bool First=true;
  for(const auto& L:State->Lanes){if(!First)Text+=TEXT(",");First=false;
   Text+=FString::Printf(TEXT("{\"weapon\":\"%s\",\"health\":%.3f,\"damageApplied\":%.3f,\"hits\":%d,\"impactError\":%.6f}"),*L.Name,L.Target->GetIntegrity(),L.Lost,L.Hits,L.Error);
  }
  Text+=TEXT("]}");FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("BeamLiveResults.json")));

 }
}
