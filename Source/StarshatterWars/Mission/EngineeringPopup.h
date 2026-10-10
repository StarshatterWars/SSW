#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Ship.h"
#include "Power.h"
#include "SimSystem.h"
#include "SimComponent.h"

// Self-contained panel body following EngDlg.frm; the host owns registration,
// visibility and input. ResolveShip/OnClose also support a future registered host.
// Live adapter for legacy EngDlg. Resolve ship on every action; never retain
// a simulation object across mission teardown. The owning dialog manages input.
class SEngineeringPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEngineeringPopup) : _Embedded(false) {}
        SLATE_ARGUMENT(bool, Embedded)
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        Embedded=Args._Embedded;
        ResolveShip = Args._ResolveShip;
        Close = Args._OnClose;
        if (Embedded)
        {
            // The Blueprint owns the 1024x1024 frame. Fill its RuntimeHost;
            // do not letterbox a second, landscape-sized panel inside it.
            ChildSlot
            [SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor::Transparent).Padding(0)
                .HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                [SAssignNew(Root, SVerticalBox)]];
        }
        else
        {
            ChildSlot
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0,0,0,0.75f)).Padding(24)
                .HAlign(HAlign_Center).VAlign(VAlign_Center)
                [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                    [SNew(SBox).WidthOverride(1024).HeightOverride(1024)
                        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                            .BorderBackgroundColor(FLinearColor(0.015f,0.035f,0.06f,1)).Padding(24)
                            [SAssignNew(Root, SVerticalBox)]]]]];
        }
        Rebuild();
    }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        if (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Delete) {
            Close.ExecuteIfBound(); return FReply::Handled();
        }
        return FReply::Unhandled();
    }
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        Ship* Current=GetShip();
        if (Current!=LastShip) {
            LastShip=Current; Selected=-1; Component=-1; Queue=-1; Dirty=true;
        }
        FString Signature;
        if (Current) {
            for (int i=0;i<Current->RepairQueue().size();++i)
                Signature+=FString::Printf(TEXT("%p;"),static_cast<void*>(Current->RepairQueue()[i]));
        }
        if (Signature!=QueueSignature) { QueueSignature=Signature; Dirty=true; }
        if (Dirty) Rebuild();
    }
private:
    bool Embedded=false;
    TFunction<Ship*()> ResolveShip;
    FSimpleDelegate Close;
    TSharedPtr<SVerticalBox> Root;
    Ship* LastShip=nullptr; // Identity comparison only; never dereferenced.
    int32 Selected=-1, Component=-1, Queue=-1;
    bool Dirty=true;
    FString QueueSignature;
    Ship* GetShip() const { return ResolveShip ? ResolveShip() : nullptr; }
    SimSystem* SystemAt(int32 Index) const {
        Ship* P=GetShip(); return P && Index>=0 && Index<P->GetSystems().size() ? P->GetSystems()[Index] : nullptr;
    }
    int32 SystemIndex(SimSystem* S) const {
        Ship* P=GetShip(); if (!P) return -1;
        for (int i=0;i<P->GetSystems().size();++i) if (P->GetSystems()[i]==S) return i;
        return -1;
    }
    SimSystem* CurrentSystem() const { return GetShip()==LastShip ? SystemAt(Selected) : nullptr; }
    SimComponent* CurrentComponent() const {
        SimSystem* S=CurrentSystem();
        return S && Component>=0 && Component<S->GetComponents().size() ? S->GetComponents()[Component] : nullptr;
    }
    static FString Name(SimSystem* S) { return S ? FString(ANSI_TO_TCHAR(S->GetName())) : TEXT("Select a system"); }
    static FText Text(const FString& S) { return FText::FromString(S); }
    TSharedRef<SWidget> Label(const FString& S, int32 Size=14) {
        return SNew(STextBlock).Text(Text(S)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).AutoWrapText(true);
    }
    TSharedRef<SWidget> ButtonLabel(const FString& S, int32 Size=14) {
        return SNew(STextBlock).Text(Text(S.ToUpper())).ColorAndOpacity(FLinearColor::Black)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).AutoWrapText(true);
    }
    static FText ButtonText(const FString& S) { return Text(S.ToUpper()); }
    TSharedRef<SWidget> Button(const FString& S, TFunction<void()> Action, TFunction<bool()> Enabled=[](){return true;}) {
        return SNew(SButton).ContentPadding(FMargin(8,6)).IsEnabled_Lambda([Enabled](){return Enabled();})
            .OnClicked_Lambda([Action](){Action();return FReply::Handled();})[ButtonLabel(S)];
    }
    static FString RepairETR(Ship* P, SimSystem* S)
    {
        if (!P || !S || P->RepairSpeed()<=0) return TEXT("--");
        float Remaining=0;
        for (int i=0;i<S->GetComponents().size();++i)
            if (auto* C=S->GetComponents()[i]) Remaining=FMath::Max(Remaining,C->TimeRemaining());
        const int32 Seconds=FMath::Max(0,FMath::CeilToInt(Remaining/P->RepairSpeed()));
        return FString::Printf(TEXT("%d:%02d"),Seconds/60,Seconds%60);
    }
    void Select(int32 Index) { Selected=Index; Component=-1; Dirty=true; }
    void Rebuild()
    {
        Dirty=false; Root->ClearChildren();
        Ship* P=GetShip(); LastShip=P;
        if(!Embedded) Root->AddSlot().AutoHeight().Padding(0,0,0,14)
        [Label(P ? FString::Printf(TEXT("ENGINEERING — %hs"),P->GetName()) : TEXT("ENGINEERING"),22)];
        if (!P) {
            Root->AddSlot()[Label(TEXT("No active player ship."))];
            if (!Embedded) Root->AddSlot().AutoHeight().HAlign(HAlign_Right)[Button(TEXT("Close"),[this](){Close.ExecuteIfBound();})];
            return;
        }
        TSharedPtr<SVerticalBox> SourceColumns;
        TSharedPtr<SVerticalBox> Details, Components, Repairs;
        Root->AddSlot().FillHeight(1)
        [SNew(SHorizontalBox)
            // Full-height systems column: long client lists remain scrollable.
            +SHorizontalBox::Slot().FillWidth(0.34f).Padding(0,0,16,0)
            [SNew(SScrollBox)
                +SScrollBox::Slot()[SAssignNew(SourceColumns,SVerticalBox)]]
            +SHorizontalBox::Slot().FillWidth(0.66f)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().FillHeight(0.62f).Padding(0,0,0,16)
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(0.48f).Padding(0,0,16,0)
                    [SNew(SScrollBox)
                        +SScrollBox::Slot()[SAssignNew(Details,SVerticalBox)]]
                    +SHorizontalBox::Slot().FillWidth(0.52f)
                    [SNew(SScrollBox)
                        +SScrollBox::Slot()[SAssignNew(Components,SVerticalBox)]]]
                +SVerticalBox::Slot().FillHeight(0.38f)
                [SNew(SScrollBox)
                    +SScrollBox::Slot()[SAssignNew(Repairs,SVerticalBox)]]]];
        if (!Embedded) Root->AddSlot().AutoHeight().Padding(0,16,0,0).HAlign(HAlign_Right)
        [SNew(SBox).MinDesiredWidth(112).MinDesiredHeight(36)
            [Button(TEXT("Close"),[this](){Close.ExecuteIfBound();})]];
        SourceColumns->AddSlot().AutoHeight().Padding(0,0,0,12)
            [Label(TEXT("POWER SOURCES / SYSTEMS"),16)];
        for (int i=0;i<FMath::Min(4,P->GetReactors().size());++i) {
            PowerSource* Source=P->GetReactors()[i]; if (!Source) continue;
            const int SourceIndex=SystemIndex(Source);
            TSharedPtr<SVerticalBox> Sources;
            SourceColumns->AddSlot().AutoHeight().Padding(0,0,0,20)
                [SAssignNew(Sources,SVerticalBox)];
            Sources->AddSlot().AutoHeight()[SNew(SProgressBar)
                .Percent_Lambda([this,SourceIndex](){auto* S=SystemAt(SourceIndex);
                    return TOptional<float>(S ? float(FMath::Clamp(S->GetPowerLevel()/100.0,0.0,1.0)) : 0.0f);})];
            Sources->AddSlot().AutoHeight().Padding(0,10,0,3)[Button(Name(Source),[this,SourceIndex](){Select(SourceIndex);})];
            Sources->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1)).Text_Lambda([this,SourceIndex](){
                SimSystem* S=SystemAt(SourceIndex); return Text(S ? FString::Printf(TEXT("Output %.0f%%   Charge %d%%"),S->GetPowerLevel(),S->GetCapacity()>0 ? S->Charge():0):TEXT("Unavailable"));})];
            Sources->AddSlot().AutoHeight().Padding(0,6)[Label(TEXT("SYSTEM / POWER"))];
            for (int c=0;c<Source->Clients().size();++c) {
                const int Index=SystemIndex(Source->Clients()[c]); if (Index<0) continue;
                Sources->AddSlot().AutoHeight().Padding(12,2)
                [SNew(SButton).OnClicked_Lambda([this,Index](){Select(Index);return FReply::Handled();})
                 [SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this,Index](){SimSystem* S=SystemAt(Index);return ButtonText(S ? FString::Printf(TEXT("%hs   %.0f%%   %s"),S->GetName(),S->GetPowerLevel(),S->IsPowerOn()?TEXT("ON"):TEXT("OFF")):TEXT("Unavailable"));})]];
            }
        }
        Details->AddSlot().AutoHeight().Padding(0,0,0,8)[Label(Name(CurrentSystem()),16)];
        Details->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1)).Text_Lambda([this](){SimSystem* S=CurrentSystem();return Text(S ? FString::Printf(TEXT("Availability %.0f%%\nSafety %.0f%%   Stability %.0f%%\nPower %.0f%%"),S->GetAvailability(),S->GetSafety(),S->GetStability(),S->GetPowerLevel()):TEXT(""));})];
        Details->AddSlot().AutoHeight().Padding(0,8)
        [SNew(SHorizontalBox)
         +SHorizontalBox::Slot()[Button(TEXT("Off"),[this](){if(auto* S=CurrentSystem())S->SetPowerOff();},[this](){return CurrentSystem()!=nullptr;})]
         +SHorizontalBox::Slot()[Button(TEXT("On"),[this](){if(auto* S=CurrentSystem())S->SetPowerOn();},[this](){return CurrentSystem()!=nullptr;})]
         +SHorizontalBox::Slot()[Button(TEXT("Override"),[this](){if(auto* S=CurrentSystem())S->SetOverride(S->GetPowerLevel()<=100);},[this](){return CurrentSystem()!=nullptr;})]];
        Details->AddSlot().AutoHeight()[Label(TEXT("POWER ALLOCATION"))];
        Details->AddSlot().AutoHeight().Padding(0,4)
        [SNew(SSlider).IsEnabled_Lambda([this](){return CurrentSystem() && CurrentSystem()->IsPowerOn();})
         .Value_Lambda([this](){return CurrentSystem()?float(FMath::Clamp(CurrentSystem()->GetPowerLevel()/100.0,0.0,1.0)):0.0f;})
         .OnValueChanged_Lambda([this](float V){if(auto* S=CurrentSystem())S->SetPowerLevel(V*100.0);})];
        Details->AddSlot().AutoHeight().Padding(0,8)[Label(TEXT("CAPACITOR CHARGE"))];
        Details->AddSlot().AutoHeight()[SNew(SProgressBar).Percent_Lambda([this](){
            auto* S=CurrentSystem(); return TOptional<float>(S && S->GetCapacity()>0 ?
                FMath::Clamp(float(S->Charge())/100.0f,0.0f,1.0f) : 0.0f);})];
        Details->AddSlot().AutoHeight().Padding(0,10)[Label(TEXT("ROUTE SELECTED CLIENT TO"))];
        for(int i=0;i<P->GetReactors().size();++i) {
            Details->AddSlot().AutoHeight()[Button(Name(P->GetReactors()[i]),[this,i](){
                Ship* ShipNow=GetShip(); SimSystem* S=CurrentSystem();
                if(!ShipNow || !S || i>=ShipNow->GetReactors().size())return;
                for(int n=0;n<ShipNow->GetReactors().size();++n)if(ShipNow->GetReactors()[n])ShipNow->GetReactors()[n]->RemoveClient(S);
                ShipNow->GetReactors()[i]->AddClient(S); Dirty=true;
            },[this,i](){Ship* Now=GetShip();SimSystem* S=CurrentSystem();if(!Now || !S || i>=Now->GetReactors().size() || !Now->GetReactors()[i])return false;
                for(int n=0;n<Now->GetReactors().size();++n)if(Now->GetReactors()[n]==S)return false;
                return S->GetCapacity()>0 || S->GetSinkRate()>0;})];
        }
        Components->AddSlot().AutoHeight().Padding(0,0,0,12)[Label(TEXT("COMPONENTS / SPARES"),16)];
        if(SimSystem* S=CurrentSystem())for(int i=0;i<S->GetComponents().size();++i) {
            SimComponent* C=S->GetComponents()[i];if(!C)continue;
            Components->AddSlot().AutoHeight()[SNew(SButton).OnClicked_Lambda([this,i](){Component=i;return FReply::Handled();})
             [SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this,i](){auto* Sys=CurrentSystem();auto* Cmp=Sys && i<Sys->GetComponents().size()?Sys->GetComponents()[i]:nullptr;
                return ButtonText(Cmp?FString::Printf(TEXT("%s%hs   %.0f%%   Spares %d"),Component==i?TEXT("> "):TEXT(""),Cmp->Name(),Cmp->Availability(),Cmp->SpareCount()):TEXT(""));})]];
        }
        Components->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1)).Text_Lambda([this](){auto* C=CurrentComponent();auto* Now=GetShip();if(!C || !Now)return Text(TEXT("Select a component to repair."));
            double Speed=FMath::Max(0.001,Now->RepairSpeed());return Text(FString::Printf(TEXT("Repair %.0fs / Replace %.0fs\nRemaining %.0fs"),C->RepairTime()/Speed,C->ReplaceTime()/Speed,double(C->TimeRemaining())));})];
        Components->AddSlot().AutoHeight()[SNew(SHorizontalBox)
         +SHorizontalBox::Slot()[Button(TEXT("Repair"),[this](){if(auto* C=CurrentComponent()){C->Repair();GetShip()->RepairSystem(C->GetSystem());Dirty=true;}},[this](){auto* C=CurrentComponent();return C && C->Availability()<100 && C->TimeRemaining()<=0;})]
         +SHorizontalBox::Slot()[Button(TEXT("Replace"),[this](){if(auto* C=CurrentComponent()){C->Replace();GetShip()->RepairSystem(C->GetSystem());Dirty=true;}},[this](){auto* C=CurrentComponent();return C && C->SpareCount()>0 && C->TimeRemaining()<=0;})]];
        Repairs->AddSlot().AutoHeight()[Label(TEXT("REPAIR QUEUE"),16)];
        Details->AddSlot().AutoHeight().Padding(0,8)[SNew(SButton).OnClicked_Lambda([this](){if(auto* Now=GetShip())Now->EnableRepair(!Now->AutoRepair());return FReply::Handled();})
         [SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return ButtonText(GetShip() && GetShip()->AutoRepair()?TEXT("Automatic repair: ON"):TEXT("Automatic repair: OFF"));})]];
        Repairs->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor(0.25f,0.7f,1)).Text_Lambda([this](){return Text(GetShip()?FString::Printf(TEXT("Repair teams: %d"),GetShip()->RepairTeams()):TEXT(""));})];
        Repairs->AddSlot().AutoHeight().Padding(0,6)[Label(TEXT("SYSTEM / ETR"))];
        for(int i=0;i<P->RepairQueue().size();++i) {
            Repairs->AddSlot().AutoHeight().Padding(0,4)[SNew(SButton).OnClicked_Lambda([this,i](){auto* Now=GetShip();if(Now && i<Now->RepairQueue().size()){Queue=i;Select(SystemIndex(Now->RepairQueue()[i]));}return FReply::Handled();})
             [SNew(STextBlock).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this,i](){auto* Now=GetShip();return ButtonText(Now && i<Now->RepairQueue().size()?FString::Printf(TEXT("%s%d. %s   %s"),Queue==i?TEXT("> "):TEXT(""),i+1,*Name(Now->RepairQueue()[i]),*RepairETR(Now,Now->RepairQueue()[i])):TEXT(""));})]];
        }
        Repairs->AddSlot().AutoHeight().Padding(0,8)[SNew(SHorizontalBox)
         +SHorizontalBox::Slot()[Button(TEXT("Move up"),[this](){GetShip()->IncreaseRepairPriority(Queue);--Queue;Dirty=true;},[this](){return GetShip() && Queue>0 && Queue<GetShip()->RepairQueue().size();})]
         +SHorizontalBox::Slot()[Button(TEXT("Move down"),[this](){GetShip()->DecreaseRepairPriority(Queue);++Queue;Dirty=true;},[this](){return GetShip() && Queue>=0 && Queue+1<GetShip()->RepairQueue().size();})]];
    }
};
