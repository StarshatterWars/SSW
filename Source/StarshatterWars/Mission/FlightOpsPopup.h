#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Ship.h"
#include "Hangar.h"
#include "SimElement.h"
#include "GameStructs.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"

// FltDlg.frm layout, backed by live hangar slots. No retained simulation pointers.
class SFlightOpsPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFlightOpsPopup) {}
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveCarrier)
        SLATE_ARGUMENT(TFunction<bool()>, CanOperate)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {
        Resolve=Args._ResolveCarrier; Operate=Args._CanOperate;
        ChildSlot[SAssignNew(Root,SVerticalBox)];
        Rebuild();
    }
    virtual void Tick(const FGeometry& G,double T,float D) override {
        SCompoundWidget::Tick(G,T,D);
        if(T>=NextRefresh){NextRefresh=T+0.5;Refresh();}
    }
private:
    TFunction<Ship*()> Resolve;
    TFunction<bool()> Operate;
    TSharedPtr<SVerticalBox> Root,Rows;
    Ship* LastCarrier=nullptr; // identity comparison only
    int32 LastSquadrons=-1,Filter=-1,SelectedSquad=-1,SelectedSlot=-1;
    double NextRefresh=0;
    FString Signature;
    Ship* Carrier()const{return Resolve?Resolve():nullptr;}
    Hangar* Bay()const{auto* C=Carrier();return C?C->GetHangar():nullptr;}
    const HangarSlot* Selected()const{auto* H=Bay();return H && SelectedSquad>=0 && SelectedSquad<H->NumSquadrons() && SelectedSlot>=0 && SelectedSlot<H->SquadronSize(SelectedSquad)?H->GetSlot(SelectedSquad,SelectedSlot):nullptr;}
    TSharedRef<SWidget> Label(const FString& S,bool Button=false) {
        return SNew(STextBlock).Text(FText::FromString(Button?S.ToUpper():S))
            .ColorAndOpacity(Button?FLinearColor::Black:FLinearColor(0.25f,0.7f,1))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).AutoWrapText(true);
    }
    bool Allowed(int32 Command)const {
        if(Operate && !Operate())return false;
        auto* H=Bay();auto* Slot=Selected();if(!H || !Slot)return false;
        const int State=H->GetState(Slot);
        if(Command==0)return State==Hangar::ALERT;
        if(Command==1)return State>Hangar::STORAGE && State<Hangar::LAUNCH;
        return State>=Hangar::ACTIVE && H->GetShip(Slot)!=nullptr;
    }
    FReply Execute(int32 Command) {
        if(!Allowed(Command))return FReply::Handled();
        auto* H=Bay();
        if(Command==0)H->Launch(SelectedSquad,SelectedSlot);
        else if(Command==1)H->StandDown(SelectedSquad,SelectedSlot);
        else if(auto* Craft=H->GetShip(Selected()))
            RadioTraffic::Transmit(new RadioMessage(Craft,Carrier(),RadioMessageAction::RTB));
        Signature.Empty();Refresh();return FReply::Handled();
    }
    void Rebuild() {
        Root->ClearChildren();Rows.Reset();Signature.Empty();
        LastCarrier=Carrier();auto* H=Bay();LastSquadrons=H?H->NumSquadrons():0;
        SelectedSquad=SelectedSlot=-1;
        if(!H){Root->AddSlot().AutoHeight()[Label(TEXT("No carrier hangar available for this ship."))];return;}
        TSharedPtr<SHorizontalBox> Filters;
        Root->AddSlot().AutoHeight().Padding(0,0,0,12)[SAssignNew(Filters,SHorizontalBox)];
        for(int32 I=-1;I<H->NumSquadrons()+2;++I){
            const FString Caption=I<0?TEXT("ALL"):I==H->NumSquadrons()?TEXT("PENDING"):I==H->NumSquadrons()+1?TEXT("ACTIVE"):FString(UTF8_TO_TCHAR(H->SquadronName(I).data()));
            Filters->AddSlot().FillWidth(1).Padding(2)[SNew(SButton).OnClicked_Lambda([this,I](){Filter=I;SelectedSquad=SelectedSlot=-1;Signature.Empty();Refresh();return FReply::Handled();})[Label(Caption,true)]];
        }
        Root->AddSlot().AutoHeight().Padding(0,0,0,8)[Label(TEXT("SQUADRON / SLOT       CRAFT       STATUS       TIME REMAINING"))];
        Root->AddSlot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Rows,SVerticalBox)]];
        TSharedPtr<SHorizontalBox> Actions;
        Root->AddSlot().AutoHeight().Padding(0,12)[SAssignNew(Actions,SHorizontalBox)];
        const TCHAR* Names[]={TEXT("LAUNCH"),TEXT("STAND DOWN"),TEXT("RECALL")};
        for(int32 I=0;I<3;++I)Actions->AddSlot().FillWidth(1).Padding(4)
            [SNew(SButton).IsEnabled_Lambda([this,I](){return Allowed(I);}).OnClicked_Lambda([this,I](){return Execute(I);})[Label(Names[I],true)]];
        Root->AddSlot().AutoHeight().Padding(0,8)[Label(TEXT("MISSION PLANNING"))];
        TSharedPtr<SHorizontalBox> Missions;
        Root->AddSlot().AutoHeight()[SAssignNew(Missions,SHorizontalBox)];
        for(const TCHAR* Name:{TEXT("PATROL"),TEXT("INTERCEPT"),TEXT("ASSAULT"),TEXT("STRIKE"),TEXT("ESCORT"),TEXT("SCOUT")})
            Missions->AddSlot().FillWidth(1).Padding(2)[SNew(SButton).IsEnabled(false)[Label(Name,true)]];
        Root->AddSlot().AutoHeight().Padding(0,12)[Label(TEXT("OBJECTIVE / LOADOUT\nPackage planning and Alert preparation unavailable."))];
        Root->AddSlot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().Padding(4)[SNew(SButton).IsEnabled(false)[Label(TEXT("PACKAGE"),true)]]
            +SHorizontalBox::Slot().Padding(4)[SNew(SButton).IsEnabled(false)[Label(TEXT("ALERT"),true)]]];
        Refresh();
    }
    void Refresh() {
        auto* H=Bay();
        if(Carrier()!=LastCarrier || (H?H->NumSquadrons():0)!=LastSquadrons){Filter=-1;Rebuild();return;}
        if(!H || !Rows)return;
        FString State=FString::Printf(TEXT("%d:%d:%d"),Filter,SelectedSquad,SelectedSlot);
        for(int32 Q=0;Q<H->NumSquadrons();++Q)for(int32 I=0;I<H->SquadronSize(Q);++I){auto* Slot=H->GetSlot(Q,I);if(Slot)State+=FString::Printf(TEXT(";%d:%d:%d:%d:%p"),Q,I,H->GetState(Slot),FMath::CeilToInt(H->TimeRemaining(Slot)),static_cast<void*>(H->GetShip(Slot)));}
        if(State==Signature)return;Signature=State;Rows->ClearChildren();
        for(int32 Q=0;Q<H->NumSquadrons();++Q)for(int32 I=0;I<H->SquadronSize(Q);++I){
            auto* Slot=H->GetSlot(Q,I);if(!Slot)continue;const int Status=H->GetState(Slot);
            if(Filter>=0 && Filter<H->NumSquadrons() && Filter!=Q)continue;
            if(Filter==H->NumSquadrons() && !(Status>Hangar::STORAGE && Status<Hangar::ACTIVE))continue;
            if(Filter==H->NumSquadrons()+1 && Status<Hangar::ACTIVE)continue;
            auto* Craft=H->GetShip(Slot);
            const FString Caption=FString::Printf(TEXT("%s%s / %d    %s    %s    %ds"),SelectedSquad==Q && SelectedSlot==I?TEXT("> "):TEXT(""),UTF8_TO_TCHAR(H->SquadronName(Q).data()),I+1,Craft?UTF8_TO_TCHAR(Craft->GetName()):TEXT("--"),UTF8_TO_TCHAR(H->StatusName(Slot).data()),FMath::Max(0,FMath::CeilToInt(H->TimeRemaining(Slot))));
            Rows->AddSlot().AutoHeight().Padding(0,2)[SNew(SButton).OnClicked_Lambda([this,Q,I](){SelectedSquad=Q;SelectedSlot=I;Signature.Empty();Refresh();return FReply::Handled();})[Label(Caption,true)]];
        }
    }
};
