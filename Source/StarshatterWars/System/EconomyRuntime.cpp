#include "EconomyRuntime.h"
#include "CargoCatalogSubsystem.h"
#include "GameStructs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FName FEconomyRuntime::Warehouse(const FString& System, const FString& Station, const TCHAR* Compartment)
{ return FName(*(System + TEXT("/") + Station + TEXT("/") + Compartment)); }
void FEconomyRuntime::EnsureWarehouse(FEconomyState& State, FName Key, FName System, double Capacity, bool bMarket)
{
    if (auto* Inv = FEconomyLedger::CreateInventory(State, Key, IndustryOwner(), System, Capacity)) Inv->bMarket = bMarket;
}
void FEconomyRuntime::InitializeMarkets(FEconomyState& State, const UCargoCatalogSubsystem& Catalog)
{
    // Never record an empty startup pass before native cargo ingestion completes.
    if (!Catalog.HasDefinitions()) return;
    const FName Marker(TEXT("Authored initial market stock v1"));
    const FName DroneMarker(TEXT("Authored initial mining drone stock v1"));
    const FName DroneItem = DataTableUtils::MakeValidName(TEXT("mining drone"));
    bool ExistingMarkets = State.InitializedMarkets.Contains(Marker);
    bool ExistingDrones = State.InitializedMarkets.Contains(DroneMarker);
    const FName ReadyMarker(TEXT("Catalog-ready NPC warehouse stock v2"));
    if (ExistingMarkets && ExistingDrones && State.InitializedMarkets.Contains(ReadyMarker)) return;
    const FName RepairMarker(TEXT("Catalog-ready NPC warehouse startup repair v1"));
    bool HasWarehouse=false,EmptyReadyWarehouses=true;
    for (const auto& Pair : State.Inventories)
        if (Pair.Key == Warehouse(Pair.Value.System.ToString(), TEXT("NPC Market"), TEXT("Stock")))
        {
            HasWarehouse=true;
            if (FEconomyLedger::Used(Pair.Value)>UE_SMALL_NUMBER || Pair.Value.ReservedOutputTonnes>0 ||
                !Pair.Value.bOperational || Pair.Value.CapacityTonnes<=0)EmptyReadyWarehouses=false;
        }
    const bool RepairEmptyStartup = ExistingMarkets && (!ExistingDrones || !HasWarehouse) && EmptyReadyWarehouses &&
        !State.InitializedMarkets.Contains(RepairMarker) && State.MarketOrders.IsEmpty() &&
        !State.Journal.ContainsByPredicate([](const auto& Entry){return Entry.Category==EWalletTransactionCategory::Market;});
    // Repair only the never-traded, empty-catalog startup (or orphaned markers).
    // Completed initialization and spent stock never trigger a replenishment.
    if (RepairEmptyStartup) { ExistingMarkets=false;ExistingDrones=false; }
    if (ExistingMarkets && ExistingDrones) { State.InitializedMarkets.Add(ReadyMarker); return; }
    FString Json; TArray<TSharedPtr<FJsonValue>> Entries;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("GameData/Cargo/economy.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Entries)) return;
    // Initial NPC stock is authored startup supply, not simulated hourly production or delivery.
    // New catalog items migrate independently: never restock consumed legacy goods.
    bool DroneMigrationComplete = Catalog.FindCargo(TEXT("mining drone"), TEXT("units")) != nullptr;
    int32 DroneMarkets = 0;
    TMap<FName, FName> Exclusive;
    for (const auto& Entry : Entries)
    {
        const auto& Object = Entry->AsObject(); if (!Object) continue;
        FString System; const TArray<TSharedPtr<FJsonValue>>* Goods;
        if (!Object->TryGetStringField(TEXT("Name"), System)) continue;
        if (Object->TryGetArrayField(TEXT("ExclusiveSupply"), Goods))
            for (const auto& Item : *Goods) Exclusive.Add(DataTableUtils::MakeValidName(Item->AsString()), FName(*System));
    }
    for (const auto& Entry : Entries)
    {
        const auto& Object = Entry->AsObject(); if (!Object) continue;
        FString System; if (!Object->TryGetStringField(TEXT("Name"), System)) continue;
        const FName Location = Warehouse(System, TEXT("NPC Market"), TEXT("Stock"));
        TSet<FName> Products;
        const TArray<TSharedPtr<FJsonValue>>* Goods;
        if (Object->TryGetArrayField(TEXT("Production"), Goods))
            for (const auto& Item : *Goods) Products.Add(DataTableUtils::MakeValidName(Item->AsString()));
        for (const auto& Pair : Exclusive) if (Pair.Value == FName(*System)) Products.Add(Pair.Key);
        const bool ProducesDrones = Products.Contains(DroneItem);
        if (ProducesDrones) ++DroneMarkets;
        if (ExistingMarkets && !ProducesDrones) continue;
        EnsureWarehouse(State, Location, FName(*System), 1.e12, true);
        const auto* MarketInventory = State.Inventories.Find(Location);
        if (!MarketInventory || MarketInventory->Owner != IndustryOwner() || MarketInventory->System != FName(*System))
        {
            if (ProducesDrones && !ExistingDrones) DroneMigrationComplete = false;
            continue;
        }
        bool Illicit = false; Object->TryGetBoolField(TEXT("bIllicitTrade"), Illicit);
        Catalog.ForEachDefinition([&](FName Item, const FS_CargoCatalogRow& Row)
        {
            if (ExistingMarkets && Item != DroneItem) return;
            const FName DroneLocationMarker(*(DroneMarker.ToString() + TEXT("/") + System));
            if (Item == DroneItem && (ExistingDrones || State.InitializedMarkets.Contains(DroneLocationMarker))) return;
            const auto* Only = Exclusive.Find(Item);
            if (!Products.Contains(Item) || (Only && *Only != FName(*System)) || (Row.bIllicit && !Illicit && !Only) ||
                !FMath::IsFinite(Row.Size) || Row.Size <= 0) return;
            const double Count = FMath::Max(1, FMath::Min(Row.PreviewQuantity, 160 >> static_cast<uint8>(Row.Rarity))) * 2.0;
            const bool Deposited = FEconomyLedger::Deposit(State, Location, Item, Count, Row.Size);
            if (Item == DroneItem)
            {
                if (Deposited) State.InitializedMarkets.Add(DroneLocationMarker);
                else DroneMigrationComplete = false;
            }
        });
    }
    State.InitializedMarkets.Add(Marker);
    if (RepairEmptyStartup) State.InitializedMarkets.Add(RepairMarker);
    if (DroneMigrationComplete && DroneMarkets > 0) State.InitializedMarkets.Add(DroneMarker);
    if (State.InitializedMarkets.Contains(Marker) && State.InitializedMarkets.Contains(DroneMarker))
        State.InitializedMarkets.Add(ReadyMarker);
}
