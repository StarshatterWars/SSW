#include "StarshatterSystemDesignSubsystem.h"

#include "Engine/GameInstance.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SimComponent.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "SystemDesign.h"
#include "ShipEquipment.h"

namespace
{
constexpr TCHAR SystemSource[] = TEXT("GameData/Systems/sys.json");

bool HasCompleteGrades(const TArray<FSystemComponentGrade>& Grades)
{
    if (Grades.Num() != 5) return false;
    uint32 Levels = 0;
    for (const auto& Grade : Grades)
    {
        if (Grade.Level < 1 || Grade.Level > 5 ||
            !FMath::IsFinite(Grade.RepairScale) || Grade.RepairScale <= 0 ||
            !FMath::IsFinite(Grade.ReplacementScale) || Grade.ReplacementScale <= 0 ||
            !FMath::IsFinite(Grade.PriceMultiplier) || Grade.PriceMultiplier <= 0 ||
            !FMath::IsFinite(Grade.PerformanceMultiplier) || Grade.PerformanceMultiplier < 0.5 || Grade.PerformanceMultiplier > 2 ||
            !FMath::IsFinite(Grade.PowerDemandMultiplier) || Grade.PowerDemandMultiplier < 0.5 || Grade.PowerDemandMultiplier > 2)
            return false;
        Levels |= 1u << (Grade.Level - 1);
    }
    return Levels == 0x1f;
}
void PublishSystemComponents(const TMap<FName, FSystemDesign>& Rows)
{
    // Runtime components own a copy of their design and survive a catalog reload.
    FShipEquipment::PublishCatalog(Rows);
    SystemDesign::Close();
    for (const auto& Row : Rows)
    {
        const FSystemDesign& Source = Row.Value;
        auto* Design = new SystemDesign;
        Design->name = TCHAR_TO_UTF8(*Source.Name);
        for (const FComponentDesign& Entry : Source.Components)
        {
            auto* Component = new ComponentDesign;
            Component->name = TCHAR_TO_UTF8(*Entry.Name);
            Component->abrv = TCHAR_TO_UTF8(*Entry.Abbrev);
            Component->repair_time = Entry.RepairTime;
            Component->replace_time = Entry.ReplaceTime;
            Component->base_cost = Entry.BaseCost;
            Component->performance_multiplier = Entry.PerformanceMultiplier;
            Component->power_demand_multiplier = Entry.PowerDemandMultiplier;
            Component->spares = Entry.Spares;
            Component->affects = Entry.Affects;
            Design->components.append(Component);
        }
        SystemDesign::catalog.append(Design);
    }
}
}

bool UStarshatterSystemDesignSubsystem::ReadJsonCatalog(const FString& Path,TMap<FName,FSystemDesign>& Out,FString& Error)
{
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)){Error=TEXT("Cannot read system catalog: ")+Path;return false;}

    FSystemDesignDocument Document;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Document) ||
        Document.Systems.IsEmpty() || !HasCompleteGrades(Document.ComponentGrades))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Invalid %s: expected systems and one definition for each grade I-V"), *Path);
        Error=TEXT("Invalid system/component definitions: ")+Path;return false;
    }
    int32 ComponentCount = 0;
    TSet<FName> Systems;
    for (auto& System : Document.Systems)
    {
        System.Name.TrimStartAndEndInline();
        const FName Key(*System.Name);
        if (Key.IsNone() || Systems.Contains(Key) || System.Components.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Missing or duplicate system or empty component list in %s: %s"), *Path, *System.Name);
            Error=TEXT("Invalid system/component definitions: ")+Path;return false;
        }
        Systems.Add(Key);
        TSet<FName> Components;
        for (auto& Component : System.Components)
        {
            Component.Name.TrimStartAndEndInline();
            const FName Name(*Component.Name);
            if (Name.IsNone() || Components.Contains(Name) || Component.Abbrev.TrimStartAndEnd().IsEmpty() ||
                !FMath::IsFinite(Component.RepairTime) || Component.RepairTime < 0 ||
                !FMath::IsFinite(Component.ReplaceTime) || Component.ReplaceTime < 0 || Component.Spares < 0 || Component.BaseCost < 0 ||
                !FMath::IsFinite(Component.PerformanceMultiplier) || Component.PerformanceMultiplier<0.5 || Component.PerformanceMultiplier>2 ||
                !FMath::IsFinite(Component.PowerDemandMultiplier) || Component.PowerDemandMultiplier<0.5 || Component.PowerDemandMultiplier>2)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Invalid or duplicate component in %s: %s / %s"),
                    *Path, *System.Name, *Component.Name);
                Error=TEXT("Invalid system/component definitions: ")+Path;return false;
            }
            Components.Add(Name);
            ++ComponentCount;
        }
        // Optional native per-system profiles allow weapon quality to use its
        // own reviewed damage/fitting ratios; ordinary systems retain defaults.
        if (System.ComponentGrades.IsEmpty()) System.ComponentGrades = Document.ComponentGrades;
        if (!HasCompleteGrades(System.ComponentGrades))
        {
            UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Invalid per-system grade profile: %s"), *System.Name);
            Error=TEXT("Invalid system/component definitions: ")+Path;return false;
        }
        System.SourceFile = SystemSource;
    }
    TMap<FName,FSystemDesign> Candidate;
    for(const auto& System:Document.Systems)Candidate.Add(FName(*System.Name),System);
    Out=MoveTemp(Candidate);Error.Reset();return true;
}

bool UStarshatterSystemDesignSubsystem::ImportDefinitions(UDataTable& Table)
{
    TMap<FName,FSystemDesign> Rows;FString Error;
    if(!ReadJsonCatalog(FPaths::ProjectContentDir()/SystemSource,Rows,Error))return false;
    if(Table.GetRowStruct() && Table.GetRowStruct()!=FSystemDesign::StaticStruct())return false;
    Table.EmptyTable();Table.RowStruct=FSystemDesign::StaticStruct();
    for(const auto& Pair:Rows)Table.AddRow(Pair.Key,Pair.Value);
    return true;
}

bool UStarshatterSystemDesignSubsystem::ReloadJsonCatalog(const FString& Path,FString& Error)
{
    TMap<FName,FSystemDesign> Candidate;
    if(!ReadJsonCatalog(Path,Candidate,Error))return false;
    DesignsByName=MoveTemp(Candidate);SystemDesignDataTable=nullptr;
    PublishSystemComponents(DesignsByName);
    UE_LOG(LogTemp,Display,TEXT("[SYSTEM] Loaded %d native system designs from JSON; no runtime DataTable"),DesignsByName.Num());
    return true;
}

void UStarshatterSystemDesignSubsystem::Deinitialize()
{
    DesignsByName.Reset();
    SystemDesignDataTable = nullptr;
    Super::Deinitialize();
}

void UStarshatterSystemDesignSubsystem::LoadSystemDesigns()
{
    FString Error;
    if(!ReloadJsonCatalog(FPaths::ProjectContentDir()/SystemSource,Error))
        UE_LOG(LogTemp,Error,TEXT("[SYSTEM] %s; existing native catalog retained"),*Error);
}

void UStarshatterSystemDesignSubsystem::LoadAll(bool bImportJson)
{
    if (bImportJson || !SystemDesignDataTable)
    {
        LoadSystemDesigns();
        return;
    }

    DesignsByName.Reset();
    if (SystemDesignDataTable && SystemDesignDataTable->GetRowStruct() == FSystemDesign::StaticStruct())
        SystemDesignDataTable->ForeachRow<FSystemDesign>(TEXT("System catalog"),
            [this](const FName& Key, const FSystemDesign& Row) { DesignsByName.Add(Key, Row); });

    if (DesignsByName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[SYSTEM] No system designs could be loaded"));
        return;
    }
    PublishSystemComponents(DesignsByName);
    int32 Count = 0;
    bool bHasGrades = true;
    for (const auto& Row : DesignsByName)
    {
        Count += Row.Value.Components.Num();
        bHasGrades &= HasCompleteGrades(Row.Value.ComponentGrades);
    }
    UE_LOG(LogTemp, Display, TEXT("[SYSTEM] Loaded %d systems, %d components and %d grades per system from %s"),
        DesignsByName.Num(), Count, bHasGrades ? 5 : 0, *SystemDesignDataTable->GetPathName());
    if (!bHasGrades)
        UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] System design table is missing grade data; reimport sys.json"));
}
TOptional<FComponentDesign> UStarshatterSystemDesignSubsystem::FindComponentGrade(FName System, FName Component, int32 Grade) const
{
    const FSystemDesign* Design = FindDesign(System);
    if (!Design || Grade < 1 || Grade > 5) return {};
    const auto* Multipliers = Design->ComponentGrades.FindByPredicate([Grade](const FSystemComponentGrade& Entry)
    {
        return Entry.Level == Grade;
    });
    if (!Multipliers) return {};
    const auto* Base = Design->Components.FindByPredicate([Component](const FComponentDesign& Entry)
    {
        return FName(*Entry.Name) == Component;
    });
    if (!Base) return {};
    static const TCHAR* Labels[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V")};
    FComponentDesign Variant = *Base;
    Variant.Name += FString(TEXT(" ")) + Labels[Grade - 1];
    Variant.PerformanceMultiplier *= Multipliers->PerformanceMultiplier;
    Variant.PowerDemandMultiplier *= Multipliers->PowerDemandMultiplier;
    Variant.RepairTime *= Multipliers->RepairScale;
    Variant.ReplaceTime *= Multipliers->ReplacementScale;
    Variant.BaseCost = FMath::CeilToInt64(FMath::Clamp(
        static_cast<double>(Base->BaseCost) * Multipliers->PriceMultiplier, 0.0, 1.e15));
    return Variant;
}
