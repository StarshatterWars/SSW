#pragma once

#include "Engine/DataTable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CargoCatalogSubsystem.generated.h"

struct FS_Cargo;
struct FS_CargoCatalogRow;
class Ship;

UCLASS()
class STARSHATTERWARS_API UCargoCatalogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void LoadCatalog();
    static bool ImportDefinitions(UDataTable& Table);
    void ForEachDefinition(TFunctionRef<void(FName, const FS_CargoCatalogRow&)> Visit) const;
    const FS_CargoCatalogRow* FindCargo(const FString& Item, const FString& Unit) const;
    bool DescribeCargo(const Ship& Vessel, FS_Cargo& Row) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UDataTable> Catalog;
};
