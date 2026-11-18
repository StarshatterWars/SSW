#include "AwardsDlg.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "PlayerCharacter.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

namespace
{
class SPilotAwards : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPilotAwards) {} SLATE_END_ARGS()
    void Construct(const FArguments&, const TArray<FMedalInfo>& Earned)
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        TSharedRef<STextBlock> Details = SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
            .Text(Earned.IsEmpty() ? INVTEXT("No decorations earned yet.") : INVTEXT("Select a decoration to view its citation."));
        for (const FMedalInfo& Medal : Earned)
        {
            Rows->AddSlot().AutoHeight().Padding(0,2)
                [SNew(SButton).ToolTipText(FText::FromString(Medal.Desc))
                    .ButtonColorAndOpacity_Lambda([this, Id=Medal.MedalId]() { return Selected==Id ? FLinearColor(.08f,.3f,.42f) : FLinearColor(.03f,.09f,.13f); })
                    .OnClicked_Lambda([this, Medal, Details]() { Selected=Medal.MedalId; Details->SetText(FText::FromString(Medal.Name+TEXT("\n\n")+Medal.Desc+TEXT("\n\n")+Medal.AwardText)); return FReply::Handled(); })
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(6)[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Awards)]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6)
                            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(FText::FromString(Medal.Name).ToUpper())]]];
        }
        ChildSlot[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(15))
                .Text(FText::Format(INVTEXT("DECORATIONS EARNED: {0}"),FText::AsNumber(Earned.Num())))]
            + SVerticalBox::Slot().FillHeight(.6f).Padding(8)[SNew(SScrollBox)+SScrollBox::Slot()[Rows]]
            + SVerticalBox::Slot().FillHeight(.4f).Padding(8)[SNew(SScrollBox)+SScrollBox::Slot()[Details]])];
    }
private:
    int32 Selected=0;
};
}
TSharedRef<SWidget> UAwardsDlg::RebuildWidget()
{
    UGameInstance* Instance=GetGameInstance();
    const auto* Players=Instance ? Instance->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
    const auto* Data=Instance ? Instance->GetSubsystem<UStarshatterGameDataSubsystem>() : nullptr;
    FS_PlayerGameInfo Pilot=Players ? Players->GetPlayerInfo() : FS_PlayerGameInfo{};
    if (const auto* Live=PlayerCharacter::GetCurrentPlayer(); Live && (Pilot.Name.IsEmpty() || Pilot.Name.Equals(Live->GetName(),ESearchCase::IgnoreCase))) Live->ToPlayerInfo(Pilot);
    TArray<FMedalInfo> Earned;
    for (uint32 Bit=0;Data && Bit<32;++Bit)
    {
        const uint32 Flag=1u<<Bit; FMedalInfo Medal;
        if ((uint32(Pilot.MedalsMask)&Flag) && Data->GetMedalInfoByFlag(Flag,Medal)) Earned.Add(Medal);
    }
    return SNew(SPilotAwards,Earned);
}
