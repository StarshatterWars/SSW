#include "ShipSkinCatalog.h"
#include "ShipDesignRegistry.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

bool ShipSkinCatalog::Load(const FString& Path, TArray<FShipSkinCatalogEntry>& Out, FString& Error)
{
    FString Text;
    TSharedPtr<FJsonObject> Root;
    const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
    double Version = 0;
    if (IFileManager::Get().FileSize(*Path) > 4*1024*1024 || !FFileHelper::LoadFileToString(Text,*Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root ||
        !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
        !Root->TryGetArrayField(TEXT("skins"),Rows))
    { Error=TEXT("Skin catalog is missing or invalid."); return false; }
    TArray<FShipSkinCatalogEntry> Candidate;
    TSet<FName> Seen;
    const TSharedPtr<FJsonObject>* Prices=nullptr;
    TMap<EShipSkinCategory,int64> CategoryPrices;
    if(!Root->TryGetObjectField(TEXT("prices"),Prices))
    {Error=TEXT("Skin category prices are missing.");return false;}
    for(EShipSkinCategory Category:{EShipSkinCategory::Normal,EShipSkinCategory::Special,EShipSkinCategory::Event})
    {
        double Price=0;const FString Key=StaticEnum<EShipSkinCategory>()->GetNameStringByValue(int64(Category));
        if(!(*Prices)->TryGetNumberField(Key,Price) || !FMath::IsFinite(Price) || Price<1 || Price>1000000000 || Price!=FMath::FloorToDouble(Price))
        {Error=TEXT("Skin category prices must be positive whole credits.");return false;}
        CategoryPrices.Add(Category,int64(Price));
    }
    for (const auto& Value:*Rows)
    {
        const auto Object=Value.IsValid() && Value->Type==EJson::Object ? Value->AsObject() : nullptr;
        FString Id,Hull; FShipSkinCatalogEntry Entry;
        if(!Object || !Object->TryGetStringField(TEXT("id"),Id) || Id.IsEmpty() ||
            !Object->TryGetStringField(TEXT("hull"),Hull) || Hull.IsEmpty() ||
            !Object->TryGetStringField(TEXT("name"),Entry.Name) || Entry.Name.IsEmpty() ||
            !Object->TryGetStringField(TEXT("skinName"),Entry.SkinName) || Entry.SkinName.IsEmpty() || Seen.Contains(FName(*Id)))
        { Error=TEXT("Skin catalog contains an invalid or duplicate identity."); return false; }
        Entry.Id=FName(*Id); Entry.Hull=FName(*Hull);
        Object->TryGetBoolField(TEXT("included"),Entry.bIncluded);
        Object->TryGetStringField(TEXT("description"),Entry.Description);
        FString CategoryName=TEXT("Normal");Object->TryGetStringField(TEXT("category"),CategoryName);
        const int64 Category=StaticEnum<EShipSkinCategory>()->GetValueByNameString(CategoryName);
        if(Category<0 || Category>int64(EShipSkinCategory::Event))
        {Error=TEXT("Unknown skin category.");return false;}
        Entry.Category=static_cast<EShipSkinCategory>(Category);
        Entry.Rarity=Entry.Category==EShipSkinCategory::Normal?ESSWAsteroidRarity::Common:ESSWAsteroidRarity::Rare;
        Entry.PriceCredits=CategoryPrices.FindChecked(Entry.Category);
        Seen.Add(Entry.Id); Candidate.Add(MoveTemp(Entry));
    }
    Out=MoveTemp(Candidate); Error.Reset(); return true;
}

bool ShipSkinCatalog::IsOwned(const FS_PlayerGameInfo& Pilot,const FShipSkinCatalogEntry& Entry)
{ return Entry.bIncluded || Pilot.OwnedShipSkins.Contains(Entry.Id); }

bool ShipSkinCatalog::IsPurchased(const FS_PlayerGameInfo& Pilot,const FShipSkinCatalogEntry& Entry)
{ return Pilot.OwnedShipSkins.Contains(Entry.Id); }

bool ShipSkinCatalog::Purchase(FS_PlayerGameInfo& Pilot,const FShipSkinCatalogEntry& Entry,const FString& Dock,FText& Reason)
{
    if(Dock.IsEmpty()){Reason=INVTEXT("Dock to purchase a skin license.");return false;}
    if(IsOwned(Pilot,Entry)){Reason=INVTEXT("This skin is already owned.");return false;}
    if(!Resolve(Entry) || Entry.PriceCredits<=0)
    {Reason=INVTEXT("This skin is not available.");return false;}
    if(!FEconomyLedger::Pay(Pilot.Economy,FEconomyLedger::Pilot(),TEXT("Hull cosmetics"),Entry.PriceCredits,FGuid::NewGuid(),TEXT("Ship skin license"),ECommercePurpose::Purchase))
    {Reason=INVTEXT("Insufficient credits. No license was purchased.");return false;}
    if(!Pilot.Economy.Journal.IsEmpty())
    {
        auto& Receipt=Pilot.Economy.Journal.Last();Receipt.Category=EWalletTransactionCategory::Market;
        Receipt.Item=Entry.Id;Receipt.Quantity=1;Receipt.UnitPrice=double(Entry.PriceCredits);
    }
    Pilot.OwnedShipSkins.Add(Entry.Id);
    Reason=INVTEXT("Skin license purchased. It remains in your pilot collection if a ship is lost.");return true;
}

const FShipSkin* ShipSkinCatalog::Resolve(const FShipSkinCatalogEntry& Entry)
{
    const auto* Design=ShipDesignRegistry::Find(Entry.Hull);
    const auto* Finish=Design ? Design->Skin.FindByPredicate([&](const FShipSkin& Skin){return Skin.Name==Entry.SkinName;}) : nullptr;
    if(!Finish || Finish->Cells.IsEmpty())return nullptr;
    for(const auto& Cell:Finish->Cells)if(Cell.UnrealMaterial.IsNull() || !Cell.UnrealMaterial.LoadSynchronous())return nullptr;
    return Finish;
}

bool ShipSkinCatalog::Apply(FS_PlayerGameInfo& Pilot,FGuid HullId,const FShipSkinCatalogEntry* Entry,
    const FString& Dock,FText& Reason)
{
    auto* Hull=Pilot.HangarShips.FindByPredicate([&](const FPlayerHangarShip& Row){return Row.HullId==HullId && HullId.IsValid();});
    if(!Hull || Hull->bLost || Hull->bAssigned)
    {Reason=INVTEXT("Select an owned, available ship or fighter.");return false;}
    if(Dock.IsEmpty() || !Hull->Location.Equals(Dock,ESearchCase::IgnoreCase))
    {Reason=INVTEXT("Dock at the base holding this craft to change its appearance.");return false;}
    if(Entry && (!IsOwned(Pilot,*Entry) || Entry->Hull!=Hull->Design || !Resolve(*Entry)))
    {Reason=INVTEXT("This skin is not owned, compatible, or available in the installed catalog.");return false;}
    Hull->SelectedSkin=Entry ? Entry->Id : NAME_None;
    Reason=Entry ? INVTEXT("Skin applied. Ship performance and equipment are unchanged.") : INVTEXT("Original hull appearance restored.");
    return true;
}
