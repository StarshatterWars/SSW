#include "HangarDlg.h"
#include "OwnedCommandAI.h"
#include "StarterCraftOwnership.h"
#include "StationServices.h"
#include "StarshatterPlayerSubsystem.h"
#include "ShipInsuranceSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "ShipDesignRegistry.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UHangarDlg::RebuildWidget()
{
    // UMG can rebuild this panel in the editor without a live game instance.
    if (IsDesignTime())
        return SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(18)).Text(INVTEXT("HANGAR"));
    auto* Instance = GetGameInstance();
    auto* Insurance = Instance ? Instance->GetSubsystem<UShipInsuranceSubsystem>() : nullptr;
    if (!Insurance)
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
            .Text(INVTEXT("Hangar services are available during a game session."));
    Insurance->RefreshAssignment();
    const auto Rows = SNew(SVerticalBox);
    const auto Selected = MakeShared<FGuid>();
    const auto Feedback = MakeShared<FText>();
    const auto Confirm = MakeShared<FGuid>();
    const auto Header = SNew(SHorizontalBox);
    for (const TCHAR* Label : {TEXT("SHIP / FIGHTER"), TEXT("DESIGN"), TEXT("HULL ID"), TEXT("LOCATION"), TEXT("COVERAGE"), TEXT("HULL"), TEXT("STATUS")})
        Header->AddSlot().FillWidth(1).Padding(8, 5)
            [SNew(STextBlock).Text(FText::FromString(Label)).Font(MissionUIStyle::GetRowFont(12))];
    Header->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(220)];
    Rows->AddSlot().AutoHeight()[Header];
    for (const auto& Hull : Insurance->GetShips())
    {
        const auto* Players=Instance->GetSubsystem<UStarshatterPlayerSubsystem>();
        if(!Players || !StarterCraftOwnership::VisibleInHangar(Hull,Players->GetPlayerInfo().AssignedHangarHull))continue;
        const auto Line = SNew(SHorizontalBox);
        for (const FString& Label : {Hull.Name, Hull.Design.ToString()})
            Line->AddSlot().FillWidth(1).Padding(8, 6)
                [SNew(STextBlock).Text(FText::FromString(Label).ToUpper())
                    .Font(MissionUIStyle::GetRowFont(12)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                    .ToolTipText(FText::FromString(Label))];
        Line->AddSlot().FillWidth(1).Padding(8,6)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11))
                .Text(FText::FromString(Hull.HullId.ToString(EGuidFormats::DigitsWithHyphens).ToUpper()))
                .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                .ToolTipText(FText::FromString(TEXT("HULL ID: ")+Hull.HullId.ToString(EGuidFormats::DigitsWithHyphens).ToUpper()))];
        const auto LocationText = [Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance), Id = Hull.HullId]()
        {
            const auto* Entry = Weak.IsValid() ? Weak->FindShip(Id) : nullptr;
            return Entry && !Entry->Location.IsEmpty() ? FText::FromString(Entry->Location).ToUpper() : INVTEXT("LOCATION UNKNOWN");
        };
        Line->AddSlot().FillWidth(1).Padding(8, 6)
            [SNew(STextBlock).Text_Lambda(LocationText).ToolTipText_Lambda(LocationText)
                .Font(MissionUIStyle::GetRowFont(12)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)];
        Line->AddSlot().FillWidth(1).Padding(8, 6)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance), Id = Hull.HullId]()
                {
                    const auto* Service = Weak.Get();
                    const auto* Entry = Service ? Service->FindShip(Id) : nullptr;
                    if (!Entry) return FText::GetEmpty();
                    if (Entry->bLost) return INVTEXT("LOST");
                    if (Entry->RentalId.IsValid()) return INVTEXT("RENTED / BASIC");
                    return FText::FromString(UShipInsuranceSubsystem::TierName(
                        UShipInsuranceSubsystem::EffectiveTier(*Entry, FDateTime::UtcNow().ToUnixTimestamp()))).ToUpper();
                })];
        Line->AddSlot().FillWidth(1).Padding(8,6)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Weak=TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance),Id=Hull.HullId]()
                {
                    const auto* Entry=Weak.IsValid()?Weak->FindShip(Id):nullptr;
                    if (!Entry)return FText::GetEmpty();
                    if (Entry->bAssigned)return INVTEXT("LOANER");
                    return FText::FromString(FString::Printf(TEXT("%.0f%%"),100.f*(Entry->bHasCondition?Entry->HullCondition:1.f)));
                })];
        Line->AddSlot().FillWidth(1).Padding(8, 6)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Game = TWeakObjectPtr<UGameInstance>(Instance), Id = Hull.HullId]()
                {
                    const auto* Players = Game.IsValid() ? Game->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
                    if (!Players) return FText::GetEmpty();
                    const auto& Profile = Players->GetPlayerInfo();
                    if (Profile.AssignedHangarHull == Id) return INVTEXT("ACTIVE");
                    if (Profile.SelectedHangarHull == Id) return INVTEXT("SELECTED");
                    return INVTEXT("STORED");
                })
                .ToolTipText(INVTEXT("Active: craft currently assigned to you. Selected: saved choice for your next Free Verse entry. Stored: available in your hangar."))];
        const auto* Design = ShipDesignRegistry::Find(Hull.Design);
        const bool bFighter = Design && Design->Category == EShipCategory::Fighter;
        const auto Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance);
        const auto Docked = [Weak, Id = Hull.HullId]()
        {
            const auto* Service = Weak.Get();
            const auto* Entry = Service ? Service->FindShip(Id) : nullptr;
            return Entry && !Entry->bLost && Service->CanModifyHull(Id);
        };
        const auto Actions=SNew(SHorizontalBox);
        Line->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(220)[Actions]];
        Actions->AddSlot().AutoWidth().Padding(4)
            [
                SNew(SButton).ToolTipText(INVTEXT("Select hull for your next Free Verse entry — docked service. Training missions board their required fighter automatically.")).IsEnabled_Lambda(Docked)
                .OnClicked_Lambda([Weak, Id = Hull.HullId, Feedback, Selected]()
                {
                    if (auto* Service = Weak.Get())
                        if (Service->SelectHull(Id, *Feedback)) *Selected = Id;
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]
            ];
        if(bFighter)
        {
            const FOpenShipInsurance Loadout=OnLoadout;
            Actions->AddSlot().AutoWidth().Padding(4)
                [SNew(SButton).ToolTip(MissionUIStyle::MakeToolTip(INVTEXT("Loadout - weapons and missiles; docked service")))
                    .IsEnabled_Lambda(Docked)
                    .OnClicked_Lambda([Loadout,Id=Hull.HullId](){Loadout.ExecuteIfBound(Id);return FReply::Handled();})
                    [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Loadout)]];
        }
        const FOpenShipInsurance Fit=OnFit;
        Actions->AddSlot().AutoWidth().Padding(4)
            [SNew(SButton).ToolTip(MissionUIStyle::MakeToolTip(INVTEXT("Fit - install owned components purchased in Ship Store; requires a docked fitting service")))
                .IsEnabled_Lambda(Docked)
                .OnClicked_Lambda([Fit,Id=Hull.HullId](){Fit.ExecuteIfBound(Id);return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::SettingsGear)]];
        const FOpenShipInsurance Open = OnInsurance;
        Actions->AddSlot().AutoWidth().Padding(4)
            [
                SNew(SButton).ToolTipText(INVTEXT("Insurance")).IsEnabled_Lambda(Docked)
                .OnClicked_Lambda([Open, Id = Hull.HullId, Selected]()
                {
                    *Selected = Id;
                    Open.ExecuteIfBound(Id);
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Insurance)]
            ];
        const auto Repairable = [Instance = TWeakObjectPtr<UGameInstance>(GetGameInstance()), Id = Hull.HullId]()
        {
            const auto* Players = Instance.IsValid() ? Instance->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
            return Players && Players->GetPlayerInfo().AssignedHangarHull == Id && FStationServices::IsDocked();
        };
        Actions->AddSlot().AutoWidth().Padding(4)
            [SNew(SButton).ToolTipText_Lambda([Repairable]()
                { return Repairable() ? FText::Format(INVTEXT("Full repair — {0} credits"), FText::AsNumber(FStationServices::FullRepairCost())) : INVTEXT("Repair — dock the assigned fighter or ship first"); })
                .IsEnabled_Lambda([Repairable]() { return Repairable() && FStationServices::FullRepairCost() > 0; })
                .OnClicked_Lambda([Instance = TWeakObjectPtr<UGameInstance>(GetGameInstance()), Feedback, Repairable]()
                { if (Repairable()) FStationServices::RequestRepair(Instance.Get(),[Feedback](bool,const FText& Why,FGuid){*Feedback=Why;},*Feedback); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Repair)]];
        Rows->AddSlot().AutoHeight().Padding(0, 2)
            [
                SNew(SButton).ContentPadding(0).HAlign(HAlign_Fill)
                .Visibility_Lambda([Weak,Id=Hull.HullId](){const auto* H=Weak.IsValid()?Weak->FindShip(Id):nullptr;return H && !H->bSold && !H->bLost?EVisibility::Visible:EVisibility::Collapsed;})
                .ButtonColorAndOpacity_Lambda([Selected, Id = Hull.HullId]()
                { return *Selected == Id ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
                .OnClicked_Lambda([Selected,Confirm, Id = Hull.HullId]() { *Selected = Id;Confirm->Invalidate();return FReply::Handled(); })
                [Line]
            ];
    }
    if (Insurance->GetShips().IsEmpty())
        Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("No ships in your hangar."))];
    const auto RentalRows=SNew(SVerticalBox);
    const auto Weak=TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance);
    if(const auto* Players=Instance->GetSubsystem<UStarshatterPlayerSubsystem>())for(const auto& C:Players->GetPlayerInfo().ShipRentals)
    {
        if(C.State!=EShipRentalState::Active && C.UnpaidLiability<=0)continue;
        RentalRows->AddSlot().AutoHeight().Padding(8,3)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)
                .Text_Lambda([Game=TWeakObjectPtr<UGameInstance>(Instance),Id=C.Id]()
                {
                    const auto* P=Game.IsValid()?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
                    const auto* R=P?P->GetPlayerInfo().ShipRentals.FindByPredicate([Id](const auto& E){return E.Id==Id;}):nullptr;if(!R)return FText::GetEmpty();
                    return FText::FromString(FString::Printf(TEXT("RENTAL %s | %s | DUE %s GMT\nRETURN: %s | OWNER: %s | FEE %lld CR / PREMIUM %lld CR | UNPAID LIABILITY %lld CR"),
                        *R->HullId.ToString(EGuidFormats::Short),R->State==EShipRentalState::Active?(FDateTime::UtcNow().ToUnixTimestamp()>=R->DueUtc?TEXT("OVERDUE - RETURN REQUIRED"):TEXT("ACTIVE")):TEXT("CLOSED"),
                        *FDateTime::FromUnixTimestamp(R->DueUtc).ToString(TEXT("%Y-%m-%d %H:%M")),*R->ReturnDock.ToUpper(),*R->Lessor.ToString().ToUpper(),R->Fee,R->Premium,R->UnpaidLiability));
                })]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Pay outstanding rental loss liability from your wallet; docked service"))
                .IsEnabled_Lambda([Game=TWeakObjectPtr<UGameInstance>(Instance),Id=C.Id,Weak](){const auto* P=Game.IsValid()?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;return P && Weak.IsValid() && Weak->CanInsure() && P->GetPlayerInfo().ShipRentals.ContainsByPredicate([Id](const auto& R){return R.Id==Id && R.UnpaidLiability>0;});})
                .OnClicked_Lambda([Weak,Id=C.Id,Feedback](){if(Weak.IsValid()){FOwnedActionSelection W;W.Action=EOwnedCommand::Debt;W.Hull=Id;Weak->RequestOwnedAction(W,[Feedback](bool,const FText& Why,FGuid){*Feedback=Why;},*Feedback);}return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Marketplace)]]];
    }
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)
        [MissionUIStyle::MakeDataPane(SNew(SScrollBox) + SScrollBox::Slot()[Rows])]
        +SVerticalBox::Slot().AutoHeight().Padding(8,4)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(11))
                .Text_Lambda([Weak,Selected](){FText Why;if(Weak.IsValid()){const auto* H=Weak->FindShip(*Selected);if(H && H->RentalId.IsValid())Weak->CanReturnRental(*Selected,Why);else Weak->CanSellHull(*Selected,Why);}return Why;})]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton)
                .ToolTipText(INVTEXT("Sell an owned hull or return a rental. Select a row, review the quote and click twice to confirm. Active, last owned fighter, loaned and cargo-bearing hulls are protected."))
                .IsEnabled_Lambda([Weak,Selected](){FText Why;if(!Weak.IsValid())return false;const auto* H=Weak->FindShip(*Selected);return H && (H->RentalId.IsValid()?Weak->CanReturnRental(*Selected,Why):Weak->CanSellHull(*Selected,Why));})
                .OnClicked_Lambda([Weak,Selected,Confirm,Feedback](){if(!Weak.IsValid())return FReply::Handled();if(*Confirm!=*Selected){*Confirm=*Selected;*Feedback=INVTEXT("Review the quote above. Click CONFIRM again to hand over this hull.");return FReply::Handled();}const auto* H=Weak->FindShip(*Selected);FOwnedActionSelection W;W.Action=H && H->RentalId.IsValid()?EOwnedCommand::Return:EOwnedCommand::Sell;W.Hull=*Selected;Weak->RequestOwnedAction(W,[Feedback](bool,const FText& Why,FGuid){*Feedback=Why;},*Feedback);Confirm->Invalidate();return FReply::Handled();})
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Marketplace)]
                    +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text_Lambda([Weak,Selected,Confirm](){const auto* H=Weak.IsValid()?Weak->FindShip(*Selected):nullptr;return *Confirm==*Selected && Selected->IsValid()?INVTEXT("CONFIRM"):H && H->RentalId.IsValid()?INVTEXT("RETURN RENTAL"):INVTEXT("SELL HULL");})]]]]
        +SVerticalBox::Slot().AutoHeight()[SNew(SBox).MaxDesiredHeight(120)[SNew(SScrollBox)+SScrollBox::Slot()[RentalRows]]]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(STextBlock).Text_Lambda([Feedback]() { return *Feedback; })];
}
