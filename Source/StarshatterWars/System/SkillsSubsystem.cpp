#include "SkillsSubsystem.h"
#include "SimDataEvents.h"
#include "GalaxyTrafficSubsystem.h"

#include "StationServices.h"
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
    if (Skill.Name.IsNone() || !Skill.bIsPublished || !Skill.bIsAvailableForPurchase || Skill.BookPrice < 0 || Skill.MaximumLevel < 1 || Skill.MaximumLevel > 10)
    { Reason = INVTEXT("Invalid skill book."); return false; }
    if (const auto* Entry = Learned.Find(Skill.Name); Entry && (Entry->Id.IsValid() || Entry->bComplete || Entry->LearnedUtc>0))
    { Reason = INVTEXT("This skill book is already owned."); return false; }
    if (Rank < Skill.MinimumRank)
    { Reason = FText::Format(INVTEXT("Requires rank: {0}."), FText::AsNumber(Skill.MinimumRank)); return false; }
    if (!Skill.Empire.IsNone() && Skill.Empire != Empire)
    { Reason = INVTEXT("This skill book belongs to another empire."); return false; }
    for (FName Required : Skill.Prerequisites)
        if (const auto* Entry = Learned.Find(Required); !Entry || (!Entry->bComplete && Entry->TrainedLevel < FMath::Max(1,Skill.RequiredSkillLevels.FindRef(Required))))
        { Reason = FText::Format(INVTEXT("Complete the prerequisite skill: {0}."), FText::FromName(Required)); return false; }
    Reason = INVTEXT("Buying this book unlocks timed training. Add the skill to your training queue."); return true;
}
bool FSkillRules::Purchase(FEconomyState& State, TMap<FName, FSkillProgress>& Learned, const FSkillDefinition& Skill,
    int32 Rank, FName Empire, int64 Now, FText& Reason)
{
    if (!CanPurchase(Skill, Learned, Rank, Empire, Reason)) return false;
    const FGuid Id = FGuid::NewGuid();
    if (!FEconomyLedger::Pay(State, FEconomyLedger::Pilot(), TEXT("Skill book publisher"), Skill.BookPrice, Id, TEXT("Skill book"),ECommercePurpose::Purchase))
    { Reason = INVTEXT("Insufficient credits for this skill book."); return false; }
    auto& Receipt = State.Journal.Last(); Receipt.Item = Skill.Name; Receipt.Quantity = 1;
    Receipt.UnitPrice = double(Skill.BookPrice); Receipt.Reference = Id;
    FSkillProgress Progress; Progress.Id = Id; Progress.bComplete = false; Progress.LearnedUtc = Now;
    // Ownership and learned levels are independent; no level is granted by purchase.
    Learned.Add(Skill.Name, Progress);
    Reason = INVTEXT("Skill book purchased. Add Level I to the training queue."); return true;
}
void USkillsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{ Super::Initialize(Collection); FSkillTrainingRules::Duration(1); LoadCatalog(); }
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
            Skill.BookPrice < 0 || Skill.BookPrice > 1000000000000LL || Skill.MinimumRank < 0 || Skill.MaximumLevel < 1 || Skill.MaximumLevel > 10 ||
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
            !FMath::IsFinite(Book.PurchaseTaxRate) || Book.PurchaseTaxRate<0 || Book.PurchaseTaxRate>1) return false;
        auto& Skill=Imported.FindChecked(NamesByType.FindChecked(Book.SkillTypeID));
        Skill.BookPrice=FMath::CeilToInt64(Book.BasePrice*(1+Book.PurchaseTaxRate));
        Skill.bIsPublished=Book.IsPublished; Skill.bIsAvailableForPurchase=Book.IsAvailableForPurchase;
        Offered.Add(Book.SkillTypeID);
    }
    if (Offered.Num()!=Imported.Num()) return false;
    TArray<TSharedPtr<FJsonValue>> Advanced;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Skills/advanced_skillbooks.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Advanced))return false;
    TSet<int32> AdvancedTypes;
    for(const auto& V:Advanced){const auto O=V.IsValid()?V->AsObject():nullptr;int32 Type=0,Level=0;double Price=0;
        if(!O || !O->TryGetNumberField(TEXT("SkillTypeID"),Type) || !NamesByType.Contains(Type) || AdvancedTypes.Contains(Type) || !O->TryGetNumberField(TEXT("UnlockLevel"),Level) || Level!=10 || !O->TryGetNumberField(TEXT("BasePrice"),Price) || !FMath::IsFinite(Price) || Price<0 || Price>1.e12)return false;
        Imported.FindChecked(NamesByType.FindChecked(Type)).AdvancedBookPrice=FMath::CeilToInt64(Price);AdvancedTypes.Add(Type);}
    if(AdvancedTypes.Num()!=Imported.Num())return false;

    // Validate the prerequisite graph before replacing a working catalog.
    TSet<FName> Visiting, Done;
    TFunction<bool(FName)> Visit = [&](FName Name)
    {
        if (Done.Contains(Name)) return true;
        if (Visiting.Contains(Name) || !Imported.Contains(Name)) return false;
        Visiting.Add(Name);
        for (const auto& Requirement : Imported.FindChecked(Name).RequiredSkillLevels)
            if (!Imported.FindChecked(Name).Prerequisites.Contains(Requirement.Key) || Requirement.Value<1 || Requirement.Value>10) return false;
        for (FName Parent : Imported.FindChecked(Name).Prerequisites) if (!Visit(Parent)) return false;
        Visiting.Remove(Name); Done.Add(Name); return true;
    };
    for (const auto& Pair : Imported) if (!Visit(Pair.Key)) return false;
    Definitions = MoveTemp(Imported);++CatalogRevision;
    UE_LOG(LogTemp, Display, TEXT("[Skills] Loaded %d skill books; timed Level I-X training enabled."), Definitions.Num());
    return !Definitions.IsEmpty();
}
bool USkillsSubsystem::HasSkill(FName Id, int32 Level) const
{
    const auto* Def = Definitions.Find(Id);
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Progress = Players ? Players->GetPlayerInfo().Skills.Find(Id) : nullptr;
    return Def && Level >= 1 && Level <= Def->MaximumLevel && Progress && FSkillTrainingRules::Level(Progress,Def->MaximumLevel)>=Level;
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
    if(bActionPending){Reason=INVTEXT("A skill request is awaiting saved confirmation.");return false;}
    const auto* Definition = Definitions.Find(Id);
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Definition || !Players || !Pilot) { Reason = INVTEXT("Skill catalog or pilot unavailable."); return false; }
    if (!Simulation || !Simulation->HasSimulationAuthority() || !Craft || Craft->IsDead() || Craft->GetFlightPhase() > EOPSMode::ALERT)
    { Reason = INVTEXT("Dock to purchase skill books."); return false; }
    if(!FStationServices::Offers(ESSWStationService::SkillBooks)){Reason=INVTEXT("This dock has no skill-book service.");return false;}
    const FName Empire(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire())));
    if (!FSkillRules::CanPurchase(*Definition, Players->GetPlayerInfo().Skills, Pilot->GetRank(), Empire, Reason)) return false;
    const auto& Economy = Players->GetPlayerInfo().Economy;
    const int64 Balance = TransactionAI::BalanceIncludingLegacy(Economy, Players->GetPlayerInfo().InsuranceCredits);
    if (Balance < Definition->BookPrice) { Reason = INVTEXT("Insufficient credits for this skill book."); return false; }
    return true;
}
bool USkillsSubsystem::PurchaseBook(FName Id,FText& Reason,TFunction<void(bool,const FText&)> Completed)
{return RequestAction(ESkillCommand::Book,Id,{},0,MoveTemp(Completed),Reason);}

int64 FSkillTrainingRules::Duration(int32 Level)
{
    static const TArray<int64> Times=[](){TArray<int64> T={1500,3067,6272,12825,26224,53624,109651,224215,458477,937500};
        FString Json;TSharedPtr<FJsonObject> Root;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Skills/training.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) && Root && Root->TryGetArrayField(TEXT("LevelSeconds"),Values) && Values->Num()==10)
        {TArray<int64> Candidate;for(const auto& V:*Values){const double N=V->AsNumber();if(!FMath::IsFinite(N) || N<1 || N>31536000 || N!=FMath::FloorToDouble(N))return T;Candidate.Add(static_cast<int64>(N));}T=MoveTemp(Candidate);}return T;}();
    return Times.IsValidIndex(Level-1)?Times[Level-1]:0;
}
int32 FSkillTrainingRules::Level(const FSkillProgress* P,int32 Maximum)
{
    // Old instant books represented complete I-V. They do not grant the new advanced levels.
    return !P?0:FMath::Clamp(P->TrainedLevel>0?P->TrainedLevel:P->bComplete?5:0,0,Maximum);
}
bool FSkillTrainingRules::CanQueue(const TMap<FName,FSkillDefinition>& Definitions,const TMap<FName,FSkillProgress>& Learned,const TArray<FSkillTrainingEntry>& Queue,FName Skill,int32 Rank,FName Empire,FText& Reason)
{
    const auto* Def=Definitions.Find(Skill);const auto* Owned=Learned.Find(Skill);
    if(!Def || !Owned || (!Owned->Id.IsValid() && !Owned->bComplete && Owned->LearnedUtc<=0 && Owned->TrainedLevel<=0)){Reason=INVTEXT("Purchase the standard skill book first.");return false;}
    if(Queue.Num()>=128){Reason=INVTEXT("The training queue has reached its safety limit.");return false;}
    if(Rank<Def->MinimumRank || (!Def->Empire.IsNone() && Empire!=Def->Empire)){Reason=INVTEXT("Your rank or empire does not permit this skill.");return false;}
    TMap<FName,int32> Projected;for(const auto& P:Learned)Projected.Add(P.Key,Level(&P.Value));
    for(const auto& Entry:Queue)Projected.Add(Entry.Skill,Entry.TargetLevel);
    const int32 Next=Projected.FindRef(Skill)+1;const int32 Cap=Owned->bAdvancedBookOwned?Def->MaximumLevel:FMath::Min(5,Def->MaximumLevel);
    if(Next>Cap){Reason=Cap<Def->MaximumLevel?INVTEXT("Purchase the advanced skill book to unlock Levels VI-X."):INVTEXT("All levels are trained or queued.");return false;}
    for(FName Required:Def->Prerequisites)if(Projected.FindRef(Required)<FMath::Max(1,Def->RequiredSkillLevels.FindRef(Required)))
    {Reason=FText::Format(INVTEXT("Train or queue the prerequisite first: {0}."),FText::FromName(Required));return false;}
    Reason=FText::Format(INVTEXT("Queue Level {0}. Training continues offline."),FText::AsNumber(Next));return true;
}
bool FSkillTrainingRules::Queue(const TMap<FName,FSkillDefinition>& Definitions,TMap<FName,FSkillProgress>& Learned,TArray<FSkillTrainingEntry>& Entries,FName Skill,int32 Rank,FName Empire,int64 Now,FText& Reason)
{
    if(Now<=0 || Now>1000000000000LL || !CanQueue(Definitions,Learned,Entries,Skill,Rank,Empire,Reason))return false;
    auto& P=Learned.FindChecked(Skill);const int32 CurrentLevel=Level(&P);
    int32 Target=CurrentLevel+1;for(const auto& Entry:Entries)if(Entry.Skill==Skill)Target=Entry.TargetLevel+1;
    FSkillTrainingEntry Entry;Entry.Id=FGuid::NewGuid();Entry.Skill=Skill;Entry.TargetLevel=Target;
    Entry.StartedUtc=Entries.IsEmpty()?Now:FMath::Max(Now,Entries.Last().FinishesUtc);
    const int64 Accumulated=Target==CurrentLevel+1?static_cast<int64>(FMath::Clamp(FMath::IsFinite(P.SkillPoints)?P.SkillPoints:0.0,0.0,double(Duration(Target)-1))):0;
    Entry.FinishesUtc=Entry.StartedUtc+Duration(Target)-Accumulated;
    if(Entry.FinishesUtc>Now+30LL*86400){Reason=INVTEXT("This level would extend the training queue beyond 30 days.");return false;}P.TrainedLevel=CurrentLevel;P.bComplete=CurrentLevel>=Definitions.FindChecked(Skill).MaximumLevel;Entries.Add(Entry);
    if(Entries.Num()==1){P.TrainingTargetLevel=Target;P.TrainingStartedUtc=Entry.StartedUtc;P.TrainingFinishesUtc=Entry.FinishesUtc;}
    return true;
}
bool FSkillTrainingRules::Advance(const TMap<FName,FSkillDefinition>& Definitions,TMap<FName,FSkillProgress>& Learned,TArray<FSkillTrainingEntry>& Entries,int64 Now)
{
    bool Changed=false;
    while(!Entries.IsEmpty() && Entries[0].FinishesUtc>0 && Entries[0].FinishesUtc<=Now)
    {
        const auto Entry=Entries[0];const auto* Def=Definitions.Find(Entry.Skill);auto* P=Learned.Find(Entry.Skill);
        if(!Def || !P || Entry.StartedUtc<=0 || Entry.FinishesUtc<=Entry.StartedUtc || Entry.TargetLevel!=Level(P)+1 || Entry.TargetLevel>(P->bAdvancedBookOwned?Def->MaximumLevel:5))break;
        bool Ready=true;for(FName Required:Def->Prerequisites)Ready&=Level(Learned.Find(Required))>=FMath::Max(1,Def->RequiredSkillLevels.FindRef(Required));if(!Ready)break;
        P->TrainedLevel=Entry.TargetLevel;P->bComplete=P->TrainedLevel>=Def->MaximumLevel;P->SkillPoints=0;P->TrainingTargetLevel=0;P->TrainingStartedUtc=0;P->TrainingFinishesUtc=0;
        Entries.RemoveAt(0);Changed=true;
    }
    if(!Entries.IsEmpty())if(auto* P=Learned.Find(Entries[0].Skill)){P->TrainingTargetLevel=Entries[0].TargetLevel;P->TrainingStartedUtc=Entries[0].StartedUtc;P->TrainingFinishesUtc=Entries[0].FinishesUtc;}
    return Changed;
}
bool FSkillTrainingRules::Cancel(TMap<FName,FSkillProgress>& Learned,TArray<FSkillTrainingEntry>& Entries,FGuid Id,int64 Now)
{
    const int32 Index=Entries.IndexOfByPredicate([Id](const auto& E){return E.Id==Id;});if(Index==INDEX_NONE || Now<=0)return false;
    const auto Removed=Entries[Index];
    if(Index==0)if(auto* P=Learned.Find(Removed.Skill))
    {P->SkillPoints=FMath::Clamp(P->SkillPoints+double(Now-Removed.StartedUtc),0.0,double(Duration(Removed.TargetLevel)-1));P->TrainingTargetLevel=0;P->TrainingStartedUtc=0;P->TrainingFinishesUtc=0;}
    Entries.RemoveAt(Index);
    int64 Start=Index==0?Now:Entries[Index-1].FinishesUtc;
    for(int32 I=Index;I<Entries.Num();++I)
    {
        auto& E=Entries[I];const auto* P=Learned.Find(E.Skill);
        // Removing a prerequisite level also removes its now-invalid later queued levels.
        int32 Expected=Level(P)+1;for(int32 Earlier=0;Earlier<I;++Earlier)if(Entries[Earlier].Skill==E.Skill)Expected=Entries[Earlier].TargetLevel+1;
        if(E.TargetLevel!=Expected){Entries.RemoveAt(I--);continue;}
        E.StartedUtc=Start;const int64 Credit=P && E.TargetLevel==Level(P)+1?static_cast<int64>(FMath::Clamp(P->SkillPoints,0.0,double(Duration(E.TargetLevel)-1))):0;
        E.FinishesUtc=Start+Duration(E.TargetLevel)-Credit;Start=E.FinishesUtc;
    }
    return true;
}
int32 USkillsSubsystem::GetLevel(FName Id) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Def=Definitions.Find(Id);
    return Def && Players?FSkillTrainingRules::Level(Players->GetPlayerInfo().Skills.Find(Id),Def->MaximumLevel):0;
}
TConstArrayView<FSkillTrainingEntry> USkillsSubsystem::GetQueue() const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();return Players?TConstArrayView<FSkillTrainingEntry>(Players->GetPlayerInfo().SkillQueue):TConstArrayView<FSkillTrainingEntry>();
}
bool USkillsSubsystem::CanQueueTraining(FName Id,FText& Reason) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    const auto* Simulation=Sim::GetSim();if(!Players || !Pilot || (Simulation && Simulation->IsNetworked())){Reason=INVTEXT("Skill training requires an offline career.");return false;}
    return FSkillTrainingRules::CanQueue(Definitions,Players->GetPlayerInfo().Skills,Players->GetPlayerInfo().SkillQueue,Id,Pilot->GetRank(),FName(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire()))),Reason);
}
bool USkillsSubsystem::QueueTraining(FName Id,FText& Reason)
{
    int32 Target=GetLevel(Id)+1;for(const auto& E:GetQueue())if(E.Skill==Id)Target=E.TargetLevel+1;
    return QueueToLevel(Id,Target,Reason);
}
bool USkillsSubsystem::CancelTraining(FGuid Entry,FText& Reason,TFunction<void(bool,const FText&)> Completed)
{return RequestAction(ESkillCommand::Cancel,NAME_None,Entry,0,MoveTemp(Completed),Reason);}
void USkillsSubsystem::Tick(float)
{
    const double Clock=FPlatformTime::Seconds();if(Clock-LastTrainingCheck<1)return;LastTrainingCheck=Clock;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Simulation=Sim::GetSim();
    if(!Players || !Players->HasLoaded() || (Simulation && Simulation->IsNetworked()))return;
    const auto& View=Players->GetPlayerInfo();if(View.SkillQueue.IsEmpty() || View.SkillQueue[0].FinishesUtc>FDateTime::UtcNow().ToUnixTimestamp())return;
    if(!bActionPending){FText Reason;RequestAction(ESkillCommand::Advance,NAME_None,{},0,{},Reason);}
}
FText USkillsSubsystem::TrainingStatus(FName Id) const
{
    static const TCHAR* Roman[]={TEXT("0"),TEXT("I"),TEXT("II"),TEXT("III"),TEXT("IV"),TEXT("V"),TEXT("VI"),TEXT("VII"),TEXT("VIII"),TEXT("IX"),TEXT("X")};
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* P=Players?Players->GetPlayerInfo().Skills.Find(Id):nullptr;
    const int32 Cap=P && P->bAdvancedBookOwned?10:5;
    return FText::FromString(FString::Printf(TEXT("%s / %s"),Roman[FMath::Clamp(GetLevel(Id),0,10)],Roman[Cap]));
}
bool USkillsSubsystem::CanPurchaseAdvancedBook(FName Id,FText& Reason) const
{
    if(bActionPending){Reason=INVTEXT("A skill request is awaiting saved confirmation.");return false;}
    const auto* Character=PlayerCharacter::GetCurrentPlayer();auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;
    if(!Character || !S || !S->HasSimulationAuthority() || S->IsNetworked() || !Craft || Craft->IsDead() || !FStationServices::IsDocked())
    {Reason=INVTEXT("Dock in an offline career to purchase skill books.");return false;}
    const auto* Def=Definitions.Find(Id);const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* P=Players?Players->GetPlayerInfo().Skills.Find(Id):nullptr;
    if(!Def || !P || GetLevel(Id)<5){Reason=INVTEXT("Train Level V before buying the advanced book.");return false;}
    if(P->bAdvancedBookOwned){Reason=INVTEXT("Advanced training is already unlocked.");return false;}
    const FName Empire(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Character->GetEmpire())));
    if(!Def->bIsPublished || !Def->bIsAvailableForPurchase || Character->GetRank()<Def->MinimumRank || (!Def->Empire.IsNone() && Empire!=Def->Empire))
    {Reason=INVTEXT("Rank, empire or catalog restrictions prevent this purchase.");return false;}
    if(!FStationServices::Offers(ESSWStationService::SkillBooks)){Reason=INVTEXT("Dock at a station that sells skill books.");return false;}
    if(TransactionAI::BalanceIncludingLegacy(Players->GetPlayerInfo().Economy,Players->GetPlayerInfo().InsuranceCredits)<Def->AdvancedBookPrice){Reason=INVTEXT("Insufficient credits for the advanced book.");return false;}
    Reason=FText::Format(INVTEXT("Purchase advanced book: {0} CR. Unlocks VI-X; training is still required."),FText::AsNumber(Def->AdvancedBookPrice));return true;
}
bool USkillsSubsystem::PurchaseAdvancedBook(FName Id,FText& Reason,TFunction<void(bool,const FText&)> Completed)
{return RequestAction(ESkillCommand::Advanced,Id,{},0,MoveTemp(Completed),Reason);}

int32 USkillsSubsystem::TrainingCap(FName Id) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* P=Players?Players->GetPlayerInfo().Skills.Find(Id):nullptr;
    return P && P->bAdvancedBookOwned?10:5;
}
bool USkillsSubsystem::QueueToLevel(FName Id,int32 Target,FText& Reason,TFunction<void(bool,const FText&)> Completed)
{return RequestAction(ESkillCommand::Queue,Id,{},Target,MoveTemp(Completed),Reason);}

int64 FSkillTrainingRules::InstantCost(const FSkillTrainingEntry& Entry,int64 Now)
{
    if(Entry.TargetLevel<1 || Entry.TargetLevel>10)return 0;
    const double Full=500.0*FMath::Pow(2.0,Entry.TargetLevel-1);
    const double Remaining=FMath::Clamp(double(Entry.FinishesUtc-FMath::Max(Now,Entry.StartedUtc))/FMath::Max(1.0,double(Duration(Entry.TargetLevel))),0.0,1.0);
    return FMath::Max(1LL,FMath::CeilToInt64(Full*Remaining));
}
bool FSkillTrainingRules::InstantTrain(FEconomyState& Economy,const TMap<FName,FSkillDefinition>& Definitions,TMap<FName,FSkillProgress>& Learned,TArray<FSkillTrainingEntry>& Queue,FGuid Id,int64 Now,FText& Reason)
{
    const int32 Index=Queue.IndexOfByPredicate([Id](const auto& E){return E.Id==Id;});if(Index==INDEX_NONE)return false;
    const auto Entry=Queue[Index];const auto* Def=Definitions.Find(Entry.Skill);auto* Progress=Learned.Find(Entry.Skill);
    if(!Def || !Progress || Entry.TargetLevel!=Level(Progress)+1 || Entry.TargetLevel>(Progress->bAdvancedBookOwned?Def->MaximumLevel:5))
    {Reason=INVTEXT("Train the previous levels and unlock advanced training first.");return false;}
    for(FName Required:Def->Prerequisites)if(Level(Learned.Find(Required))<FMath::Max(1,Def->RequiredSkillLevels.FindRef(Required)))
    {Reason=INVTEXT("Complete the prerequisite skills first.");return false;}
    const int64 Cost=InstantCost(Entry,Now);
    if(!FEconomyLedger::Pay(Economy,FEconomyLedger::Pilot(),TEXT("Skill training office"),Cost,FGuid::NewGuid(),TEXT("Instant skill training"),ECommercePurpose::Service))
    {Reason=INVTEXT("Insufficient credits for instant training.");return false;}
    Economy.Journal.Last().Item=Entry.Skill;Economy.Journal.Last().Reference=Entry.Id;
    Progress->TrainedLevel=Entry.TargetLevel;Progress->bComplete=Entry.TargetLevel>=Def->MaximumLevel;Progress->SkillPoints=0;
    Progress->TrainingTargetLevel=0;Progress->TrainingStartedUtc=0;Progress->TrainingFinishesUtc=0;
    Queue.RemoveAt(Index);int64 Start=Index==0?Now:Queue[Index-1].FinishesUtc;
    for(int32 I=Index;I<Queue.Num();++I){auto& E=Queue[I];E.StartedUtc=Start;E.FinishesUtc=Start+Duration(E.TargetLevel);Start=E.FinishesUtc;}
    Reason=FText::Format(INVTEXT("Level trained for {0} CR."),FText::AsNumber(Cost));return true;
}
bool USkillsSubsystem::InstantTrain(FGuid Entry,FText& Reason,TFunction<void(bool,const FText&)> Completed)
{return RequestAction(ESkillCommand::Instant,NAME_None,Entry,0,MoveTemp(Completed),Reason);}

bool USkillsSubsystem::RequestAction(ESkillCommand Action,FName Id,FGuid Entry,int32 Target,TFunction<void(bool,const FText&)>&& Completed,FText& Reason)
{
    check(IsInGameThread());auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* Domain=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();
    const auto* Character=PlayerCharacter::GetCurrentPlayer();auto* S=Sim::GetSim();
    if(bActionPending || !Players || !Players->HasLoaded() || !Domain || !Character || (S && (S->IsNetworked() || !S->HasSimulationAuthority())))
    {Reason=INVTEXT("Skill service is busy or unavailable for this pilot.");return false;}
    if(Action==ESkillCommand::Book && !CanPurchaseBook(Id,Reason))return false;
    if(Action==ESkillCommand::Advanced && !CanPurchaseAdvancedBook(Id,Reason))return false;
    FNativeSkillCommand C;C.Action=Action;C.Skill=Id;C.Entry=Entry;C.Target=Target;C.Receipt=FGuid::NewGuid();C.Definitions=Definitions;
    C.Rank=Character->GetRank();C.Empire=FName(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Character->GetEmpire())));
    if(Action==ESkillCommand::Book)C.MaximumCost=Definitions.FindChecked(Id).BookPrice;
    if(Action==ESkillCommand::Advanced)C.MaximumCost=Definitions.FindChecked(Id).AdvancedBookPrice;
    if(Action==ESkillCommand::Instant)
    {
        const auto* Q=Players->GetPlayerInfo().SkillQueue.FindByPredicate([Entry](const auto& E){return E.Id==Entry;});
        if(!Q){Reason=INVTEXT("Training entry is no longer queued.");return false;}
        C.MaximumCost=FSkillTrainingRules::InstantCost(*Q,FDateTime::UtcNow().ToUnixTimestamp());
    }
    const bool Purchase=Action==ESkillCommand::Book || Action==ESkillCommand::Advanced;
    auto* Craft=S?S->GetPlayerShip():nullptr;const FGuid Hull=Craft?Craft->GetHullId():FGuid();
    const FGuid Base=Craft && Craft->GetCarrier()?Craft->GetCarrier()->GetHullId():FGuid();
    const FName BaseName(Craft && Craft->GetCarrier()?UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()):TEXT(""));
    const uint64 Catalog=CatalogRevision;const int32 Rank=C.Rank;const FName Empire=C.Empire;const TWeakObjectPtr<USkillsSubsystem> Weak(this);
    bActionPending=true;
    const bool Queued=Domain->QueueSkillCommand(MoveTemp(C),[Weak,Catalog,Rank,Empire,Purchase,Hull,Base,BaseName](FText& Why)
    {
        const auto* Self=Weak.Get();const auto* Pilot=PlayerCharacter::GetCurrentPlayer();auto* CurrentSim=Sim::GetSim();auto* Current=CurrentSim?CurrentSim->GetPlayerShip():nullptr;
        if(!Self || Self->CatalogRevision!=Catalog || !Pilot || Pilot->GetRank()!=Rank ||
            FName(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire())))!=Empire ||
            (CurrentSim && (CurrentSim->IsNetworked() || !CurrentSim->HasSimulationAuthority())))
        {Why=INVTEXT("Pilot, authority or skill catalog changed. Nothing charged.");return false;}
        if(Purchase && (!Current || Current->IsDead() || Current->IsDying() || Current->GetHullId()!=Hull || !FStationServices::IsDocked() ||
            !Current->GetCarrier() || Current->GetCarrier()->GetHullId()!=Base || FName(UTF8_TO_TCHAR(Current->GetCarrier()->GetName()))!=BaseName || !FStationServices::Offers(ESSWStationService::SkillBooks)))
        {Why=INVTEXT("Skill-book docking service changed. Nothing charged.");return false;}return true;
    },[Weak](){auto* Self=Weak.Get();auto* P=Self?Self->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;return P && P->SaveActivePlayerProgress();},
    [Weak,Done=MoveTemp(Completed)](bool Saved,const FText& Why){if(auto* Self=Weak.Get())Self->bActionPending=false;if(Done)Done(Saved,Why);},Reason);
    if(!Queued)bActionPending=false;return Queued;
}
