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
    const FName Marker(TEXT("Authored initial market stock v1"));
    if (State.InitializedMarkets.Contains(Marker)) return;
    FString Json; TArray<TSharedPtr<FJsonValue>> Entries;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("GameData/Cargo/economy.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Entries)) return;
    // Initial NPC stock is authored startup supply, not simulated hourly production or delivery.
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
        EnsureWarehouse(State, Location, FName(*System), 1.e12, true);
        TSet<FName> Products;
        const TArray<TSharedPtr<FJsonValue>>* Goods;
        if (Object->TryGetArrayField(TEXT("Production"), Goods))
            for (const auto& Item : *Goods) Products.Add(DataTableUtils::MakeValidName(Item->AsString()));
        for (const auto& Pair : Exclusive) if (Pair.Value == FName(*System)) Products.Add(Pair.Key);
        bool Illicit = false; Object->TryGetBoolField(TEXT("bIllicitTrade"), Illicit);
        Catalog.ForEachDefinition([&](FName Item, const FS_CargoCatalogRow& Row)
        {
            const auto* Only = Exclusive.Find(Item);
            if (!Products.Contains(Item) || (Only && *Only != FName(*System)) || (Row.bIllicit && !Illicit && !Only) ||
                !FMath::IsFinite(Row.Size) || Row.Size <= 0) return;
            const double Count = FMath::Max(1, FMath::Min(Row.PreviewQuantity, 160 >> static_cast<uint8>(Row.Rarity))) * 2.0;
            FEconomyLedger::Deposit(State, Location, Item, Count, Row.Size);
        });
    }
    State.InitializedMarkets.Add(Marker);
}
