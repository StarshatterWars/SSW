#pragma once

#include "CoreMinimal.h"

class UDataTable;

namespace DesignTableJson
{
    bool Import(UDataTable& Table, UScriptStruct& RowType, const FString& Path);
#if WITH_EDITOR
    bool Export(UDataTable& Table, FString& Json);
    bool Compare(const UDataTable& Expected, const UDataTable& Actual);
#endif
}
