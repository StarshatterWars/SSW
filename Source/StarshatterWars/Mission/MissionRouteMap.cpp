#include "MissionRouteMap.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
class SMissionRouteMap : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMissionRouteMap){} SLATE_END_ARGS()
    void Construct(const FArguments&,UMissionRouteMap* Owner){Source=Owner;}
    virtual bool ComputeVolatility() const override {return true;}
    virtual bool SupportsKeyboardFocus() const override{return true;}
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(700,400);}
    FString Region()const {return Source.IsValid()&&Source->RegionName?Source->RegionName():FString();}
    bool InRegion(const char* Name)const {return Region().IsEmpty()||Region().Equals(ANSI_TO_TCHAR(Name),ESearchCase::IgnoreCase);}
    FVector2D Project(FVector P,FVector2D Size)const{return Size*.5+FVector2D(P.X-Center.X,-P.Z+Center.Y)/Scale;}
    FVector Position(FVector2D P,FVector2D Size,double Altitude)const {P=(P-Size*.5)*Scale;return FVector(Center.X+P.X,Altitude,Center.Y-P.Y);}
    MissionElement* Selected()const {auto* M=Source.IsValid()?Source->Model:nullptr;return M&&Element>=0&&Element<M->GetElements().size()?M->GetElements()[Element]:nullptr;}
    Instruction* Waypoint()const {auto* E=Selected();return E&&Nav>=0&&Nav<E->NavList().size()?E->NavList()[Nav]:nullptr;}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override
    {
        const FVector2D Size=G.GetLocalSize();
        auto* ActiveModel=Source.IsValid()?Source->Model:nullptr;
        const FString CurrentRegion=Region();
        if(FitPending||ActiveModel!=LastModel||CurrentRegion!=LastRegion)
        {
            LastModel=ActiveModel;LastRegion=CurrentRegion;FitPending=false;
            FVector2D Min(MAX_dbl,MAX_dbl),Max(-MAX_dbl,-MAX_dbl);bool Found=false;
            auto Add=[&](const FVector& P){Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Z);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Z);Found=true;};
            if(ActiveModel)for(int I=0;I<ActiveModel->GetElements().size();++I)
            {auto* E=ActiveModel->GetElements()[I];if(InRegion(E->GetRegion().data()))Add(E->GetLocation());for(int J=0;J<E->NavList().size();++J)if(InRegion(E->NavList()[J]->GetRegionName()))Add(E->NavList()[J]->GetLocation());}
            if(Found){Center=(Min+Max)*.5;Scale=FMath::Max(50.,FMath::Max((Max.X-Min.X)/FMath::Max(1.,Size.X*.8),(Max.Y-Min.Y)/FMath::Max(1.,Size.Y*.7)));}
        }
        FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015,.025,.04,1));
        auto Line=[&](FVector2D A,FVector2D B,FLinearColor C){TArray<FVector2D> P{A,B};FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,1);};
        for(int I=0;I<10;++I){Line(FVector2D(Size.X*I/10,0),FVector2D(Size.X*I/10,Size.Y),FLinearColor(.08,.12,.16));Line(FVector2D(0,Size.Y*I/10),FVector2D(Size.X,Size.Y*I/10),FLinearColor(.08,.12,.16));}
        auto Label=[&](FVector2D P,FString Text,FLinearColor Color){FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(FVector2D(600,20),FSlateLayoutTransform(P)),Text,FCoreStyle::GetDefaultFontStyle("Regular",10),ESlateDrawEffect::None,Color);};
        Label(FVector2D(8,4),TEXT("Drag ships/points | Shift-click: add point | Delete: remove | wheel: zoom | right-drag: pan | F: fit"),FLinearColor::White);
        Label(FVector2D(8,22),TEXT("Up/Down: altitude +/-100 | +/-: speed +/-50 | A: cycle waypoint action"),FLinearColor::White);
        auto* M=Source.IsValid()?Source->Model:nullptr;
        if(!M)return Layer+2;
        for(int I=0;I<M->GetElements().size();++I)
        {
            auto* E=M->GetElements()[I];if(!InRegion(E->GetRegion().data()))continue;
            const FLinearColor Color=I==Element?FLinearColor::Yellow:FLinearColor(.2,.8,1);
            FVector2D Previous=Project(E->GetLocation(),Size);Line(Previous-FVector2D(6,0),Previous+FVector2D(6,0),Color);Line(Previous-FVector2D(0,6),Previous+FVector2D(0,6),Color);
            Label(Previous+FVector2D(8,0),ANSI_TO_TCHAR(E->GetName()),Color);
            for(int J=0;J<E->NavList().size();++J)
            {
                auto* N=E->NavList()[J];if(!InRegion(N->GetRegionName()))continue;
                const FVector2D P=Project(N->GetLocation(),Size);Line(Previous,P,Color);Line(P-FVector2D(4,4),P+FVector2D(4,4),Color);Line(P-FVector2D(-4,4),P+FVector2D(-4,4),Color);
                Label(P+FVector2D(6,5),FString::Printf(TEXT("%d %s H%.0f V%d"),J+1,ANSI_TO_TCHAR(Instruction::ActionName(N->GetAction())),N->GetLocation().Y,N->GetSpeed()),Color);Previous=P;
            }
        }
        return Layer+2;
    }
    virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& Event)override
    {
        Last=G.AbsoluteToLocal(Event.GetScreenSpacePosition());
        if(Event.GetEffectingButton()==EKeys::RightMouseButton){Pan=true;return FReply::Handled().CaptureMouse(SharedThis(this));}
        if(Event.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
        auto* M=Source.IsValid()?Source->Model:nullptr;if(!M)return FReply::Unhandled();
        if(Event.IsShiftDown()&&Selected())
        {
            auto* E=Selected();const FVector P=Position(Last,G.GetLocalSize(),Waypoint()?Waypoint()->GetLocation().Y:E->GetLocation().Y);
            auto* N=new Instruction(TCHAR_TO_ANSI(*Region()),P);N->SetSpeed(300);E->AddNavPoint(N,Waypoint());Nav=E->GetNavIndex(N)-1;
        }
        else
        {
            double Best=144;Element=-1;Nav=-1;
            for(int I=0;I<M->GetElements().size();++I)
            {
                auto* E=M->GetElements()[I];if(!InRegion(E->GetRegion().data()))continue;
                double D=(Project(E->GetLocation(),G.GetLocalSize())-Last).SizeSquared();if(D<Best){Best=D;Element=I;Nav=-1;}
                for(int J=0;J<E->NavList().size();++J){auto* N=E->NavList()[J];if(!InRegion(N->GetRegionName()))continue;D=(Project(N->GetLocation(),G.GetLocalSize())-Last).SizeSquared();if(D<Best){Best=D;Element=I;Nav=J;}}
            }
        }
        Drag=Selected()!=nullptr;return FReply::Handled().SetUserFocus(SharedThis(this)).CaptureMouse(SharedThis(this));
    }
    virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& Event)override
    {
        const FVector2D P=G.AbsoluteToLocal(Event.GetScreenSpacePosition());
        if(Pan)Center+=FVector2D(Last.X-P.X,P.Y-Last.Y)*Scale;
        if(Drag&&Selected()){if(auto* N=Waypoint())N->SetLocation(Position(P,G.GetLocalSize(),N->GetLocation().Y));else Selected()->SetLocation(Position(P,G.GetLocalSize(),Selected()->GetLocation().Y));}
        Last=P;return Pan||Drag?FReply::Handled():FReply::Unhandled();
    }
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&)override {Drag=Pan=false;return FReply::Handled().ReleaseMouseCapture();}
    virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent& E)override {Scale=FMath::Clamp(Scale*FMath::Pow(0.8,E.GetWheelDelta()),1.,100000.);return FReply::Handled();}
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E)override
    {
        if(E.GetKey()==EKeys::F){FitPending=true;return FReply::Handled();}
        auto* S=Selected();if(!S)return FReply::Unhandled();auto* N=Waypoint();const FKey K=E.GetKey();
        if(K==EKeys::Delete&&N){S->DelNavPoint(N);Nav=-1;return FReply::Handled();}
        if(K==EKeys::Up||K==EKeys::Down){FVector P=N?N->GetLocation():S->GetLocation();P.Y+=K==EKeys::Up?100:-100;if(N)N->SetLocation(P);else S->SetLocation(P);return FReply::Handled();}
        if(N&&(K==EKeys::Add||K==EKeys::Equals||K==EKeys::Subtract||K==EKeys::Hyphen)){N->SetSpeed(FMath::Max(0,N->GetSpeed()+((K==EKeys::Add||K==EKeys::Equals)?50:-50)));return FReply::Handled();}
        if(N&&K==EKeys::A){N->SetAction(static_cast<EInstruction>((int(N->GetAction())+1)%int(EInstruction::NumActions)));return FReply::Handled();}
        return FReply::Unhandled();
    }
private:
    TWeakObjectPtr<UMissionRouteMap> Source;int Element=-1,Nav=-1;bool Drag=false,Pan=false;mutable double Scale=1000;mutable FVector2D Center=FVector2D::ZeroVector;FVector2D Last=FVector2D::ZeroVector;
    mutable bool FitPending=true;mutable Mission* LastModel=nullptr;mutable FString LastRegion;
};
TSharedRef<SWidget> UMissionRouteMap::RebuildWidget(){return SNew(SMissionRouteMap,this);}
