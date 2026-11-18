#include "HangarDlg.h"
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
    const auto Header = SNew(SHorizontalBox);
    for (const TCHAR* Label : {TEXT("SHIP / FIGHTER"), TEXT("DESIGN"), TEXT("LOCATION"), TEXT("COVERAGE"), TEXT("HULL")})
        Header->AddSlot().FillWidth(1).Padding(8, 5)
            [SNew(STextBlock).Text(FText::FromString(Label)).Font(MissionUIStyle::GetRowFont(12))];
    Header->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(176)];
    Rows->AddSlot().AutoHeight()[Header];
    for (const auto& Hull : Insurance->GetShips())
    {
        const auto Line = SNew(SHorizontalBox);
        for (const FString& Label : {Hull.Name, Hull.Design.ToString(), Hull.Location})
            Line->AddSlot().FillWidth(1).Padding(8, 6)
                [SNew(STextBlock).Text(FText::FromString(Label).ToUpper())
                    .Font(MissionUIStyle::GetRowFont(12)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                    .ToolTipText(FText::FromString(Label))];
        Line->AddSlot().FillWidth(1).Padding(8, 6)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance), Id = Hull.HullId]()
                {
                    const auto* Service = Weak.Get();
                    const auto* Entry = Service ? Service->FindShip(Id) : nullptr;
                    if (!Entry) return FText::GetEmpty();
                    if (Entry->bLost) return INVTEXT("LOST");
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
        const auto* Design = ShipDesignRegistry::Find(Hull.Design);
        const bool bFighter = Design && Design->Category == EShipCategory::Fighter;
        const auto Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Insurance);
        const auto Docked = [Weak, Id = Hull.HullId]()
        {
            const auto* Service = Weak.Get();
            const auto* Entry = Service ? Service->FindShip(Id) : nullptr;
            return Entry && !Entry->bLost && Service->CanModifyHull(Id);
        };
        Line->AddSlot().AutoWidth().Padding(4)
            [
                SNew(SButton).ToolTipText(INVTEXT("Select hull — docked service")).IsEnabled_Lambda(Docked)
                .OnClicked_Lambda([Weak, Id = Hull.HullId, Feedback, Selected]()
                {
                    if (auto* Service = Weak.Get())
                        if (Service->SelectHull(Id, *Feedback)) *Selected = Id;
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]
            ];
        const FOpenShipInsurance Edit = bFighter ? OnLoadout : OnFit;
        Line->AddSlot().AutoWidth().Padding(4)
            [
                SNew(SButton).ToolTipText(bFighter ? INVTEXT("Loadout — docked service") : INVTEXT("Fit — docked service"))
                .IsEnabled_Lambda(Docked)
                .OnClicked_Lambda([Edit, Id = Hull.HullId]() { Edit.ExecuteIfBound(Id); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(bFighter ? EShipSystemIcon::Loadout : EShipSystemIcon::SettingsGear)]
            ];
        const FOpenShipInsurance Open = OnInsurance;
        Line->AddSlot().AutoWidth().Padding(4)
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
        Line->AddSlot().AutoWidth().Padding(4)
            [SNew(SButton).ToolTipText_Lambda([Repairable]()
                { return Repairable() ? FText::Format(INVTEXT("Full repair — {0} credits"), FText::AsNumber(FStationServices::FullRepairCost())) : INVTEXT("Repair — dock the assigned fighter or ship first"); })
                .IsEnabled_Lambda([Repairable]() { return Repairable() && FStationServices::FullRepairCost() > 0; })
                .OnClicked_Lambda([Instance = TWeakObjectPtr<UGameInstance>(GetGameInstance()), Feedback, Repairable]()
                { if (Repairable()) FStationServices::RepairAll(Instance.Get(), *Feedback); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Repair)]];
        Rows->AddSlot().AutoHeight().Padding(0, 2)
            [
                SNew(SButton).ContentPadding(0).HAlign(HAlign_Fill)
                .ButtonColorAndOpacity_Lambda([Selected, Id = Hull.HullId]()
                { return *Selected == Id ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
                .OnClicked_Lambda([Selected, Id = Hull.HullId]() { *Selected = Id; return FReply::Handled(); })
                [Line]
            ];
    }
    if (Insurance->GetShips().IsEmpty())
        Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("No ships in your hangar."))];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)
        [MissionUIStyle::MakeDataPane(SNew(SScrollBox) + SScrollBox::Slot()[Rows])]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(STextBlock).Text_Lambda([Feedback]() { return *Feedback; })];
}
