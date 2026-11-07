#pragma once
#include "Ship.h"
#include "LandingGear.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
namespace SSWGearIndicator
{
inline bool Contains(FVector2D Point,FVector2D Origin,FVector2D Extent)
{
    const FVector2D Q=(Point-Origin)/Extent;
    return Q.Y>=0 && Q.Y<=1 && Q.X>=0 && Q.X<=1 && Q.Y>=2*FMath::Abs(Q.X-.5);
}
inline void Draw(FSlateWindowElementList& E,const FGeometry& G,int32 Layer,FVector2D P,FVector2D Size,Ship* Player)
{
    auto* Gear=Player?Player->GetGear():nullptr;if(!Gear)return;
    const auto Transform=G.GetAccumulatedRenderTransform();
    const auto Handle=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
    auto Point=[&](double X,double Y){return P+FVector2D(X*Size.X,Y*Size.Y);};
    auto Fill=[&](int32 Index,std::initializer_list<FVector2D> Points)
    {
        const bool Down=Index<Gear->NumGear() && Gear->GetState()==LandingGear::GEAR_DOWN;
        const FColor Color=Down?FColor(30,220,85):FColor(235,35,40);
        TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
        for(const auto& V:Points)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,FVector2f(V),FVector2f(.5f,.5f),Color));
        for(int32 I=1;I+1<Vertices.Num();++I){Indices.Add(0);Indices.Add(I);Indices.Add(I+1);}
        FSlateDrawElement::MakeCustomVerts(E,Layer,Handle,Vertices,Indices,nullptr,0,0);
    };
    Fill(0,{Point(.5,0),Point(.25,.5),Point(.75,.5)});
    Fill(1,{Point(.25,.5),Point(0,1),Point(.5,1),Point(.5,.5)});
    Fill(2,{Point(.5,.5),Point(.5,1),Point(1,1),Point(.75,.5)});
    auto Line=[&](TArray<FVector2D> V){FSlateDrawElement::MakeLines(E,Layer+1,G.ToPaintGeometry(),V,ESlateDrawEffect::None,FLinearColor::White,true,1.5f);};
    Line({Point(.5,0),Point(0,1),Point(1,1),Point(.5,0)});
    Line({Point(.25,.5),Point(.75,.5)});Line({Point(.5,.5),Point(.5,1)});
}
}
