#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/GameInstance.h"
#include "EconomyServiceSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "SimDataEvents.h"
#include "MissionUIStyle.h"
class SEconomyServiceStatusPanel:public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEconomyServiceStatusPanel){}SLATE_ARGUMENT(UGameInstance*,GameInstance)SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Instance=Args._GameInstance;
        ChildSlot[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[SNew(STextBlock).Text(INVTEXT("ECONOMY SERVER")).Font(MissionUIStyle::GetHeaderFont(16))]
            +SVerticalBox::Slot().AutoHeight()[SAssignNew(Summary,STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[SAssignNew(Pending,STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)])];
        if(auto* Service=Instance.IsValid()?Instance->GetSubsystem<UEconomyServiceSubsystem>():nullptr)ConnectionHandle=Service->OnStatusChanged.AddSP(this,&SEconomyServiceStatusPanel::Refresh);
        DataHandle=FSimDataEvents::DetailedChanged().AddSP(this,&SEconomyServiceStatusPanel::DataChanged);Refresh();
    }
    ~SEconomyServiceStatusPanel()
    {
        if(auto* Service=Instance.IsValid()?Instance->GetSubsystem<UEconomyServiceSubsystem>():nullptr)Service->OnStatusChanged.Remove(ConnectionHandle);
        FSimDataEvents::DetailedChanged().Remove(DataHandle);
    }
private:
    void DataChanged(const FEconomyState&,const FSimDataChange& Change)
    {if(Change.AffectsOwner(FEconomyLedger::Pilot()))Refresh();}
    void Refresh()
    {
        auto* Service=Instance.IsValid()?Instance->GetSubsystem<UEconomyServiceSubsystem>():nullptr;
        Summary->SetText(FText::FromString(Service?Service->Summary():TEXT("NOT CONNECTED\nNo economy service is attached to this session.")));
        Summary->SetColorAndOpacity(Service&&Service->IsReady()?FLinearColor(.4f,.85f,.65f):FLinearColor(.72f,.8f,.84f));
        int32 Count=0;if(auto* Player=Instance.IsValid()?Instance->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr)
            for(const auto& P:Player->GetPlayerInfo().Economy.NpcTradeOutbox)if(P.Value.Phase==ENpcTradePhase::Escrowed)++Count;
        Pending->SetText(FText::FromString(FString::Printf(TEXT("PENDING PLAYER TRADES: %d\nPlayer assets remain in the player record. NPC stock and production belong to the economy service."),Count)));
    }
    TWeakObjectPtr<UGameInstance> Instance;
    TSharedPtr<STextBlock> Summary,Pending;
    FDelegateHandle ConnectionHandle,DataHandle;
};
