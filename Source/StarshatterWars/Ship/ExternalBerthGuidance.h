#pragma once
#include "CoreMinimal.h"
// Value-only corridor around the simulation's existing external transfer berth.
// This is guidance, not landing clearance, occupancy or a fictional flight deck.
struct FExternalBerthGuidance
{
    FVector Berth=FVector::ZeroVector;
    FVector Outward=FVector::UpVector;
    FVector Right=FVector::RightVector;
    FVector Up=FVector::ForwardVector;
    double Length=1000,HalfWidth=25;
    bool Initialize(const FVector& Position,const FVector& Away,const FVector& Lateral,double CraftRadius)
    {
        if(Position.ContainsNaN() || Away.ContainsNaN() || Lateral.ContainsNaN() || !FMath::IsFinite(CraftRadius) || CraftRadius<0)return false;
        const FVector Axis=Away.GetSafeNormal();
        const FVector Across=(Lateral-Axis*FVector::DotProduct(Lateral,Axis)).GetSafeNormal();
        if(Axis.IsNearlyZero() || Across.IsNearlyZero())return false;
        Berth=Position;Outward=Axis;Right=Across;Up=FVector::CrossProduct(Outward,Right).GetSafeNormal();
        Length=FMath::Max(1000.0,CraftRadius*8+500);HalfWidth=FMath::Max(25.0,CraftRadius*1.5);return true;
    }
    FVector Center(int32 Index) const {return Berth+Outward*(Length*(FMath::Clamp(Index,0,3)+1)/4.0);}
    bool Corners(int32 Index,FVector (&Points)[4]) const
    {
        if(Index<0 || Index>=4)return false;
        const FVector At=Center(Index);const double Width=HalfWidth*(1.0+.1*Index);
        Points[0]=At-Right*Width-Up*Width;Points[1]=At+Right*Width-Up*Width;
        Points[2]=At+Right*Width+Up*Width;Points[3]=At-Right*Width+Up*Width;return true;
    }
};
