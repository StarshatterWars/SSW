#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "MissionUIStyle.h"

// Presentation only: observes warp/streaming state; never moves simulation ships.
class STravelTransition : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STravelTransition) {} SLATE_END_ARGS()
    void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
    void Update(float DeltaSeconds, double Warp, bool Loading, const FString& Destination)
    {
        Phase += FMath::Max(0.f, DeltaSeconds);
        Strength = Loading ? 1.f : FMath::Clamp(static_cast<float>(FMath::Loge(FMath::Max(1.0,Warp))/FMath::Loge(5000.0)),0.f,1.f);
        Caption = Loading ? TEXT("HYPERSPACE — ARRIVING ") + Destination : TEXT("HYPERSPACE TRANSIT");
        Invalidate(EInvalidateWidget::Paint);
    }
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geometry,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
    {
        const FVector2D Size=Geometry.GetLocalSize();
        const FVector2D Center=Size*0.5;
        const float Radius=FMath::Max(Size.X,Size.Y)*0.8f;
        FSlateDrawElement::MakeBox(Out,Layer,Geometry.ToPaintGeometry(),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(0.012f,0.003f,0.035f,Strength));
        // Violet energy rings and cold blue velocity streaks form a moving tunnel.
        for(int Ring=0;Ring<18;++Ring)
        {
            const float Z=FMath::Frac(Phase*0.75f+Ring/18.f);
            const float R=(0.025f+Z*Z)*Radius;
            TArray<FVector2D> Points;
            for(int Step=0;Step<=64;++Step)
            {
                const float Angle=2.f*PI*Step/64.f;
                const float Ripple=1.f+0.035f*FMath::Sin(Angle*7.f+Phase*3.f+Ring);
                Points.Add(Center+FVector2D(FMath::Cos(Angle)*R*Ripple,FMath::Sin(Angle)*R*0.68f*Ripple));
            }
            const FLinearColor Color=FMath::Lerp(FLinearColor(0.6f,0.07f,1.f),FLinearColor(0.15f,0.65f,1.f),Z);
            FSlateDrawElement::MakeLines(Out,Layer+1,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color.CopyWithNewOpacity(Strength*(0.15f+0.7f*Z)),true,1.f+Z*3.f);
        }
        for(int Streak=0;Streak<100;++Streak)
        {
            const float Angle=Streak*2.399963f+0.07f*FMath::Sin(Phase);
            const float Z=FMath::Frac(Streak*0.618034f+Phase*(0.8f+Strength));
            const FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle)*0.68f);
            TArray<FVector2D> Points={Center+Direction*(Radius*Z*Z),Center+Direction*(Radius*(Z+0.09f)*(Z+0.09f))};
            FSlateDrawElement::MakeLines(Out,Layer+2,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(0.45f,0.8f,1.f,Strength*Z),true,1.f+2.f*Z);
        }
        FSlateDrawElement::MakeText(Out,Layer+3,Geometry.ToPaintGeometry(FVector2D(620,40),FSlateLayoutTransform(FVector2D(Center.X-310,Size.Y*0.84))),Caption,MissionUIStyle::GetHeaderFont(20),ESlateDrawEffect::None,FLinearColor(0.7f,0.9f,1.f,Strength));
        return Layer+3;
    }
private:
    float Phase=0,Strength=0;
    FString Caption;
};
