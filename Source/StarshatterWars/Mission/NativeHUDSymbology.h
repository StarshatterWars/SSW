#pragma once
#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
class Ship;class SimObject;class APlayerController;
// Shared by fighter and capital-ship native HUDs. Values remain in legacy units.
class FNativeHUDSymbology {
public:
 FNativeHUDSymbology();
 static int32 CrosshairIndex(int32 Role,double Range,double PrimaryRange,bool HasPrimary=true);
 static double CompassDelta(double CurrentDegrees,double OrderedDegrees);
 static FVector LeadPoint(const FVector& P,const FVector& V,const FVector& Forward,const FVector& T,const FVector& TV,double Speed,bool bLCOS);
 static bool HelmCueVisible(double Heading,double OrderedHeading,double Pitch,double OrderedPitch);
 static constexpr double TargetSize=64,LeadSize=32;
 void Refresh(Ship* Player);
 void Paint(const FGeometry& G,FSlateWindowElementList& E,int32 Layer,bool Mini,AActor* Selected,bool DrawWeapons)const;
 void PaintCompass(const FGeometry& G,FSlateWindowElementList& E,int32 Layer,double Heading,double OrderedHeading,double Pitch,double OrderedPitch,bool Helm,bool Mini,double RollDegrees=0)const;
private:
 struct FMarker{FVector Position;TWeakObjectPtr<AActor> Actor;int32 Cross=1;FLinearColor Color;};
 TArray<FMarker> Markers;
 TWeakObjectPtr<APlayerController> Controller;
 TStrongObjectPtr<UTexture2D> Textures[9];FSlateBrush Brushes[9];
 void Art(const FGeometry& G,FSlateWindowElementList& E,int32 Layer,int32 Index,const FVector2D& Center,double Size,const FLinearColor& Color,double Rotation=0)const;
};
