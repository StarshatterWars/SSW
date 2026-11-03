#include "BoltTurretValidationRigActor.h"
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
#include "BoltWeaponActor.h"
#include "TurretActor.h"
#include "TurretCatalog.h"
#include "Weapon.h"
class FRangeAimWeapon : public Weapon {
public: FRangeAimWeapon(WeaponDesign* D,FVector* P):Weapon(D,D->nbarrels>0?0:1,P,0,0) {}
 void Aim(double Az,double El){aim_cam=Camera();aim_cam.Yaw(Az);aim_cam.Pitch(El);SetAzimuth(Az);SetElevation(El);}
};
static FVector ToDisplay(const FVector& P){return FVector(P.Z,P.X,P.Y);}
static FVector ToLegacy(const FVector& P){return FVector(P.Y,P.Z,P.X);}
struct FBoltTurretValidationState {
 struct FResult{FString Name;int Hits=0,Shots=0;TArray<int32> BarrelShots;double Damage=0,MaxMuzzleError=0,MaxAimError=0;bool IsBolt=false,HasTurret=false,Mesh=false,Visual=false;};
 struct FLane{int Index=0;FVector Start;TUniquePtr<Ship> Target;TUniquePtr<SimShot> Shot;TUniquePtr<FRangeAimWeapon> Weapon;TWeakObjectPtr<ABoltWeaponActor> Bolt;TWeakObjectPtr<ABoltWeaponActor> Swatch;TWeakObjectPtr<ATurretActor> Turret;TWeakObjectPtr<UTextRenderComponent> Text;TWeakObjectPtr<UStaticMeshComponent> TargetMesh;float Next=0;int ActiveBarrel=0;};
 TArray<FResult> Results;TArray<FLane> Lanes;ACameraActor* Camera=nullptr;ShipDesign* TargetDesign=nullptr;float Time=0,ReportAt=0;int Page=-1;bool Ready=false;
};
ABoltTurretValidationRigActor::ABoltTurretValidationRigActor(){PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));}
ABoltTurretValidationRigActor::~ABoltTurretValidationRigActor(){delete State;}
void ABoltTurretValidationRigActor::BeginPlay(){
 Super::BeginPlay();State=new FBoltTurretValidationState;
 auto* Loader=NewObject<UStarshatterWeaponDesignSubsystem>(GetGameInstance());Loader->LoadWeaponDesigns();
 auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));const auto* Row=Table?Table->FindRow<FShipDesign>(TEXT("TargetDrone"),TEXT("Bolt range"),false):nullptr;if(!Row)return;
 ShipDesignRegistry::RegisterDesign(FName(TEXT("BoltValidationTarget")),*Row);State->TargetDesign=ShipDesignRegistry::FindLegacy(FName(TEXT("BoltValidationTarget")));State->TargetDesign->radius=160;State->TargetDesign->integrity=1000000;
 State->Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(1300,-7200,5400),FRotator::ZeroRotator);State->Camera->SetActorRotation((FVector(1300,0,100)-State->Camera->GetActorLocation()).Rotation());
 auto* C=State->Camera->GetCameraComponent();C->ProjectionMode=ECameraProjectionMode::Perspective;C->FieldOfView=45;C->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;C->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;C->PostProcessSettings.AutoExposureMinBrightness=1;C->PostProcessSettings.AutoExposureMaxBrightness=1;
 auto* Light=NewObject<UPointLightComponent>(this);AddInstanceComponent(Light);Light->SetupAttachment(GetRootComponent());Light->SetIntensity(8000000);Light->SetAttenuationRadius(20000);Light->RegisterComponent();Light->SetWorldLocation(FVector(1000,-2000,2500));
 const TCHAR* Names[]={TEXT("Alpha Cannon"),TEXT("Delta Cannon"),TEXT("Heavy Cannon"),TEXT("Fighter Turret"),TEXT("LCA Turret"),TEXT("Blockade Cannon"),TEXT("Vulcan Cannon"),TEXT("Phalanx Cannon"),TEXT("Viking Cannon"),TEXT("Ursa Cannon"),TEXT("Orca Cannon"),TEXT("Orca Inverted"),TEXT("Orca 2 Cannon"),TEXT("Orca 2 Inverted"),TEXT("Vanguard"),TEXT("Helicon"),TEXT("Light-PDB"),TEXT("Rail-5"),TEXT("Rail-12"),TEXT("Rail-12 Inverted"),TEXT("Rail-15"),TEXT("Mag-18"),TEXT("AAA Cannon"),TEXT("Quad AAA"),TEXT("Zolon Cannon"),TEXT("Plasma Cannon"),TEXT("Rockets"),TEXT("Recon Pod"),TEXT("Rostov"),TEXT("Rostov-Starboard"),TEXT("Graser Turret"),TEXT("SAM-1")};
 for(auto* N:Names){FBoltTurretValidationState::FResult R;R.Name=N;State->Results.Add(R);}
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 for(int I=0;I<4;++I){FBoltTurretValidationState::FLane L;L.Start=FVector((I-1.5)*1150,100,0);
  auto* Mesh=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Mesh);Mesh->SetupAttachment(GetRootComponent());Mesh->SetStaticMesh(Sphere);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->RegisterComponent();Mesh->SetWorldScale3D(FVector(3.2));L.TargetMesh=Mesh;
  auto* Text=NewObject<UTextRenderComponent>(this);AddInstanceComponent(Text);Text->SetupAttachment(GetRootComponent());Text->SetWorldSize(70);Text->SetHorizontalAlignment(EHTA_Center);Text->SetTextRenderColor(FColor::White);Text->RegisterComponent();Text->SetWorldLocation(FVector(1500,L.Start.X,600));Text->SetWorldRotation((State->Camera->GetActorLocation()-Text->GetComponentLocation()).Rotation());L.Text=Text;State->Lanes.Add(MoveTemp(L));
 }State->Ready=true;
}
void ABoltTurretValidationRigActor::Tick(float Seconds){
 Super::Tick(Seconds);if(!State||!State->Ready)return;if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(State->Camera);State->Time+=Seconds;
 const int Pages=(State->Results.Num()+3)/4,Page=int(State->Time/12)%Pages;const bool Changed=State->Page!=Page;State->Page=Page;
 for(int I=0;I<4;++I){auto& L=State->Lanes[I];const int Index=Page*4+I;
  if(Changed){if(L.Swatch.IsValid())L.Swatch->Destroy();L.Swatch.Reset();if(L.Turret.IsValid())L.Turret->Destroy();if(L.Bolt.IsValid())L.Bolt->Destroy();L.Turret.Reset();L.Bolt.Reset();L.Shot.Reset();L.Weapon.Reset();L.Target.Reset();L.Index=Index;L.Next=State->Time+1;
   if(Index<State->Results.Num()){auto& R=State->Results[Index];auto* D=WeaponDesignRegistry::FindLegacy(R.Name);if(!D)continue;R.IsBolt=D->graphic_type==3 && !D->beam;
    
    L.Target=MakeUnique<Ship>("range target","",State->TargetDesign,2,0,nullptr,false);
    if(D->turret.length()>0){auto* T=GetWorld()->SpawnActor<ATurretActor>(TurretCatalog::VisualClass(FName(R.Name)),ToDisplay(L.Start),FRotator::ZeroRotator);L.Turret=T;R.HasTurret=T!=nullptr;if(T){R.Mesh=T->GunMesh&&T->GunMesh->GetStaticMesh();FVector M(0,0,60);if(D->nbarrels==0 && T->GunMesh && T->GunMesh->GetStaticMesh()){const FBox Box=T->GunMesh->GetStaticMesh()->GetBoundingBox();M=FVector(Box.GetCenter().X,Box.GetCenter().Z,Box.Max.Y)*T->LegacyModelScale;}L.Weapon=MakeUnique<FRangeAimWeapon>(D,&M);}}
   }
  }
  if(Index>=State->Results.Num()){L.Text->SetText(FText::GetEmpty());L.TargetMesh->SetVisibility(false);continue;}
  L.TargetMesh->SetVisibility(true);auto& R=State->Results[Index];auto* D=WeaponDesignRegistry::FindLegacy(R.Name);if(!D||!L.Target)continue;
  // A fixture aim frame exercises the same base/gun transform bridge used by ships.
  const double Az=.12*FMath::Sin(State->Time*.6),El=.07*FMath::Cos(State->Time*.6);Camera Aim;Aim.Yaw(Az);Aim.Pitch(El);FVector Start=L.Start;const int BarrelCount=L.Weapon?FMath::Max(1,L.Weapon->GetTurretBarrelCount()):1;const int Barrel=L.Shot?L.ActiveBarrel:R.Shots%BarrelCount;if(R.BarrelShots.Num()!=BarrelCount)R.BarrelShots.Init(0,BarrelCount);
  if(auto* T=L.Turret.Get()){L.Weapon->Aim(Az,El);Camera Identity;T->SyncFromWeaponFrame(*L.Weapon,Identity,FQuat::Identity);FTransform M;
   if(T->GetMuzzleTransform(Barrel,M)){Start=ToLegacy(M.GetLocation());const FVector P=L.Weapon->GetTurretBarrelOffset(Barrel);const FVector Expected=ToDisplay(L.Start+Aim.vrt()*P.X+Aim.vup()*P.Y+Aim.vpn()*P.Z);R.MaxMuzzleError=FMath::Max(R.MaxMuzzleError,FVector::Dist(Expected,M.GetLocation()));R.MaxAimError=FMath::Max(R.MaxAimError,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(M.GetRotation().GetForwardVector(),ToDisplay(Aim.vpn())),-1.,1.))));}}
  const double Distance=FMath::Clamp(double(D->speed)*D->life*.35,500.,2500.);const FVector Target=Start+Aim.vpn()*Distance;
  if(!L.Shot){L.Target->MoveTo(Target);L.TargetMesh->SetWorldLocation(ToDisplay(Target));}
  if(R.IsBolt && !L.Shot && State->Time>=L.Next){L.Shot=MakeUnique<SimShot>(Start,Aim,D,nullptr);L.Shot->SetCharge(D->charge);L.ActiveBarrel=Barrel;++R.BarrelShots[Barrel];++R.Shots;
   auto* B=ABoltWeaponActor::SpawnBolt(GetWorld(),ABoltWeaponActor::BlueprintClassForWeapon(R.Name),ToDisplay(Start),ToDisplay(Aim.vpn()),D->speed,D->life);if(B){B->BeginSimulationFlight();L.Bolt=B;R.Visual=true;}}
  if(L.Shot){double Remaining=FMath::Min(double(Seconds),.1);while(Remaining>0&&L.Shot){const double Step=FMath::Min(Remaining,.005);Remaining-=Step;L.Shot->ExecFrame(Step);FVector Impact;const double Before=L.Target->GetIntegrity();int Hit=L.Target->HitBy(L.Shot.Get(),Impact);R.Damage+=Before-L.Target->GetIntegrity();
    if(Hit||L.Shot->GetLife()<=0){if(Hit)++R.Hits;if(L.Bolt.IsValid())L.Bolt->Destroy();L.Bolt.Reset();L.Shot.Reset();L.Next=State->Time+FMath::Max(.15,double(D->refire_delay));break;}}
   if(L.Shot&&L.Bolt.IsValid())L.Bolt->SetActorLocation(ToDisplay(L.Shot->GetLocation()));}
  L.Text->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s | shots %d hits %d\ndamage %.0f aim %.3fdeg"),*R.Name,R.IsBolt?(L.Turret.IsValid()?(D->nbarrels>0?TEXT("BOLT: individual Wep.def barrels (1x game speed, normal size)"):TEXT("BOLT: fixture mesh-tip (1x game speed, normal size)")):TEXT("BOLT: fixed test source (1x game speed, normal size)")):TEXT("TURRET POSE ONLY"),R.Shots,R.Hits,R.Damage,R.MaxAimError)));
 }
 if(State->Time-State->ReportAt>1){State->ReportAt=State->Time;FString J=FString::Printf(TEXT("{\"elapsed\":%.3f,\"page\":%d,\"profiles\":["),State->Time,Page);bool First=true;
  for(const auto& R:State->Results){if(!First)J+=TEXT(",");First=false;J+=FString::Printf(TEXT("{\"weapon\":\"%s\",\"bolt\":%s,\"turret\":%s,\"mesh\":%s,\"visual\":%s,\"shots\":%d,\"hits\":%d,\"damage\":%.3f,\"muzzleError\":%.6f,\"aimErrorDegrees\":%.6f}"),*R.Name,R.IsBolt?TEXT("true"):TEXT("false"),R.HasTurret?TEXT("true"):TEXT("false"),R.Mesh?TEXT("true"):TEXT("false"),R.Visual?TEXT("true"):TEXT("false"),R.Shots,R.Hits,R.Damage,R.MaxMuzzleError,R.MaxAimError);J.RemoveAt(J.Len()-1);J+=TEXT(",\"barrelShots\":[");for(int B=0;B<R.BarrelShots.Num();++B){if(B)J+=TEXT(",");J+=FString::FromInt(R.BarrelShots[B]);}J+=TEXT("]}");}
  J+=TEXT("]}");FFileHelper::SaveStringToFile(J,*(FPaths::ProjectSavedDir()/TEXT("BoltTurretLiveResults.json")));}
}

