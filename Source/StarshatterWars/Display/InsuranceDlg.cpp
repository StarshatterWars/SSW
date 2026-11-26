#include "InsuranceDlg.h"
#include "OwnedCommandAI.h"
#include "ShipInsuranceSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Misc/DateTime.h"
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
        [SNew(STextBlock).Text(FText::FromString(Ship->Name).ToUpper()).Font(MissionUIStyle::GetHeaderFont(16))]
        + SVerticalBox::Slot().AutoHeight().Padding(12,0,12,8)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)
            .Text_Lambda([Weak,Id=Hull]()
            {
                const auto* Entry=Weak.IsValid()?Weak->FindShip(Id):nullptr;
                return Entry ? FText::FromString(FString::Printf(TEXT("%s  |  %s"),
                    *Entry->Design.ToString(),Entry->Location.IsEmpty()?TEXT("LOCATION UNKNOWN"):*Entry->Location)).ToUpper()
                    : INVTEXT("HULL UNAVAILABLE");
            })
            .ToolTipText(FText::FromString(FString::Printf(TEXT("HULL ID: %s"),*Hull.ToString())))]
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
        [MissionUIStyle::MakeDataPane(SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
            .Text_Lambda([Weak,Id=Hull]()
            {
                const auto* Entry=Weak.IsValid()?Weak->FindShip(Id):nullptr;
                if(!Entry)return INVTEXT("POLICY UNAVAILABLE");
                if(Entry->bAssigned)return INVTEXT("MISSION LOANER - PERSONAL INSURANCE DOES NOT APPLY");
                if(Entry->bLost)return INVTEXT("HULL LOST - NO ACTIVE COVERAGE");
                const auto Tier=UShipInsuranceSubsystem::EffectiveTier(*Entry,FDateTime::UtcNow().ToUnixTimestamp());
                if(Tier==EShipInsuranceTier::Lifetime)
                    return INVTEXT("CURRENT COVERAGE: LIFETIME FULL COVER - NO EXPIRY");
                const int64 Expiry=Entry->Insurance.ExpiresUtc;
                if(Tier==EShipInsuranceTier::Uninsured)
                {
                    if(Entry->Insurance.bSettled)return INVTEXT("CURRENT COVERAGE: UNINSURED - PREVIOUS POLICY SETTLED");
                    if(Expiry>0 && Expiry<=253402300799LL)
                        return FText::FromString(TEXT("CURRENT COVERAGE: UNINSURED - PREVIOUS POLICY EXPIRED ")+
                            FDateTime::FromUnixTimestamp(Expiry).ToString(TEXT("%Y-%m-%d %H:%M GMT")));
                    return INVTEXT("CURRENT COVERAGE: UNINSURED");
                }
                const FString Ends=Expiry>0 && Expiry<=253402300799LL
                    ? FDateTime::FromUnixTimestamp(Expiry).ToString(TEXT("%Y-%m-%d %H:%M GMT")) : TEXT("NOT RECORDED");
                return FText::FromString(FString::Printf(TEXT("CURRENT COVERAGE: %s  |  EXPIRES: %s"),
                    *UShipInsuranceSubsystem::TierName(Tier),*Ends)).ToUpper();
            }))]
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
                SNew(SButton).ToolTipText(INVTEXT("Buy insurance - docked service"))
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
                    if (auto* Insurance = Weak.Get()){FOwnedActionSelection W;W.Action=EOwnedCommand::Policy;W.Hull=Id;W.Tier=*Selected;Insurance->RequestOwnedAction(W,[Feedback](bool,const FText& Why,FGuid){*Feedback=Why;},*Feedback);}
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Insurance)]
            ]
        ];
}
