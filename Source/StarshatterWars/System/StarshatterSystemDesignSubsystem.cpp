#include "StarshatterSystemDesignSubsystem.h"

#include "Engine/GameInstance.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SimComponent.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "SystemDesign.h"

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
            !FMath::IsFinite(Grade.ReplacementScale) || Grade.ReplacementScale <= 0)
            return false;
        Levels |= 1u << (Grade.Level - 1);
    }
    return Levels == 0x1f;
}
void PublishSystemComponents(const TMap<FName, FSystemDesign>& Rows)
{
    // Runtime components own a copy of their design and survive a catalog reload.
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
            Component->spares = Entry.Spares;
            Component->affects = Entry.Affects;
            Design->components.append(Component);
        }
        SystemDesign::catalog.append(Design);
    }
}
}

bool UStarshatterSystemDesignSubsystem::ImportDefinitions(UDataTable& Table)
{
    const FString Path = FPaths::ProjectContentDir() / SystemSource;
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)) return false;

    FSystemDesignDocument Document;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Document) ||
        Document.Systems.IsEmpty() || !HasCompleteGrades(Document.ComponentGrades))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Invalid %s: expected systems and one definition for each grade I-V"), *Path);
        return false;
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
            return false;
        }
        Systems.Add(Key);
        TSet<FName> Components;
        for (auto& Component : System.Components)
        {
            Component.Name.TrimStartAndEndInline();
            const FName Name(*Component.Name);
            if (Name.IsNone() || Components.Contains(Name) || Component.Abbrev.TrimStartAndEnd().IsEmpty() ||
                !FMath::IsFinite(Component.RepairTime) || Component.RepairTime < 0 ||
                !FMath::IsFinite(Component.ReplaceTime) || Component.ReplaceTime < 0 || Component.Spares < 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] Invalid or duplicate component in %s: %s / %s"),
                    *Path, *System.Name, *Component.Name);
                return false;
            }
            Components.Add(Name);
            ++ComponentCount;
        }
        System.ComponentGrades = Document.ComponentGrades;
        System.SourceFile = SystemSource;
    }
    if (Table.GetRowStruct() && Table.GetRowStruct() != FSystemDesign::StaticStruct()) return false;
    Table.EmptyTable();
    Table.RowStruct = FSystemDesign::StaticStruct();
    for (const auto& System : Document.Systems)
        Table.AddRow(FName(*System.Name), System);
    UE_LOG(LogTemp, Display, TEXT("[SYSTEM] Imported %d systems, %d components and %d grades from %s"),
        Systems.Num(), ComponentCount, Document.ComponentGrades.Num(), *Path);
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
    if (auto* Imported = NewObject<UDataTable>(this); ImportDefinitions(*Imported))
        SystemDesignDataTable = Imported;
    else
    {
        auto* Assets = GetGameInstance()->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
        SystemDesignDataTable = Assets ? Assets->GetDataTable(TEXT("Data.SystemDesignTable")) : nullptr;
    }
    LoadAll(false);
}

void UStarshatterSystemDesignSubsystem::LoadAll(bool bImportJson)
{
    if (bImportJson)
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
    Variant.RepairTime *= Multipliers->RepairScale;
    Variant.ReplaceTime *= Multipliers->ReplacementScale;
    return Variant;
}
