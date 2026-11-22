#include "FractalVersePilotPromotion.h"
#include "FractalVerseAuthority.h"
#include "FractalVersePilotMigration.h"
#include "Misc/SecureHash.h"

namespace
{
FName FVImportName(FName Principal,FName Old)
{
    if (Old.IsNone()) return NAME_None;
    const FString Text=TEXT("FV/")+FMD5::HashAnsiString(*Principal.ToString())+TEXT("/")+Old.ToString();
    return Text.Len()<NAME_SIZE ? FName(*Text) : NAME_None;
}
bool FVImportAmount(double Value,bool Positive=false)
{ return FMath::IsFinite(Value) && (Positive ? Value>0 : Value>=0) && Value<=1.e15; }
bool FVImportAmounts(const TArray<FCommodityAmount>& Values)
{
    if (Values.IsEmpty()) return false;
    TSet<FName> Types;
    for (const auto& V:Values)
    {
        if (V.Item.IsNone() || Types.Contains(V.Item) || !FVImportAmount(V.Quantity,true) || !FVImportAmount(V.UnitTonnes,true) ||
            !FVImportAmount(V.Quantity*V.UnitTonnes,true) || (V.bWholeUnits && !FMath::IsNearlyEqual(V.Quantity,FMath::RoundToDouble(V.Quantity),1.e-9))) return false;
        Types.Add(V.Item);
    }
    return true;
}
}

FGuid FFractalVersePilotPromotion::ScopedId(FName Principal,FGuid Legacy)
{
    if (Principal.IsNone() || !Legacy.IsValid()) return {};
    FGuid Result;
    FGuid::ParseExact(FMD5::HashAnsiString(*(Principal.ToString()+TEXT("/")+Legacy.ToString(EGuidFormats::Digits))),EGuidFormats::Digits,Result);
    return Result;
}

bool FFractalVersePilotPromotion::Build(const FFractalVerseWorld& Current,FName Principal,
    const FFractalVersePilotCandidate& Prepared,FFractalVerseWorld& Out,FString& Reason)
{
    Reason.Empty();
    if (Principal.IsNone() || Principal==FEconomyLedger::Pilot() || Current.Schema!=1 || !Current.Universe.IsValid() || Current.Revision==MAX_int64)
    { Reason=TEXT("Verified account and a writable universe are required."); return false; }
    FFractalVersePilotCandidate Canonical;
    if (!FFractalVersePilotMigration::Revalidate(Prepared,Canonical,Reason)) return false;
    if (const auto* Existing=Current.Pilots.Find(Principal))
    {
        if (Existing->LegacyDigest==Canonical.BundleDigest && !Existing->LegacyDigest.IsEmpty()) { Out=Current;return true; }
        Reason=TEXT("A server career already exists; it cannot be overwritten by a local save."); return false;
    }
    if (Current.MigrationSources.Contains(Canonical.BundleDigest) || Current.Economy.Wallets.Contains(Principal))
    { Reason=TEXT("This source or wallet already belongs to a server migration."); return false; }
    const auto& Legacy=Canonical.Pilot.Economy; const FName OldPilot=FEconomyLedger::Pilot();
    FFractalVerseWorld Result=Current;
    auto& Ledger=Result.Economy;
    FFractalVersePilot Pilot; Pilot.Callsign=Canonical.Pilot.Callsign.IsEmpty()?Canonical.Pilot.Name:Canonical.Pilot.Callsign;
    if (Pilot.Callsign.IsEmpty()) Pilot.Callsign=TEXT("Cadet");
    Pilot.Callsign.ReplaceInline(TEXT("\r"),TEXT(" ")); Pilot.Callsign.ReplaceInline(TEXT("\n"),TEXT(" "));
    Pilot.Callsign=Pilot.Callsign.TrimStartAndEnd().Left(64); Pilot.HomeSystem=Canonical.Pilot.HomeSystem; Pilot.HomeRegion=Canonical.Pilot.HomeRegion;
    Pilot.CareerJson=Canonical.CareerJson; Pilot.LegacyDigest=Canonical.BundleDigest; Pilot.LegacyBaseSlot=Canonical.BaseSlot;
    const auto Owner=[&](FName Name){return Name==OldPilot?Principal:Name;};
    const auto Id=[&](FGuid Value){return ScopedId(Principal,Value);};
    for (const auto& Pair:Legacy.Contracts)
        if (Pair.Value.Issuer==OldPilot || Pair.Value.Acceptor==OldPilot || Pair.Value.HighBidder==OldPilot || Pair.Value.Terms.AssignedToId==OldPilot)
        {
            const FName Key=FVImportName(Principal,Pair.Key);
            if (Key.IsNone() || Key.ToString().Len()+8>=NAME_SIZE || Ledger.Contracts.Contains(Key)) { Reason=TEXT("Contract identity collision.");return false; }
            Pilot.ContractAliases.Add(Pair.Key,Key);
        }
    const auto ItemName=[&](FName Item)->FName
    {
        FString Name=Item.ToString();
        if (Name.RemoveFromStart(TEXT("Courier/")))
        {
            if (const auto* Key=Pilot.ContractAliases.Find(FName(*Name))) return FName(*(TEXT("Courier/")+Key->ToString()));
            return NAME_None;
        }
        return Item;
    };
    for (const auto& Pair:Legacy.Inventories) if (Pair.Value.Owner==OldPilot)
    {
        FName Key=FVImportName(Principal,Pair.Key);
        FString ContractName=Pair.Key.ToString();
        if (ContractName.RemoveFromStart(TEXT("ContractInventory/Pilot/")))
        {
            const FString Text=TEXT("ContractInventory/")+Principal.ToString()+TEXT("/")+ContractName;
            Key=Text.Len()<NAME_SIZE?FName(*Text):NAME_None;
        }
        if (Key.IsNone() || Ledger.Inventories.Contains(Key)) { Reason=TEXT("Owned inventory identity collision.");return false; }
        const auto& Original=Pair.Value;
        if (!FVImportAmount(Original.CapacityTonnes) || !FVImportAmount(Original.ReservedOutputTonnes) ||
            !FMath::IsFinite(FEconomyLedger::Used(Original)) || FEconomyLedger::Used(Original)+Original.ReservedOutputTonnes>Original.CapacityTonnes+UE_SMALL_NUMBER)
        { Reason=TEXT("Invalid saved cargo capacity or reserved output.");return false; }
        auto Copy=Original; Copy.Owner=Principal; Copy.TradingOwner=Owner(Copy.TradingOwner); Copy.Id=Id(Copy.Id);
        TSet<FName> Types;
        for (auto& Stack:Copy.Items)
        {
            if (Stack.Type.IsNone() || Types.Contains(Stack.Type) || !FVImportAmount(Stack.Quantity) || !FVImportAmount(Stack.UnitTonnes,true))
            { Reason=TEXT("Invalid saved cargo stack.");return false; }
            Types.Add(Stack.Type); Stack.Type=ItemName(Stack.Type); // Courier seals follow the imported contract key.
            if (Stack.Type.IsNone()) { Reason=TEXT("A sealed courier package has no resolvable contract.");return false; }
        }
        TSet<FName> Manifest; for (FName Item:Copy.ManifestItems)
        { const FName Mapped=ItemName(Item); if (Mapped.IsNone()) {Reason=TEXT("A courier manifest has no resolvable contract.");return false;} Manifest.Add(Mapped); }
        Copy.ManifestItems=MoveTemp(Manifest);
        Pilot.InventoryAliases.Add(Pair.Key,Key); Ledger.Inventories.Add(Key,MoveTemp(Copy));
    }
    const auto Inventory=[&](FName Name){const auto* New=Pilot.InventoryAliases.Find(Name);return New?*New:Name;};
    for (const auto& Pair:Legacy.Reservations)
    {
        if (Pair.Value.Owner!=OldPilot)
        {
            if (Pilot.InventoryAliases.Contains(Pair.Value.Inventory)) { Reason=TEXT("Cargo has a foreign reservation requiring explicit recovery.");return false; }
            continue;
        }
        const FGuid NewId=Id(Pair.Key);
        if (!NewId.IsValid() || Pair.Key!=Pair.Value.Id || Ledger.Reservations.Contains(NewId) ||
            !Pilot.InventoryAliases.Contains(Pair.Value.Inventory) || !FVImportAmount(Pair.Value.Quantity,true))
        { Reason=TEXT("Invalid saved cargo reservation.");return false; }
        auto Copy=Pair.Value; Copy.Id=NewId; Copy.Owner=Principal; Copy.Inventory=Inventory(Copy.Inventory); Copy.Item=ItemName(Copy.Item);
        const auto& Hold=Ledger.Inventories.FindChecked(Copy.Inventory);
        if (!Hold.Items.ContainsByPredicate([&](const auto& Stack){return Stack.Type==Copy.Item;}))
        { Reason=TEXT("Saved cargo reservation refers to a missing item.");return false; }
        Ledger.Reservations.Add(NewId,Copy);
    }
    for (const auto& Pair:Pilot.InventoryAliases)
    {
        const auto& Hold=Ledger.Inventories.FindChecked(Pair.Value);
        for (const auto& Stack:Hold.Items)
        {
            double Reserved=0;
            for (const auto& Claim:Ledger.Reservations) if (Claim.Value.Inventory==Pair.Value && Claim.Value.Item==Stack.Type) Reserved+=Claim.Value.Quantity;
            if (!FVImportAmount(Reserved) || Reserved>Stack.Quantity+UE_SMALL_NUMBER) { Reason=TEXT("Saved cargo is over-reserved.");return false; }
        }
    }
    for (FGuid Receipt:Legacy.CommittedTransactions) if (Receipt.IsValid()) Ledger.CommittedTransactions.Add(Id(Receipt));
    for (const auto& Pair:Legacy.Blueprints) if (Pair.Value.Owner==OldPilot)
    {
        const auto& B=Pair.Value; const FGuid NewId=Id(Pair.Key); const FGuid Receipt=Legacy.BlueprintReceipts.FindRef(Pair.Key);
        if (!NewId.IsValid() || B.Id!=Pair.Key || Ledger.Blueprints.Contains(NewId) || !Receipt.IsValid() || !Legacy.CommittedTransactions.Contains(Receipt) ||
            B.Recipe.IsNone() || B.RunsRemaining<0 || !FMath::IsFinite(B.MaterialEfficiency) || B.MaterialEfficiency<0 || B.MaterialEfficiency>.1 ||
            !FMath::IsFinite(B.TimeEfficiency) || B.TimeEfficiency<0 || B.TimeEfficiency>.2)
        { Reason=TEXT("Saved blueprint ownership or purchase proof is invalid.");return false; }
        auto Copy=B; Copy.Id=NewId; Copy.Owner=Principal; Ledger.Blueprints.Add(NewId,Copy); Ledger.BlueprintReceipts.Add(NewId,Id(Receipt));
    }
    TSet<FGuid> Jobs;
    for (const auto& Job:Ledger.Jobs) Jobs.Add(Job.Id);
    for (const auto& Original:Legacy.Jobs) if (Original.Owner==OldPilot)
    {
        const FGuid NewId=Id(Original.Id);
        const bool Active=Original.State!=EIndustryJobState::Completed && Original.State!=EIndustryJobState::Cancelled;
        if (static_cast<uint8>(Original.State)>static_cast<uint8>(EIndustryJobState::Cancelled) || Original.Runs<=0 || Original.Runs>1000000 || !NewId.IsValid() || Jobs.Contains(NewId) || !Pilot.InventoryAliases.Contains(Original.Input) ||
            !Pilot.InventoryAliases.Contains(Original.Output) || Original.Input==Original.Output || Original.Facility.IsNone() ||
            !FVImportAmount(Original.Duration,true) || !FVImportAmount(Original.Remaining) || Original.Remaining>Original.Duration ||
            !FVImportAmounts(Original.Materials) || !FVImportAmounts(Original.Products) ||
            (Active && (!Original.Blueprint.IsValid() || !Ledger.Blueprints.Contains(Id(Original.Blueprint)))))
        { Reason=TEXT("Saved production job has unresolved inputs, outputs or blueprint.");return false; }
        auto Copy=Original; Copy.Id=NewId; Copy.Owner=Principal; Copy.Input=Inventory(Copy.Input); Copy.Output=Inventory(Copy.Output);
        Copy.Blueprint=Id(Copy.Blueprint); Ledger.Jobs.Add(Copy); Jobs.Add(NewId);
    }
    TArray<FMarketOrder> Orders;
    for (const auto& Pair:Legacy.MarketOrders) if (Pair.Value.Owner==OldPilot)
    {
        if (Pair.Key!=Pair.Value.Id) { Reason=TEXT("Market order identity mismatch.");return false; }
        Orders.Add(Pair.Value);
    }
    Orders.StableSort([](const auto& A,const auto& B){return A.Sequence!=B.Sequence?A.Sequence<B.Sequence:A.Id.ToString()<B.Id.ToString();});
    for (const auto& Original:Orders)
    {
        const FGuid NewId=Id(Original.Id); const bool Open=Original.State==EMarketOrderState::Open;
        if (static_cast<uint8>(Original.State)>static_cast<uint8>(EMarketOrderState::Expired) || static_cast<uint8>(Original.Side)>static_cast<uint8>(EMarketOrderSide::Sell) || !NewId.IsValid() || Ledger.MarketOrders.Contains(NewId) || !Pilot.InventoryAliases.Contains(Original.Inventory) ||
            (Open && !Ledger.Inventories.Contains(Inventory(Original.Market))) || !FVImportAmount(Original.Quantity,true) ||
            !FVImportAmount(Original.Remaining) || Original.Remaining>Original.Quantity || !FVImportAmount(Original.UnitTonnes,true) ||
            Original.UnitPrice<=0 || Original.EscrowCredits<0 || !FVImportAmount(Original.ReservedTonnes) || Ledger.NextMarketSequence==MAX_int64 ||
            (Open && Original.Side==EMarketOrderSide::Sell && !Ledger.Reservations.Contains(Id(Original.CargoReservation))))
        { Reason=TEXT("Saved market order or escrow is unresolved.");return false; }
        auto Copy=Original; Copy.Id=NewId; Copy.Owner=Principal; Copy.Inventory=Inventory(Copy.Inventory); Copy.Market=Inventory(Copy.Market);
        Copy.CargoReservation=Id(Copy.CargoReservation); Copy.Sequence=++Ledger.NextMarketSequence;
        Ledger.MarketOrders.Add(NewId,Copy); // Escrow remains escrow; it is never added to the spendable wallet.
    }
    for (const auto& Hold:Pilot.InventoryAliases)
    {
        double ReservedSpace=0;
        for (const auto& Job:Ledger.Jobs) if (Job.Owner==Principal && Job.Output==Hold.Value && Job.State==EIndustryJobState::Running)
            for (const auto& Product:Job.Products) ReservedSpace+=Product.Quantity*Product.UnitTonnes;
        for (const auto& Pair:Ledger.MarketOrders) if (Pair.Value.Owner==Principal && Pair.Value.Inventory==Hold.Value &&
            Pair.Value.State==EMarketOrderState::Open && Pair.Value.Side==EMarketOrderSide::Buy) ReservedSpace+=Pair.Value.ReservedTonnes;
        if (!FVImportAmount(ReservedSpace) || !FMath::IsNearlyEqual(ReservedSpace,Ledger.Inventories.FindChecked(Hold.Value).ReservedOutputTonnes,1.e-6))
        { Reason=TEXT("Saved reserved cargo space does not match jobs and market orders.");return false; }
    }
    for (const auto& Pair:Pilot.ContractAliases)
    {
        auto Contract=Legacy.Contracts.FindChecked(Pair.Key);
        if (static_cast<uint8>(Contract.State)>static_cast<uint8>(EContractState::Expired) || static_cast<uint8>(Contract.Terms.Type)>static_cast<uint8>(ESSWContractType::Auction) || !Contract.Id.IsValid() || Contract.Terms.RewardCredits<0 || Contract.Terms.PriceCredits<0 || Contract.Terms.CollateralCredits<0 || Contract.HighBid<0)
        { Reason=TEXT("Invalid saved contract or escrow.");return false; }
        Contract.Id=Id(Contract.Id); Contract.Issuer=Owner(Contract.Issuer); Contract.Acceptor=Owner(Contract.Acceptor);
        Contract.HighBidder=Owner(Contract.HighBidder); Contract.Terms.AssignedToId=Owner(Contract.Terms.AssignedToId);
        for (FName Counterparty:{Contract.Issuer,Contract.Acceptor,Contract.HighBidder})
            if (!Counterparty.IsNone() && Counterparty!=Principal && !Ledger.Wallets.Contains(Counterparty)) Ledger.Wallets.Add(Counterparty,0);
        Ledger.Contracts.Add(Pair.Value,Contract);
    }
    for (const auto& Pair:Legacy.MiningDrones) if (const auto* Hold=Pilot.InventoryAliases.Find(Pair.Key))
    {
        for (const auto& Drone:Pair.Value.Drones)
            if (static_cast<uint8>(Drone.State)>static_cast<uint8>(EMiningDroneState::Lost) || !FVImportAmount(Drone.OreTonnes) || Drone.Position.ContainsNaN()) { Reason=TEXT("Invalid saved mining drone state.");return false; }
        Ledger.MiningDrones.Add(*Hold,Pair.Value); // In-flight ore stays on drones until an actual return/unload.
    }
    for (const auto& Pair:Legacy.MiningOptics)
    {
        FString Key;
        for (const auto& Hold:Pilot.InventoryAliases)
        {
            const FString Prefix=Hold.Key.ToString()+TEXT("/Optics/");
            if (Pair.Key==Hold.Key) { Key=Hold.Value.ToString();break; }
            if (Pair.Key.ToString().StartsWith(Prefix)) { Key=Hold.Value.ToString()+Pair.Key.ToString().Mid(Hold.Key.ToString().Len());break; }
        }
        if (Key.IsEmpty()) continue;
        if (Key.Len()>=NAME_SIZE || !FVImportAmount(Pair.Value.ExtractedTonnes) || !FVImportAmount(Pair.Value.ServiceLifeTonnes,true))
        { Reason=TEXT("Invalid saved mining crystal state.");return false; }
        Ledger.MiningOptics.Add(FName(*Key),Pair.Value);
    }
    for (const auto& Entry:Legacy.Journal)
    {
        if (Entry.From!=OldPilot && Entry.To!=OldPilot && !Entry.WalletDeltas.Contains(OldPilot)) continue;
        if (!Entry.Id.IsValid() || Entry.Credits<0) { Reason=TEXT("Invalid saved wallet journal.");return false; }
        auto Copy=Entry; Copy.Id=Id(Copy.Id); Copy.Reference=Id(Copy.Reference); Copy.From=Owner(Copy.From); Copy.To=Owner(Copy.To);
        Copy.Source=Inventory(Copy.Source); Copy.Destination=Inventory(Copy.Destination); Copy.BuyOrder=Id(Copy.BuyOrder); Copy.SellOrder=Id(Copy.SellOrder);
        Copy.WalletDeltas.Reset(); for (const auto& Delta:Entry.WalletDeltas) Copy.WalletDeltas.Add(Owner(Delta.Key),Delta.Value);
        Copy.WalletBalancesAfter.Reset(); for (const auto& Balance:Entry.WalletBalancesAfter) Copy.WalletBalancesAfter.Add(Owner(Balance.Key),Balance.Value);
        Ledger.Journal.Add(Copy);
    }
    Ledger.Wallets.Add(Principal,Canonical.Credits); Ledger.PlayerAccounts.Add(Principal);
    Result.Pilots.Add(Principal,MoveTemp(Pilot)); Result.MigrationSources.Add(Canonical.BundleDigest,Principal);
    ++Result.Revision; Out=MoveTemp(Result); return true;
}

bool FFractalVersePilotCommit::Commit(FFractalVerseAuthority& Authority,FName Principal,
    const FFractalVersePilotCandidate& Prepared,TFunctionRef<bool()> Save,FString& Reason)
{
    const FFractalVerseWorld Before=Authority.Read();
    if (!Authority.ImportPilot(Principal,Prepared,Reason)) return false;
    if (Authority.Read().Revision==Before.Revision) return true; // Already committed migration receipt.
    if (Save()) return true;
    FString RestoreReason;
    Authority.Restore(Before,RestoreReason);
    Reason=TEXT("Pilot import could not be saved; no new career was acknowledged.");
    return false;
}
