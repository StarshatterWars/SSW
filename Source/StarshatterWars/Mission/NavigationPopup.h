#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Ship.h"
#include "Sim.h"
#include "SimRegion.h"
#include "NavSystem.h"
#include "Instruction.h"
#include "InputCoreTypes.h"

// Hosts SSW's existing navigation map. No duplicate actors or scene capture.
class SNavigationPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNavigationPopup) : _Embedded(false), _FooterStyle(nullptr) {}
        SLATE_ARGUMENT(bool, Embedded)
        SLATE_ARGUMENT(TSharedPtr<SBox>, FooterHost)
        SLATE_ARGUMENT(const FButtonStyle*, FooterStyle)
        SLATE_ARGUMENT(TSharedPtr<SWidget>, MapWidget)
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
        SLATE_ARGUMENT(TFunction<bool()>, CanOperate)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {
        ResolveShip=Args._ResolveShip; CanOperate=Args._CanOperate; Close=Args._OnClose;
        auto Content=SNew(SVerticalBox)
         +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SBox).Visibility(Args._Embedded?EVisibility::Collapsed:EVisibility::Visible)[Label(TEXT("NAVIGATION"))]]
         +SVerticalBox::Slot().FillHeight(1)[Args._MapWidget.ToSharedRef()]
         +SVerticalBox::Slot().AutoHeight().Padding(8)
          [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Readout());})
           .ColorAndOpacity(FLinearColor(0.25f,0.7f,1)).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))]
;
        auto Footer = SNew(SHorizontalBox)
           +SHorizontalBox::Slot().FillWidth(1)
            [SNew(SButton).ButtonStyle(Args._FooterStyle ? Args._FooterStyle : &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).ContentPadding(FMargin(24,10)).IsEnabled_Lambda([this](){auto* P=Player();auto* N=P?P->GetNavSystem():nullptr;return (!CanOperate || CanOperate()) && N && (N->AutoNavEngaged() || (N->IsPowerOn() && P->GetNextNavPoint()));})
             .OnClicked_Lambda([this](){auto* P=Player();auto* N=P?P->GetNavSystem():nullptr;
                 if(N && (!CanOperate || CanOperate())){
                     if(N->AutoNavEngaged())N->DisengageAutoNav();
                     else {N->EngageAutoNav();if(N->AutoNavEngaged())Close.ExecuteIfBound();}
                 }return FReply::Handled();})
             [SNew(STextBlock).Text_Lambda([this](){auto* P=Player();auto* N=P?P->GetNavSystem():nullptr;return FText::FromString(N && N->AutoNavEngaged()?TEXT("Cancel Autonav"):TEXT("Commit Autonav"));})]]
           +SHorizontalBox::Slot().AutoWidth().Padding(16,0)
            [SNew(SButton).ButtonStyle(Args._FooterStyle ? Args._FooterStyle : &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button")).ContentPadding(FMargin(24,10)).OnClicked_Lambda([this](){Close.ExecuteIfBound();return FReply::Handled();})[Label(TEXT("Close"))]];
        if (Args._FooterHost.IsValid()) Args._FooterHost->SetContent(Footer);
        else Content->AddSlot().AutoHeight().Padding(8)[Footer];
        ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(1200).HeightOverride(760)
             [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
              .BorderBackgroundColor(Args._Embedded?FLinearColor::Transparent:FLinearColor(0.005f,0.015f,0.03f,1)).Padding(Args._Embedded?0:10)[Content]]]];
    }
    virtual bool SupportsKeyboardFocus() const override {return true;}
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E) override {
        if(E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::Delete){Close.ExecuteIfBound();return FReply::Handled();}
        return FReply::Unhandled();
    }
private:
    TFunction<Ship*()> ResolveShip;
    TFunction<bool()> CanOperate;
    FSimpleDelegate Close;
    Ship* Player() const {return ResolveShip?ResolveShip():nullptr;}
    TSharedRef<SWidget> Label(const FString& S) {
        return SNew(STextBlock).Text(FText::FromString(S)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",18));
    }
    FString Readout() const {
        auto* P=Player(); if(!P)return TEXT("No active player ship.");
        auto* R=P->GetRegion();auto* N=P->GetNextNavPoint();const FVector Location=P->GetLocation();
        FString Text=FString::Printf(TEXT("LOCATION  %hs    X %.1f  Y %.1f  Z %.1f km"),R?R->GetName():"--",Location.X/1000,Location.Y/1000,Location.Z/1000);
        if(N){Text+=FString::Printf(TEXT("\nDESTINATION  %hs — %hs"),N->GetRegionName(),N->GetDescription());
            if(N->GetRegion()==R)Text+=FString::Printf(TEXT("    RANGE %.1f km"),FVector::Distance(Location,N->GetLocation())/1000);}
        else Text+=TEXT("\nDESTINATION  No remaining waypoint");
        return Text;
    }
};
