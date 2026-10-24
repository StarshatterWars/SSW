#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Ship.h"
#include "Hangar.h"
#include "FlightOpsPreparation.h"
#include "Widgets/Input/SComboBox.h"
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
    void ChooseMission(int32 Type){MissionType=FMath::Clamp(Type,0,5);ObjectiveName.Empty();RefreshChoices();}
    void ChooseFilter(int32 Index){Filter=Index;SelectedSquad=SelectedSlot=-1;SelectedSlots.Empty();Signature.Empty();Refresh();}
    void PreparePackage(){Prepare(true);}
    void PrepareAlert(){Prepare(false);}
    void LaunchSelected(){Execute(0);}
    void StandDownSelected(){Execute(1);}
    void RecallSelected(){Execute(2);}
private:
    TFunction<Ship*()> Resolve;
    TFunction<bool()> Operate;
    TSharedPtr<SVerticalBox> Root,Rows;
    Ship* LastCarrier=nullptr; // identity comparison only
    int32 LastSquadrons=-1,Filter=-1,SelectedSquad=-1,SelectedSlot=-1;
    TArray<int32> SelectedSlots;
    int32 MissionType=0,PatrolPattern=0;
    FString ObjectiveName,LoadName,StatusMessage;
    TArray<TSharedPtr<FString>> ObjectiveOptions,LoadOptions;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> ObjectiveCombo,LoadCombo;
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
        auto* H=Bay();if(!H || SelectedSquad<0 || SelectedSquad>=H->NumSquadrons())return false;
        for(int Index:SelectedSlots) {
            if(Index<0 || Index>=H->SquadronSize(SelectedSquad))continue;
            auto* Slot=H->GetSlot(SelectedSquad,Index);if(!Slot)continue;
            const int State=H->GetState(Slot);
            if(Command==0 && State==Hangar::ALERT)return true;
            if(Command==1 && State>Hangar::STORAGE && State<Hangar::LAUNCH)return true;
            if(Command==2 && State>=Hangar::ACTIVE && H->GetShip(Slot))return true;
        }
        return false;
    }
    bool CanPrepare(bool Package)const {
        if(Operate && !Operate())return false;
        auto* H=Bay();if(!H || SelectedSquad<0 || SelectedSquad>=H->NumSquadrons() || SelectedSlots.IsEmpty())return false;
        for(int Index:SelectedSlots)if(Index<0 || Index>=H->SquadronSize(SelectedSquad) || H->GetState(H->GetSlot(SelectedSquad,Index))!=Hangar::STORAGE)return false;
        return !Package || MissionType==0 || MissionType==5 || !ObjectiveName.IsEmpty();
    }
    void SelectSlot(int32 Squadron,int32 Index) {
        if(SelectedSquad!=Squadron){SelectedSlots.Empty();LoadName.Empty();}
        SelectedSquad=Squadron;SelectedSlot=Index;
        if(SelectedSlots.Contains(Index))SelectedSlots.Remove(Index);
        else if(SelectedSlots.Num()<4)SelectedSlots.Add(Index);
        else StatusMessage=TEXT("Maximum four fighters per element.");
        Signature.Empty();Refresh();
    }
    FReply Execute(int32 Command) {
        if(!Allowed(Command))return FReply::Handled();
        auto* H=Bay();int Changed=0;
        for(int Index:SelectedSlots) {
            if(Index<0 || Index>=H->SquadronSize(SelectedSquad))continue;
            auto* Slot=H->GetSlot(SelectedSquad,Index);
            if(Command==0){if(H->Launch(SelectedSquad,Index))++Changed;}
            else if(Command==1){if(H->StandDown(SelectedSquad,Index))++Changed;}
            else if(auto* Craft=H->GetShip(Slot)){
                RadioTraffic::Transmit(new RadioMessage(Craft,Carrier(),RadioMessageAction::RTB));++Changed;
            }
        }
        StatusMessage=FString::Printf(TEXT("Command accepted for %d fighter(s)."),Changed);
        Signature.Empty();Refresh();return FReply::Handled();
    }
    FReply Prepare(bool Package) {
        if(CanPrepare(Package) && FFlightOpsPreparation::Prepare(Carrier(),SelectedSquad,SelectedSlots,MissionType,
            ObjectiveName,LoadName,Package,PatrolPattern,StatusMessage))SelectedSlots.Empty();
        Signature.Empty();Refresh();return FReply::Handled();
    }
    void RefreshChoices() {
        auto Update=[](const TArray<FString>& Names,TArray<TSharedPtr<FString>>& Options,FString& Choice,
            TSharedPtr<SComboBox<TSharedPtr<FString>>> Combo) {
            bool Changed=Names.Num()!=Options.Num();
            if(!Changed)for(int I=0;I<Names.Num();++I)if(Names[I]!=*Options[I]){Changed=true;break;}
            if(Changed){Options.Empty();for(const auto& Name:Names)Options.Add(MakeShared<FString>(Name));if(Combo)Combo->RefreshOptions();}
            if(!Names.Contains(Choice))Choice=Names.IsEmpty()?FString():Names[0];
        };
        Update(FFlightOpsPreparation::Objectives(Carrier(),MissionType),ObjectiveOptions,ObjectiveName,ObjectiveCombo);
        Update(FFlightOpsPreparation::Loadouts(Carrier(),SelectedSquad),LoadOptions,LoadName,LoadCombo);
    }
    void Rebuild() {
        Root->ClearChildren();Rows.Reset();Signature.Empty();
        LastCarrier=Carrier();auto* H=Bay();LastSquadrons=H?H->NumSquadrons():0;
        SelectedSquad=SelectedSlot=-1;SelectedSlots.Empty();
        if(!H){Root->AddSlot().AutoHeight()[Label(TEXT("No carrier hangar available for this ship."))];return;}
        TSharedPtr<SHorizontalBox> Filters;
        Root->AddSlot().AutoHeight().Padding(0,0,0,12)[SAssignNew(Filters,SHorizontalBox)];
        for(int32 I=-1;I<H->NumSquadrons()+2;++I){
            const FString Caption=I<0?TEXT("ALL"):I==H->NumSquadrons()?TEXT("PENDING"):I==H->NumSquadrons()+1?TEXT("ACTIVE"):FString(UTF8_TO_TCHAR(H->SquadronName(I).data()));
            Filters->AddSlot().FillWidth(1).Padding(2)[SNew(SButton).OnClicked_Lambda([this,I](){Filter=I;SelectedSquad=SelectedSlot=-1;SelectedSlots.Empty();Signature.Empty();Refresh();return FReply::Handled();})[Label(Caption,true)]];
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
        const TCHAR* MissionNames[]={TEXT("PATROL"),TEXT("INTERCEPT"),TEXT("ASSAULT"),TEXT("STRIKE"),TEXT("ESCORT"),TEXT("SCOUT")};
        for(int I=0;I<6;++I)Missions->AddSlot().FillWidth(1).Padding(2)
            [SNew(SButton).ButtonColorAndOpacity_Lambda([this,I](){return MissionType==I?FLinearColor(0.3f,0.7f,1):FLinearColor::White;})
                .OnClicked_Lambda([this,I](){MissionType=I;ObjectiveName.Empty();RefreshChoices();return FReply::Handled();})[Label(MissionNames[I],true)]];
        Root->AddSlot().AutoHeight().Padding(0,8)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)
                [SAssignNew(ObjectiveCombo,SComboBox<TSharedPtr<FString>>).OptionsSource(&ObjectiveOptions)
                    .OnGenerateWidget_Lambda([this](TSharedPtr<FString> Item){return Label(Item?*Item:FString());})
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item,ESelectInfo::Type){if(Item)ObjectiveName=*Item;})
                    [SNew(STextBlock).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return FText::FromString(ObjectiveName.IsEmpty()?
                        (MissionType==0 || MissionType==5?TEXT("OBJECTIVE: PATROL ROUTE"):TEXT("NO ELIGIBLE OBJECTIVES")):TEXT("OBJECTIVE: ")+ObjectiveName);})]]
                +SHorizontalBox::Slot().FillWidth(1).Padding(2)
                [SAssignNew(LoadCombo,SComboBox<TSharedPtr<FString>>).OptionsSource(&LoadOptions)
                    .OnGenerateWidget_Lambda([this](TSharedPtr<FString> Item){return Label(Item?*Item:FString());})
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item,ESelectInfo::Type){if(Item)LoadName=*Item;})
                    [SNew(STextBlock).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return FText::FromString(LoadName.IsEmpty()?TEXT("LOADOUT: STORED"):TEXT("LOADOUT: ")+LoadName);})]]];
        Root->AddSlot().AutoHeight()[Label(TEXT("Select up to four fighters in one squadron. PACKAGE uses the chosen loadout and auto-launches; ALERT retains stored loadouts and waits for LAUNCH."))];
        Root->AddSlot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().Padding(4)[SNew(SButton).IsEnabled_Lambda([this](){return CanPrepare(true);}).OnClicked_Lambda([this](){return Prepare(true);})[Label(TEXT("PACKAGE"),true)]]
            +SHorizontalBox::Slot().Padding(4)[SNew(SButton).IsEnabled_Lambda([this](){return CanPrepare(false);}).OnClicked_Lambda([this](){return Prepare(false);})[Label(TEXT("ALERT"),true)]]];
        Root->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.25f,0.7f,1))
            .Text_Lambda([this](){return FText::FromString(StatusMessage);})];
        Refresh();
    }
    void Refresh() {
        auto* H=Bay();
        if(Carrier()!=LastCarrier || (H?H->NumSquadrons():0)!=LastSquadrons){Filter=-1;Rebuild();return;}
        if(!H || !Rows)return;
        RefreshChoices();
        FString State=FString::Printf(TEXT("%d:%d:%d"),Filter,SelectedSquad,SelectedSlot);
        for(int32 Q=0;Q<H->NumSquadrons();++Q)for(int32 I=0;I<H->SquadronSize(Q);++I){auto* Slot=H->GetSlot(Q,I);if(Slot)State+=FString::Printf(TEXT(";%d:%d:%d:%d:%p"),Q,I,H->GetState(Slot),FMath::CeilToInt(H->TimeRemaining(Slot)),static_cast<void*>(H->GetShip(Slot)));}
        if(State==Signature)return;Signature=State;Rows->ClearChildren();
        for(int32 Q=0;Q<H->NumSquadrons();++Q)for(int32 I=0;I<H->SquadronSize(Q);++I){
            auto* Slot=H->GetSlot(Q,I);if(!Slot)continue;const int Status=H->GetState(Slot);
            if(Filter>=0 && Filter<H->NumSquadrons() && Filter!=Q)continue;
            if(Filter==H->NumSquadrons() && !(Status>Hangar::STORAGE && Status<Hangar::ACTIVE))continue;
            if(Filter==H->NumSquadrons()+1 && Status<Hangar::ACTIVE)continue;
            auto* Craft=H->GetShip(Slot);
            const FString Caption=FString::Printf(TEXT("%s%s / %d    %s    %s    %ds"),SelectedSquad==Q && SelectedSlots.Contains(I)?TEXT("> "):TEXT(""),UTF8_TO_TCHAR(H->SquadronName(Q).data()),I+1,Craft?UTF8_TO_TCHAR(Craft->GetName()):TEXT("--"),Status==Hangar::MAINT?TEXT("TURNAROUND (repair / refuel / rearm)"):UTF8_TO_TCHAR(H->StatusName(Slot).data()),FMath::Max(0,FMath::CeilToInt(H->TimeRemaining(Slot))));
            Rows->AddSlot().AutoHeight().Padding(0,2)[SNew(SButton).OnClicked_Lambda([this,Q,I](){SelectSlot(Q,I);return FReply::Handled();})[Label(Caption,true)]];
        }
    }
};
