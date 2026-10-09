#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Sim.h"
#include "Ship.h"
#include "Mission.h"
#include "SimElement.h"
#include "Instruction.h"

// Read-only live mission display; host owns input and panel registration.
class SObjectivesPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SObjectivesPopup) {}
        SLATE_ARGUMENT(TFunction<bool()>, IsMissionActive)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {
        Active=Args._IsMissionActive; Close=Args._OnClose;
        auto Content=SNew(SVerticalBox)
          +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)
           [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Title);})
            .ColorAndOpacity(Blue()).Font(FCoreStyle::GetDefaultFontStyle("Bold",22))]
          +SVerticalBox::Slot().FillHeight(1)
           [SNew(SScrollBox)+SScrollBox::Slot()
            [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Body);})
             .WrapTextAt(860).ColorAndOpacity(Blue()).Font(FCoreStyle::GetDefaultFontStyle("Regular",17))]]
          +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0,12,0,0)
           [SNew(SButton).OnClicked_Lambda([this](){Close.ExecuteIfBound();return FReply::Handled();})
            [SNew(STextBlock).Text(FText::FromString(TEXT("Close")))]];
        auto Panel=SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.01f,0.025f,0.04f,0.97f)).Padding(24)[Content];
        ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [SNew(SBox).WidthOverride(920).HeightOverride(640)[Panel]]];
        Refresh();
    }
    virtual bool SupportsKeyboardFocus() const override {return true;}
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& Event) override {
        if(Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Delete){
            Close.ExecuteIfBound();return FReply::Handled();
        }
        return FReply::Unhandled();
    }
    virtual void Tick(const FGeometry& G,double T,float D) override {
        SCompoundWidget::Tick(G,T,D);
        if(T>=NextRefresh){NextRefresh=T+0.25;Refresh();}
    }
private:
    TFunction<bool()> Active;
    FSimpleDelegate Close;
    FString Title,Body;
    double NextRefresh=0;
    static FLinearColor Blue(){return FLinearColor(0.25f,0.7f,1);}
    static FString String(const char* Value){return Value?FString(UTF8_TO_TCHAR(Value)):FString();}
    void Refresh() {
        Title=TEXT("MISSION OBJECTIVES"); Body=TEXT("No active mission.");
        if(!Active || !Active())return;
        auto* Simulation=Sim::GetSim();
        auto* M=Simulation?Simulation->GetMission():nullptr;
        auto* P=Simulation?Simulation->GetPlayerShip():nullptr;
        if(!M)return;
        Title+=TEXT(" — ")+String(M->GetName());
        Body=TEXT("OBJECTIVES\n")+String(M->GetObjective())+TEXT("\n\nSITUATION\n")+String(M->GetSituation());
        auto* Element=P?P->GetElement():nullptr;
        if(!Element)return;
        // Legacy HUDView::DrawInstructions uses the current player element.
        if(Element->NumInstructions()>0){
            Body+=TEXT("\n\nCURRENT INSTRUCTIONS\n");
            for(int32 I=0;I<Element->NumInstructions();++I)
                Body+=String(Element->GetInstruction(I).data())+TEXT("\n");
        }
        if(Element->NumObjectives()>0){
            Body+=TEXT("\n\nELEMENT OBJECTIVES\n");
            for(int32 I=0;I<Element->NumObjectives();++I)if(auto* O=Element->GetObjective(I)){
                Body+=TEXT("[")+String(Instruction::StatusName(O->GetStatus()))+TEXT("] ");
                Body+=String(O->GetShortDescription())+TEXT("\n");
            }
        }
    }
};
