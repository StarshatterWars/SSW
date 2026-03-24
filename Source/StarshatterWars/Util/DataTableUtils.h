
#include "CoreMinimal.h"
#include "Engine/DataTable.h"

template<typename RowStructT>
static bool UpsertRow(UDataTable* Table, const FName RowName, const RowStructT& Value, const TCHAR* Context)
{
    if (!Table)
        return false;

    // Optional: enforce correct DT row type (prevents silent corruption / asserts)
    const UScriptStruct* Expected = RowStructT::StaticStruct();
    const UScriptStruct* Actual = Table->GetRowStruct();

    if (Actual != Expected)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[DT] UpsertRow: Row struct mismatch for '%s' (DT=%s) Expected=%s Actual=%s"),
            *RowName.ToString(),
            *GetNameSafe(Table),
            *GetNameSafe(Expected),
            *GetNameSafe(Actual));
        return false;
    }

    if (RowStructT* Existing = Table->FindRow<RowStructT>(RowName, Context, /*bWarnIfRowMissing=*/false))
    {
        *Existing = Value;   // overwrite in-place
    }
    else
    {
        Table->AddRow(RowName, Value);
    }

    return true;
}

static FName MakeRowNameTrimmed(const FString& In)
{
    FString Clean = In;
    Clean.TrimStartAndEndInline();
    return FName(*Clean);
}
