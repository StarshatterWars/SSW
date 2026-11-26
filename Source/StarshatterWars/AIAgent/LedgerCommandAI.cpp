#include "LedgerCommandAI.h"
#include "ContractLedger.h"
#include "AcceptanceAI.h"
#include "BiddingAI.h"
#include "SettlementAI.h"
#include "SimDataEvents.h"

namespace
{
void SeedTraining(FEconomyState& State,TMap<FName,FContractTransaction>& Contracts,FContractAccounts& Accounts,FDateTime Now)
{
        const FName Depot(TEXT("Janek Logistics"));
        const FName Terminal(TEXT("Janus Transfer Station"));
        const FName Supplies(TEXT("Training Supplies"));
        const FName Parts(TEXT("Training Spare Parts"));
        auto& DepotAccount = Accounts.Add(Depot);
        DepotAccount.Credits = State.Wallets.FindRef(Depot);
        if (!State.InitializedMarkets.Contains(TEXT("Training issuer grant")))
        { DepotAccount.Credits += FMath::Min(20000LL, MAX_int64 - DepotAccount.Credits); State.InitializedMarkets.Add(TEXT("Training issuer grant")); }
        auto& Stock = DepotAccount.StationStock.Add(Terminal);
        Stock.Add(Parts, 1);
        auto& PilotAccount = Accounts.Add(FEconomyLedger::Pilot());
        PilotAccount.Credits = State.Wallets.FindRef(FEconomyLedger::Pilot());
        if (!State.InitializedMarkets.Contains(TEXT("Career starting grant")) && !State.InitializedMarkets.Contains(TEXT("Training pilot grant")))
        { PilotAccount.Credits += FMath::Min(10000LL, MAX_int64 - PilotAccount.Credits); State.InitializedMarkets.Add(TEXT("Training pilot grant")); }
        PilotAccount.StationStock.FindOrAdd(Terminal).Add(Parts, 1);

        FContractTransaction Courier;
        Courier.Issuer = Depot;
        Courier.Terms.Title = TEXT("Training courier delivery");
        Courier.Terms.Description = TEXT("Accept a sealed 0.1 t training package, then fly within 2 km of Janus Transfer Station to deliver it.");
        Courier.Terms.PickupStationId = TEXT("Archon");
        Courier.Terms.DestinationStationId = Terminal;
        Courier.Terms.OfferExpiryMinutes = 120;
        Courier.Terms.CompletionDays = 1;
        Courier.Terms.CollateralCredits = 500;
        Courier.Terms.RewardCredits = 750;
        auto& CourierCargo = Courier.Terms.OfferedItems.AddDefaulted_GetRef();
        CourierCargo.ItemId = Supplies;
        CourierCargo.Quantity = 1;
        CourierCargo.Unit = TEXT("crate");
        Accounts.FindChecked(Depot).StationStock.FindOrAdd(Courier.Terms.PickupStationId).Add(Supplies, 1);
        if (AcceptanceAI::Publish(Courier, Accounts, Now))
            Contracts.Add(TEXT("Courier"), MoveTemp(Courier));

        FContractTransaction Auction;
        Auction.Issuer = Depot;
        Auction.Terms.Type = ESSWContractType::Auction;
        Auction.Terms.Title = TEXT("Spare parts auction");
        Auction.Terms.Description = TEXT("Place a bid, or choose Buyout to receive the training parts immediately. A winning bid can be claimed after the auction closes.");
        Auction.Terms.PickupStationId = Terminal;
        Auction.Terms.OfferExpiryMinutes = 5;
        Auction.Terms.PriceCredits = 1000;
        Auction.Buyout = 2500;
        auto& AuctionCargo = Auction.Terms.OfferedItems.AddDefaulted_GetRef();
        AuctionCargo.ItemId = Parts;
        AuctionCargo.Quantity = 1;
        AuctionCargo.Unit = TEXT("crate");
        if (AcceptanceAI::Publish(Auction, Accounts, Now))
            Contracts.Add(TEXT("Auction"), Auction);

        FContractTransaction Sale = Auction;
        Sale.Id=FGuid::NewGuid();
        Sale.State = EContractState::Draft;
        Sale.Issuer = FEconomyLedger::Pilot();
        Sale.Terms.Title = TEXT("Training auction payout");
        Sale.Terms.Description = TEXT("Janek Logistics purchased your training parts. Claim the sale proceeds.");
        Sale.Buyout = 1500;
        if (AcceptanceAI::Publish(Sale, Accounts, Now) &&
            BiddingAI::PlaceBid(Sale, Accounts, Depot, Sale.Buyout, Now))
            Contracts.Add(TEXT("Sale"), MoveTemp(Sale));
}
bool ContractAction(FEconomyState& State,const FNativeLedgerCommand& C,FNativeLedgerReply& Out)
{
    TMap<FName,FContractTransaction> Contracts;FContractAccounts Accounts;FContractLedger::Restore(State,Contracts,Accounts);
    const FDateTime Now=FDateTime::FromUnixTimestamp(C.NowUtc);const FName Pilot=FEconomyLedger::Pilot();bool Changed=false;
    if(C.Action==ELedgerCommand::ContractMaintain)
    {
        if(C.bSeedTraining && Contracts.IsEmpty()){SeedTraining(State,Contracts,Accounts,Now);Changed=true;}
        Changed|=FContractLedger::RepairIdentities(Contracts);
        for(auto& Pair:Contracts)
        {
            auto& Contract=Pair.Value;
            if(Contract.State==EContractState::InProgress && Contract.Terms.Type==ESSWContractType::Courier &&
                (C.LostPackages.Contains(Pair.Key) || Now>=Contract.DeliverBy))
            {
                if(SettlementAI::FailCourier(Contract,Accounts,Contract.Acceptor,Now,true))
                {
                    Changed=true;const FName Package(*(TEXT("Courier/")+Pair.Key.ToString()));
                    const double Count=FEconomyLedger::Quantity(State,C.Hold,Package);
                    if(Count>0)FEconomyLedger::Withdraw(State,Pilot,C.Hold,Package,Count);
                }
            }
            Changed|=SettlementAI::CloseExpired(Contract,Accounts,Now);
            if(Contract.State==EContractState::AwaitingClaims && Contract.Issuer!=Pilot && !Contract.bSellerClaimed)
                Changed|=SettlementAI::ClaimAuction(Contract,Accounts,Contract.Issuer,true);
        }
        Out.bChanged=Changed;
        if(!Changed)return true;
    }
    else
    {
        auto* Contract=Contracts.Find(C.Contract);if(!Contract){Out.Reason=INVTEXT("Contract unavailable.");return false;}
        const FName Package(*(TEXT("Courier/")+C.Contract.ToString()));
        switch(C.Action)
        {
        case ELedgerCommand::ContractAccept:
        {
            const auto* Hold=State.Inventories.Find(C.Hold);
            if(Contract->Terms.Type!=ESSWContractType::Courier || !Hold || Hold->Owner!=Pilot || Hold->Items.ContainsByPredicate([](const auto& S){return S.Type.ToString().StartsWith(TEXT("Courier/"));}) ||
                !FEconomyLedger::Deposit(State,C.Hold,Package,1,.1))return false;
            Changed=AcceptanceAI::Accept(*Contract,Accounts,Pilot,Now,true);break;
        }
        case ELedgerCommand::ContractBid:Changed=BiddingAI::PlaceBid(*Contract,Accounts,Pilot,C.Credits,Now);break;
        case ELedgerCommand::ContractDeliver:
            if(Now>=Contract->DeliverBy || FEconomyLedger::Quantity(State,C.Hold,Package)!=1 || !FEconomyLedger::Withdraw(State,Pilot,C.Hold,Package,1))return false;
            Changed=SettlementAI::DeliverCourier(*Contract,Accounts,Pilot,C.Destination);break;
        case ELedgerCommand::ContractAbandon:
            Changed=SettlementAI::FailCourier(*Contract,Accounts,Pilot,Now,true);
            if(Changed){const double Count=FEconomyLedger::Quantity(State,C.Hold,Package);if(Count>0)FEconomyLedger::Withdraw(State,Pilot,C.Hold,Package,Count);}break;
        case ELedgerCommand::ContractClaim:Changed=SettlementAI::ClaimAuction(*Contract,Accounts,Pilot,true);break;
        default:return false;
        }
        if(!Changed){Out.Reason=INVTEXT("Contract action rejected. Check status, deadline, cargo and credits.");return false;}
        Out.bChanged=true;Out.Object=Contract->Id;
    }
    if(!FContractLedger::CanCapture(State,Accounts))return false;
    FContractLedger::Capture(State,Contracts,Accounts);return true;
}
}
FNativeLedgerReply LedgerCommandAI::Execute(FEconomyState& State,const FNativeLedgerCommand& C)
{
    FNativeLedgerReply Out;Out.Object=C.Object;FScopedSimDataChangeBatch Batch(State);
    if(!C.Receipt.IsValid() || State.CommittedTransactions.Contains(C.Receipt) || C.NowUtc<0 || C.NowUtc>253402300799LL)
    {Out.Reason=INVTEXT("Invalid or already processed request. Nothing exchanged.");return Out;}
    const auto Before=State;const FName Pilot=FEconomyLedger::Pilot();
    if(C.Action==ELedgerCommand::Transfer)
    {
        const auto* From=State.Inventories.Find(C.Source);
        Out.bSucceeded=From && From->Owner==Pilot && !C.Item.ToString().StartsWith(TEXT("Courier/")) &&
            FEconomyLedger::CreateInventory(State,C.Destination,Pilot,C.System,C.Capacity) &&
            FEconomyLedger::Transfer(State,Pilot,C.Source,C.Destination,C.Item,C.Quantity);
    }
    else if(C.Action==ELedgerCommand::BlueprintPurchase)
    {
        if(C.Credits>0 && !C.Recipe.IsNone() && (C.bOriginal || C.Runs>0) &&
            FEconomyLedger::Pay(State,Pilot,TEXT("NPC blueprint publisher"),C.Credits,C.Receipt,TEXT("Blueprint purchase"),ECommercePurpose::Purchase))
        {
            Out.Object=FEconomyLedger::GrantBlueprint(State,Pilot,C.Recipe,C.bOriginal,C.Runs);
            Out.bSucceeded=Out.Object.IsValid();if(Out.bSucceeded)
            {
                State.BlueprintReceipts.Add(Out.Object,C.Receipt);auto* Payment=State.Journal.FindByPredicate([&C](const auto& J){return J.Id==C.Receipt;});
                if(Payment){Payment->Item=C.Recipe;Payment->Quantity=1;Payment->UnitPrice=double(C.Credits);Payment->Reference=Out.Object;}
            }
        }
    }
    else if(C.Action==ELedgerCommand::IndustryInstall)
    {
        if(FEconomyLedger::CreateInventory(State,C.Source,Pilot,C.System,C.Capacity) && FEconomyLedger::CreateInventory(State,C.Destination,Pilot,C.System,C.Capacity))
        {
            Out.Object=FEconomyLedger::QueueBlueprintJob(State,Pilot,C.Facility,C.Source,C.Destination,C.Object,C.Runs,C.Materials,C.Products,C.Duration);
            Out.bSucceeded=Out.Object.IsValid() && (C.Credits==0 || (C.Credits>0 && FEconomyLedger::Pay(State,Pilot,TEXT("NPC facility services"),C.Credits,Out.Object,TEXT("Manufacturing installation"),ECommercePurpose::Service)));
        }
    }
    else if(C.Action==ELedgerCommand::IndustryCancel)Out.bSucceeded=FEconomyLedger::CancelJob(State,Pilot,C.Object);
    else if(C.Action==ELedgerCommand::MarketMaintain)
    {
        Out.bOrders=true;Out.SourceOrders=State.MarketOrders;const uint64 BeforeMutation=FSimDataEvents::CurrentThreadRevision();FEconomyLedger::AdvanceMarketOrders(State,C.NowUtc);Out.bChanged=FSimDataEvents::CurrentThreadRevision()!=BeforeMutation;Out.bSucceeded=true;
    }
    else
    {
        Out.bContracts=true;Out.SourceContracts=State.Contracts;Out.bSucceeded=ContractAction(State,C,Out);
    }
    const bool Maintenance=C.Action==ELedgerCommand::MarketMaintain || C.Action==ELedgerCommand::ContractMaintain;
    if(Out.bSucceeded && !Maintenance)Out.bChanged=true;
    if(Out.bSucceeded && Out.bChanged && !State.CommittedTransactions.Contains(C.Receipt))
        {State.CommittedTransactions.Add(C.Receipt);FSimDataEvents::Publish(State);}
    if(!Out.bSucceeded){State=Before;if(Out.Reason.IsEmpty())Out.Reason=INVTEXT("Request rejected. Stock, cargo, credits and reservations retained.");return Out;}
    Out.Reason=INVTEXT("REQUEST SAVED. CARGO, CREDITS AND WORK ORDERS CONFIRMED.");return Out;
}
