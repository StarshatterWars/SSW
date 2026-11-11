#include "NativeHUDSymbology.h"
#include "Ship.h"
#include "ShipActor.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "WeaponDesign.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "SimSystem.h"
#include "HUDView.h"
#include "SimContact.h"
#include "FighterHUDDetails.h"
#include "SSWSessionLobbyGameMode.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"
#include "Styling/CoreStyle.h"
FNativeHUDSymbology::FNativeHUDSymbology(){const TCHAR* Names[]={TEXT("Cross"),TEXT("Cross1"),TEXT("Cross2"),TEXT("Cross3"),TEXT("Cross4"),TEXT("lead"),TEXT("CompassRing"),TEXT("CompassPointer"),TEXT("CompassRing1")};for(int I=0;I<9;++I){Textures[I].Reset(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/UI/HUD/%s.%s"),Names[I],Names[I])));Brushes[I].SetResourceObject(Textures[I].Get());Brushes[I].DrawAs=ESlateBrushDrawType::Image;}}
int32 FNativeHUDSymbology::CrosshairIndex(int32 Role,double Range,double Max,bool HasPrimary){return Role==1?(!HasPrimary||Range>Max?4:1):FMath::Clamp(Role,2,3);}
double FNativeHUDSymbology::CompassDelta(double Current,double Ordered){return FMath::FindDeltaAngleDegrees(Current,Ordered);}
bool FNativeHUDSymbology::HelmCueVisible(double H,double O,double P,double OP){return FMath::Abs(CompassDelta(H,O))>5||FMath::Abs(OP-P)>5;}
FVector FNativeHUDSymbology::LeadPoint(const FVector& P,const FVector& V,const FVector& F,const FVector& T,const FVector& TV,double Speed,bool LCOS){const FVector ShotVelocity=V+F.GetSafeNormal()*Speed;const double S=ShotVelocity.Size();if(S<=SMALL_NUMBER)return T;const double Time=FVector::Distance(P,T)/S;return LCOS?P+ShotVelocity*Time-TV*Time:T+(TV-V)*Time;}
void FNativeHUDSymbology::Refresh(Ship* P){
 Markers.Reset();Controller.Reset();if(!P)return;auto* Actor=P->GetNativePresentation();if(!IsValid(Actor)||!Actor->GetWorld())return;Controller=Actor->GetWorld()->GetFirstPlayerController();auto* S=Sim::GetSim();auto* R=S?S->GetActiveRegion():nullptr;if(!R)return;
 const auto Convert=[](const FVector& V){return FVector(V.Z,V.X,V.Y);};const FVector Offset=Actor->GetActorLocation()-Convert(P->GetLocation());TSet<SimObject*> Used;
 auto Valid=[&](SimObject* T){if(!T)return false;for(int I=0;I<R->GetShips().size();++I)if(R->GetShips()[I]==T)return true;for(auto* Shot:R->GetProjectiles())if(Shot==T)return true;return false;};
 auto Add=[&](SimObject* T,SimSystem* Sub,int Role){if(!Valid(T)||Used.Contains(T)||T->GetLife()==0)return;Used.Add(T);FVector Position=T->GetLocation();AShipActor* TargetActor=nullptr;
 if(auto* TargetShip=dynamic_cast<Ship*>(T)){TargetActor=TargetShip->GetNativePresentation();if(Sub&&TargetShip->GetSystems().contains(Sub))Position=Sub->GetMountLocation();}
 FVector TargetOffset=Offset;if(IsValid(TargetActor))TargetOffset=TargetActor->GetActorLocation()-Convert(T->GetLocation());
 const auto* D=P->GetPrimaryDesign();FMarker M;M.Position=Convert(Position)+TargetOffset;M.Actor=TargetActor;M.Cross=CrosshairIndex(Role,FVector::Distance(P->GetLocation(),T->GetLocation()),D?D->max_range:0,D!=nullptr);int IFF=0;if(auto* TS=dynamic_cast<Ship*>(T)){const auto* Contact=P->FindContact(TS);IFF=Contact?Contact->GetIFF(P):TS->GetIFF();}else if(auto* Shot=dynamic_cast<SimShot*>(T))IFF=Shot->GetIFF();M.Color=SFighterHUDDetails::IdentificationColor(IFF,P->GetIFF());Markers.Add(M);
 if(Role==1&&!P->IsStarship()&&D&&FVector::Distance(P->GetLocation(),T->GetLocation())<=D->max_range){FMarker Lead=M;Lead.Actor.Reset();Lead.Cross=HUDView::GetGunsight()==0?0:5;Lead.Position=Convert(LeadPoint(P->GetLocation(),P->GetVelocity(),P->GetHeading(),T->GetLocation(),T->GetVelocity(),D->speed,HUDView::GetGunsight()==0))+TargetOffset;Lead.Color=FLinearColor(.3,.85,1);Markers.Add(Lead);}
 };
 if(!P->IsStarship())Add(P->GetTarget(),P->GetSubTarget(),1);
 else {int Other=3;for(int I=0;I<P->GetWeapons().size();++I){auto* Group=P->GetWeapons()[I];if(!Group)continue;int Role=Group->Contains(P->GetPrimary())?1:Group->Contains(P->GetSecondary())?2:Other;auto* T=Group->GetTarget();if(Role==1&&!T)T=P->GetTarget();if(Role>3)continue;const int Before=Used.Num();Add(T,Group->GetSubTarget(),Role);if(Role==3&&Used.Num()>Before)++Other;}}
}
void FNativeHUDSymbology::RefreshNetwork(APlayerController* PlayerController,
    const FSSWReplicatedShipState* Player, const FSSWReplicatedShipState* Target)
{
    Controller = PlayerController;
    Markers.Reset();
    if (!Player || !Target) return;

    const double Range = FVector::Dist(Player->Position, Target->Position);
    const bool bHasPrimary = Player->PrimaryShotSpeed > 0;
    FMarker& Reticle = Markers.AddDefaulted_GetRef();
    Reticle.Position = Target->VisualPosition;
    Reticle.Cross = CrosshairIndex(Player->bMissileLocked ? 2 : 1, Range, Player->PrimaryRange, bHasPrimary);
    Reticle.Color = SFighterHUDDetails::IdentificationColor(Target->IFF, Player->IFF);
    if (!bHasPrimary || Range > Player->PrimaryRange) return;

    const bool bLCOS = HUDView::GetGunsight() == 0;
    const FVector Aim = LeadPoint(Player->Position, Player->Velocity, Player->Heading,
        Target->Position, Target->Velocity, Player->PrimaryShotSpeed, bLCOS);
    const FVector Offset = Aim - Target->Position;
    FMarker& Lead = Markers.AddDefaulted_GetRef();
    Lead.Position = Target->VisualPosition + FVector(Offset.Z, Offset.X, Offset.Y);
    Lead.Cross = bLCOS ? 0 : 5;
    Lead.Color = FLinearColor(.3f, .85f, 1.f);
}

void FNativeHUDSymbology::Art(const FGeometry& G,FSlateWindowElementList& E,int32 L,int32 I,const FVector2D& Center,double Size,const FLinearColor& Color,double Angle)const{if(!Textures[I].IsValid())return;const FVector2D Extent(Size,Size);FSlateDrawElement::MakeRotatedBox(E,L,G.ToPaintGeometry(FVector2f(Extent),FSlateLayoutTransform(FVector2f(Center-Extent*.5))),&Brushes[I],ESlateDrawEffect::None,float(Angle),TOptional<FVector2D>(),FSlateDrawElement::RelativeToElement,Color);}
void FNativeHUDSymbology::Paint(const FGeometry& G,FSlateWindowElementList& E,int32 L,bool Mini,AActor* Selected,bool DrawWeapons)const{
 if(!DrawWeapons)return;auto* PC=Controller.Get();if(!PC)return;auto* LP=PC->GetLocalPlayer();FSceneViewProjectionData Projection;if(!LP||!LP->ViewportClient||!LP->ViewportClient->Viewport||!LP->GetProjectionData(LP->ViewportClient->Viewport,Projection))return;int W,H;PC->GetViewportSize(W,H);if(W<=0||H<=0)return;const FVector2D Size=G.GetLocalSize();const FIntRect R=Projection.GetConstrainedViewRect();
 if(!Mini)Art(G,E,L,0,Size*.5,LeadSize*Size.X/W,FLinearColor(.25,.7,1,.5));
 for(const auto& M:Markers){if(M.Actor.IsValid()&&M.Actor.Get()==Selected)continue;const FVector4 Clip=Projection.ComputeViewProjectionMatrix().TransformFVector4(FVector4(M.Position,1));if(Clip.W<=0)continue;const double X=Clip.X/Clip.W,Y=Clip.Y/Clip.W;if(FMath::Abs(X)>1||FMath::Abs(Y)>1)continue;const FVector2D Point((R.Min.X+(X*.5+.5)*R.Width())*Size.X/W,(R.Min.Y+(.5-Y*.5)*R.Height())*Size.Y/H);Art(G,E,L+1,M.Cross,Point,(M.Cross==0||M.Cross==5?LeadSize:TargetSize)*Size.X/W,M.Color);}
}
void FNativeHUDSymbology::PaintCompass(const FGeometry& G,FSlateWindowElementList& E,int32 L,double Heading,double Ordered,double Pitch,double OrderedPitch,bool Helm,bool Mini,double Roll)const{
 const FVector2D Size=G.GetLocalSize();const double Scale=FMath::Min(Size.X/1280,Size.Y/720);const FVector2D Center=Mini?FVector2D(Size.X*.5-274*Scale,70*Scale):FVector2D(Size.X-82*Scale,94*Scale);const double D=(Mini?80:104)*Scale;const FLinearColor Tint(.45,.85,1,.85);
 if(Mini){
  // Three gimbals share a fixed ship reference. Only presentation uses these angles.
  const FVector2D C(Size.X*.5-274*Scale,64*Scale);
  const FLinearColor Cyan(.25,.85,1,.95),Amber(1,.68,.2,.95),Violet(.76,.48,1,.95);
  auto Line=[&](const FVector2D& A,const FVector2D& B,FLinearColor Color,float Width=1.f){TArray<FVector2D> P;P.Add(A);P.Add(B);FSlateDrawElement::MakeLines(E,L+2,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width*Scale);};
  // Keep the original numbered heading artwork and its fixed reference pointer.
  Art(G,E,L,6,C,90*Scale,Cyan,FMath::DegreesToRadians(180-Heading));
  Art(G,E,L+1,7,C,90*Scale,FLinearColor(.9,1,1,.8),0);
  auto Curve=[&](const TArray<FVector2D>& Points,FLinearColor Color){FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,float(Scale));};
  auto PitchPoint=[&](double V,double Radius){const double A=FMath::DegreesToRadians((V-FMath::Clamp(Pitch,-15.0,15.0))*8-90);return C+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius*Scale;};
  TArray<FVector2D> Circle;for(int I=0;I<=96;++I){const double A=2*PI*I/96;Circle.Add(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*32*Scale);}Curve(Circle,Amber.CopyWithNewOpacity(.6));
  for(int V=-15;V<=15;V+=5){Line(PitchPoint(V,32),PitchPoint(V,28),Amber,1.4);

  }
  Line(C+FVector2D(0,-35)*Scale,C+FVector2D(0,-29)*Scale,FLinearColor::White,1.5);
  // Orthographic wire globe: meridians and parallels bank with the actual roll.
  const double Bank=FMath::DegreesToRadians(-Roll);
  auto GlobePoint=[&](double Latitude,double Longitude){const double X=FMath::Cos(Latitude)*FMath::Sin(Longitude),Y=FMath::Sin(Latitude);return C+FVector2D(X*FMath::Cos(Bank)-Y*FMath::Sin(Bank),X*FMath::Sin(Bank)+Y*FMath::Cos(Bank))*17*Scale;};
  Circle.Reset();for(int I=0;I<=96;++I){const double A=2*PI*I/96;Circle.Add(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*17*Scale);}Curve(Circle,Violet);
  for(int Lat=-60;Lat<=60;Lat+=30){TArray<FVector2D> Points;for(int I=-90;I<=90;I+=5)Points.Add(GlobePoint(FMath::DegreesToRadians(double(Lat)),FMath::DegreesToRadians(double(I))));Curve(Points,Violet.CopyWithNewOpacity(Lat==0?.95:.5));}
  for(int Lon=-60;Lon<=60;Lon+=30){TArray<FVector2D> Points;for(int I=-90;I<=90;I+=5)Points.Add(GlobePoint(FMath::DegreesToRadians(double(I)),FMath::DegreesToRadians(double(Lon))));Curve(Points,Violet.CopyWithNewOpacity(.65));}
  // Fixed wings retain an unambiguous ship reference against the rotating globe.
  Line(C-FVector2D(6,0)*Scale,C+FVector2D(6,0)*Scale,FLinearColor(.9,.97,1),1.5);
  Line(C-FVector2D(0,4)*Scale,C+FVector2D(0,4)*Scale,FLinearColor(.9,.97,1),1.5);  return;
 } // Source ring has north at the bottom; rotate 180deg before heading rotation.
 Art(G,E,L,6,Center,D,Tint,FMath::DegreesToRadians(180-Heading));
 if(Helm&&HelmCueVisible(Heading,Ordered,Pitch,OrderedPitch)){
  Art(G,E,L+1,7,Center,D,FLinearColor(.25,1,.55,1),FMath::DegreesToRadians(CompassDelta(Heading,Ordered)));
  if(!Mini && FMath::Abs(OrderedPitch-Pitch)>5){const FVector2D PitchCenter=Center-FVector2D(D+12*Scale,0);Art(G,E,L,8,PitchCenter,D,Tint);Art(G,E,L+1,7,PitchCenter,D,FLinearColor(.25,1,.55,1),FMath::DegreesToRadians(OrderedPitch-Pitch));}
  if(FMath::Abs(OrderedPitch-Pitch)>5){const FVector2D P=Center+FVector2D(0,D*.6);const FString Caption=FString::Printf(TEXT("PITCH %+.0f"),OrderedPitch);FSlateDrawElement::MakeText(E,L+1,G.ToPaintGeometry(FVector2f(D+40*Scale,16*Scale),FSlateLayoutTransform(FVector2f(P-FVector2D(D*.5,0)))),Caption,FCoreStyle::GetDefaultFontStyle("Bold",FMath::Max(9,int(10*Scale))),ESlateDrawEffect::None,Tint);}
 }else Art(G,E,L+1,7,Center,D,FLinearColor(.9,1,1,.75),0);
}
