#include "SkillsSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "TransactionAI.h"
#include "Engine/GameInstance.h"
#include "GameStructs.h"
#include "PlayerCharacter.h"
#include "Sim.h"
#include "Ship.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FSkillRules::CanPurchase(const FSkillDefinition& Skill, const TMap<FName, FSkillProgress>& Learned, int32 Rank, FName Empire, FText& Reason)
{
    if (Skill.Name.IsNone() || !Skill.bIsPublished || !Skill.bIsAvailableForPurchase || Skill.BookPrice < 0 || Skill.MaximumLevel < 1 || Skill.MaximumLevel > 5)
    { Reason = INVTEXT("Invalid skill book."); return false; }
    if (const auto* Entry = Learned.Find(Skill.Name); Entry && Entry->bComplete)
    { Reason = INVTEXT("This skill is already complete."); return false; }
    if (Rank < Skill.MinimumRank)
    { Reason = FText::Format(INVTEXT("Requires rank: {0}."), FText::FromString(UTF8_TO_TCHAR(PlayerCharacter::RankName(Skill.MinimumRank)))); return false; }
    if (!Skill.Empire.IsNone() && Skill.Empire != Empire)
    { Reason = INVTEXT("This skill book belongs to another empire."); return false; }
    for (FName Required : Skill.Prerequisites)
        if (const auto* Entry = Learned.Find(Required); !Entry || (!Entry->bComplete && Entry->TrainedLevel < FMath::Max(1,Skill.RequiredSkillLevels.FindRef(Required))))
        { Reason = FText::Format(INVTEXT("Complete the prerequisite skill: {0}."), FText::FromName(Required)); return false; }
    Reason = INVTEXT("Buying this book grants the complete skill immediately."); return true;
}
bool FSkillRules::Purchase(FEconomyState& State, TMap<FName, FSkillProgress>& Learned, const FSkillDefinition& Skill,
    int32 Rank, FName Empire, int64 Now, FText& Reason)
{
    if (!CanPurchase(Skill, Learned, Rank, Empire, Reason)) return false;
    const FGuid Id = FGuid::NewGuid();
    if (!FEconomyLedger::Pay(State, FEconomyLedger::Pilot(), TEXT("Skill book publisher"), Skill.BookPrice, Id, TEXT("Skill book")))
    { Reason = INVTEXT("Insufficient credits for this skill book."); return false; }
    auto& Receipt = State.Journal.Last(); Receipt.Item = Skill.Name; Receipt.Quantity = 1;
    Receipt.UnitPrice = double(Skill.BookPrice); Receipt.Reference = Id;
    FSkillProgress Progress; Progress.Id = Id; Progress.bComplete = true; Progress.LearnedUtc = Now;
    // No timer or level progression runs in this version. bComplete satisfies all levels.
    Learned.Add(Skill.Name, Progress);
    Reason = INVTEXT("Skill book purchased. Complete skill learned."); return true;
}
void USkillsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{ Super::Initialize(Collection); LoadCatalog(); }
bool USkillsSubsystem::LoadCatalog()
{
    FString Json; TArray<TSharedPtr<FJsonValue>> Rows;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("GameData/Skills/skills.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows)) return false;
    TMap<FName, FSkillDefinition> Imported;
    TMap<int32,FName> NamesByType;
    for (const auto& Value : Rows)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr; FSkillDefinition Skill;
        if (!Value->TryGetObject(Object) || !Object || !Object->IsValid() || !FJsonObjectConverter::JsonObjectToUStruct(Object->ToSharedRef(), &Skill) ||
            Skill.Name.IsNone() || Skill.SkillTypeID <= 0 || NamesByType.Contains(Skill.SkillTypeID) || Skill.SkillRank < 1 || Skill.DisplayName.IsEmpty() || Skill.Description.IsEmpty() || Imported.Contains(Skill.Name) ||
            Skill.BookPrice < 0 || Skill.BookPrice > 1000000000000LL || Skill.MinimumRank < 0 || Skill.MaximumLevel < 1 || Skill.MaximumLevel > 5 ||
            !FMath::IsFinite(Skill.TrainingMultiplier) || Skill.TrainingMultiplier <= 0) return false;
        Imported.Add(Skill.Name, Skill);
        NamesByType.Add(Skill.SkillTypeID,Skill.Name);
    }
    // Skill definitions and NPC book offers are independent JSON catalogs. Neither needs a DataTable.
    TArray<TSharedPtr<FJsonValue>> Books;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir()/TEXT("GameData/Skills/skillbooks.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Books)) return false;
    TSet<int32> Offered;
    for (const auto& Value : Books)
    {
        const TSharedPtr<FJsonObject>* Object=nullptr; FSkillBookOffer Book;
        if (!Value->TryGetObject(Object) || !Object || !Object->IsValid() || !FJsonObjectConverter::JsonObjectToUStruct(Object->ToSharedRef(),&Book) ||
            !NamesByType.Contains(Book.SkillTypeID) || Offered.Contains(Book.SkillTypeID) || Book.BasePrice<0 || Book.BasePrice>1000000000000LL ||
            !FMath::IsFinite(Book.PurchaseTaxRate) || Book.PurchaseTaxRate<0 || Book.PurchaseTaxRate>1 || !Book.GrantCompleteSkill) return false;
        auto& Skill=Imported.FindChecked(NamesByType.FindChecked(Book.SkillTypeID));
        Skill.BookPrice=FMath::CeilToInt64(Book.BasePrice*(1+Book.PurchaseTaxRate));
        Skill.bIsPublished=Book.IsPublished; Skill.bIsAvailableForPurchase=Book.IsAvailableForPurchase;
        Offered.Add(Book.SkillTypeID);
    }
    if (Offered.Num()!=Imported.Num()) return false;
    // Validate the prerequisite graph before replacing a working catalog.
    TSet<FName> Visiting, Done;
    TFunction<bool(FName)> Visit = [&](FName Name)
    {
        if (Done.Contains(Name)) return true;
        if (Visiting.Contains(Name) || !Imported.Contains(Name)) return false;
        Visiting.Add(Name);
        for (const auto& Requirement : Imported.FindChecked(Name).RequiredSkillLevels)
            if (!Imported.FindChecked(Name).Prerequisites.Contains(Requirement.Key) || Requirement.Value<1 || Requirement.Value>5) return false;
        for (FName Parent : Imported.FindChecked(Name).Prerequisites) if (!Visit(Parent)) return false;
        Visiting.Remove(Name); Done.Add(Name); return true;
    };
    for (const auto& Pair : Imported) if (!Visit(Pair.Key)) return false;
    Definitions = MoveTemp(Imported);
    UE_LOG(LogTemp, Display, TEXT("[Skills] Loaded %d skill books; instant completion enabled, timed training inactive."), Definitions.Num());
    return !Definitions.IsEmpty();
}
bool USkillsSubsystem::HasSkill(FName Id, int32 Level) const
{
    const auto* Def = Definitions.Find(Id);
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Progress = Players ? Players->GetPlayerInfo().Skills.Find(Id) : nullptr;
    return Def && Level >= 1 && Level <= Def->MaximumLevel && Progress && (Progress->bComplete || Progress->TrainedLevel >= Level);
}
FName USkillsSubsystem::RequiredHullSkill(uint32 Type)
{
    switch (static_cast<CLASSIFICATION>(Type))
    {
    case CLASSIFICATION::CORVETTE: return TEXT("corvette_operations");
    case CLASSIFICATION::FRIGATE: return TEXT("frigate_operations");
    case CLASSIFICATION::DESTROYER: return TEXT("destroyer_operations");
    case CLASSIFICATION::CRUISER: return TEXT("cruiser_operations");
    case CLASSIFICATION::CARRIER: return TEXT("carrier_operations");
    case CLASSIFICATION::BATTLESHIP: return TEXT("battleship_operations");
    case CLASSIFICATION::DREADNAUGHT: return TEXT("dreadnought_operations");
    default: return NAME_None; // Existing fighter training remains accessible.
    }
}
bool USkillsSubsystem::CanLearn(FName Id, FText& Reason) const
{
    const auto* Definition=Definitions.Find(Id); const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    if (!Definition || !Players || !Pilot) return false;
    const FName Empire(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire())));
    return FSkillRules::CanPurchase(*Definition,Players->GetPlayerInfo().Skills,Pilot->GetRank(),Empire,Reason);
}
bool USkillsSubsystem::CanPurchaseBook(FName Id, FText& Reason) const
{
    const auto* Definition = Definitions.Find(Id);
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Definition || !Players || !Pilot) { Reason = INVTEXT("Skill catalog or pilot unavailable."); return false; }
    if (!Simulation || !Simulation->HasSimulationAuthority() || !Craft || Craft->IsDead() || Craft->GetFlightPhase() > EOPSMode::ALERT)
    { Reason = INVTEXT("Dock to purchase skill books."); return false; }
    const FName Empire(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire())));
    if (!FSkillRules::CanPurchase(*Definition, Players->GetPlayerInfo().Skills, Pilot->GetRank(), Empire, Reason)) return false;
    const auto& Economy = Players->GetPlayerInfo().Economy;
    const int64 Balance = TransactionAI::BalanceIncludingLegacy(Economy, Players->GetPlayerInfo().InsuranceCredits);
    if (Balance < Definition->BookPrice) { Reason = INVTEXT("Insufficient credits for this skill book."); return false; }
    return true;
}
bool USkillsSubsystem::PurchaseBook(FName Id, FText& Reason)
{
    if (!CanPurchaseBook(Id, Reason)) return false;
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto& Economy = Players->GetEconomy(); auto& Pilot = Players->GetMutablePlayerInfo(); const auto Before = Pilot;
    const auto* Character = PlayerCharacter::GetCurrentPlayer();
    const FName Empire(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Character->GetEmpire())));
    if (!FSkillRules::Purchase(Economy, Pilot.Skills, Definitions.FindChecked(Id), Character->GetRank(), Empire, FDateTime::UtcNow().ToUnixTimestamp(), Reason)) return false;
    if (!Players->SaveActivePlayerProgress()) { Pilot = Before; Reason = INVTEXT("Could not save this skill purchase. No credits were charged."); return false; }
    return true;
}
