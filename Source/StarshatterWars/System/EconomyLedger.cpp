#include "EconomyAI.h"
#include "ProductionEvents.h"
#include "SimDataEvents.h"
#include "GameStructs_economy.h"
#include "Misc/DateTime.h"

namespace
{
constexpr double EconomyQuantityLimit = 1.e15;
bool Positive(double Value) { return FMath::IsFinite(Value) && Value > 0 && Value <= EconomyQuantityLimit; }
FEconomyItem* Find(FEconomyInventory& Inv, FName Item)
{ return Inv.Items.FindByPredicate([Item](const auto& Stack) { return Stack.Type == Item; }); }
bool HasAccess(const FEconomyInventory* Inv, FName Owner) { return Inv && !Owner.IsNone() && Inv->Owner == Owner; }
// Shared validation for normal deposits and an uncommitted job-output candidate.
bool DepositInto(FEconomyInventory& Inv, FName Item, double Count, double Mass)
{
    if (!Inv.bOperational || (!Inv.AcceptedItems.IsEmpty() && !Inv.AcceptedItems.Contains(Item)) || Item.IsNone() || !Positive(Count) || !Positive(Mass) || !Positive(Count * Mass) ||
        FEconomyLedger::Used(Inv) + Count * Mass > Inv.CapacityTonnes + UE_SMALL_NUMBER) return false;
    auto* Stack = Find(Inv, Item);
    if (Stack && (!FMath::IsNearlyEqual(Stack->UnitTonnes, Mass) || Stack->Quantity > EconomyQuantityLimit - Count)) return false;
    if (!Stack) { Stack = &Inv.Items.AddDefaulted_GetRef(); Stack->Id = FGuid::NewGuid(); Stack->Type = Item; Stack->UnitTonnes = Mass; }
    Stack->Quantity += Count; return true;
}
void Record(FEconomyState& State, FGuid Id, FName Reason, FName From, FName To, int64 Credits)
{
    FEconomyTransaction Entry; Entry.Id = Id; Entry.Reason = Reason; Entry.From = From; Entry.To = To; Entry.Credits = Credits; Entry.TimestampUtc = FDateTime::UtcNow().ToUnixTimestamp();
    Entry.bHasWalletDeltas = true;
    Entry.Reference = Id;
    if (!From.IsNone()) Entry.WalletDeltas.FindOrAdd(From) -= Credits;
    if (!To.IsNone()) Entry.WalletDeltas.FindOrAdd(To) += Credits;
    for (const auto& Party : Entry.WalletDeltas) if (const auto* Balance = State.Wallets.Find(Party.Key)) Entry.WalletBalancesAfter.Add(Party.Key, *Balance);
    const FString Kind = Reason.ToString();
    Entry.Category = Kind.Contains(TEXT("order"), ESearchCase::IgnoreCase) ? EWalletTransactionCategory::Escrow
        : Kind.Contains(TEXT("Market")) ? EWalletTransactionCategory::Market
        : Kind.Contains(TEXT("Manufactur")) ? EWalletTransactionCategory::Manufacturing
        : Kind.Contains(TEXT("Insurance")) ? EWalletTransactionCategory::Insurance
        : Kind.Contains(TEXT("refit"), ESearchCase::IgnoreCase) || Kind.Contains(TEXT("repair"), ESearchCase::IgnoreCase) || Kind.Contains(TEXT("hull"), ESearchCase::IgnoreCase) ? EWalletTransactionCategory::Shipyard
        : Kind.Contains(TEXT("Skill")) ? EWalletTransactionCategory::Skill
        : Kind.Contains(TEXT("Blueprint")) ? EWalletTransactionCategory::Blueprint : EWalletTransactionCategory::General;
    State.Journal.Add(Entry); State.CommittedTransactions.Add(Id);
    if (State.Journal.Num() > 512) State.Journal.RemoveAt(0, State.Journal.Num() - 512);
    FSimDataEvents::Publish(State);
}
bool ValidateAmounts(const TArray<FCommodityAmount>& Amounts)
{
    TSet<FName> Seen;
    for (const auto& Amount : Amounts)
    {
        if (Amount.Item.IsNone() || !Positive(Amount.Quantity) || !Positive(Amount.UnitTonnes) ||
            !Positive(Amount.Quantity * Amount.UnitTonnes) || (Amount.bWholeUnits && !FMath::IsNearlyEqual(Amount.Quantity,FMath::RoundToDouble(Amount.Quantity),1.e-9)) || Seen.Contains(Amount.Item)) return false;
        Seen.Add(Amount.Item);
    }
    return !Amounts.IsEmpty();
}
}

FEconomyInventory* FEconomyLedger::CreateInventory(FEconomyState& State, FName Key, FName Owner, FName System, double Capacity)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (Key.IsNone() || Owner.IsNone() || !FMath::IsFinite(Capacity) || Capacity < 0 || Capacity > EconomyQuantityLimit) return nullptr;
    if (auto* Existing = State.Inventories.Find(Key))
        return Existing->Owner == Owner && Existing->System == System ? Existing : nullptr;
    FEconomyInventory Inv; Inv.Id = FGuid::NewGuid(); Inv.Owner = Owner; Inv.System = System; Inv.CapacityTonnes = Capacity;
    auto* Added=&State.Inventories.Add(Key,MoveTemp(Inv)); FSimDataEvents::Publish(State);return Added;
}
bool FEconomyLedger::ReconcileManifest(FEconomyState& State, FName Key, const TArray<FCommodityAmount>& Desired)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Inventory = State.Inventories.Find(Key);
    if (!Inventory || (!Desired.IsEmpty() && !ValidateAmounts(Desired))) return false;
    if(Inventory->ManifestItems.Num()==Desired.Num() && !Desired.ContainsByPredicate([Inventory](const auto& Amount)
        {const auto* Stack=Find(*Inventory,Amount.Item);return !Inventory->ManifestItems.Contains(Amount.Item) || !Stack || Stack->Quantity!=Amount.Quantity || Stack->UnitTonnes!=Amount.UnitTonnes;}))return true;
    auto Candidate = *Inventory;
    TSet<FName> Managed = Candidate.ManifestItems;
    for (const auto& Amount : Desired) Managed.Add(Amount.Item);
    for (FName Item : Managed)
    {
        const auto* Amount = Desired.FindByPredicate([Item](const auto& Entry) { return Entry.Item == Item; });
        double Reserved = 0;
        for (const auto& Pair : State.Reservations)
            if (Pair.Value.Inventory == Key && Pair.Value.Item == Item) Reserved += Pair.Value.Quantity;
        if (Reserved > (Amount ? Amount->Quantity : 0) + UE_SMALL_NUMBER) return false;
        auto* Stack = Find(Candidate, Item);
        if (Amount)
        {
            if (Stack && !FMath::IsNearlyEqual(Stack->UnitTonnes, Amount->UnitTonnes)) return false;
            if (!Stack) { Stack = &Candidate.Items.AddDefaulted_GetRef(); Stack->Id = FGuid::NewGuid(); Stack->Type = Item; }
            Stack->Quantity = Amount->Quantity; Stack->UnitTonnes = Amount->UnitTonnes;
        }
        else Candidate.Items.RemoveAll([Item](const auto& Entry) { return Entry.Type == Item; });
    }
    if (Used(Candidate) > Candidate.CapacityTonnes + UE_SMALL_NUMBER) return false;
    Candidate.ManifestItems.Reset();
    for (const auto& Amount : Desired) Candidate.ManifestItems.Add(Amount.Item);
    const bool Changed=Inventory->Items.Num()!=Candidate.Items.Num() || Inventory->ManifestItems.Num()!=Candidate.ManifestItems.Num() ||
        Inventory->Items.ContainsByPredicate([&Candidate](const auto& A){const auto* B=Candidate.Items.FindByPredicate([&A](const auto& Entry){return Entry.Id==A.Id;});return !B || B->Type!=A.Type || B->Quantity!=A.Quantity || B->UnitTonnes!=A.UnitTonnes;});
    *Inventory = MoveTemp(Candidate); if(Changed)FSimDataEvents::Publish(State); return true;
}

double FEconomyLedger::Used(const FEconomyInventory& Inventory)
{
    double Total = Inventory.ReservedOutputTonnes;
    for (const auto& Stack : Inventory.Items) Total += Stack.Quantity * Stack.UnitTonnes;
    return Total;
}
double FEconomyLedger::Quantity(const FEconomyState& State, FName Inventory, FName Item)
{
    const auto* Inv = State.Inventories.Find(Inventory);
    if (!Inv) return 0;
    const auto* Stack = Inv->Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; });
    return Stack ? Stack->Quantity : 0;
}
double FEconomyLedger::Available(const FEconomyState& State, FName Inventory, FName Item)
{
    double Free = Quantity(State, Inventory, Item);
    for (const auto& Pair : State.Reservations)
        if (Pair.Value.Inventory == Inventory && Pair.Value.Item == Item) Free -= Pair.Value.Quantity;
    return FMath::Max(0.0, Free);
}
bool FEconomyLedger::Deposit(FEconomyState& State, FName Inventory, FName Item, double Count, double Mass)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Inv = State.Inventories.Find(Inventory);
    const bool Changed=Inv && DepositInto(*Inv, Item, Count, Mass);
    if(Changed)FSimDataEvents::Publish(State);return Changed;
}
bool FEconomyLedger::Withdraw(FEconomyState& State, FName Owner, FName Inventory, FName Item, double Count)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Inv = State.Inventories.Find(Inventory);
    if (!HasAccess(Inv, Owner) || !Positive(Count) || Available(State, Inventory, Item) < Count) return false;
    Find(*Inv, Item)->Quantity -= Count; FSimDataEvents::Publish(State); return true;
}
FGuid FEconomyLedger::Reserve(FEconomyState& State, FName Owner, FName Inventory, FName Item, double Count)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (!HasAccess(State.Inventories.Find(Inventory), Owner) || !Positive(Count) || Available(State, Inventory, Item) < Count) return {};
    FEconomyReservation Claim; Claim.Id = FGuid::NewGuid(); Claim.Owner = Owner; Claim.Inventory = Inventory; Claim.Item = Item; Claim.Quantity = Count;
    State.Reservations.Add(Claim.Id, Claim); FSimDataEvents::Publish(State); return Claim.Id;
}
bool FEconomyLedger::Release(FEconomyState& State, FName Owner, FGuid Id)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    const auto* Claim = State.Reservations.Find(Id);
    const bool Changed=Claim && Claim->Owner == Owner && State.Reservations.Remove(Id) == 1;
    if(Changed)FSimDataEvents::Publish(State);return Changed;
}
bool FEconomyLedger::Transfer(FEconomyState& State, FName Owner, FName Source, FName Destination, FName Item, double Count, FGuid Id)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* From = State.Inventories.Find(Source); auto* To = State.Inventories.Find(Destination);
    if (Source == Destination || !HasAccess(From, Owner) || !HasAccess(To, Owner) || !Positive(Count)) return false;
    auto* Claim = Id.IsValid() ? State.Reservations.Find(Id) : nullptr;
    if (Id.IsValid() && (!Claim || Claim->Owner != Owner || Claim->Inventory != Source || Claim->Item != Item || Claim->Quantity < Count)) return false;
    if ((Claim ? Quantity(State, Source, Item) : Available(State, Source, Item)) < Count) return false;
    auto* Stack = Find(*From, Item);
    const bool DestinationEmpty = Quantity(State, Destination, Item) == 0;
    const FGuid ItemId = Stack ? Stack->Id : FGuid();
    if (!Stack || !Deposit(State, Destination, Item, Count, Stack->UnitTonnes)) return false;
    Stack->Quantity -= Count;
    if (Stack->Quantity == 0)
    {
        if (DestinationEmpty) Find(*To, Item)->Id = ItemId;
        From->Items.RemoveAll([Item](const auto& Entry) { return Entry.Type == Item && Entry.Quantity == 0; });
    }
    if (Claim) { Claim->Quantity -= Count; if (Claim->Quantity <= UE_SMALL_NUMBER) State.Reservations.Remove(Id); }
    return true;
}
namespace
{
TMap<FName,int64> ChargeDeltas(FName From,FName To,const FCommercePrice& Quote)
{
    TMap<FName,int64> Deltas;Deltas.FindOrAdd(From)-=Quote.Total;Deltas.FindOrAdd(To)+=Quote.Base;
    if(Quote.Tax)Deltas.FindOrAdd(CommerceCharges::TaxAccount())+=Quote.Tax;
    if(Quote.Fee)Deltas.FindOrAdd(CommerceCharges::FeeAccount())+=Quote.Fee;
    return Deltas;
}
bool CanApplyDeltas(const FEconomyState& State,const TMap<FName,int64>& Deltas)
{
    for(const auto& Pair:Deltas)
    {
        const int64 Balance=State.Wallets.FindRef(Pair.Key);
        if(Balance<0 || (Pair.Value<0 && Balance < -Pair.Value) || (Pair.Value>0 && Balance>MAX_int64-Pair.Value))return false;
    }
    return true;
}
void ApplyDeltas(FEconomyState& State,const TMap<FName,int64>& Deltas)
{for(const auto& Pair:Deltas)State.Wallets.FindOrAdd(Pair.Key)+=Pair.Value;}
void RecordCharges(FEconomyState& State,const FCommercePrice& Quote,const TMap<FName,int64>& Deltas)
{
    auto& Entry=State.Journal.Last();Entry.BaseCredits=Quote.Base;Entry.TaxCredits=Quote.Tax;Entry.FeeCredits=Quote.Fee;
    Entry.WalletDeltas=Deltas;Entry.WalletBalancesAfter.Reset();
    for(const auto& Pair:Deltas)Entry.WalletBalancesAfter.Add(Pair.Key,State.Wallets.FindRef(Pair.Key));
}
}
bool FEconomyLedger::Pay(FEconomyState& State,FName From,FName To,int64 Credits,FGuid Id,FName Reason,ECommercePurpose Purpose)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    const auto Quote=CommerceCharges::Quote(Credits,Purpose);const auto Deltas=ChargeDeltas(From,To,Quote);
    if(!Id.IsValid() || From.IsNone() || To.IsNone() || From==To || !Quote.bValid || State.CommittedTransactions.Contains(Id) || !CanApplyDeltas(State,Deltas))return false;
    ApplyDeltas(State,Deltas);Record(State,Id,Reason,From,To,Credits);RecordCharges(State,Quote,Deltas);return true;
}
bool FEconomyLedger::Trade(FEconomyState& State, FName Buyer, FName Seller, FName Source, FName Destination, FName Item, double Count, int64 Price, FGuid Id,ECommercePurpose Purpose)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    const auto Quote=CommerceCharges::Quote(Price,Purpose);const auto Deltas=ChargeDeltas(Buyer,Seller,Quote);
    const auto* From = State.Inventories.Find(Source); auto* To = State.Inventories.Find(Destination);
    if (Buyer == Seller || Source == Destination || !HasAccess(From, Seller) || !HasAccess(To, Buyer) || !Positive(Count) ||
        Available(State, Source, Item) < Count || !Id.IsValid() || State.CommittedTransactions.Contains(Id) || Price < 0 ||
        !Quote.bValid || !CanApplyDeltas(State,Deltas)) return false;
    const auto* Stack = From->Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; });
    const FGuid SoldId = Stack ? Stack->Id : FGuid();
    const bool FullStack = Stack && Stack->Quantity == Count;
    const bool DestinationEmpty = Quantity(State, Destination, Item) == 0;
    if (!Stack || !Deposit(State, Destination, Item, Count, Stack->UnitTonnes)) return false;
    if (FullStack && DestinationEmpty) Find(State.Inventories.FindChecked(Destination), Item)->Id = SoldId;
    auto& SourceInventory = State.Inventories.FindChecked(Source);
    auto* Sold = Find(SourceInventory, Item); Sold->Quantity -= Count;
    SourceInventory.Items.RemoveAll([Item](const auto& Entry) { return Entry.Type == Item && Entry.Quantity == 0; });
    ApplyDeltas(State,Deltas);
    Record(State, Id, TEXT("Market purchase"), Buyer, Seller, Price);RecordCharges(State,Quote,Deltas);
    auto& Entry=State.Journal.Last();Entry.Item=Item;Entry.Quantity=Count;Entry.UnitPrice=Count>0?double(Price)/Count:0;Entry.Source=Source;Entry.Destination=Destination;
    EconomyAI::RecordTrade(State,SourceInventory.System,Item,Count,double(Price));
    return true;
}
FGuid FEconomyLedger::QueueJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output, FName Recipe,
    const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (IsPlayerOwner(State,Owner)) return {};
    return QueueAuthorizedJob(State, Owner, Facility, Input, Output, Recipe, Materials, Products, Duration);
}
FGuid FEconomyLedger::QueueAuthorizedJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output, FName Recipe,
    const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (Owner.IsNone() || Facility.IsNone() || Recipe.IsNone() || Input == Output ||
        !HasAccess(State.Inventories.Find(Input), Owner) || !HasAccess(State.Inventories.Find(Output), Owner) ||
        !Positive(Duration) || !ValidateAmounts(Materials) || !ValidateAmounts(Products)) return {};
    FIndustryJob Job; Job.Id = FGuid::NewGuid(); Job.Owner = Owner; Job.Facility = Facility; Job.Input = Input; Job.Output = Output;
    Job.Recipe = Recipe; Job.Materials = Materials; Job.Products = Products; Job.Duration = Duration; Job.Remaining = Duration;
    State.Jobs.Add(Job); FSimDataEvents::Publish(State); return Job.Id;
}
FGuid FEconomyLedger::GrantBlueprint(FEconomyState& State, FName Owner, FName Recipe, bool Original, int32 Runs, double ME, double TE)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (Owner.IsNone() || Recipe.IsNone() || (!Original && Runs <= 0) || !FMath::IsFinite(ME) || ME < 0 || ME > 0.1 ||
        !FMath::IsFinite(TE) || TE < 0 || TE > 0.2) return {};
    FBlueprintInstance BP; BP.Id = FGuid::NewGuid(); BP.Owner = Owner; BP.Recipe = Recipe;
    BP.bOriginal = Original; BP.RunsRemaining = Original ? 0 : Runs; BP.MaterialEfficiency = ME; BP.TimeEfficiency = TE;
    State.Blueprints.Add(BP.Id, BP); FSimDataEvents::Publish(State); return BP.Id;
}
FGuid FEconomyLedger::QueueBlueprintJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output,
    FGuid Blueprint, int32 Runs, const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    const auto* BP = State.Blueprints.Find(Blueprint);
    if (!BP || BP->Owner != Owner || (IsPlayerOwner(State,Owner) && (!State.BlueprintReceipts.Contains(Blueprint) || !State.CommittedTransactions.Contains(State.BlueprintReceipts.FindRef(Blueprint)))) || Runs <= 0 || Runs > 1000000 || (!BP->bOriginal && BP->RunsRemaining < Runs) ||
        State.Jobs.ContainsByPredicate([Blueprint](const auto& Job) { return Job.Blueprint == Blueprint &&
            Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled; })) return {};
    auto Inputs = Materials; auto Outputs = Products;
    for (auto& Amount : Inputs)
        Amount.Quantity = Amount.bWholeUnits ? FMath::CeilToDouble(Amount.Quantity * (1.0 - BP->MaterialEfficiency) - 1.e-9) * Runs
            : Amount.Quantity * Runs * (1.0 - BP->MaterialEfficiency);
    for (auto& Amount : Outputs) Amount.Quantity *= Runs;
    const FGuid Id = QueueAuthorizedJob(State, Owner, Facility, Input, Output, BP->Recipe, Inputs, Outputs, Duration * Runs * (1.0 - BP->TimeEfficiency));
    if (Id.IsValid()) { State.Jobs.Last().Blueprint = Blueprint; State.Jobs.Last().Runs = Runs; }
    return Id;
}
bool FEconomyLedger::CanStartJob(const FEconomyState& State, const FIndustryJob& Job, bool Operational, FText& Reason, bool CheckFacilitySlot)
{
    if (Job.State == EIndustryJobState::Completed || Job.State == EIndustryJobState::Cancelled)
    { Reason = INVTEXT("This job has finished."); return false; }
    if (!Operational) { Reason = INVTEXT("Facility is offline or unavailable."); return false; }
    if (Job.State == EIndustryJobState::Running) { Reason = INVTEXT("Production in progress; output is delivered automatically."); return true; }
    if (CheckFacilitySlot && State.Jobs.ContainsByPredicate([&Job](const auto& Other) { return Other.Id != Job.Id && Other.Facility == Job.Facility && Other.State == EIndustryJobState::Running; }))
    { Reason = INVTEXT("Waiting for the facility's current job to finish."); return false; }
    const auto* In = State.Inventories.Find(Job.Input); const auto* Out = State.Inventories.Find(Job.Output);
    if (!HasAccess(In, Job.Owner) || !HasAccess(Out, Job.Owner) || Job.Input == Job.Output)
    { Reason = INVTEXT("The job's input or output inventory is unavailable to its owner."); return false; }
    if (!ValidateAmounts(Job.Materials) || !ValidateAmounts(Job.Products) || !Positive(Job.Duration))
    { Reason = INVTEXT("The job has invalid material, product or time data."); return false; }
    if (Job.Blueprint.IsValid())
    {
        const auto* BP = State.Blueprints.Find(Job.Blueprint);
        if (!BP || BP->Owner != Job.Owner || BP->Recipe != Job.Recipe ||
            (IsPlayerOwner(State,Job.Owner) && (!State.BlueprintReceipts.Contains(Job.Blueprint) || !State.CommittedTransactions.Contains(State.BlueprintReceipts.FindRef(Job.Blueprint)))))
        { Reason = INVTEXT("The required blueprint is unavailable."); return false; }
        if (!BP->bOriginal && BP->RunsRemaining < Job.Runs)
        { Reason = INVTEXT("The blueprint copy has insufficient remaining runs."); return false; }
    }
    for (const auto& Material : Job.Materials)
    {
        const auto* Stack = In->Items.FindByPredicate([&](const auto& Entry) { return Entry.Type == Material.Item; });
        const double Free = Available(State, Job.Input, Material.Item);
        if (!Stack || !FMath::IsNearlyEqual(Stack->UnitTonnes, Material.UnitTonnes) || Free + UE_SMALL_NUMBER < Material.Quantity)
        { Reason = FText::Format(INVTEXT("INPUT: {0} — {1} required, {2} unreserved."), FText::FromName(Material.Item), FText::AsNumber(Material.Quantity), FText::AsNumber(Free)); return false; }
    }
    double OutputMass = 0;
    for (const auto& Product : Job.Products)
    {
        OutputMass += Product.Quantity * Product.UnitTonnes;
        const auto* Stack = Out->Items.FindByPredicate([&](const auto& Entry) { return Entry.Type == Product.Item; });
        if (Stack && !FMath::IsNearlyEqual(Stack->UnitTonnes, Product.UnitTonnes))
        { Reason = INVTEXT("The output inventory contains an incompatible product stack."); return false; }
    }
    const double FreeSpace = Out->CapacityTonnes - Used(*Out);
    if (!FMath::IsFinite(OutputMass) || OutputMass > FreeSpace + UE_SMALL_NUMBER)
    { Reason = FText::Format(INVTEXT("OUTPUT STORAGE: {0} t required, {1} t free."), FText::AsNumber(OutputMass), FText::AsNumber(FMath::Max(0.0, FreeSpace))); return false; }
    Reason = INVTEXT("Ready to install. Materials are consumed when production begins; output is delivered automatically."); return true;
}
bool FEconomyLedger::CancelJob(FEconomyState& State, FName Owner, FGuid Id)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Job = State.Jobs.FindByPredicate([Id](const auto& Entry) { return Entry.Id == Id; });
    if (!Job || Job->Owner != Owner || Job->State == EIndustryJobState::Completed || Job->State == EIndustryJobState::Cancelled) return false;
    if (Job->State == EIndustryJobState::Running)
    {
        // Inputs have been consumed. Cancellation releases output space, never manufactures a refund.
        auto* Out = State.Inventories.Find(Job->Output);
        if (!Out || Out->Owner != Owner) return false;
        for (const auto& Product : Job->Products) Out->ReservedOutputTonnes = FMath::Max(0.0, Out->ReservedOutputTonnes - Product.Quantity * Product.UnitTonnes);
    }
    Job->State = EIndustryJobState::Cancelled; Job->CompletionAt=0;ProductionEvents::Emit(Job->Facility,Job->Id,EProductionEvent::Cancelled,INVTEXT("Job cancelled; reserved output space released.")); FSimDataEvents::Publish(State); return true;
}
int32 FEconomyLedger::AdvanceJobs(FEconomyState& State, double Seconds, TFunctionRef<bool(FName)> Operational)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if (!FMath::IsFinite(Seconds) || Seconds <= 0) return 0;
    int32 Completed = 0;
    TMap<FName, double> TimeLeft;
    for (auto& Job : State.Jobs)
    {
        if (Job.State == EIndustryJobState::Completed || Job.State == EIndustryJobState::Cancelled || !Operational(Job.Facility)) continue;
        if (Job.State != EIndustryJobState::Running)
        {
            FText Reason;
            if (!CanStartJob(State, Job, true, Reason)) { if(Job.State!=EIndustryJobState::Blocked){Job.State=EIndustryJobState::Blocked;FSimDataEvents::Publish(State);} continue; }
            auto* In = State.Inventories.Find(Job.Input); auto* Out = State.Inventories.Find(Job.Output);
            auto* BP = Job.Blueprint.IsValid() ? State.Blueprints.Find(Job.Blueprint) : nullptr;
            double OutputMass = 0;
            for (const auto& Product : Job.Products) OutputMass += Product.Quantity * Product.UnitTonnes;
            for (const auto& Material : Job.Materials) Find(*In, Material.Item)->Quantity = FMath::Max(0.0, Find(*In, Material.Item)->Quantity - Material.Quantity);
            if (BP && !BP->bOriginal) BP->RunsRemaining -= Job.Runs;
            for (const auto& Product : Job.Products)
                if (!Find(*Out, Product.Item))
                {
                    auto& Stack = Out->Items.AddDefaulted_GetRef(); Stack.Id = FGuid::NewGuid();
                    Stack.Type = Product.Item; Stack.UnitTonnes = Product.UnitTonnes;
                }
            Out->ReservedOutputTonnes += OutputMass; Job.State = EIndustryJobState::Running; FSimDataEvents::Publish(State);
        }
        double& Budget = TimeLeft.FindOrAdd(Job.Facility, Seconds);
        const double UsedTime = FMath::Min(Job.Remaining, Budget);
        Job.Remaining = FMath::Max(0.0, Job.Remaining - UsedTime);
        Budget -= UsedTime;
        if (Job.Remaining > 0) continue;
        auto* Out = State.Inventories.Find(Job.Output);
        if (!HasAccess(Out, Job.Owner) || !ValidateAmounts(Job.Products)) continue;
        // The destination can change after a job starts (or be restored from a
        // save). Commit all products and reservation release together. A failed
        // deposit must not leave partial output or falsely report completion.
        auto Candidate = *Out;
        for (const auto& Product : Job.Products)
            Candidate.ReservedOutputTonnes = FMath::Max(0.0, Candidate.ReservedOutputTonnes - Product.Quantity * Product.UnitTonnes);
        bool Deposited = true;
        for (const auto& Product : Job.Products)
            if (!DepositInto(Candidate, Product.Item, Product.Quantity, Product.UnitTonnes))
            { Deposited = false; break; }
        if (!Deposited) continue;
        // Failed attempts stay Running at zero remaining time: retries never
        // re-consume inputs or blueprint runs, and the facility remains occupied.
        *Out = MoveTemp(Candidate);
        Job.State = EIndustryJobState::Completed; FSimDataEvents::Publish(State); ++Completed;
    }
    return Completed;
}
double FEconomyLedger::SystemQuantity(const FEconomyState& State, FName System, FName Item)
{
    double Total = 0;
    for (const auto& Pair : State.Inventories)
        if (Pair.Value.System == System && Pair.Value.bMarket)
        {
            const auto* Stack = Pair.Value.Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; });
            if (Stack) Total += Available(State, Pair.Key, Item) * Stack->UnitTonnes;
        }
    return Total;
}

namespace
{
bool OrderTotal(double Quantity,int64 Price,int64& Total)
{
    if(!Positive(Quantity) || !FMath::IsNearlyEqual(Quantity,FMath::RoundToDouble(Quantity),1.e-9) ||
        Price<=0 || Price>1000000000000000LL || Quantity>9000000000000000.0/double(Price))return false;
    Total=static_cast<int64>(FMath::RoundToDouble(Quantity))*Price;return Total>0;
}
void OrderHistory(FEconomyState& State,FGuid Receipt,FName Reason,const FMarketOrder& Order,int64 Credits,FName From,FName To)
{
    Record(State,Receipt,Reason,From,To,Credits);auto& Entry=State.Journal.Last();
    Entry.Item=Order.Item;Entry.Quantity=Order.Remaining;Entry.UnitPrice=double(Order.UnitPrice);
    Entry.Source=Order.Side==EMarketOrderSide::Sell?Order.Inventory:Order.Market;
    Entry.Destination=Order.Side==EMarketOrderSide::Buy?Order.Inventory:Order.Market;
    if(Order.Side==EMarketOrderSide::Buy)Entry.BuyOrder=Order.Id;else Entry.SellOrder=Order.Id;
}
bool FillOrders(FEconomyState& State,FMarketOrder& Buy,FMarketOrder& Sell,int64 NowUtc)
{
    if(Buy.Owner==Sell.Owner || Buy.Item!=Sell.Item || Buy.Market!=Sell.Market || Buy.UnitPrice<Sell.UnitPrice ||
        Buy.State!=EMarketOrderState::Open || Sell.State!=EMarketOrderState::Open ||
        Buy.ExpiresUtc<=NowUtc || Sell.ExpiresUtc<=NowUtc || !FMath::IsNearlyEqual(Buy.UnitTonnes,Sell.UnitTonnes))return false;
    const double Quantity=FMath::Min(Buy.Remaining,Sell.Remaining);
    const int64 Price=Buy.Sequence<Sell.Sequence?Buy.UnitPrice:Sell.UnitPrice;
    int64 Cost=0;if(!OrderTotal(Quantity,Price,Cost))return false;
    auto* Source=State.Inventories.Find(Sell.Inventory);auto* Destination=State.Inventories.Find(Buy.Inventory);
    const auto* Market=State.Inventories.Find(Buy.Market);
    auto* Claim=State.Reservations.Find(Sell.CargoReservation);
    const double Mass=Quantity*Buy.UnitTonnes;
    if(!HasAccess(Source,Sell.Owner) || !HasAccess(Destination,Buy.Owner) || !Market || !Market->bMarket ||
        Source->System!=Market->System || Destination->System!=Market->System || !Positive(Mass) ||
        !Claim || Claim->Owner!=Sell.Owner || Claim->Inventory!=Sell.Inventory || Claim->Item!=Sell.Item || Claim->Quantity<Quantity ||
        FEconomyLedger::Available(State,Sell.Inventory,Sell.Item)+Claim->Quantity<Quantity || Buy.ReservedTonnes+UE_SMALL_NUMBER<Mass)return false;
    const auto* Stock=Find(*Source,Sell.Item);
    if(!Stock || Stock->Quantity<Quantity || !FMath::IsNearlyEqual(Stock->UnitTonnes,Buy.UnitTonnes))return false;
    const double Remaining=Buy.Remaining-Quantity;
    int64 Required=0;if(Remaining>0 && !OrderTotal(Remaining,Buy.UnitPrice,Required))return false;
    if(Buy.EscrowCredits<Cost || Buy.EscrowCredits-Cost<Required)return false;
    const int64 Refund=Buy.EscrowCredits-Cost-Required;
    const auto Charge=CommerceCharges::Quote(Cost,(Sell.bApplyCommerceCharges || Buy.bApplyCommerceCharges)?ECommercePurpose::MarketSale:ECommercePurpose::None);
    TMap<FName,int64> FillDeltas;FillDeltas.FindOrAdd(Sell.Owner)+=Charge.Base;FillDeltas.FindOrAdd(Buy.Owner)+=Refund;
    if(Charge.Tax)FillDeltas.FindOrAdd(CommerceCharges::TaxAccount())+=Charge.Tax;
    if(!CanApplyDeltas(State,FillDeltas))return false;
    auto Out=*Destination;Out.ReservedOutputTonnes=FMath::Max(0.0,Out.ReservedOutputTonnes-Mass);
    const bool FullStack=Stock->Quantity==Quantity,EmptyDestination=FEconomyLedger::Quantity(State,Buy.Inventory,Buy.Item)==0;
    if(!DepositInto(Out,Buy.Item,Quantity,Buy.UnitTonnes))return false;
    if(FullStack && EmptyDestination)Find(Out,Buy.Item)->Id=Stock->Id;
    auto In=*Source;Find(In,Sell.Item)->Quantity-=Quantity;
    In.Items.RemoveAll([&](const auto& Item){return Item.Type==Sell.Item && Item.Quantity==0;});
    *Source=MoveTemp(In);*Destination=MoveTemp(Out);
    Claim->Quantity-=Quantity;if(Claim->Quantity<=UE_SMALL_NUMBER)State.Reservations.Remove(Sell.CargoReservation);
    ApplyDeltas(State,FillDeltas);
    Buy.Remaining=Remaining;Buy.EscrowCredits=Required;Buy.ReservedTonnes=FMath::Max(0.0,Buy.ReservedTonnes-Mass);
    Sell.Remaining-=Quantity;
    if(Buy.Remaining==0)Buy.State=EMarketOrderState::Filled;
    if(Sell.Remaining==0)Sell.State=EMarketOrderState::Filled;
    Record(State,FGuid::NewGuid(),TEXT("Limit order fill"),Buy.Owner,Sell.Owner,Cost);
    auto& Entry=State.Journal.Last();Entry.TimestampUtc=NowUtc;Entry.Item=Buy.Item;Entry.Quantity=Quantity;Entry.UnitPrice=double(Price);
    RecordCharges(State,Charge,FillDeltas);
    Entry.Category=EWalletTransactionCategory::Market;
    Entry.Source=Sell.Inventory;Entry.Destination=Buy.Inventory;Entry.BuyOrder=Buy.Id;Entry.SellOrder=Sell.Id;
    EconomyAI::RecordTrade(State,Source->System,Buy.Item,Quantity,double(Cost));
    return true;
}
}
FGuid FEconomyLedger::PlaceMarketOrder(FEconomyState& State,FName Owner,FName Market,FName Inventory,FName Item,
    EMarketOrderSide Side,double Count,double Mass,int64 Price,int64 NowUtc,int64 ExpiresUtc,FGuid Receipt,FText& Reason,ECommercePurpose Purpose)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    int64 Total=0;
    if((Side!=EMarketOrderSide::Buy && Side!=EMarketOrderSide::Sell) || Owner.IsNone() || Market.IsNone() || Item.IsNone() || Item.ToString().StartsWith(TEXT("Courier/")) ||
        !Receipt.IsValid() || State.CommittedTransactions.Contains(Receipt) || State.MarketOrders.Contains(Receipt) ||
        !OrderTotal(Count,Price,Total) || !Positive(Mass) || !Positive(Count*Mass) || NowUtc<0 || ExpiresUtc<=NowUtc ||
        ExpiresUtc-NowUtc>30*24*3600 || State.NextMarketSequence==MAX_int64)
    {Reason=INVTEXT("Choose whole units/tonnes, a positive whole-credit limit price, and an expiry within 30 days.");return {};}
    const auto* Location=State.Inventories.Find(Market);auto* Hold=State.Inventories.Find(Inventory);
    if(!HasAccess(Hold,Owner) || !Location || !Location->bMarket || Location->System!=Hold->System)
    {Reason=INVTEXT("The order inventory is unavailable in this market.");return {};}
    int32 Open=0,Owned=0;for(const auto& Pair:State.MarketOrders)if(Pair.Value.State==EMarketOrderState::Open){++Open;if(Pair.Value.Owner==Owner)++Owned;}
    if(Open>=1024 || Owned>=64){Reason=INVTEXT("The market or owner has reached its open-order limit.");return {};}
    const int64 Brokerage=Purpose==ECommercePurpose::Brokerage?CommerceCharges::BrokerFee(Total):0;
    if(State.Wallets.FindRef(Owner)<Brokerage+(Side==EMarketOrderSide::Buy?Total:0))
    {Reason=INVTEXT("Insufficient credits for escrow and the disclosed broker fee.");return {};}
    const auto Before=State;
    FMarketOrder Order;Order.Id=Receipt;Order.Owner=Owner;Order.Market=Market;Order.Inventory=Inventory;Order.Item=Item;Order.Side=Side;
    Order.Quantity=Order.Remaining=FMath::RoundToDouble(Count);Order.UnitTonnes=Mass;Order.UnitPrice=Price;Order.CreatedUtc=NowUtc;Order.ExpiresUtc=ExpiresUtc;
    if(Side==EMarketOrderSide::Buy)
    {
        const auto* Existing=Find(*Hold,Item);
        if(State.Wallets.FindRef(Owner)<Total || Used(*Hold)+Count*Mass>Hold->CapacityTonnes+UE_SMALL_NUMBER ||
            (Existing && !FMath::IsNearlyEqual(Existing->UnitTonnes,Mass)))
        {Reason=INVTEXT("Insufficient credits or destination space for the buy order.");return {};}
        Order.EscrowCredits=Total;Order.ReservedTonnes=Order.Remaining*Mass;
        State.Wallets.FindOrAdd(Owner)-=Total;Hold->ReservedOutputTonnes+=Order.ReservedTonnes;
    }
    else
    {
        const auto* Stock=Find(*Hold,Item);
        if(!Stock || !FMath::IsNearlyEqual(Stock->UnitTonnes,Mass) || Available(State,Inventory,Item)<Order.Remaining)
        {Reason=INVTEXT("Insufficient unreserved stock for the sell order.");return {};}
        Order.CargoReservation=Reserve(State,Owner,Inventory,Item,Order.Remaining);
        if(!Order.CargoReservation.IsValid()){Reason=INVTEXT("The selected cargo could not be reserved.");return {};}
    }
    if(Brokerage>0 && !Pay(State,Owner,CommerceCharges::FeeAccount(),Brokerage,FGuid::NewGuid(),TEXT("Market order broker fee"),ECommercePurpose::Brokerage))
    {State=Before;Reason=INVTEXT("The broker fee could not be committed. Escrow and reservations restored.");return {};}
    if(Brokerage>0){State.Journal.Last().Reference=Receipt;State.Journal.Last().TimestampUtc=NowUtc;}
    Order.bApplyCommerceCharges=Purpose==ECommercePurpose::Brokerage;
    Order.Sequence=++State.NextMarketSequence;State.MarketOrders.Add(Order.Id,Order);
    OrderHistory(State,Receipt,Side==EMarketOrderSide::Buy?FName(TEXT("Buy order escrow")):FName(TEXT("Sell order reservation")),Order,Order.EscrowCredits,Owner,TEXT("Market escrow"));
    State.Journal.Last().TimestampUtc=NowUtc;Reason=INVTEXT("Limit order placed. Credits or cargo are reserved until settlement, cancellation or expiry.");return Order.Id;
}
bool FEconomyLedger::ModifyMarketOrder(FEconomyState& State,FName Owner,FGuid Id,int64 Price,FGuid Receipt,FText& Reason,ECommercePurpose Purpose)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Order=State.MarketOrders.Find(Id);int64 Required=0;
    if(!Order || Order->Owner!=Owner || Order->State!=EMarketOrderState::Open || !Receipt.IsValid() || State.CommittedTransactions.Contains(Receipt) ||
        !OrderTotal(Order->Remaining,Price,Required) || State.Wallets.FindRef(Owner)<0 || Order->EscrowCredits<0 || Order->EscrowCredits>9000000000000000LL || State.NextMarketSequence==MAX_int64)
    {Reason=INVTEXT("Select your own open order and a valid limit price.");return false;}
    const int64 Brokerage=Purpose==ECommercePurpose::Brokerage && Order->UnitPrice!=Price?CommerceCharges::BrokerFee(Required):0;
    const auto Before=State;
    int64 Delta=0;
    if(Order->Side==EMarketOrderSide::Buy)
    {
        Delta=Required-Order->EscrowCredits;
        if((Delta>0 && State.Wallets.FindRef(Owner)<Delta) || (Delta<0 && State.Wallets.FindRef(Owner)>MAX_int64+Delta))
        {Reason=INVTEXT("Insufficient credits or wallet capacity for this price change.");return false;}
        State.Wallets.FindOrAdd(Owner)-=Delta;Order->EscrowCredits=Required;
    }
    if(Brokerage>0 && !Pay(State,Owner,CommerceCharges::FeeAccount(),Brokerage,FGuid::NewGuid(),TEXT("Market order repricing broker fee"),ECommercePurpose::Brokerage))
    {State=Before;Reason=INVTEXT("Insufficient credits for the broker fee. Order and escrow restored.");return false;}
    if(Brokerage>0)State.Journal.Last().Reference=Id;
    if(Purpose==ECommercePurpose::Brokerage)Order->bApplyCommerceCharges=true;
    if(Order->UnitPrice!=Price)Order->Sequence=++State.NextMarketSequence;
    Order->UnitPrice=Price;
    OrderHistory(State,Receipt,TEXT("Limit order repriced"),*Order,FMath::Abs(Delta),Delta<0?FName(TEXT("Market escrow")):Owner,Delta<0?Owner:FName(TEXT("Market escrow")));
    Reason=INVTEXT("Order price changed. Repricing resets time priority.");return true;
}
bool FEconomyLedger::CancelMarketOrder(FEconomyState& State,FName Owner,FGuid Id,FGuid Receipt,FText& Reason)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    auto* Order=State.MarketOrders.Find(Id);
    if(!Order || Order->Owner!=Owner || Order->State!=EMarketOrderState::Open || !Receipt.IsValid() || State.CommittedTransactions.Contains(Receipt) ||
        Order->EscrowCredits<0 || State.Wallets.FindRef(Owner)<0 || State.Wallets.FindRef(Owner)>MAX_int64-Order->EscrowCredits)
    {Reason=INVTEXT("Select your own open order to cancel.");return false;}
    const int64 Refund=Order->EscrowCredits;
    if(Order->Side==EMarketOrderSide::Buy)
    {
        State.Wallets.FindOrAdd(Owner)+=Refund;Order->EscrowCredits=0;
        if(auto* Inventory=State.Inventories.Find(Order->Inventory))Inventory->ReservedOutputTonnes=FMath::Max(0.0,Inventory->ReservedOutputTonnes-Order->ReservedTonnes);
        Order->ReservedTonnes=0;
    }
    else Release(State,Owner,Order->CargoReservation);
    OrderHistory(State,Receipt,TEXT("Limit order cancelled"),*Order,Refund,TEXT("Market escrow"),Owner);
    Order->State=EMarketOrderState::Cancelled;Reason=INVTEXT("Order cancelled. Unspent escrow or unsold cargo reservations were released.");return true;
}
int32 FEconomyLedger::AdvanceMarketOrders(FEconomyState& State,int64 NowUtc)
{
    FScopedSimDataChangeBatch DataChangeBatch(State);
    if(NowUtc<0)return 0;
    struct FBook {TArray<FGuid> Buys,Sells;};
    TMap<FName,TMap<FName,FBook>> Markets;
    for(auto& Pair:State.MarketOrders)
    {
        auto& Order=Pair.Value;if(Order.State!=EMarketOrderState::Open)continue;
        if(Order.ExpiresUtc<=NowUtc)
        {
            FText Reason;
            if(CancelMarketOrder(State,Order.Owner,Order.Id,FGuid::NewGuid(),Reason))
            {Order.State=EMarketOrderState::Expired;State.Journal.Last().Reason=TEXT("Limit order expired");State.Journal.Last().TimestampUtc=NowUtc;}
            continue;
        }
        auto& Book=Markets.FindOrAdd(Order.Market).FindOrAdd(Order.Item);
        (Order.Side==EMarketOrderSide::Buy?Book.Buys:Book.Sells).Add(Order.Id);
    }
    int32 Fills=0;
    for(auto& Market:Markets)for(auto& Item:Market.Value)
    {
        auto& Book=Item.Value;
        const auto Sort=[&](TArray<FGuid>& Ids,bool Buy)
        {
            Ids.Sort([&](FGuid A,FGuid B)
            {
                const auto& Left=State.MarketOrders.FindChecked(A);const auto& Right=State.MarketOrders.FindChecked(B);
                return Left.UnitPrice!=Right.UnitPrice?(Buy?Left.UnitPrice>Right.UnitPrice:Left.UnitPrice<Right.UnitPrice):Left.Sequence<Right.Sequence;
            });
        };
        Sort(Book.Buys,true);Sort(Book.Sells,false);
        for(FGuid BuyId:Book.Buys)for(FGuid SellId:Book.Sells)
        {
            auto& Buy=State.MarketOrders.FindChecked(BuyId);auto& Sell=State.MarketOrders.FindChecked(SellId);
            if(Buy.State!=EMarketOrderState::Open)break;
            if(Sell.UnitPrice>Buy.UnitPrice)break;
            if(FillOrders(State,Buy,Sell,NowUtc))++Fills;
        }
    }
    return Fills;
}
