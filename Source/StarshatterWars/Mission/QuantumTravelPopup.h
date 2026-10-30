#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "MissionUIStyle.h"
#include "QuantumTravel.h"
#include "QuantumDrive.h"
#include "Ship.h"
#include "Sim.h"
#include "SimRegion.h"

class SQuantumTravelPopup : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SQuantumTravelPopup) {}
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
        SLATE_ARGUMENT(TFunction<bool()>, CanOperate)
        SLATE_EVENT(FSimpleDelegate, OnClose)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        ResolveShip = Args._ResolveShip; CanOperate = Args._CanOperate; Close = Args._OnClose;
        Blue = LoadStyle(TEXT("Blue")); Green = LoadStyle(TEXT("Green")); Red = LoadStyle(TEXT("Red"));
        SAssignNew(Rows, SVerticalBox);
        Rebuild();
        auto Content = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(16)[Text(TEXT("QUANTUM / FARCASTER NAVIGATION"), 24)]
            + SVerticalBox::Slot().AutoHeight().Padding(16,4)[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.3f,0.8f,1.f)).Font(MissionUIStyle::GetRowFont(16)).Text_Lambda([this](){Ship* P=Player();FString Status=QuantumTravel::Readiness(P);return FText::FromString(Status.IsEmpty()?TEXT("Quantum drive ready. Select a destination to begin its countdown."):Status);})]
            + SVerticalBox::Slot().FillHeight(1).Padding(16,8)[SNew(SScrollBox)+SScrollBox::Slot()[Rows.ToSharedRef()]]
            + SVerticalBox::Slot().AutoHeight().Padding(16,4)[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(1.f,0.8f,0.4f)).Font(MissionUIStyle::GetRowFont(15)).Text_Lambda([this](){return FText::FromString(Message);})]
            + SVerticalBox::Slot().AutoHeight().Padding(16,8)
             [SNew(SHorizontalBox)
              +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).ButtonStyle(&Blue).OnClicked_Lambda([this](){Rebuild();return FReply::Handled();})[Text(TEXT("REFRESH"))]]
              +SHorizontalBox::Slot().FillWidth(2).Padding(12,0)[SNew(SButton).ButtonStyle(&Red).IsEnabled_Lambda([this](){Ship* P=Player();auto* D=P?P->GetQuantumDrive():nullptr;return Operate() && D && (D->ActiveState()==QuantumDrive::ACTIVE_COUNTDOWN || D->ActiveState()==QuantumDrive::ACTIVE_PREWARP);}).OnClicked_Lambda([this](){Ship* P=Player();if(Operate() && P && P->GetQuantumDrive())P->GetQuantumDrive()->CancelJump();Message=TEXT("Quantum countdown aborted; drive discharged.");return FReply::Handled();})[Text(TEXT("ABORT COUNTDOWN"))]]
              +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).ButtonStyle(&Red).OnClicked_Lambda([this](){Close.ExecuteIfBound();return FReply::Handled();})[Text(TEXT("CLOSE"))]]];
        ChildSlot[SNew(SOverlay)
            +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.005f,0.01f,0.025f,0.8f))]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1040).HeightOverride(720)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.015f,0.035f,0.065f,1)).Padding(12)[Content]]]]];
    }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override
    {
        if (Event.IsRepeat()) return FReply::Handled();
        if (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Q) { Close.ExecuteIfBound(); return FReply::Handled(); }
        const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
        for(int Index=0;Index<9;++Index)if(Event.GetKey()==Keys[Index] && Destinations.IsValidIndex(Index)){Engage(Destinations[Index]);break;}
        return FReply::Handled();
    }
private:
    TFunction<Ship*()> ResolveShip;
    TFunction<bool()> CanOperate;
    FSimpleDelegate Close;
    TSharedPtr<SVerticalBox> Rows;
    TArray<QuantumTravel::FDestination> Destinations;
    TArray<TStrongObjectPtr<UTexture2D>> Textures;
    FButtonStyle Blue, Green, Red;
    FString Message;
    Ship* Player() const {return ResolveShip?ResolveShip():nullptr;}
    bool Operate() const {return !CanOperate || CanOperate();}
    TSharedRef<STextBlock> Text(const FString& Value,int Size=16)
    {return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(FLinearColor::White).AutoWrapText(true).Font(MissionUIStyle::GetHeaderFont(Size));}
    FButtonStyle LoadStyle(const TCHAR* Color)
    {
        FButtonStyle Style=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
        auto Brush=[&](const TCHAR* State){FSlateBrush B;const FString Path=FString::Printf(TEXT("/Game/SciFiUIComponents/Textures/Button/T_Btn01%s_%s.T_Btn01%s_%s"),Color,State,Color,State);if(auto* T=LoadObject<UTexture2D>(nullptr,*Path)){Textures.Emplace(T);B.SetResourceObject(T);B.DrawAs=ESlateBrushDrawType::Image;}return B;};
        Style.Normal=Brush(TEXT("n"));Style.Hovered=Brush(TEXT("p"));Style.Pressed=Style.Hovered;return Style;
    }
    void Engage(const QuantumTravel::FDestination& Destination)
    {
        if(!Operate())return;
        if(QuantumTravel::Engage(Sim::GetSim(),Player(),Destination.System,Destination.Region,Message))Close.ExecuteIfBound();
    }
    void Rebuild()
    {
        Rows->ClearChildren();
        Destinations=QuantumTravel::GetDestinations(Sim::GetSim(),Player());
        Rows->AddSlot().AutoHeight().Padding(4,10)[Text(TEXT("QUANTUM DESTINATIONS"),18)];
        if(Destinations.IsEmpty())Rows->AddSlot().AutoHeight().Padding(4)[Text(TEXT("No reachable space regions in this mission."))];
        for(int Index=0;Index<Destinations.Num();++Index)
        {
            const QuantumTravel::FDestination Destination=Destinations[Index];
            Rows->AddSlot().AutoHeight().Padding(4)[SNew(SButton).ButtonStyle(&Green).ContentPadding(FMargin(16,10)).IsEnabled_Lambda([this](){return Operate() && QuantumTravel::Readiness(Player()).IsEmpty();}).OnClicked_Lambda([this,Destination](){Engage(Destination);return FReply::Handled();})[Text(FString::Printf(TEXT("%d.  %s"),Index+1,*Destination.Label))]];
        }
        Rows->AddSlot().AutoHeight().Padding(4,18,4,6)[Text(TEXT("LOCAL FARCASTERS"),18)];
        Rows->AddSlot().AutoHeight().Padding(4)[Text(TEXT("Target a gate, then fly within 1 km of it. Transit begins only when the linked gate is operational. No onboard quantum drive is required."),14)];
        const auto Gates=QuantumTravel::GetGates(Sim::GetSim(),Player());
        if(Gates.IsEmpty())Rows->AddSlot().AutoHeight().Padding(4)[Text(TEXT("No Farcaster in the current region."))];
        for(const auto& Gate:Gates)
        {
            Rows->AddSlot().AutoHeight().Padding(4)[SNew(SButton).ButtonStyle(&Blue).ContentPadding(FMargin(16,10)).IsEnabled_Lambda([this](){return Operate();}).OnClicked_Lambda([this,Gate](){if(Operate() && QuantumTravel::TargetGate(Sim::GetSim(),Player(),Gate.Name,Message))Close.ExecuteIfBound();return FReply::Handled();})[Text(FString::Printf(TEXT("TARGET  %s   |   %s   |   %.1f km   |   %s"),*Gate.Name,*Gate.Destination,Gate.RangeKm,Gate.Ready?TEXT("READY"):TEXT("NOT READY")))]];
        }
    }
};
