#pragma once
#include "Styling/CoreStyle.h"

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Engine/GameInstance.h"
#include "FractalVerseSubsystem.h"
#include "MissionUIStyle.h"
#include "Styling/CoreStyle.h"

namespace FractalVerseUI
{
inline FString Connection(UGameInstance* Instance)
{
    auto* Server=Instance?Instance->GetSubsystem<UFractalVerseSubsystem>():nullptr;
    if (!Server) return TEXT("OFFLINE");
    if (Server->IsConnected()) return TEXT("ONLINE");
    return Server->Status().Universe.IsValid()?TEXT("RECONNECTING"):TEXT("CONNECTING");
}
}

class SFractalVerseStatusPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFractalVerseStatusPanel) {} SLATE_ARGUMENT(UGameInstance*, GameInstance) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Instance=Args._GameInstance;
        ChildSlot[SNew(SBox).WidthOverride(540).MaxDesiredHeight(430)
            [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
                    [SNew(STextBlock).Text(INVTEXT("UNIVERSE SERVER")).Font(MissionUIStyle::GetHeaderFont(16))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                    [SAssignNew(Summary,STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).AutoWrapText(true)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)
                    [SNew(STextBlock).Text(INVTEXT("CONNECTED PLAYERS                                      PING"))
                    .Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(FLinearColor(.45f,.8f,.9f))]
                +SVerticalBox::Slot().MaxHeight(240)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Rows,SVerticalBox)]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
                    [SNew(STextBlock).Text(INVTEXT("Universe time continues while panels are open."))
                    .Font(MissionUIStyle::GetRowFont(10)).ColorAndOpacity(FLinearColor(.6f,.72f,.8f))])]];
        Refresh();
    }
    void Tick(const FGeometry& Geometry,double Time,float Delta) override
    { SCompoundWidget::Tick(Geometry,Time,Delta); if(Time>=NextRefresh){NextRefresh=Time+1;Refresh();} }
private:
    void Refresh()
    {
        auto* Server=Instance.IsValid()?Instance->GetSubsystem<UFractalVerseSubsystem>():nullptr;
        if (!Server || !Server->IsConnected())
        {
            Summary->SetText(FText::FromString(FractalVerseUI::Connection(Instance.Get())+TEXT("\nNo confirmed universe connection. Last data is not treated as live.")));
            Rows->ClearChildren(); return;
        }
        const auto State=Server->Status();
        const FString Ping=Server->PingMilliseconds()>=0?FString::Printf(TEXT("%.0f ms"),Server->PingMilliseconds()):TEXT("—");
        const FString Date=State.UniverseBaseUtc>0?FDateTime::FromUnixTimestamp(State.UniverseBaseUtc+State.UniverseSeconds).ToString(TEXT("%Y-%m-%d %H:%M:%S GMT")):TEXT("Awaiting server clock");
        Summary->SetText(FText::FromString(FString::Printf(TEXT("ONLINE  ·  FRACTALVERSE  ·  %d / %d CLIENTS\nPING %s  ·  UPTIME %s\n%s\nREVISION %lld  ·  PROTOCOL %d  ·  CHECKPOINT %s"),
            State.ConnectedClients,State.MaximumClients,*Ping,*FTimespan::FromSeconds(State.UptimeSeconds).ToString(),*Date,
            State.Revision,State.Protocol,State.bPersistenceReady?TEXT("READY"):TEXT("UNAVAILABLE"))));
        Rows->ClearChildren();
        for(const auto& Peer:State.Players)
            Rows->AddSlot().AutoHeight().Padding(0,2)
                [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(MissionUIStyle::RowBG).Padding(FMargin(8,6))
                    [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(Peer.Name.ToUpper())).Font(FCoreStyle::GetDefaultFontStyle("Regular",12))]
                    +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.0f MS"),Peer.PingMs))).Font(FCoreStyle::GetDefaultFontStyle("Regular",12))]]];
    }
    TWeakObjectPtr<UGameInstance> Instance;
    TSharedPtr<STextBlock> Summary;
    TSharedPtr<SVerticalBox> Rows;
    double NextRefresh=0;
};

class SFractalVerseIndicator : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFractalVerseIndicator) {} SLATE_ARGUMENT(UGameInstance*, GameInstance) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Instance=Args._GameInstance;
        ChildSlot[SAssignNew(Anchor,SMenuAnchor).Placement(MenuPlacement_BelowAnchor)
            .OnGetMenuContent_Lambda([this](){return SNew(SFractalVerseStatusPanel).GameInstance(Instance.Get());})
            [SNew(SButton).ToolTipText(INVTEXT("Universe connection, players, ping and server status"))
                .ContentPadding(FMargin(12,6)).ButtonColorAndOpacity(MissionUIStyle::RowBG)
                .OnClicked_Lambda([this](){Anchor->SetIsOpen(!Anchor->IsOpen());return FReply::Handled();})
                [SAssignNew(Label,STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",12))]]];
        Refresh();
    }
    void Tick(const FGeometry& Geometry,double Time,float Delta) override
    { SCompoundWidget::Tick(Geometry,Time,Delta); if(Time>=NextRefresh){NextRefresh=Time+1;Refresh();} }
private:
    void Refresh()
    {
        const auto State=FractalVerseUI::Connection(Instance.Get());
        Label->SetText(FText::FromString(TEXT("●  UNIVERSE · ")+State));
        Label->SetColorAndOpacity(State==TEXT("ONLINE")?FLinearColor(.35f,.9f,.6f):FLinearColor(.85f,.65f,.35f));
    }
    TWeakObjectPtr<UGameInstance> Instance;
    TSharedPtr<SMenuAnchor> Anchor;
    TSharedPtr<STextBlock> Label;
    double NextRefresh=0;
};
