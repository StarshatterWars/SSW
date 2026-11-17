#include "InsuranceDlg.h"
#include "ShipInsuranceSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UInsuranceDlg::RebuildWidget()
{
    // UMG can rebuild this panel in the editor without a live game instance.
    if (IsDesignTime())
        return SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(18)).Text(INVTEXT("INSURANCE"));
    auto* Instance = GetGameInstance();
    auto* Service = Instance ? Instance->GetSubsystem<UShipInsuranceSubsystem>() : nullptr;
    if (!Service)
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
            .Text(INVTEXT("Hangar services are available during a game session."));
    const auto* Ship = Service->FindShip(Hull);
    if (!Ship) return SNew(STextBlock).Text(INVTEXT("Select a ship in Hangar."));
    const auto Selected = MakeShared<EShipInsuranceTier>(EShipInsuranceTier::Basic);
    const auto Feedback = MakeShared<FText>(Ship->bAssigned
        ? INVTEXT("Mission-assigned craft. Personal insurance applies to owned hulls.")
        : Ship->bCashPurchased ? INVTEXT("Lifetime full cover — hull and installed components.") : FText::GetEmpty());
    const auto Table = SNew(SVerticalBox);
    const auto Header = SNew(SHorizontalBox);
    for (const TCHAR* Label : {TEXT("COVERAGE"), TEXT("PREMIUM (CR)"), TEXT("PAYOUT (CR)"), TEXT("DURATION")})
        Header->AddSlot().FillWidth(1).Padding(8)
            [SNew(STextBlock).Text(FText::FromString(Label)).Font(MissionUIStyle::GetRowFont(12))];
    Table->AddSlot().AutoHeight()[Header];
    for (int32 Index = 0; Index <= 6; ++Index)
    {
        const auto Offer = UShipInsuranceSubsystem::Quote(*Ship, static_cast<EShipInsuranceTier>(Index));
        const auto Line = SNew(SHorizontalBox);
        const FText Labels[] = {
            FText::Format(INVTEXT("{0} ({1})"), FText::FromString(UShipInsuranceSubsystem::TierName(Offer.Tier)),
                FText::AsPercent(Offer.Coverage)),
            FText::AsNumber(Offer.Premium), FText::AsNumber(Offer.Payout),
            Ship->bCashPurchased ? INVTEXT("Lifetime") : Index == 0 ? INVTEXT("Default") : INVTEXT("12 weeks")};
        for (const FText& Label : Labels)
            Line->AddSlot().FillWidth(1).Padding(8)
                [SNew(STextBlock).Text(Label.ToUpper()).Font(MissionUIStyle::GetRowFont(12))];
        Table->AddSlot().AutoHeight().Padding(0, 2)
            [
                SNew(SButton).HAlign(HAlign_Fill)
                .ButtonColorAndOpacity_Lambda([Selected, Tier = Offer.Tier]()
                { return *Selected == Tier ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
                .OnClicked_Lambda([Selected, Tier = Offer.Tier]()
                { *Selected = Tier; return FReply::Handled(); })
                [Line]
            ];
        if (Ship->bCashPurchased) { *Selected = EShipInsuranceTier::Lifetime; break; }
    }
    const auto Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Service);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(12)
        [SNew(STextBlock).Text(FText::FromString(Ship->Name).ToUpper()).Font(MissionUIStyle::GetHeaderFont(18))]
        + SVerticalBox::Slot().FillHeight(1)
        [MissionUIStyle::MakeDataPane(SNew(SScrollBox) + SScrollBox::Slot()[Table])]
        + SVerticalBox::Slot().AutoHeight().Padding(12)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
                .Text_Lambda([Weak, Feedback]()
                {
                    if (!Feedback->IsEmpty()) return *Feedback;
                    return FText::Format(INVTEXT("Balance: {0} CR"), FText::AsNumber(Weak.IsValid() ? Weak->GetBalance() : 0));
                })
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton).ToolTipText(INVTEXT("Buy insurance â€” docked service"))
                .IsEnabled_Lambda([Weak, Selected, Id = Hull]()
                {
                    auto* Insurance = Weak.Get();
                    const auto* Entry = Insurance ? Insurance->FindShip(Id) : nullptr;
                    return Entry && !Entry->bLost && !Entry->bAssigned && !Entry->bCashPurchased && Insurance->CanModifyHull(Id) &&
                        *Selected >= EShipInsuranceTier::Basic && *Selected <= EShipInsuranceTier::Platinum &&
                        UShipInsuranceSubsystem::EffectiveTier(*Entry, FDateTime::UtcNow().ToUnixTimestamp()) == EShipInsuranceTier::Uninsured &&
                        Insurance->GetBalance() >= UShipInsuranceSubsystem::Quote(*Entry, *Selected).Premium;
                })
                .OnClicked_Lambda([Weak, Selected, Feedback, Id = Hull]()
                {
                    if (auto* Insurance = Weak.Get()) Insurance->PurchasePolicy(Id, *Selected, *Feedback);
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Insurance)]
            ]
        ];
}
