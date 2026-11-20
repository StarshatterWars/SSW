#pragma once
#include "CoreMinimal.h"
class UDataTable;
struct FS_CombatGroup;
// Native runtime OOB catalog. DataTable export is retained only for authoring/parity checks.
namespace OOBJsonCatalog
{
 bool LoadDirectory(TMap<FName,FS_CombatGroup>& Out,const FString& Directory,FString& Error);
 bool ImportDirectory(UDataTable& Out,const FString& Directory,FString& Error);
}
