#pragma once
#include "CoreMinimal.h"
#include "Layout/Geometry.h"

// Shared local coordinates keep the instrument aligned with its allocated column.
namespace SSWMiniHUDLayout
{
inline FVector2D Origin(FVector2D Size,float Scale)
{return FVector2D(Size.X*.5-330*Scale-16,20*Scale);}
inline FVector2D GizmoCenter(FVector2D Size,float Scale)
{return Origin(Size,Scale)+FVector2D(46,48)*Scale;}
inline FSlateRect DamageImage(FVector2D Size,float Scale)
{
 const FVector2D P=Origin(Size,Scale)+FVector2D(531,23)*Scale;
 return FSlateRect(P.X,P.Y,P.X+108*Scale,P.Y+54*Scale);
}
inline FSlateRect DamagePopup(FVector2D Size,float Scale)
{
 const FVector2D P(FMath::Clamp(Origin(Size,Scale).X+440*Scale,8.0,FMath::Max(8.0,Size.X-428*Scale)),138*Scale);
 return FSlateRect(P.X,P.Y,P.X+420*Scale,P.Y+260*Scale);
}
}
