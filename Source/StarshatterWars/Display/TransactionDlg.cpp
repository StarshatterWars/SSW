#include "TransactionDlg.h"
#include "RecordDetails.h"
#include "TransactionAI.h"
#include "StarshatterPlayerSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/DateTime.h"

namespace
{
FString WalletName(FName Name)
{
    return Name.IsNone() ? TEXT("—") : Name.ToString().Replace(TEXT("_"), TEXT(" ")).ToUpper();
}

class STransactionWallet : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STransactionWallet) {} SLATE_END_ARGS()
    void Construct(const FArguments&, UStarshatterPlayerSubsystem* InPlayers, const UTransactionDlg* Owner)
    {
        Players = InPlayers;
        // Perform the existing one-time legacy-wallet migration, then keep view reads const.
        if (Players.IsValid()) Players->GetEconomy();
        Categories.Add(MakeShared<FString>(TEXT("ALL CATEGORIES")));
        for (int32 Index=0;Index<=static_cast<int32>(EWalletTransactionCategory::OpeningBalance);++Index)
            Categories.Add(MakeShared<FString>(StaticEnum<EWalletTransactionCategory>()->GetNameStringByValue(Index).ToUpper()));
        auto Actions = SNew(SHorizontalBox);
        auto AddAction = [&](const FText& Label, const FText& Tip, TFunction<void()> Action)
        {
            Actions->AddSlot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ToolTipText(Tip).OnClicked_Lambda([this,Action]() { PageIndex=0; Action(); return FReply::Handled(); })
                [SNew(STextBlock).Text(Label).Font(MissionUIStyle::GetRowFont(12))]];
        };
        AddAction(INVTEXT("JOURNAL"), INVTEXT("All personal credit movements, including purchases, fees, escrow and payouts."),
            [this]() { Query.bMarketOnly=false; Refresh(); });
        AddAction(INVTEXT("MARKET TRANSACTIONS"), INVTEXT("Completed purchases and sales. Open orders remain in Marketplace."),
            [this]() { Query.bMarketOnly=true; Refresh(); });
        AddAction(INVTEXT("ALL"), INVTEXT("Show credits received and spent."),
            [this]() { Query.Direction=ETransactionDirection::All; Refresh(); });
        AddAction(INVTEXT("INCOME / SALES"), INVTEXT("Incoming credits in Journal; completed sales in Market Transactions."),
            [this]() { Query.Direction=ETransactionDirection::Incoming; Refresh(); });
        AddAction(INVTEXT("EXPENSES / BUYS"), INVTEXT("Outgoing credits in Journal; completed purchases in Market Transactions."),
            [this]() { Query.Direction=ETransactionDirection::Outgoing; Refresh(); });
        AddAction(INVTEXT("DATE RANGE"), INVTEXT("Switch between all retained history and the last 30 days."),
            [this]() { bRecentOnly=!bRecentOnly; Refresh(); });

        auto Headers = SNew(SHorizontalBox);
        const TCHAR* Captions[] = { TEXT("DATE (GMT)"), TEXT("TYPE / ITEM"), TEXT("COUNTERPARTY"), TEXT("QUANTITY"), TEXT("UNIT PRICE"), TEXT("AMOUNT (CR)"), TEXT("BALANCE (CR)") };
        for (int32 Index=0; Index<7; ++Index)
            Headers->AddSlot().FillWidth(Index<3 ? 1.4f : 1.f).Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Sort this column; click again to reverse."))
                .OnClicked_Lambda([this,Index]() { if (SortColumn==Index) bDescending=!bDescending; else { SortColumn=Index; bDescending=Index==0; } Refresh(); return FReply::Handled(); })
                [SNew(STextBlock).Text(FText::FromString(Captions[Index])).Font(MissionUIStyle::GetRowFont(11))]];
        SAssignNew(Rows,SVerticalBox);
        auto Services=SNew(SHorizontalBox);
        const auto AddService=[&](EShipSystemIcon Icon,const FText& Tip,FSimpleDelegate Action)
        {
            Services->AddSlot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ToolTipText(Tip).IsEnabled(Action.IsBound())
                .OnClicked_Lambda([Action]() { Action.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(Icon)]];
        };
        AddService(EShipSystemIcon::Marketplace,INVTEXT("Open Marketplace — orders, purchases and sales"),Owner?Owner->OnOpenMarketplace:FSimpleDelegate());
        AddService(EShipSystemIcon::Contracts,INVTEXT("Open Contracts — collateral, bids and settlement"),Owner?Owner->OnOpenContracts:FSimpleDelegate());
        AddService(EShipSystemIcon::Production,INVTEXT("Open Production Facilities — manufacturing jobs"),Owner?Owner->OnOpenProduction:FSimpleDelegate());
        AddService(EShipSystemIcon::Hangar,INVTEXT("Open Hangar — insurance, fitting and repairs"),Owner?Owner->OnOpenHangar:FSimpleDelegate());
        ChildSlot[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(10,8)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1)
                [SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(17)).Text_Lambda([this]()
                { return FText::Format(INVTEXT("AVAILABLE BALANCE  {0} CR"),FText::AsNumber(Players.IsValid() ? TransactionAI::Balance(Players->GetPlayerInfo().Economy, FEconomyLedger::Pilot()) : 0)); })]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text_Lambda([this]()
                { return FText::FromString(Players.IsValid() ? Players->GetPlayerInfo().GetRankedDisplayName().ToUpper() : TEXT("NO ACTIVE PILOT")); })]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)[Actions]
            + SVerticalBox::Slot().AutoHeight().Padding(10,2)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text_Lambda([this]()
                { return FText::FromString(FString(Query.bMarketOnly ? TEXT("MARKET TRANSACTIONS") : TEXT("CREDIT JOURNAL"))+
                    (bRecentOnly ? TEXT("  •  LAST 30 DAYS") : TEXT("  •  ALL RETAINED HISTORY"))+
                    TEXT("  •  LATEST 512 RECEIPTS RETAINED")); })]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)
            [SNew(SSearchBox).HintText(INVTEXT("Search type, item, counterparty or receipt"))
                .OnTextChanged_Lambda([this](const FText& Text) { PageIndex=0; Query.Search=Text.ToString(); Refresh(); })]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)
            [SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Categories).InitiallySelectedItem(Categories[0])
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Label)
                { return SNew(STextBlock).Text(FText::FromString(*Label)).Font(MissionUIStyle::GetRowFont(11)); })
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item,ESelectInfo::Type)
                { PageIndex=0; CategoryIndex=Categories.IndexOfByKey(Item)-1; Refresh(); })
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text_Lambda([this]()
                { return FText::FromString(*Categories[FMath::Clamp(CategoryIndex+1,0,Categories.Num()-1)]); })]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)
                [SNew(SEditableTextBox).Font(MissionUIStyle::GetRowFont(11)).HintText(INVTEXT("From GMT: YYYY-MM-DD (optional)"))
                    .OnTextCommitted_Lambda([this](const FText& Text,ETextCommit::Type) { SetDate(Text,true); })]
                + SHorizontalBox::Slot().FillWidth(1)
                [SNew(SEditableTextBox).Font(MissionUIStyle::GetRowFont(11)).HintText(INVTEXT("Through GMT: YYYY-MM-DD (optional)"))
                    .OnTextCommitted_Lambda([this](const FText& Text,ETextCommit::Type) { SetDate(Text,false); })]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text(INVTEXT("MIN QUANTITY"))]
                + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,16,0)
                [SNew(SSpinBox<double>).MinValue(0).MaxValue(1.e15).Value_Lambda([this]() { return MinimumQuantity; })
                    .OnValueCommitted_Lambda([this](double Value,ETextCommit::Type) { MinimumQuantity=Value; Refresh(); })]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text(INVTEXT("MIN AMOUNT (CR)"))]
                + SHorizontalBox::Slot().FillWidth(1)
                [SNew(SSpinBox<double>).MinValue(0).MaxValue(1.e15).Value_Lambda([this]() { return MinimumAmount; })
                    .OnValueCommitted_Lambda([this](double Value,ETextCommit::Type) { MinimumAmount=Value; Refresh(); })]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true).Text_Lambda([this]() { return Summary; })]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)[Headers]
            + SVerticalBox::Slot().FillHeight(1).Padding(10,0)[SNew(SScrollBox)+SScrollBox::Slot()[Rows.ToSharedRef()]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,6)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ToolTipText(INVTEXT("Previous page"))
                    .IsEnabled_Lambda([this]() { return PageIndex>0; })
                    .OnClicked_Lambda([this]() { --PageIndex; Refresh(); return FReply::Handled(); })
                    [SNew(STextBlock).Text(INVTEXT("‹")).Font(MissionUIStyle::GetRowFont(15))]]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [SNew(STextBlock).Justification(ETextJustify::Center).Font(MissionUIStyle::GetRowFont(11))
                    .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("PAGE %d / %d  •  %d MATCHING RECEIPTS"),PageIndex+1,PageCount,MatchCount)); })]
                + SHorizontalBox::Slot().AutoWidth()
                [SNew(SButton).ToolTipText(INVTEXT("Next page"))
                    .IsEnabled_Lambda([this]() { return PageIndex+1<PageCount; })
                    .OnClicked_Lambda([this]() { ++PageIndex; Refresh(); return FReply::Handled(); })
                    [SNew(STextBlock).Text(INVTEXT("›")).Font(MissionUIStyle::GetRowFont(15))]]]
            + SVerticalBox::Slot().AutoHeight().Padding(10,4)[Services]
            + SVerticalBox::Slot().AutoHeight().Padding(10,8)
            [SNew(SBox).MaxDesiredHeight(210)[SNew(SScrollBox)+SScrollBox::Slot()[SNew(SRecordDetails)
                .Record_Lambda([this]() { auto D=Details;D.Icon=EShipSystemIcon::Wallet;if(D.Title.IsEmpty()){D.Title=INVTEXT("SELECT A TRANSACTION");D.Description=INVTEXT("Inspect its receipt, counterparties and wallet movement. Amounts are credits; dates are GMT.");}return D; })]]])];
        Refresh();
    }
    void Tick(const FGeometry& Geometry, double Time, float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        if (Time>=NextRefresh && Players.IsValid())
        {
            NextRefresh=Time+1;
            const auto& History=Players->GetPlayerInfo().Economy.Journal;
            const FGuid Last=History.IsEmpty() ? FGuid() : History.Last().Id;
            if (Last!=LastReceipt || History.Num()!=LastCount)
            { LastReceipt=Last; LastCount=History.Num(); Refresh(); }
        }
    }
private:
    TWeakObjectPtr<UStarshatterPlayerSubsystem> Players;
    FTransactionQuery Query;
    TSharedPtr<SVerticalBox> Rows;
    FGuid Selected;
    int32 SortColumn=0;
    bool bDescending=true;
    double NextRefresh=0;
    FGuid LastReceipt;
    int32 LastCount=-1;
    bool bRecentOnly=false;
    double MinimumQuantity=0;
    double MinimumAmount=0;
    FRecordDetails Details;
    TArray<TSharedPtr<FString>> Categories;
    int32 CategoryIndex=-1;
    int32 PageIndex=0, PageCount=1, MatchCount=0;
    int64 FromUtc=0, ThroughUtc=MAX_int64;
    bool bFromValid=true,bThroughValid=true;
    FText Summary;
    void SetDate(const FText& Text,bool bFrom)
    {
        const FString Value=Text.ToString().TrimStartAndEnd(); FDateTime Date;
        const bool Valid=Value.IsEmpty() || (Value.Len()==10 && FDateTime::ParseIso8601(*(Value+TEXT("T00:00:00Z")),Date));
        if (bFrom) { bFromValid=Valid; FromUtc=Value.IsEmpty()?0:Date.ToUnixTimestamp(); }
        else { bThroughValid=Valid; ThroughUtc=Value.IsEmpty()?MAX_int64:Date.ToUnixTimestamp()+86399; }
        PageIndex=0; Refresh();
    }
    void Refresh()
    {
        if (!Rows) return;
        Rows->ClearChildren();
        if (!Players.IsValid()) return;
        const auto& History=Players->GetPlayerInfo().Economy.Journal;
        LastReceipt=History.IsEmpty()?FGuid():History.Last().Id;
        LastCount=History.Num();
        auto Entries=TransactionAI::Query(Players->GetPlayerInfo().Economy,Query);
        const int64 Cutoff=FDateTime::UtcNow().ToUnixTimestamp()-30*24*60*60;
        Entries.RemoveAll([this,Cutoff](const FTransactionEntry& Entry)
        { return (CategoryIndex>=0 && static_cast<int32>(Entry.Transaction.Category)!=CategoryIndex) ||
            !bFromValid || !bThroughValid || FromUtc>ThroughUtc || Entry.Transaction.TimestampUtc<FromUtc || Entry.Transaction.TimestampUtc>ThroughUtc ||
            (bRecentOnly && Entry.Transaction.TimestampUtc<Cutoff) || Entry.Transaction.Quantity<MinimumQuantity ||
            FMath::Abs(static_cast<double>(Query.bMarketOnly ? Entry.Transaction.Credits : Entry.WalletDelta))<MinimumAmount; });
        int64 Income=0,Expenses=0; int32 Unknown=0; bool bOverflow=false;
        for (const auto& Entry:Entries)
        {
            if (!Entry.bWalletDeltaKnown) { ++Unknown; continue; }
            if (Entry.WalletDelta==MIN_int64) { bOverflow=true; continue; }
            int64& Total=Entry.WalletDelta>=0?Income:Expenses;
            const int64 Amount=FMath::Abs(Entry.WalletDelta);
            if (Total>MAX_int64-Amount) bOverflow=true; else Total+=Amount;
        }
        Summary=(!bFromValid || !bThroughValid || FromUtc>ThroughUtc) ? INVTEXT("ENTER A VALID GMT DATE RANGE: YYYY-MM-DD.") :
            bOverflow ? INVTEXT("FILTERED TOTAL EXCEEDS DISPLAY LIMIT. INDIVIDUAL RECEIPTS REMAIN AVAILABLE.") :
            FText::FromString(FString::Printf(TEXT("FILTERED WALLET MOVEMENTS  •  INCOME / REFUNDS: %s CR  •  EXPENSES: %s CR  •  NET: %s CR  •  UNKNOWN: %d"),
                *FText::AsNumber(Income).ToString(),*FText::AsNumber(Expenses).ToString(),*FText::AsNumber(Income-Expenses).ToString(),Unknown));
        Entries.StableSort([this](const FTransactionEntry& A,const FTransactionEntry& B)
        {
            const auto& X=A.Transaction; const auto& Y=B.Transaction;
            int32 Result=0;
            auto Compare=[](auto L,auto R) { return L<R ? -1 : L>R ? 1 : 0; };
            switch (SortColumn)
            {
            case 0: Result=Compare(X.TimestampUtc,Y.TimestampUtc); if (!Result) Result=Compare(A.Sequence,B.Sequence); break;
            case 1: Result=(X.Item.IsNone()?WalletName(X.Reason):WalletName(X.Item)).Compare(Y.Item.IsNone()?WalletName(Y.Reason):WalletName(Y.Item)); break;
            case 2: Result=WalletName(X.From==FEconomyLedger::Pilot()?X.To:X.From).Compare(WalletName(Y.From==FEconomyLedger::Pilot()?Y.To:Y.From)); break;
            case 3: Result=Compare(X.Quantity,Y.Quantity); break;
            case 4: Result=Compare(X.UnitPrice,Y.UnitPrice); break;
            case 5: Result=Compare(Query.bMarketOnly ? (X.From==FEconomyLedger::Pilot() ? -X.Credits : X.Credits) : A.WalletDelta,
                Query.bMarketOnly ? (Y.From==FEconomyLedger::Pilot() ? -Y.Credits : Y.Credits) : B.WalletDelta); break;
            default: Result=Compare(X.WalletBalancesAfter.FindRef(FEconomyLedger::Pilot()),Y.WalletBalancesAfter.FindRef(FEconomyLedger::Pilot())); break;
            }
            return bDescending ? Result>0 : Result<0;
        });
        if (Entries.IsEmpty())
            Rows->AddSlot().AutoHeight().Padding(8)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(INVTEXT("NO MATCHING TRANSACTIONS"))];
        MatchCount=Entries.Num(); PageCount=FMath::Max(1,(MatchCount+49)/50); PageIndex=FMath::Clamp(PageIndex,0,PageCount-1);
        for (int32 RowIndex=PageIndex*50;RowIndex<FMath::Min(MatchCount,(PageIndex+1)*50);++RowIndex)
        {
            const auto& Entry=Entries[RowIndex];
            const auto& Tx=Entry.Transaction;
            const int64 DisplayAmount=Query.bMarketOnly ? (Tx.From==FEconomyLedger::Pilot() ? -Tx.Credits : Tx.Credits) : Entry.WalletDelta;
            auto Line=SNew(SHorizontalBox);
            const FText Cells[] = {
                FText::FromString(Tx.TimestampUtc>0 ? FDateTime::FromUnixTimestamp(Tx.TimestampUtc).ToString(TEXT("%Y-%m-%d %H:%M")) : TEXT("—")),
                FText::FromString(Tx.Item.IsNone()?WalletName(Tx.Reason):WalletName(Tx.Item)),
                FText::FromString(WalletName(Tx.From==FEconomyLedger::Pilot()?Tx.To:Tx.From)),
                Tx.Quantity>0 ? FText::AsNumber(Tx.Quantity) : INVTEXT("—"),
                Tx.Quantity>0 ? FText::AsNumber(Tx.UnitPrice) : INVTEXT("—"),
                (!Query.bMarketOnly && !Entry.bWalletDeltaKnown) ? INVTEXT("UNKNOWN") :
                    FText::FromString((DisplayAmount>0 ? TEXT("+") : TEXT(""))+FText::AsNumber(DisplayAmount).ToString()),
                Tx.WalletBalancesAfter.Contains(FEconomyLedger::Pilot()) ? FText::AsNumber(Tx.WalletBalancesAfter.FindRef(FEconomyLedger::Pilot())) : INVTEXT("—") };
            for (int32 Index=0;Index<7;++Index)
                Line->AddSlot().FillWidth(Index<3 ? 1.4f : 1.f).VAlign(VAlign_Center).Padding(6,3)
                [SNew(STextBlock).Text(Cells[Index]).Font(MissionUIStyle::GetRowFont(11))
                    .ToolTipText(Cells[Index]).Clipping(EWidgetClipping::ClipToBounds).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                    .ColorAndOpacity(Index==5 ? (DisplayAmount>=0 ? FLinearColor(.4f,.9f,.65f) : FLinearColor(1,.55f,.45f)) : FLinearColor(.85f,.93f,.97f))
                    .Justification(Index>=3 ? ETextJustify::Right : ETextJustify::Left)];
            Rows->AddSlot().AutoHeight().Padding(0,1)
            [SNew(SBox).HeightOverride(40)[SNew(SButton).HAlign(HAlign_Fill).VAlign(VAlign_Center).ContentPadding(FMargin(0))
                .ButtonColorAndOpacity_Lambda([this,Id=Tx.Id]() { return Selected==Id ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
                .OnClicked_Lambda([this,Tx,Delta=Entry.WalletDelta,Known=Entry.bWalletDeltaKnown]()
                {
                    Selected=Tx.Id;
                    Details={};Details.Icon=EShipSystemIcon::Wallet;
                    Details.Title=FText::FromString(WalletName(Tx.Reason));
                    Details.Subtitle=StaticEnum<EWalletTransactionCategory>()->GetDisplayNameTextByValue(static_cast<int64>(Tx.Category));
                    Details.Status=Known?(Delta>=0?INVTEXT("CREDIT"):INVTEXT("DEBIT")):INVTEXT("UNRECORDED BALANCE");
                    Details.Fact(INVTEXT("FROM"),FText::FromString(WalletName(Tx.From))).Fact(INVTEXT("TO"),FText::FromString(WalletName(Tx.To)));
                    Details.Fact(INVTEXT("SOURCE"),FText::FromString(WalletName(Tx.Source))).Fact(INVTEXT("DESTINATION"),FText::FromString(WalletName(Tx.Destination)));
                    Details.Fact(INVTEXT("TRADE VALUE"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Tx.Credits)));
                    Details.Fact(INVTEXT("WALLET CHANGE"),Known?FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Delta)):INVTEXT("UNKNOWN"));
                    Details.Fact(INVTEXT("RECEIPT"),FText::FromString(Tx.Id.ToString()));
                    if(Tx.Reference.IsValid())Details.Fact(INVTEXT("REFERENCE"),FText::FromString(Tx.Reference.ToString()));
                    if(Tx.BuyOrder.IsValid())Details.Fact(INVTEXT("BUY ORDER"),FText::FromString(Tx.BuyOrder.ToString()));
                    if(Tx.SellOrder.IsValid())Details.Fact(INVTEXT("SELL ORDER"),FText::FromString(Tx.SellOrder.ToString()));
                    return FReply::Handled();
                })[Line]]];
        }
        Rows->SlatePrepass();
    }
};
}
TSharedRef<SWidget> UTransactionDlg::RebuildWidget()
{
    auto* Instance=GetGameInstance();
    return BuildWalletContent(Instance ? Instance->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr,this);
}
TSharedRef<SWidget> UTransactionDlg::BuildWalletContent(UStarshatterPlayerSubsystem* Players,const UTransactionDlg* Owner)
{ return SNew(STransactionWallet,Players,Owner); }
