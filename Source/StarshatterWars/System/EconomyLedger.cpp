#include "GameStructs_economy.h"

namespace
{
constexpr double EconomyQuantityLimit = 1.e15;
bool Positive(double Value) { return FMath::IsFinite(Value) && Value > 0 && Value <= EconomyQuantityLimit; }
FEconomyItem* Find(FEconomyInventory& Inv, FName Item)
{ return Inv.Items.FindByPredicate([Item](const auto& Stack) { return Stack.Type == Item; }); }
bool HasAccess(const FEconomyInventory* Inv, FName Owner) { return Inv && !Owner.IsNone() && Inv->Owner == Owner; }
void Record(FEconomyState& State, FGuid Id, FName Reason, FName From, FName To, int64 Credits)
{
    FEconomyTransaction Entry; Entry.Id = Id; Entry.Reason = Reason; Entry.From = From; Entry.To = To; Entry.Credits = Credits;
    State.Journal.Add(Entry); State.CommittedTransactions.Add(Id);
    if (State.Journal.Num() > 512) State.Journal.RemoveAt(0, State.Journal.Num() - 512);
}
bool ValidateAmounts(const TArray<FCommodityAmount>& Amounts)
{
    TSet<FName> Seen;
    for (const auto& Amount : Amounts)
    {
        if (Amount.Item.IsNone() || !Positive(Amount.Quantity) || !Positive(Amount.UnitTonnes) ||
            !Positive(Amount.Quantity * Amount.UnitTonnes) || Seen.Contains(Amount.Item)) return false;
        Seen.Add(Amount.Item);
    }
    return !Amounts.IsEmpty();
}
}

FEconomyInventory* FEconomyLedger::CreateInventory(FEconomyState& State, FName Key, FName Owner, FName System, double Capacity)
{
    if (Key.IsNone() || Owner.IsNone() || !FMath::IsFinite(Capacity) || Capacity < 0 || Capacity > EconomyQuantityLimit) return nullptr;
    if (auto* Existing = State.Inventories.Find(Key))
        return Existing->Owner == Owner && Existing->System == System ? Existing : nullptr;
    FEconomyInventory Inv; Inv.Id = FGuid::NewGuid(); Inv.Owner = Owner; Inv.System = System; Inv.CapacityTonnes = Capacity;
    return &State.Inventories.Add(Key, MoveTemp(Inv));
}
bool FEconomyLedger::ReconcileManifest(FEconomyState& State, FName Key, const TArray<FCommodityAmount>& Desired)
{
    auto* Inventory = State.Inventories.Find(Key);
    if (!Inventory || (!Desired.IsEmpty() && !ValidateAmounts(Desired))) return false;
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
    *Inventory = MoveTemp(Candidate); return true;
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
    auto* Inv = State.Inventories.Find(Inventory);
    if (!Inv || Item.IsNone() || !Positive(Count) || !Positive(Mass) || !Positive(Count * Mass) ||
        Used(*Inv) + Count * Mass > Inv->CapacityTonnes + UE_SMALL_NUMBER) return false;
    auto* Stack = Find(*Inv, Item);
    if (Stack && (!FMath::IsNearlyEqual(Stack->UnitTonnes, Mass) || Stack->Quantity > EconomyQuantityLimit - Count)) return false;
    if (!Stack) { Stack = &Inv->Items.AddDefaulted_GetRef(); Stack->Id = FGuid::NewGuid(); Stack->Type = Item; Stack->UnitTonnes = Mass; }
    Stack->Quantity += Count; return true;
}
bool FEconomyLedger::Withdraw(FEconomyState& State, FName Owner, FName Inventory, FName Item, double Count)
{
    auto* Inv = State.Inventories.Find(Inventory);
    if (!HasAccess(Inv, Owner) || !Positive(Count) || Available(State, Inventory, Item) < Count) return false;
    Find(*Inv, Item)->Quantity -= Count; return true;
}
FGuid FEconomyLedger::Reserve(FEconomyState& State, FName Owner, FName Inventory, FName Item, double Count)
{
    if (!HasAccess(State.Inventories.Find(Inventory), Owner) || !Positive(Count) || Available(State, Inventory, Item) < Count) return {};
    FEconomyReservation Claim; Claim.Id = FGuid::NewGuid(); Claim.Owner = Owner; Claim.Inventory = Inventory; Claim.Item = Item; Claim.Quantity = Count;
    State.Reservations.Add(Claim.Id, Claim); return Claim.Id;
}
bool FEconomyLedger::Release(FEconomyState& State, FName Owner, FGuid Id)
{
    const auto* Claim = State.Reservations.Find(Id);
    return Claim && Claim->Owner == Owner && State.Reservations.Remove(Id) == 1;
}
bool FEconomyLedger::Transfer(FEconomyState& State, FName Owner, FName Source, FName Destination, FName Item, double Count, FGuid Id)
{
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
bool FEconomyLedger::Pay(FEconomyState& State, FName From, FName To, int64 Credits, FGuid Id, FName Reason)
{
    if (!Id.IsValid() || From.IsNone() || To.IsNone() || From == To || Credits < 0 || State.CommittedTransactions.Contains(Id) ||
        State.Wallets.FindRef(From) < Credits || State.Wallets.FindRef(To) > MAX_int64 - Credits) return false;
    State.Wallets.FindOrAdd(From) -= Credits; State.Wallets.FindOrAdd(To) += Credits;
    Record(State, Id, Reason, From, To, Credits); return true;
}
bool FEconomyLedger::Trade(FEconomyState& State, FName Buyer, FName Seller, FName Source, FName Destination, FName Item, double Count, int64 Price, FGuid Id)
{
    const auto* From = State.Inventories.Find(Source); auto* To = State.Inventories.Find(Destination);
    if (Buyer == Seller || Source == Destination || !HasAccess(From, Seller) || !HasAccess(To, Buyer) || !Positive(Count) ||
        Available(State, Source, Item) < Count || !Id.IsValid() || State.CommittedTransactions.Contains(Id) || Price < 0 ||
        State.Wallets.FindRef(Buyer) < Price || State.Wallets.FindRef(Seller) > MAX_int64 - Price) return false;
    const auto* Stack = From->Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; });
    const FGuid SoldId = Stack ? Stack->Id : FGuid();
    const bool FullStack = Stack && Stack->Quantity == Count;
    const bool DestinationEmpty = Quantity(State, Destination, Item) == 0;
    if (!Stack || !Deposit(State, Destination, Item, Count, Stack->UnitTonnes)) return false;
    if (FullStack && DestinationEmpty) Find(State.Inventories.FindChecked(Destination), Item)->Id = SoldId;
    auto& SourceInventory = State.Inventories.FindChecked(Source);
    auto* Sold = Find(SourceInventory, Item); Sold->Quantity -= Count;
    SourceInventory.Items.RemoveAll([Item](const auto& Entry) { return Entry.Type == Item && Entry.Quantity == 0; });
    State.Wallets.FindOrAdd(Buyer) -= Price; State.Wallets.FindOrAdd(Seller) += Price;
    Record(State, Id, TEXT("Market purchase"), Buyer, Seller, Price); return true;
}
FGuid FEconomyLedger::QueueJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output, FName Recipe,
    const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    if (Owner == Pilot()) return {};
    return QueueAuthorizedJob(State, Owner, Facility, Input, Output, Recipe, Materials, Products, Duration);
}
FGuid FEconomyLedger::QueueAuthorizedJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output, FName Recipe,
    const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    if (Owner.IsNone() || Facility.IsNone() || Recipe.IsNone() || Input == Output ||
        !HasAccess(State.Inventories.Find(Input), Owner) || !HasAccess(State.Inventories.Find(Output), Owner) ||
        !Positive(Duration) || !ValidateAmounts(Materials) || !ValidateAmounts(Products)) return {};
    FIndustryJob Job; Job.Id = FGuid::NewGuid(); Job.Owner = Owner; Job.Facility = Facility; Job.Input = Input; Job.Output = Output;
    Job.Recipe = Recipe; Job.Materials = Materials; Job.Products = Products; Job.Duration = Duration; Job.Remaining = Duration;
    State.Jobs.Add(Job); return Job.Id;
}
FGuid FEconomyLedger::GrantBlueprint(FEconomyState& State, FName Owner, FName Recipe, bool Original, int32 Runs, double ME, double TE)
{
    if (Owner.IsNone() || Recipe.IsNone() || (!Original && Runs <= 0) || !FMath::IsFinite(ME) || ME < 0 || ME > 0.1 ||
        !FMath::IsFinite(TE) || TE < 0 || TE > 0.2) return {};
    FBlueprintInstance BP; BP.Id = FGuid::NewGuid(); BP.Owner = Owner; BP.Recipe = Recipe;
    BP.bOriginal = Original; BP.RunsRemaining = Original ? 0 : Runs; BP.MaterialEfficiency = ME; BP.TimeEfficiency = TE;
    State.Blueprints.Add(BP.Id, BP); return BP.Id;
}
FGuid FEconomyLedger::QueueBlueprintJob(FEconomyState& State, FName Owner, FName Facility, FName Input, FName Output,
    FGuid Blueprint, int32 Runs, const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration)
{
    const auto* BP = State.Blueprints.Find(Blueprint);
    if (!BP || BP->Owner != Owner || (Owner == Pilot() && (!State.BlueprintReceipts.Contains(Blueprint) || !State.CommittedTransactions.Contains(State.BlueprintReceipts.FindRef(Blueprint)))) || Runs <= 0 || Runs > 1000000 || (!BP->bOriginal && BP->RunsRemaining < Runs) ||
        State.Jobs.ContainsByPredicate([Blueprint](const auto& Job) { return Job.Blueprint == Blueprint &&
            Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled; })) return {};
    auto Inputs = Materials; auto Outputs = Products;
    for (auto& Amount : Inputs) Amount.Quantity *= Runs * (1.0 - BP->MaterialEfficiency);
    for (auto& Amount : Outputs) Amount.Quantity *= Runs;
    const FGuid Id = QueueAuthorizedJob(State, Owner, Facility, Input, Output, BP->Recipe, Inputs, Outputs, Duration * Runs * (1.0 - BP->TimeEfficiency));
    if (Id.IsValid()) { State.Jobs.Last().Blueprint = Blueprint; State.Jobs.Last().Runs = Runs; }
    return Id;
}
bool FEconomyLedger::CancelJob(FEconomyState& State, FName Owner, FGuid Id)
{
    auto* Job = State.Jobs.FindByPredicate([Id](const auto& Entry) { return Entry.Id == Id; });
    if (!Job || Job->Owner != Owner || Job->State == EIndustryJobState::Completed || Job->State == EIndustryJobState::Cancelled) return false;
    if (Job->State == EIndustryJobState::Running)
    {
        // Inputs have been consumed. Cancellation releases output space, never manufactures a refund.
        auto& Out = State.Inventories.FindChecked(Job->Output);
        for (const auto& Product : Job->Products) Out.ReservedOutputTonnes = FMath::Max(0.0, Out.ReservedOutputTonnes - Product.Quantity * Product.UnitTonnes);
    }
    Job->State = EIndustryJobState::Cancelled; return true;
}
int32 FEconomyLedger::AdvanceJobs(FEconomyState& State, double Seconds, TFunctionRef<bool(FName)> Operational)
{
    if (!FMath::IsFinite(Seconds) || Seconds <= 0) return 0;
    int32 Completed = 0;
    TMap<FName, double> TimeLeft;
    for (auto& Job : State.Jobs)
    {
        if (Job.State == EIndustryJobState::Completed || Job.State == EIndustryJobState::Cancelled || !Operational(Job.Facility)) continue;
        if (Job.State != EIndustryJobState::Running)
        {
            const bool Busy = State.Jobs.ContainsByPredicate([&Job](const auto& Other)
                { return Other.Id != Job.Id && Other.Facility == Job.Facility && Other.State == EIndustryJobState::Running; });
            if (Busy) continue;
            auto* In = State.Inventories.Find(Job.Input); auto* Out = State.Inventories.Find(Job.Output);
            bool Ready = HasAccess(In, Job.Owner) && HasAccess(Out, Job.Owner);
            auto* BP = Job.Blueprint.IsValid() ? State.Blueprints.Find(Job.Blueprint) : nullptr;
            if (Job.Blueprint.IsValid()) Ready &= BP && BP->Owner == Job.Owner && BP->Recipe == Job.Recipe &&
                (BP->bOriginal || BP->RunsRemaining >= Job.Runs);
            double OutputMass = 0;
            for (const auto& Material : Job.Materials)
            {
                const auto* Stack = In ? Find(*In, Material.Item) : nullptr;
                Ready &= Stack && FMath::IsNearlyEqual(Stack->UnitTonnes, Material.UnitTonnes) && Available(State, Job.Input, Material.Item) >= Material.Quantity;
            }
            for (const auto& Product : Job.Products)
            {
                OutputMass += Product.Quantity * Product.UnitTonnes;
                const auto* Stack = Out ? Find(*Out, Product.Item) : nullptr;
                Ready &= !Stack || FMath::IsNearlyEqual(Stack->UnitTonnes, Product.UnitTonnes);
            }
            Ready &= Out && Used(*Out) + OutputMass <= Out->CapacityTonnes;
            if (!Ready) { Job.State = EIndustryJobState::Blocked; continue; }
            for (const auto& Material : Job.Materials) Find(*In, Material.Item)->Quantity -= Material.Quantity;
            if (BP && !BP->bOriginal) BP->RunsRemaining -= Job.Runs;
            for (const auto& Product : Job.Products)
                if (!Find(*Out, Product.Item))
                {
                    auto& Stack = Out->Items.AddDefaulted_GetRef(); Stack.Id = FGuid::NewGuid();
                    Stack.Type = Product.Item; Stack.UnitTonnes = Product.UnitTonnes;
                }
            Out->ReservedOutputTonnes += OutputMass; Job.State = EIndustryJobState::Running;
        }
        double& Budget = TimeLeft.FindOrAdd(Job.Facility, Seconds);
        const double UsedTime = FMath::Min(Job.Remaining, Budget);
        Job.Remaining = FMath::Max(0.0, Job.Remaining - UsedTime);
        Budget -= UsedTime;
        if (Job.Remaining > 0) continue;
        auto& Out = State.Inventories.FindChecked(Job.Output);
        for (const auto& Product : Job.Products)
        {
            Out.ReservedOutputTonnes = FMath::Max(0.0, Out.ReservedOutputTonnes - Product.Quantity * Product.UnitTonnes);
            // Reserved capacity and compatible stack mass were validated at start.
            Deposit(State, Job.Output, Product.Item, Product.Quantity, Product.UnitTonnes);
        }
        Job.State = EIndustryJobState::Completed; ++Completed;
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
