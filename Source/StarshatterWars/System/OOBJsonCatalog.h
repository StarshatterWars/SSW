#pragma once
#include "CoreMinimal.h"
class UDataTable;
// Transitional OOB ingestion: JSON is authored, existing DataTables remain the runtime bridge.
namespace OOBJsonCatalog
{
 bool ImportDirectory(UDataTable& Out,const FString& Directory,FString& Error);
}
