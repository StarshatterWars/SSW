#pragma once
#include "GameStructs.h"
// Faction variants retain training IDs and reward/rank rules; each pilot journal is empire scoped.
namespace FactionTrainingCatalog
{
 const TCHAR* Title(EEMPIRE_NAME Empire);
 const FS_Campaign* Resolve(const FS_Campaign& Original,EEMPIRE_NAME Empire,FString& Error);
}
