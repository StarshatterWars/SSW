#include "WaypointActor.h"
#include "WaypointArrival.h"
#include "ProceduralMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
AWaypointActor::AWaypointActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WaypointRoot"));
    ArrivalSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ArrivalSphere"));
    ArrivalSphere->SetupAttachment(RootComponent);
    ArrivalSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalSphere->SetGenerateOverlapEvents(false);
    ArrivalSphere->SetHiddenInGame(true);
    ArrivalSphere->SetSphereRadius(0.0f, false);
    WaypointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaypointMesh"));
    WaypointMesh->SetupAttachment(RootComponent);
    WaypointMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaypointMesh->SetGenerateOverlapEvents(false);
    WaypointMesh->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) WaypointMesh->SetStaticMesh(Sphere.Object);
    WaypointMesh->SetVisibility(false);WaypointMesh->SetHiddenInGame(true);
    WaypointMarker=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaypointMarker"));WaypointMarker->SetupAttachment(RootComponent);WaypointMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);WaypointMarker->SetGenerateOverlapEvents(false);WaypointMarker->SetCastShadow(false);
    WaypointLabel=CreateDefaultSubobject<UTextRenderComponent>(TEXT("WaypointLabel"));WaypointLabel->SetupAttachment(RootComponent);WaypointLabel->SetHorizontalAlignment(EHTA_Center);WaypointLabel->SetVerticalAlignment(EVRTA_TextCenter);WaypointLabel->SetWorldSize(28);WaypointLabel->SetTextRenderColor(FColor(65,255,130));WaypointLabel->SetCastShadow(false);
    // Kept for Blueprint compatibility; the new marker owns the visible shape.
    // Visible for authoring, hidden in gameplay along with its owning actor.
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    bReplicates = false;
    Tags.Add(TEXT("MissionNavigationTarget"));
}

double AWaypointActor::LegacyArrivalRadius(double HullRadius)
{
    return LegacyWaypointArrival::Radius(HullRadius);
}
void AWaypointActor::SetArrivalHullRadius(double HullRadius)
{
    ArrivalRadiusMeters = LegacyArrivalRadius(HullRadius);
    // Display-only volume. Simulation arrival never depends on UE overlaps.
    if (ArrivalSphere) ArrivalSphere->SetSphereRadius(float(ArrivalRadiusMeters), false);
}

FString AWaypointActor::CompactLabel() const{return SectorName.IsEmpty()?FString::Printf(TEXT("NAV %02d"),WaypointIndex):FString::Printf(TEXT("NAV %02d\n%s"),WaypointIndex,*SectorName);}
void AWaypointActor::BuildPlumbob(TArray<FVector>& V,TArray<int32>& T,TArray<FVector>& N,TArray<FVector2D>& UV,TArray<FLinearColor>& C,double H,double W){
 V.Reset();T.Reset();N.Reset();UV.Reset();C.Reset();const FVector Rim[]={FVector(W*.5,0,0),FVector(0,W*.5,0),FVector(-W*.5,0,0),FVector(0,-W*.5,0)};
 for(int Side=0;Side<2;++Side)for(int I=0;I<4;++I){const FVector A(0,0,(Side==0?1:-1)*H*.5),B=Rim[I],D=Rim[(I+1)%4];const FVector P[]={A,Side==0?B:D,Side==0?D:B};const FVector Normal=FVector::CrossProduct(P[1]-P[0],P[2]-P[0]).GetSafeNormal();const int Start=V.Num();const double Shade=.55+.12*I;
 for(int J=0;J<3;++J){V.Add(P[J]);N.Add(Normal);UV.Add(J==0?FVector2D(.5,0):J==1?FVector2D(0,1):FVector2D(1,1));C.Add(FLinearColor(Shade,Shade,Shade,1));T.Add(Start+J);}}
}
void AWaypointActor::RebuildMarker(){TArray<FVector> V,N;TArray<int32>T;TArray<FVector2D>UV;TArray<FLinearColor>C;TArray<FProcMeshTangent>Tangents;BuildPlumbob(V,T,N,UV,C,MarkerHeight,MarkerWidth);WaypointMarker->CreateMeshSection_LinearColor(0,V,T,N,UV,C,Tangents,false);WaypointLabel->SetRelativeLocation(FVector(0,0,-MarkerHeight*.5-44));WaypointMesh->SetVisibility(false);WaypointMesh->SetHiddenInGame(true);}
void AWaypointActor::OnConstruction(const FTransform& T){Super::OnConstruction(T);RebuildMarker();RefreshAppearance(!IsHidden());}
void AWaypointActor::RefreshAppearance(bool Visible){
 SetActorHiddenInGame(!Visible);SetActorEnableCollision(false);WaypointMesh->SetVisibility(false);WaypointMesh->SetHiddenInGame(true);
 WaypointLabel->SetText(FText::FromString(CompactLabel()));WaypointLabel->SetVisibility(Visible && bShowWorldLabel);WaypointMarker->SetVisibility(Visible);
 if(!MarkerMaterial){if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Models/Waypoint/M_WaypointMarker.M_WaypointMarker"))){MarkerMaterial=UMaterialInstanceDynamic::Create(Base,this);WaypointMarker->SetMaterial(0,MarkerMaterial);}}
 if(MarkerMaterial){MarkerMaterial->SetVectorParameterValue(TEXT("MarkerColor"),FLinearColor(.18f,1.f,.4f));MarkerMaterial->SetScalarParameterValue(TEXT("Intensity"),bNextWaypoint?2.5f:1.2f);MarkerMaterial->SetScalarParameterValue(TEXT("Opacity"),bNextWaypoint?.85f:.35f);}
 WaypointLabel->SetTextRenderColor(bNextWaypoint?FColor(125,255,175):FColor(50,150,85));
}
void AWaypointActor::Tick(float Delta){Super::Tick(Delta);if(IsHidden())return;WaypointMarker->AddLocalRotation(FRotator(0,18*Delta,0));if(auto* W=GetWorld())if(auto* PC=W->GetFirstPlayerController())if(PC->PlayerCameraManager)WaypointLabel->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation()-WaypointLabel->GetComponentLocation()).Rotation());}
